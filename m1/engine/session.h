#pragma once

#include "chord_engine.h"

#include <cstddef>
#include <cstdint>

namespace oc {

// Portable session. Hosts call the input surface below.
// No Arduino, Pico, or JUCE headers here.

enum class Button : uint8_t { Prev = 0, Menu = 1, Next = 2 };

enum class Harmony : uint8_t { InKey = 0, Tensions, Borrowed, Free };

enum class Trigger : uint8_t {
    Optional = 0, // chord or note sounds; the strip adds plucks
    Strip = 1     // quiet until the strip; same in Keys and Scale
};

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

    // Input surface. A finger on the box and a click in the plugin both land here.
    // Keyswitch index 0..7 is the near row first, left to right.
    static constexpr int kKeyswitchCount = 8;
    static constexpr int kMenuSettings = 5;
    void setKeyswitch(uint8_t index, bool down);
    void setButton(Button button, bool down);
    void setTrackpad(float x, float y, bool finger);
    void setStrip(uint8_t position, bool finger);

    bool keyswitchDown(uint8_t index) const;
    bool buttonDown(Button button) const;
    bool menuOpen() const { return menu_open_; }
    int menuIndex() const { return menu_index_; }
    uint8_t vary() const { return vary_; }
    Harmony harmony() const { return harmony_; }
    Trigger trigger() const { return trigger_; }
    int octave() const { return octave_; }
    void setOctave(int oct);
    void setVary(uint8_t v) { vary_ = v > 7 ? 7 : v; }
    void setHarmony(Harmony h) { if (static_cast<uint8_t>(h) <= 3) harmony_ = h; }
    void setTrigger(Trigger t) { if (static_cast<uint8_t>(t) <= 1) trigger_ = t; }
    bool trackpadFinger() const { return track_finger_; }
    float trackpadX() const { return stick_x_; }
    float trackpadY() const { return stick_y_; }
    uint8_t stripPosition() const { return strip_; }
    bool stripFinger() const { return strip_finger_; }

    // Older call names. Keys Live and Scale-hold still go through these.
    // Keys mode: idx 0=Dim .. 3=Sus
    void typePad(uint8_t idx, bool on);
    void setExtBit(uint8_t bit, bool on);

    // Scale mode: idx 0..6 = I..vii, 7 = I one octave up. Hold while pressed.
    void degreePad(uint8_t idx, bool on);

    // Older CC path. Ignores the first value until it passes near center.
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
    uint8_t  degreeHeldMask() const; // bits 0..7 for held Scale keyswitches
    uint8_t  padExt() const { return pad_ext_; }
    float    stickX() const { return stick_x_; }
    float    stickY() const { return stick_y_; }
    uint8_t  keyPc() const { return key_pc_; }
    bool     keyHeld() const { return key_held_; }
    Seat     seat() const { return last_seat_; }
    uint8_t  outChannel() const { return out_ch_; }

    void chordName(char* buf, size_t cap) const;
    bool strumWaiting() const;
    // Two OLED lines plus strip-zone marks. Plugin and firmware both draw this.
    // Top is centered. The bottom is three slots: left, middle, right.
    void fillScreen(char* top, size_t topCap, char* left, size_t leftCap, char* mid,
                    size_t midCap, char* right, size_t rightCap, int* zoneCount, int* zone) const;

private:
    struct Voice {
        int16_t  root; // -1 = empty; Keys thru/chord root, or Scale degree root
        bool     chord;
        bool     scale; // true = Scale-mode degree voice (released via degreePad)
        uint8_t  degree_idx; // 0..7 when scale
        Type     type;
        uint8_t  ext;
        Voicing  v;
    };

    void midiSendOn(uint8_t note, uint8_t vel, uint8_t channel = 0);
    void midiSendOff(uint8_t note, uint8_t channel = 0);
    void pushOut(MidiEvent::Kind kind, uint8_t d1, uint8_t d2, uint8_t channel);
    void drumHit(uint8_t index, bool down);
    void voicingOff(const Voicing& v);
    void voicingOn(const Voicing& v, uint8_t vel, bool generated);
    void diffVoicing(Voicing& cur, const Voicing& next, uint8_t vel, bool generated);
    void varyVelocities(uint8_t base, int count, uint8_t* out);
    void renderVoice(Voice& h);
    void applyInKeyColor(const Voice& h, EngineInput& in) const;
    Voice* findVoice(int16_t root);
    Voice* findScaleDegree(uint8_t degree_idx);
    const Voice* findScaleDegree(uint8_t degree_idx) const;
    bool scaleIsCurrent(const Voice& h) const;
    void quietScale(Voice& h);
    void followScale(uint8_t idx, bool on);
    Voice* allocVoice();
    bool typeStillHeld(Type t) const;
    void refreshHeldChordExts();
    void stickCc(bool* armed, float* axis, uint8_t value);
    void clearAllVoices();
    void syncScaleTonic();
    void applyKeyswitchMusic(uint8_t index, bool down);
    void applyHeldKeyswitches();
    void applyMenuX(float x);
    Seat effectiveSeat() const;
    void releaseStrip();
    void soundStripZone(const Voicing& recipe, int zone);
    const Voicing* stripRecipe() const;
    void buildStrumArm(uint8_t pitch);
    void refreshStrumArm();

    PlayMode mode_ = PlayMode::Keys;

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
    uint8_t  degree_order_[8]{}; // oldest first; the last one is what sounds
    uint8_t  degree_order_n_ = 0;
    float    stick_x_ = 0.f;
    float    stick_y_ = 0.f;
    bool     stick_x_armed_ = false;
    bool     stick_y_armed_ = false;
    bool     track_finger_ = false;
    bool     key_[kKeyswitchCount]{};
    bool     drum_on_[kKeyswitchCount]{};
    uint8_t  drum_note_[kKeyswitchCount]{};
    bool     drum_right_[kKeyswitchCount]{};
    uint8_t  drum_recent_[kKeyswitchCount]{};
    uint8_t  drum_recent_n_ = 0;
    bool     button_[3]{};
    bool     menu_open_ = false;
    int      menu_index_ = 0;
    uint8_t  vary_ = 4;
    int8_t   octave_ = 0;
    uint32_t vary_rng_ = 1;
    Harmony  harmony_ = Harmony::InKey;
    Trigger  trigger_ = Trigger::Optional;
    uint8_t  strip_ = 0;
    bool     strip_finger_ = false;
    int      strip_zone_ = -1;
    uint8_t  strip_notes_[8]{};
    bool     strip_owned_[8]{};
    uint8_t  strip_n_ = 0;
    int16_t  arm_root_ = -1;
    Voicing  arm_v_{};
    Seat     last_seat_ = Seat::Home;

    MidiEvent out_q_[kOutCap];
    int       out_n_ = 0;
};

const char* drumName(int index, bool right);

} // namespace oc
