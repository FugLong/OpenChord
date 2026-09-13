#pragma once

#include "chord_engine.h"

#include <cstddef>
#include <cstdint>

namespace oc {

// Portable session for Pro + Smart play modes.
// Used by the RP2040 proto and the JUCE plugin. No Arduino / JUCE here.

struct MidiEvent {
    enum class Kind : uint8_t { NoteOn, NoteOff, Cc };
    Kind    kind;
    uint8_t channel; // 1-16
    uint8_t data1;
    uint8_t data2;
};

class Session {
public:
    static constexpr int kMaxVoices = 16;
    static constexpr int kOutCap = 96;

    Session();

    void reset();
    void panic();

    void setPlayMode(PlayMode mode);
    PlayMode playMode() const { return mode_; }

    void noteOn(uint8_t channel, uint8_t pitch, uint8_t velocity);
    void noteOff(uint8_t channel, uint8_t pitch);

    // Pro mode: idx 0=Dim .. 3=Sus
    void typePad(uint8_t idx, bool on);
    void setExtBit(uint8_t bit, bool on);

    // Smart mode: idx 0..6 = I..vii, 7 = high I. Hold while pressed.
    void degreePad(uint8_t idx, bool on);

    // Stick: CC path has Launchkey-style pickup (ignore until 48–80 once).
    void stickCcX(uint8_t value);
    void stickCcY(uint8_t value);
    void setStickX(float x);
    void setStickY(float y);

    void setKeyHeld(bool held);
    void setKeyPc(uint8_t pc);

    void updateSeat();

    int drain(MidiEvent* out, int max);

    Type     heldType() const;
    uint8_t  typeHeldMask() const;
    uint8_t  degreeHeldMask() const; // bits 0..7 for degree pads held
    uint8_t  padExt() const { return pad_ext_; }
    float    stickX() const { return stick_x_; }
    float    stickY() const { return stick_y_; }
    uint8_t  keyPc() const { return key_pc_; }
    bool     keyHeld() const { return key_held_; }
    Seat     seat() const { return last_seat_; }
    uint8_t  outChannel() const { return out_ch_; }

    void chordName(char* buf, size_t cap) const;

private:
    struct Voice {
        int16_t  root; // -1 = empty; Pro thru/chord root, or Smart degree root
        bool     chord;
        bool     scale; // true = Smart-mode degree voice (released via degreePad)
        uint8_t  degree_idx; // 0..7 when scale
        Type     type;
        uint8_t  ext;
        Voicing  v;
    };

    void midiSendOn(uint8_t note, uint8_t vel);
    void midiSendOff(uint8_t note);
    void pushOut(MidiEvent::Kind kind, uint8_t d1, uint8_t d2);
    void voicingOff(const Voicing& v);
    void voicingOn(const Voicing& v, uint8_t vel);
    void diffVoicing(Voicing& cur, const Voicing& next, uint8_t vel);
    void renderVoice(Voice& h);
    Voice* findVoice(int16_t root);
    Voice* findScaleDegree(uint8_t degree_idx);
    Voice* allocVoice();
    bool typeStillHeld(Type t) const;
    void refreshHeldChordExts();
    void stickCc(bool* armed, float* axis, uint8_t value);
    void clearAllVoices();
    void syncScaleTonic();

    PlayMode mode_ = PlayMode::Pro;

    Voice    voices_[kMaxVoices];
    uint8_t  note_refs_[128];
    uint8_t  held_vel_ = 100;
    uint8_t  out_ch_ = 1;
    uint8_t  key_pc_ = 0;
    bool     key_held_ = false;
    int16_t  scale_tonic_midi_ = 48; // I in chosen octave (default C3)

    uint8_t  type_stack_[4];
    uint8_t  type_stack_n_ = 0;
    uint8_t  pad_ext_ = 0;
    uint8_t  degree_held_ = 0; // bits 0..7
    float    stick_x_ = 0.f;
    float    stick_y_ = 0.f;
    bool     stick_x_armed_ = false;
    bool     stick_y_armed_ = false;
    Seat     last_seat_ = Seat::Home;

    MidiEvent out_q_[kOutCap];
    int       out_n_ = 0;
};

} // namespace oc
