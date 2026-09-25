#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ClickButton : public juce::TextButton {
public:
    std::function<void()> onDown;
    std::function<void()> onUp;
    std::function<void()> onRightClick;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            if (onRightClick) onRightClick();
            return;
        }
        juce::TextButton::mouseDown(e);
        held_ = true;
        if (onDown) onDown();
    }

    void mouseUp(const juce::MouseEvent& e) override {
        juce::TextButton::mouseUp(e);
        if (!held_) return;
        held_ = false;
        if (onUp) onUp();
    }

private:
    bool held_ = false;
};

class TrackpadControl : public juce::Component {
public:
    std::function<void(float, float, bool)> onChange;
    std::function<bool()> bindOn;

    void setReading(float x, float y, bool finger);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    void setFrom(juce::Point<float> p);
    float x_ = 0.f;
    float y_ = 0.f;
    bool finger_ = false;
    bool dragging_ = false;
};

class StripControl : public juce::Component {
public:
    std::function<void(uint8_t, bool)> onChange;
    std::function<bool()> bindOn;
    std::function<void()> onBind;
    std::function<void()> onClear;

    void setReading(uint8_t position, bool finger);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    uint8_t positionFrom(float x) const;
    uint8_t pos_ = 0;
    bool finger_ = false;
    bool dragging_ = false;
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
    void refresh();
    bool latchHeld() const;
    void releaseLatches();
    void wireKeyswitch(ClickButton& b, int index);
    void wireButton(ClickButton& b, oc::Button button, ocplug::ControlId id);
    static void paintLatch(juce::Button& b, bool lit, bool armed);
    static const char* keyswitchLabel(oc::PlayMode mode, int index, bool drumRight);

    OpenChordMCoreProcessor& proc_;

    juce::Label title_;
    juce::Label hint_;
    char screen_top_[24]{};
    char screen_left_[16]{};
    char screen_mid_[16]{};
    char screen_right_[16]{};
    int screen_zones_ = 0;
    int screen_zone_ = -1;
    juce::TextButton bind_btn_{"Bind"};
    juce::TextButton latch_btn_{"Latch"};
    juce::TextButton reset_btn_{"Reset"};
    bool latchOn_ = false;
    uint8_t latchedKeys_ = 0;
    uint8_t latchedButtons_ = 0;

    ClickButton key_[8];
    ClickButton prev_, menu_, next_;
    ClickButton track_x_, track_y_;
    TrackpadControl trackpad_;
    StripControl strip_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenChordMCoreEditor)
};
