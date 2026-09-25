#pragma once

#include <JuceHeader.h>

#include "MidiMap.h"
#include "session.h"

class OpenChordMCoreEditor;

class OpenChordMCoreProcessor : public juce::AudioProcessor {
public:
    OpenChordMCoreProcessor();
    ~OpenChordMCoreProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void toggleBind();
    bool bindOn() const { return bind_on_.load(); }
    int bindTarget() const { return bind_target_.load(); }
    void chooseBind(ocplug::ControlId id);
    void clearBinding(ocplug::ControlId id);
    void resetMap();

    void uiSetKeyswitch(int index, bool down);
    void uiSetButton(oc::Button button, bool down);
    void uiSetTrackpad(float x, float y, bool finger);
    void uiSetStrip(uint8_t position, bool finger);

    ocplug::Binding binding(ocplug::ControlId id) const;

    struct UiSnapshot {
        char    chord[20]{};
        char    key_name[8]{};
        char    screen_top[24]{};
        char    screen_left[16]{};
        char    screen_mid[16]{};
        char    screen_right[16]{};
        int     screen_zones = 0;
        int     screen_zone = -1;
        uint8_t key_pc = 0;
        float   track_x = 0.f;
        float   track_y = 0.f;
        bool    track_finger = false;
        uint8_t strip = 0;
        bool    strip_finger = false;
        uint8_t keyswitches = 0;
        uint8_t buttons = 0;
        bool    menu = false;
        int     menu_index = 0;
        uint8_t vary = 0;
        int     octave = 0;
        oc::Harmony harmony = oc::Harmony::InKey;
        oc::Trigger trigger = oc::Trigger::Optional;
        int     bind_target = -1;
        bool    bind_on = false;
        oc::PlayMode mode = oc::PlayMode::Keys;
    };
    UiSnapshot snapshot() const;
    bool strumWaiting() const;

private:
    void applyMessage(ocplug::ControlId id, bool on, uint8_t ccValue);
    void emitSessionMidi(juce::MidiBuffer& midi, int samplePos);
    static const char* pcName(uint8_t pc);
    static float ccToAxis(uint8_t value);

    oc::Session session_;
    ocplug::MidiMap map_;

    std::atomic<bool> bind_on_{false};
    std::atomic<int> bind_target_{-1};

    mutable juce::SpinLock map_lock_;
    ocplug::MidiMap map_rt_;

    mutable juce::SpinLock snap_lock_;
    UiSnapshot snap_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenChordMCoreProcessor)
};
