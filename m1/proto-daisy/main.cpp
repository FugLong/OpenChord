#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include "hid/logger.h"
#include "pin_map.h"
#include "chord_engine.h"

using namespace daisy;
using namespace m1proto;

// Same port as OG: enclosure USB-C = D29/D30 = FS_EXTERNAL, not the Seed micro-USB.
using ExternalLog = Logger<LOGGER_EXTERNAL>;

static DaisySeed hw;

static GPIO row_gpio[3];
static GPIO col_gpio[4];
static GPIO joy_sw;
static AdcChannelConfig adc_cfg[2];

static OledDisplay<SSD130x4WireSpi128x64Driver> oled;
static MidiUsbHandler usb_midi;

static constexpr int kRows = 3;
static constexpr int kCols = 4;

static bool key_now[3][4];

static oc::Voicing sounding;
static oc::Voicing last_voicing;
static int16_t held_roots[16];
static int held_n = 0;
static uint8_t held_vel = 100;
static uint8_t out_ch = 0;
static uint8_t key_pc = 0;
static bool key_arm = false;
static bool chord_mode = false;
static bool panic_was = false;

static const Pin kRowsPins[3] = {PinMap::KEY_ROW_0, PinMap::KEY_ROW_1, PinMap::KEY_ROW_2};
static const Pin kColsPins[4] = {PinMap::KEY_COL_0, PinMap::KEY_COL_1, PinMap::KEY_COL_2,
                                 PinMap::KEY_COL_3};

static bool ValidKey(int r, int c) { return !(r == 1 && c == 3); }

// Same busy-wait as archive digital_manager.cpp (Seed ~480 MHz).
static void DelayUs(uint32_t us) {
    volatile uint32_t count = us * 120;
    while (count--) {
        __asm__("nop");
    }
}

static void ScanMatrix() {
    for (int r = 0; r < kRows; ++r) row_gpio[r].Write(true);
    DelayUs(10);
    for (int r = 0; r < kRows; ++r) {
        row_gpio[r].Write(false);
        DelayUs(5);
        for (int c = 0; c < kCols; ++c) {
            if (!ValidKey(r, c)) continue;
            key_now[r][c] = !col_gpio[c].Read();
        }
        row_gpio[r].Write(true);
        DelayUs(5);
    }
}

static bool Pressed(int r, int c) { return ValidKey(r, c) && key_now[r][c]; }

static oc::Type CurrentType() {
    oc::Type t = oc::Type::None;
    if (Pressed(0, 0)) t = oc::Type::Dim;
    if (Pressed(0, 1)) t = oc::Type::Min;
    if (Pressed(0, 2)) t = oc::Type::Maj;
    if (Pressed(0, 3)) t = oc::Type::Sus;
    return t;
}

static uint8_t CurrentExt() {
    uint8_t e = 0;
    if (Pressed(1, 0)) e |= oc::Ext6;
    if (Pressed(1, 1)) e |= oc::Extm7;
    if (Pressed(1, 2)) e |= oc::ExtM7;
    if (Pressed(2, 0)) e |= oc::Ext9;
    return e;
}

static bool KeyHeld() { return Pressed(2, 1); }
static bool ShiftHeld() { return Pressed(2, 2); }
static bool PanicHeld() { return Pressed(2, 3); }

static void MidiSend(uint8_t status, uint8_t d0, uint8_t d1) {
    uint8_t b[3] = {status, d0, d1};
    usb_midi.SendMessage(b, 3);
}

static void NotesOff(const oc::Voicing& v) {
    for (uint8_t i = 0; i < v.n; ++i) {
        MidiSend(static_cast<uint8_t>(0x80 | out_ch), v.notes[i], 0);
    }
}

static void NotesOn(const oc::Voicing& v, uint8_t vel) {
    if (vel == 0) vel = 1;
    for (uint8_t i = 0; i < v.n; ++i) {
        MidiSend(static_cast<uint8_t>(0x90 | out_ch), v.notes[i], vel);
    }
}

static void ThruOn(int16_t note, uint8_t vel) {
    MidiSend(static_cast<uint8_t>(0x90 | out_ch), static_cast<uint8_t>(note), vel);
}

