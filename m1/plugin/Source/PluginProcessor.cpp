#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstdio>

namespace {
constexpr uint32_t kLearnTimeoutMs = 8000;
}

OpenChordMCoreProcessor::OpenChordMCoreProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    map_.resetToLaunchkey();
    map_rt_ = map_;
    session_.reset();
}

OpenChordMCoreProcessor::~OpenChordMCoreProcessor() = default;

void OpenChordMCoreProcessor::prepareToPlay(double, int) {}
void OpenChordMCoreProcessor::releaseResources() {}

bool OpenChordMCoreProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;
    return true;
}

const char* OpenChordMCoreProcessor::pcName(uint8_t pc) {
    static const char* kNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return kNames[pc % 12];
}

void OpenChordMCoreProcessor::armLearn(ocplug::ControlId id) {
    learn_armed_.store(static_cast<int>(id));
    learn_deadline_ms_.store(juce::Time::getMillisecondCounter() + kLearnTimeoutMs);
}

void OpenChordMCoreProcessor::cancelLearn() {
    learn_armed_.store(-1);
}

void OpenChordMCoreProcessor::resetMapToLaunchkey() {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    map_.resetToLaunchkey();
    map_rt_ = map_;
}

void OpenChordMCoreProcessor::clearBinding(ocplug::ControlId id) {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    map_.clear(id);
    map_rt_ = map_;
}

void OpenChordMCoreProcessor::setPlayMode(oc::PlayMode mode) {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    session_.setPlayMode(mode);
    controls_held_.store(0);
}

oc::PlayMode OpenChordMCoreProcessor::playMode() const {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    return session_.playMode();
}

void OpenChordMCoreProcessor::setControlHeld(ocplug::ControlId id, bool on) {
    using Id = ocplug::ControlId;
    if (id == Id::StickX || id == Id::StickY || id == Id::Count) return;
    const auto bit = static_cast<uint16_t>(1u << static_cast<int>(id));
    if (on) controls_held_.fetch_or(bit);
    else controls_held_.fetch_and(static_cast<uint16_t>(~bit));
}

void OpenChordMCoreProcessor::applyControl(ocplug::ControlId id, bool on, uint8_t ccValue) {
    using Id = ocplug::ControlId;
    setControlHeld(id, on);

    if (id == Id::StickX) {
        session_.stickCcX(ccValue);
        return;
    }
    if (id == Id::StickY) {
        session_.stickCcY(ccValue);
        return;
    }
    if (id == Id::Key) {
        session_.setKeyHeld(on);
        return;
    }
    if (id == Id::Panic) {
        if (on) session_.panic();
        return;
    }
    if (id == Id::Shift) return;

    // Eight pads: Pro = triad/extras, Smart = I..vii + high I
    if (session_.playMode() == oc::PlayMode::Smart) {
        int deg = -1;
        switch (id) {
            case Id::Dim: deg = 0; break;
            case Id::Min: deg = 1; break;
            case Id::Maj: deg = 2; break;
            case Id::Sus: deg = 3; break;
            case Id::Ext6: deg = 4; break;
            case Id::Extm7: deg = 5; break;
            case Id::ExtM7: deg = 6; break;
            case Id::Ext9: deg = 7; break;
            default: break;
        }
        if (deg >= 0) session_.degreePad(static_cast<uint8_t>(deg), on);
        return;
    }

    switch (id) {
        case Id::Dim: session_.typePad(0, on); break;
        case Id::Min: session_.typePad(1, on); break;
        case Id::Maj: session_.typePad(2, on); break;
        case Id::Sus: session_.typePad(3, on); break;
        case Id::Ext6: session_.setExtBit(oc::Ext6, on); break;
        case Id::Extm7: session_.setExtBit(oc::Extm7, on); break;
        case Id::ExtM7: session_.setExtBit(oc::ExtM7, on); break;
        case Id::Ext9: session_.setExtBit(oc::Ext9, on); break;
        default: break;
    }
}

void OpenChordMCoreProcessor::uiHoldControl(ocplug::ControlId id, bool held) {
    // Called from message thread; session is also used on audio thread.
    // Brief says realtime: no locks on audio. For mouse UI we take the map lock
    // and apply to session — same as many JUCE MIDI FX demos. Keep it short.
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    applyControl(id, held, held ? 127 : 0);
    session_.updateSeat();
}

void OpenChordMCoreProcessor::uiSetStick(float x, float y) {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    session_.setStickX(x);
    session_.setStickY(y);
    session_.updateSeat();
}

