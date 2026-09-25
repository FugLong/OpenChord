#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstdio>

OpenChordMCoreProcessor::OpenChordMCoreProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    map_.resetPreset();
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

float OpenChordMCoreProcessor::ccToAxis(uint8_t value) {
    return (static_cast<float>(value) - 64.0f) / 64.0f;
}

void OpenChordMCoreProcessor::toggleBind() {
    if (bind_on_.load()) {
        bind_on_.store(false);
        bind_target_.store(-1);
        return;
    }
    bind_on_.store(true);
    bind_target_.store(-1);
}

void OpenChordMCoreProcessor::chooseBind(ocplug::ControlId id) {
    if (!bind_on_.load()) return;
    if (static_cast<int>(id) < 0 || id >= ocplug::ControlId::Count) return;
    bind_target_.store(static_cast<int>(id));
}

void OpenChordMCoreProcessor::clearBinding(ocplug::ControlId id) {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    map_.clear(id);
    map_rt_ = map_;
}

void OpenChordMCoreProcessor::resetMap() {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    map_.resetPreset();
    map_rt_ = map_;
}

ocplug::Binding OpenChordMCoreProcessor::binding(ocplug::ControlId id) const {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    return map_.get(id);
}

void OpenChordMCoreProcessor::uiSetKeyswitch(int index, bool down) {
    if (index < 0 || index >= oc::Session::kKeyswitchCount) return;
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    session_.setKeyswitch(static_cast<uint8_t>(index), down);
}

void OpenChordMCoreProcessor::uiSetButton(oc::Button button, bool down) {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    session_.setButton(button, down);
}

void OpenChordMCoreProcessor::uiSetTrackpad(float x, float y, bool finger) {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    session_.setTrackpad(x, y, finger);
}

void OpenChordMCoreProcessor::uiSetStrip(uint8_t position, bool finger) {
    const juce::SpinLock::ScopedLockType sl(map_lock_);
    session_.setStrip(position, finger);
}

void OpenChordMCoreProcessor::applyMessage(ocplug::ControlId id, bool on, uint8_t ccValue) {
    using Id = ocplug::ControlId;
    if (ocplug::IsKeyswitch(id)) {
        session_.setKeyswitch(static_cast<uint8_t>(ocplug::KeyswitchIndex(id)), on);
        return;
    }
    if (id == Id::Prev) {
        session_.setButton(oc::Button::Prev, on);
        return;
    }
    if (id == Id::Menu) {
        session_.setButton(oc::Button::Menu, on);
        return;
    }
    if (id == Id::Next) {
        session_.setButton(oc::Button::Next, on);
        return;
    }
    if (id == Id::TrackpadX || id == Id::TrackpadY) {
        float x = session_.trackpadX();
        float y = session_.trackpadY();
        const float axis = ccToAxis(ccValue);
        if (id == Id::TrackpadX) x = axis;
        else y = axis;
        session_.setTrackpad(x, y, true);
        return;
    }
    if (id == Id::Strip) {
        const int pos = (static_cast<int>(ccValue) * 255) / 127;
        session_.setStrip(static_cast<uint8_t>(pos), true);
    }
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

    const juce::SpinLock::ScopedLockType sl(map_lock_);
    ocplug::MidiMap local_map = map_rt_;

    for (const auto metadata : incoming) {
        const auto msg = metadata.getMessage();
        const int sample = metadata.samplePosition;
        const int ch = msg.getChannel();

        const int target = bind_target_.load();
        if (bind_on_.load() && target >= 0
            && (msg.isController() || (msg.isNoteOn() && msg.getVelocity() > 0))) {
            ocplug::Binding b;
            b.channel = static_cast<uint8_t>(ch);
            if (msg.isController()) {
                b.kind = ocplug::Binding::Kind::Cc;
                b.number = static_cast<uint8_t>(msg.getControllerNumber());
            } else {
                b.kind = ocplug::Binding::Kind::Note;
                b.number = static_cast<uint8_t>(msg.getNoteNumber());
            }
            map_.bindUnique(static_cast<ocplug::ControlId>(target), b);
            map_rt_ = map_;
            local_map = map_rt_;
            bind_on_.store(false);
            bind_target_.store(-1);
            continue;
        }

        if (msg.isController()) {
            const auto id = local_map.matchCc(static_cast<uint8_t>(ch),
                                              static_cast<uint8_t>(msg.getControllerNumber()));
            if (id != ocplug::ControlId::Count) {
                const uint8_t v = static_cast<uint8_t>(msg.getControllerValue());
                if (ocplug::IsAxis(id))
                    applyMessage(id, true, v);
                else
                    applyMessage(id, v >= 64, v);
                emitSessionMidi(midi, sample);
                continue;
            }
        }

        if (msg.isNoteOn() || msg.isNoteOff()) {
            const uint8_t pitch = static_cast<uint8_t>(msg.getNoteNumber());
            const auto id = local_map.matchNote(static_cast<uint8_t>(ch), pitch);
            const bool on = msg.isNoteOn() && msg.getVelocity() > 0;
            if (id != ocplug::ControlId::Count) {
                if (ocplug::IsAxis(id)) {
                    if (id == ocplug::ControlId::Strip)
                        session_.setStrip(session_.stripPosition(), on);
                    else
                        session_.setTrackpad(session_.trackpadX(), session_.trackpadY(), on);
                } else {
                    applyMessage(id, on, on ? 127 : 0);
                }
                emitSessionMidi(midi, sample);
                continue;
            }

            if (on)
                session_.noteOn(static_cast<uint8_t>(ch), pitch, static_cast<uint8_t>(msg.getVelocity()));
            else
                session_.noteOff(static_cast<uint8_t>(ch), pitch);
            emitSessionMidi(midi, sample);
        }
    }

    session_.updateSeat();
    emitSessionMidi(midi, 0);

    UiSnapshot s;
    session_.chordName(s.chord, sizeof(s.chord));
    session_.fillScreen(s.screen_top, sizeof(s.screen_top), s.screen_left, sizeof(s.screen_left),
                        s.screen_mid, sizeof(s.screen_mid), s.screen_right, sizeof(s.screen_right),
                        &s.screen_zones, &s.screen_zone);
    s.key_pc = session_.keyPc();
    std::snprintf(s.key_name, sizeof(s.key_name), "%s", pcName(s.key_pc));
    s.track_x = session_.trackpadX();
    s.track_y = session_.trackpadY();
    s.track_finger = session_.trackpadFinger();
    s.strip = session_.stripPosition();
    s.strip_finger = session_.stripFinger();
    s.menu = session_.menuOpen();
    s.menu_index = session_.menuIndex();
    s.vary = session_.vary();
    s.octave = session_.octave();
    s.harmony = session_.harmony();
    s.trigger = session_.trigger();
    s.mode = session_.playMode();
    s.bind_on = bind_on_.load();
    s.bind_target = bind_target_.load();
    uint8_t keys = 0;
    for (int i = 0; i < oc::Session::kKeyswitchCount; ++i) {
        if (session_.keyswitchDown(static_cast<uint8_t>(i)))
            keys = static_cast<uint8_t>(keys | (1u << i));
    }
    s.keyswitches = keys;
    uint8_t buttons = 0;
    if (session_.buttonDown(oc::Button::Prev)) buttons |= 1u;
    if (session_.buttonDown(oc::Button::Menu)) buttons |= 2u;
    if (session_.buttonDown(oc::Button::Next)) buttons |= 4u;
    s.buttons = buttons;
    {
        const juce::SpinLock::ScopedLockType snap(snap_lock_);
        snap_ = s;
    }
}