static void ThruOff(int16_t note) {
    MidiSend(static_cast<uint8_t>(0x80 | out_ch), static_cast<uint8_t>(note), 0);
}

static void ThruOffHeld() {
    for (int i = 0; i < held_n; ++i) ThruOff(held_roots[i]);
}

static void ThruOnHeld() {
    for (int i = 0; i < held_n; ++i) ThruOn(held_roots[i], held_vel);
}

static void Panic() {
    NotesOff(sounding);
    sounding.n = 0;
    ThruOffHeld();
    chord_mode = false;
    MidiSend(static_cast<uint8_t>(0xB0 | out_ch), 123, 0);
}

static int16_t CurrentRoot() {
    if (held_n <= 0) return -1;
    return held_roots[held_n - 1];
}

static void PushRoot(int16_t n) {
    for (int i = 0; i < held_n; ++i) {
        if (held_roots[i] == n) return;
    }
    if (held_n < 16) held_roots[held_n++] = n;
}

static void PopRoot(int16_t n) {
    int w = 0;
    for (int i = 0; i < held_n; ++i) {
        if (held_roots[i] != n) held_roots[w++] = held_roots[i];
    }
    held_n = w;
}

static void EmitChord() {
    oc::EngineInput in{};
    in.root_midi = CurrentRoot();
    in.type = CurrentType();
    in.ext = CurrentExt();
    in.key_pc = key_pc;
    in.velocity = held_vel;
    if (in.root_midi < 0 || in.type == oc::Type::None) return;

    float x = (hw.adc.GetFloat(0) - 0.5f) * 2.0f;
    float y = (0.5f - hw.adc.GetFloat(1)) * 2.0f; // invert: stick-up = musical up
    in.seat = oc::SeatFromStick(x, y, 0.42f);

    oc::Voicing next{};
    oc::Render(in, sounding.n ? &sounding : &last_voicing, &next);

    bool same = (next.n == sounding.n);
    if (same) {
        for (uint8_t i = 0; i < next.n; ++i) {
            if (next.notes[i] != sounding.notes[i]) same = false;
        }
    }
    if (same) return;

    NotesOff(sounding);
    sounding = next;
    if (sounding.n) last_voicing = sounding;
    NotesOn(sounding, held_vel);
}

