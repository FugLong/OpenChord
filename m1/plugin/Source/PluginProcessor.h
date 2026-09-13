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

    // --- UI / learn (message thread or atomics) ---
    ocplug::MidiMap& map() { return map_; }
    const ocplug::MidiMap& map() const { return map_; }

    void armLearn(ocplug::ControlId id);
    void cancelLearn();
    int  learnArmed() const { return learn_armed_.load(); } // -1 = none

    void uiHoldControl(ocplug::ControlId id, bool held);
    void uiSetStick(float x, float y);
    void resetMapToLaunchkey();
    void clearBinding(ocplug::ControlId id);

    struct UiSnapshot {
        char    chord[20]{};
        char    key_name[8]{};
        uint8_t key_pc = 0;
        float   stick_x = 0.f;
        float   stick_y = 0.f;
        uint8_t type_mask = 0; // bits 0-3 Dim..Sus held
        uint8_t ext = 0;
        bool    key_held = false;
        bool    shift_held = false;
        bool    panic_held = false;
        int     learn_armed = -1;
    };
    UiSnapshot snapshot() const;

private:
    void applyControl(ocplug::ControlId id, bool on, uint8_t ccValue);
    void setControlHeld(ocplug::ControlId id, bool on);
    void emitSessionMidi(juce::MidiBuffer& midi, int samplePos);
    static const char* pcName(uint8_t pc);

    oc::Session session_;
    ocplug::MidiMap map_;

    std::atomic<int> learn_armed_{-1};
    std::atomic<uint32_t> learn_deadline_ms_{0};
    std::atomic<uint16_t> controls_held_{0}; // bit per ControlId

    // Copy of map for audio thread; swapped under lock only on state/UI map edits.
    juce::SpinLock map_lock_;
    ocplug::MidiMap map_rt_;

    mutable juce::SpinLock snap_lock_;
    UiSnapshot snap_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenChordMCoreProcessor)
};