bool OpenChordMCoreProcessor::strumWaiting() const { return session_.strumWaiting(); }

OpenChordMCoreProcessor::UiSnapshot OpenChordMCoreProcessor::snapshot() const {
    const juce::SpinLock::ScopedLockType sl(snap_lock_);
    return snap_;
}

void OpenChordMCoreProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::MemoryOutputStream mos(destData, true);
    mos.writeInt(5);
    uint8_t blob[ocplug::MidiMap::kBlobBytes];
    {
        const juce::SpinLock::ScopedLockType sl(map_lock_);
        map_.toBlob(blob);
        mos.write(blob, sizeof(blob));
        mos.writeByte(static_cast<char>(session_.keyPc()));
        mos.writeByte(static_cast<char>(session_.playMode()));
        mos.writeByte(static_cast<char>(session_.vary()));
        mos.writeByte(static_cast<char>(session_.harmony()));
        mos.writeByte(static_cast<char>(session_.trigger()));
        mos.writeByte(static_cast<char>(session_.octave()));
    }
}

void OpenChordMCoreProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::MemoryInputStream mis(data, static_cast<size_t>(sizeInBytes), false);
    const int ver = mis.readInt();
    if (ver < 1) return;
    {
        const juce::SpinLock::ScopedLockType sl(map_lock_);
        if (ver >= 3) {
            uint8_t blob[ocplug::MidiMap::kBlobBytes];
            if (mis.read(blob, sizeof(blob)) != static_cast<int>(sizeof(blob))) return;
            map_.fromBlob(blob, sizeof(blob));
        } else {
            uint8_t blob[ocplug::MidiMap::kLegacyBlobBytes];
            if (mis.read(blob, sizeof(blob)) != static_cast<int>(sizeof(blob))) return;
            map_.fromLegacyBlob(blob, sizeof(blob));
        }
        map_rt_ = map_;
        if (!mis.isExhausted())
            session_.setKeyPc(static_cast<uint8_t>(mis.readByte()));
        if (ver >= 2 && !mis.isExhausted()) {
            const auto m = static_cast<uint8_t>(mis.readByte());
            oc::PlayMode mode = oc::PlayMode::Keys;
            if (m == 1) mode = oc::PlayMode::Scale;
            else if (m == 2 && ver >= 3) mode = oc::PlayMode::Drums;
            session_.setPlayMode(mode);
        }
        if (ver >= 4 && !mis.isExhausted())
            session_.setVary(static_cast<uint8_t>(mis.readByte()));
        if (ver >= 4 && !mis.isExhausted())
            session_.setHarmony(static_cast<oc::Harmony>(mis.readByte()));
        if (ver >= 4 && !mis.isExhausted())
            session_.setTrigger(static_cast<oc::Trigger>(mis.readByte()));
        if (ver >= 5 && !mis.isExhausted())
            session_.setOctave(static_cast<int>(static_cast<int8_t>(mis.readByte())));
    }
}

juce::AudioProcessorEditor* OpenChordMCoreProcessor::createEditor() {
    return new OpenChordMCoreEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OpenChordMCoreProcessor();
}