void OpenChordMCoreProcessor::emitSessionMidi(juce::MidiBuffer& midi, int samplePos) {
    oc::MidiEvent ev[oc::Session::kOutCap];
    int n = session_.drain(ev, oc::Session::kOutCap);
    for (int i = 0; i < n; ++i) {
        const auto& e = ev[i];
        juce::MidiMessage m;
        switch (e.kind) {
            case oc::MidiEvent::Kind::NoteOn:
                m = juce::MidiMessage::noteOn(e.channel, e.data1, e.data2);
                break;
            case oc::MidiEvent::Kind::NoteOff:
                m = juce::MidiMessage::noteOff(e.channel, e.data1);
                break;
            case oc::MidiEvent::Kind::Cc:
                m = juce::MidiMessage::controllerEvent(e.channel, e.data1, e.data2);
                break;
        }
        midi.addEvent(m, samplePos);
    }
}

void OpenChordMCoreProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    buffer.clear();

    juce::MidiBuffer incoming;
    incoming.swapWith(midi);

    const int armed = learn_armed_.load();
    if (armed >= 0) {
        const uint32_t now = juce::Time::getMillisecondCounter();
        if (now > learn_deadline_ms_.load())
            learn_armed_.store(-1);
    }

    ocplug::MidiMap local_map;
    {
        const juce::SpinLock::ScopedLockType sl(map_lock_);
        local_map = map_rt_;
    }

    for (const auto metadata : incoming) {
        const auto msg = metadata.getMessage();
        const int sample = metadata.samplePosition;
        const int ch = msg.getChannel();

        // Learn arm
        int arm = learn_armed_.load();
        if (arm >= 0) {
            const auto id = static_cast<ocplug::ControlId>(arm);
            const bool isStick = (id == ocplug::ControlId::StickX || id == ocplug::ControlId::StickY);

            if (msg.isController()) {
                ocplug::Binding b;
                b.kind = ocplug::Binding::Kind::Cc;
                b.channel = static_cast<uint8_t>(ch);
                b.number = static_cast<uint8_t>(msg.getControllerNumber());
                {
                    const juce::SpinLock::ScopedLockType sl(map_lock_);
                    map_.bindUnique(id, b);
                    map_rt_ = map_;
                    local_map = map_rt_;
                }
                learn_armed_.store(-1);
                continue;
            }
            if (!isStick && msg.isNoteOn() && msg.getVelocity() > 0) {
                ocplug::Binding b;
                b.kind = ocplug::Binding::Kind::Note;
                b.channel = static_cast<uint8_t>(ch);
                b.number = static_cast<uint8_t>(msg.getNoteNumber());
                {
                    const juce::SpinLock::ScopedLockType sl(map_lock_);
                    map_.bindUnique(id, b);
                    map_rt_ = map_;
                    local_map = map_rt_;
                }
                learn_armed_.store(-1);
                continue;
            }
        }

        if (msg.isController()) {
            const auto id = local_map.matchCc(static_cast<uint8_t>(ch),
                                              static_cast<uint8_t>(msg.getControllerNumber()));
            if (id != ocplug::ControlId::Count) {
                const uint8_t v = static_cast<uint8_t>(msg.getControllerValue());
                if (id == ocplug::ControlId::StickX || id == ocplug::ControlId::StickY)
                    applyControl(id, true, v);
                else
                    applyControl(id, v >= 64, v);
                session_.updateSeat();
                emitSessionMidi(midi, sample);
                continue;
            }
        }

        if (msg.isNoteOn() || msg.isNoteOff()) {
            const uint8_t pitch = static_cast<uint8_t>(msg.getNoteNumber());
            const auto id = local_map.matchNote(static_cast<uint8_t>(ch), pitch);
            if (id != ocplug::ControlId::Count) {
                if (id == ocplug::ControlId::StickX || id == ocplug::ControlId::StickY) {
                    // Stick axes only bind to CC (spec). Ignore note match.
                } else {
                    const bool on = msg.isNoteOn() && msg.getVelocity() > 0;
                    applyControl(id, on, on ? 127 : 0);
                    session_.updateSeat();
                    emitSessionMidi(midi, sample);
                    continue;
                }
            }

            // Default Launchkey: ignore ch 10 notes (pads) when unbound as controls.
            if (ch == 10 && msg.isNoteOn()) {
                // Prefer: if unbound, still skip drum-channel notes on default map.
                continue;
            }

            if (msg.isNoteOn() && msg.getVelocity() > 0)
                session_.noteOn(static_cast<uint8_t>(ch), pitch, static_cast<uint8_t>(msg.getVelocity()));
            else
                session_.noteOff(static_cast<uint8_t>(ch), pitch);
            emitSessionMidi(midi, sample);
            continue;
        }
    }

    session_.updateSeat();
    emitSessionMidi(midi, 0);

    // UI snapshot
    UiSnapshot s;
    session_.chordName(s.chord, sizeof(s.chord));
    s.key_pc = session_.keyPc();
    std::snprintf(s.key_name, sizeof(s.key_name), "%s maj", pcName(s.key_pc));
    s.stick_x = session_.stickX();
    s.stick_y = session_.stickY();
    s.ext = session_.padExt();
    s.type_mask = session_.typeHeldMask();
    s.degree_mask = session_.degreeHeldMask();
    s.mode = session_.playMode();
    s.learn_armed = learn_armed_.load();
    {
        const uint16_t held = controls_held_.load();
        using Id = ocplug::ControlId;
        s.key_held = (held & (1u << static_cast<int>(Id::Key))) != 0 || session_.keyHeld();
        s.shift_held = (held & (1u << static_cast<int>(Id::Shift))) != 0;
        s.panic_held = (held & (1u << static_cast<int>(Id::Panic))) != 0;
        if (s.mode == oc::PlayMode::Pro) {
            const uint8_t midi_types = static_cast<uint8_t>(
                ((held >> static_cast<int>(Id::Dim)) & 1u)
                | (((held >> static_cast<int>(Id::Min)) & 1u) << 1)
                | (((held >> static_cast<int>(Id::Maj)) & 1u) << 2)
                | (((held >> static_cast<int>(Id::Sus)) & 1u) << 3));
            if (midi_types) s.type_mask = static_cast<uint8_t>(s.type_mask | midi_types);
            uint8_t midi_ext = 0;
            if (held & (1u << static_cast<int>(Id::Ext6))) midi_ext |= oc::Ext6;
            if (held & (1u << static_cast<int>(Id::Extm7))) midi_ext |= oc::Extm7;
            if (held & (1u << static_cast<int>(Id::ExtM7))) midi_ext |= oc::ExtM7;
            if (held & (1u << static_cast<int>(Id::Ext9))) midi_ext |= oc::Ext9;
            if (midi_ext) s.ext = static_cast<uint8_t>(s.ext | midi_ext);
        } else {
            uint8_t deg = 0;
            if (held & (1u << static_cast<int>(Id::Dim))) deg |= 1u << 0;
            if (held & (1u << static_cast<int>(Id::Min))) deg |= 1u << 1;
            if (held & (1u << static_cast<int>(Id::Maj))) deg |= 1u << 2;
            if (held & (1u << static_cast<int>(Id::Sus))) deg |= 1u << 3;
            if (held & (1u << static_cast<int>(Id::Ext6))) deg |= 1u << 4;
            if (held & (1u << static_cast<int>(Id::Extm7))) deg |= 1u << 5;
            if (held & (1u << static_cast<int>(Id::ExtM7))) deg |= 1u << 6;
            if (held & (1u << static_cast<int>(Id::Ext9))) deg |= 1u << 7;
            if (deg) s.degree_mask = static_cast<uint8_t>(s.degree_mask | deg);
        }
    }
    {
        const juce::SpinLock::ScopedLockType sl(snap_lock_);
        snap_ = s;
    }
}

