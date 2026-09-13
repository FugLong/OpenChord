#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Momentary pad: short click = MIDI Learn arm; press-and-hold = Type/extra held.
class HoldButton : public juce::TextButton, private juce::Timer {
public:
    std::function<void(bool)> onHold;
    std::function<void()> onArmLearn;
    std::function<void()> onRightClick;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            if (onRightClick) onRightClick();
            return;
        }
        pressed_ = true;
        held_sent_ = false;
        startTimer(220);
        juce::TextButton::mouseDown(e);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        stopTimer();
        juce::TextButton::mouseUp(e);
        if (!pressed_) return;
        pressed_ = false;
        if (held_sent_) {
            if (onHold) onHold(false);
        } else {
            if (onArmLearn) onArmLearn();
        }
    }

    void mouseExit(const juce::MouseEvent& e) override {
        // Keep hold if dragged off while pressed — mouseUp still ends it.
        juce::TextButton::mouseExit(e);
    }

private:
    void timerCallback() override {
        stopTimer();
        if (!pressed_ || held_sent_) return;
        held_sent_ = true;
        if (onHold) onHold(true);
    }

    bool pressed_ = false;
    bool held_sent_ = false;
};

class OpenChordMCoreEditor : public juce::AudioProcessorEditor,
                          private juce::Timer {
public:
    explicit OpenChordMCoreEditor(OpenChordMCoreProcessor&);
    ~OpenChordMCoreEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void timerCallback() override;
    void refreshLabels();
    void updateButtons();
    void wireButton(HoldButton& b, ocplug::ControlId id);
    static void setPadLit(HoldButton& b, bool lit, bool armed);

    OpenChordMCoreProcessor& proc_;

    juce::Label title_;
    juce::Label preset_;
    juce::TextButton reset_map_{"Reset map"};
    juce::Label key_label_;
    juce::Label chord_label_;
    juce::Label mode_label_;
    juce::Label learn_hint_;
    juce::Label degree_disabled_{"Degree (later)"};

    HoldButton dim_, min_, maj_, sus_;
    HoldButton e6_, em7_, eM7_, e9_;
    HoldButton key_btn_, panic_btn_, shift_btn_;

    juce::TextButton stick_x_learn_{"Stick X"};
    juce::TextButton stick_y_learn_{"Stick Y"};
    juce::Slider stick_x_;
    juce::Slider stick_y_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenChordMCoreEditor)
};