static const char* KeyName(uint8_t pc) {
    static const char* n[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return n[pc % 12];
}

static void Draw() {
    oled.Fill(false);
    oled.SetCursor(0, 0);
    oled.WriteString("OpenChord M1", Font_6x8, true);
    oled.SetCursor(0, 16);
    if (sounding.n && sounding.name[0]) {
        oled.WriteString(sounding.name, Font_11x18, true);
    } else if (CurrentType() == oc::Type::None) {
        oled.WriteString("(hold type)", Font_6x8, true);
    } else {
        oled.WriteString("...", Font_11x18, true);
    }
    oled.SetCursor(0, 40);
    oled.WriteString(KeyName(key_pc), Font_6x8, true);
    oled.SetCursor(48, 40);
    oled.WriteString("major", Font_6x8, true);
    oled.SetCursor(0, 52);
    oled.WriteString(key_arm ? "KEY" : "usb midi", Font_6x8, true);
    oled.Update();
}

static void HandleIncoming(const MidiEvent& ev) {
    if (ev.type != MidiMessageType::NoteOn && ev.type != MidiMessageType::NoteOff) {
        return;
    }
    uint8_t note = ev.data[0];
    uint8_t vel = ev.data[1];
    out_ch = static_cast<uint8_t>(ev.channel & 0x0f);
    bool on = (ev.type == MidiMessageType::NoteOn) && vel > 0;

    if (on && KeyHeld()) {
        key_pc = static_cast<uint8_t>(note % 12);
        return;
    }

    if (on) {
        held_vel = vel;
        PushRoot(static_cast<int16_t>(note));
    } else {
        PopRoot(static_cast<int16_t>(note));
    }

    // Thru only when no type is held. Chord emit happens in the main loop
    // so we never note-off a thru root after the chord is already on.
    if (CurrentType() == oc::Type::None) {
        if (on) ThruOn(static_cast<int16_t>(note), vel);
        else ThruOff(static_cast<int16_t>(note));
    }
}

static void AudioCallback(AudioHandle::InputBuffer in,
                          AudioHandle::OutputBuffer out,
                          size_t size) {
    (void)in;
    for (size_t i = 0; i < size; ++i) {
        out[0][i] = 0.f;
        out[1][i] = 0.f;
    }
}

int main(void) {
    hw.Init();

    // OG order on this box (system_initializer.cpp + midi_handler.cpp):
    // 1) LOGGER_EXTERNAL StartLog — USB PHY on D29/D30 as CDC
    // 2) display / IO
    // 3) MidiUsbHandler EXTERNAL — usbd_mode = MIDI, same port
    // 4) StartAudio
    // Daisy's HS device descriptor is always CDC; MIDI is a descriptor switch
    // on that stack. OG never inits MIDI without the logger bring-up first.
    ExternalLog::StartLog(false);
    hw.DelayMs(50);

    for (int r = 0; r < kRows; ++r) {
        row_gpio[r].Init(kRowsPins[r], GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL);
        row_gpio[r].Write(true);
    }
    for (int c = 0; c < kCols; ++c) {
        col_gpio[c].Init(kColsPins[c], GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
    }
    joy_sw.Init(PinMap::JOY_SW, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);

    adc_cfg[0].InitSingle(PinMap::JOY_X);
    adc_cfg[1].InitSingle(PinMap::JOY_Y);
    hw.adc.Init(adc_cfg, 2);
    hw.DelayMs(20);
    hw.adc.Start();

    OledDisplay<SSD130x4WireSpi128x64Driver>::Config dcfg;
    dcfg.driver_config.transport_config.spi_config.periph = SpiHandle::Config::Peripheral::SPI_1;
    dcfg.driver_config.transport_config.spi_config.baud_prescaler =
        SpiHandle::Config::BaudPrescaler::PS_8;
    dcfg.driver_config.transport_config.spi_config.pin_config.sclk = PinMap::OLED_SCK;
    dcfg.driver_config.transport_config.spi_config.pin_config.miso = Pin();
    dcfg.driver_config.transport_config.spi_config.pin_config.mosi = PinMap::OLED_MOSI;
    dcfg.driver_config.transport_config.spi_config.pin_config.nss = PinMap::OLED_CS;
    dcfg.driver_config.transport_config.pin_config.dc = PinMap::OLED_DC;
    dcfg.driver_config.transport_config.pin_config.reset = PinMap::OLED_RST;
    oled.Init(dcfg);
    oled.Fill(false);
    oled.Update();

    MidiUsbHandler::Config mcfg;
    mcfg.transport_config.periph = MidiUsbTransport::Config::EXTERNAL;
    mcfg.transport_config.tx_retry_count = 3;
    usb_midi.Init(mcfg);
    usb_midi.StartReceive();

    hw.StartAudio(AudioCallback);

    sounding.n = 0;
    last_voicing.n = 0;
    uint32_t last_draw = 0;
    (void)ShiftHeld;
    (void)joy_sw;

    while (1) {
        ScanMatrix();

        bool panic = PanicHeld();
        if (panic && !panic_was) Panic();
        panic_was = panic;

        key_arm = KeyHeld();

        usb_midi.Listen();
        int guard = 0;
        while (usb_midi.HasEvents() && guard++ < 64) {
            HandleIncoming(usb_midi.PopEvent());
        }

        bool want_chord = CurrentType() != oc::Type::None && held_n > 0;
        if (want_chord) {
            if (!chord_mode) {
                ThruOffHeld();
                chord_mode = true;
            }
            EmitChord();
        } else if (chord_mode) {
            NotesOff(sounding);
            sounding.n = 0;
            chord_mode = false;
            ThruOnHeld();
        }

        uint32_t now = System::GetNow();
        if (now - last_draw > 100) {
            Draw();
            last_draw = now;
        }

        hw.DelayMs(1);
    }
}