OpenChordMCoreProcessor::UiSnapshot OpenChordMCoreProcessor::snapshot() const {
    const juce::SpinLock::ScopedLockType sl(snap_lock_);
    return snap_;
}

void OpenChordMCoreProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::MemoryOutputStream mos(destData, true);
    mos.writeInt(2); // version
    uint8_t blob[ocplug::MidiMap::kBlobBytes];
    {
        const juce::SpinLock::ScopedLockType sl(map_lock_);
        map_.toBlob(blob);
        mos.write(blob, sizeof(blob));
        mos.writeByte(session_.keyPc());
        mos.writeByte(static_cast<char>(session_.playMode()));
    }
}

void OpenChordMCoreProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::MemoryInputStream mis(data, static_cast<size_t>(sizeInBytes), false);
    const int ver = mis.readInt();
    if (ver < 1) return;
    uint8_t blob[ocplug::MidiMap::kBlobBytes];
    if (mis.read(blob, sizeof(blob)) != static_cast<int>(sizeof(blob))) return;
    {
        const juce::SpinLock::ScopedLockType sl(map_lock_);
        map_.fromBlob(blob, sizeof(blob));
        map_rt_ = map_;
        if (!mis.isExhausted())
            session_.setKeyPc(static_cast<uint8_t>(mis.readByte()));
        if (ver >= 2 && !mis.isExhausted()) {
            const auto m = static_cast<uint8_t>(mis.readByte());
            session_.setPlayMode(m == 1 ? oc::PlayMode::Smart : oc::PlayMode::Pro);
        }
    }
}

juce::AudioProcessorEditor* OpenChordMCoreProcessor::createEditor() {
    return new OpenChordMCoreEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OpenChordMCoreProcessor();
}
