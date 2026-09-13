#include "PluginEditor.h"

OpenChordMCoreEditor::OpenChordMCoreEditor(OpenChordMCoreProcessor& p)
    : AudioProcessorEditor(&p), proc_(p)
{
    setSize(520, 420);
    setWantsKeyboardFocus(true);

    auto style = [](juce::Label& l, float size, bool bold = false) {
        l.setJustificationType(juce::Justification::centredLeft);
        l.setFont(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
        l.setInterceptsMouseClicks(false, false);
    };

    title_.setText("OpenChord M Core", juce::dontSendNotification);
    style(title_, 18.0f, true);
    addAndMakeVisible(title_);

    preset_.setText("[Launchkey Mini MK4]", juce::dontSendNotification);
    style(preset_, 13.0f);
    addAndMakeVisible(preset_);

    reset_map_.onClick = [this] {
        proc_.resetMapToLaunchkey();
        updateButtons();
    };
    addAndMakeVisible(reset_map_);

    style(key_label_, 14.0f);
    style(chord_label_, 14.0f);
    style(mode_label_, 14.0f, true);
    mode_label_.setText("Type", juce::dontSendNotification);
    addAndMakeVisible(key_label_);
    addAndMakeVisible(chord_label_);
    addAndMakeVisible(mode_label_);

    degree_disabled_.setEnabled(false);
    degree_disabled_.setColour(juce::Label::textColourId, juce::Colours::grey);
    style(degree_disabled_, 12.0f);
    addAndMakeVisible(degree_disabled_);

    learn_hint_.setText(
        "Learn: click a button, then press pad / move knob. Hold button to play. Right-click clears. Esc cancels.",
        juce::dontSendNotification);
    style(learn_hint_, 12.0f);
    addAndMakeVisible(learn_hint_);

    wireButton(dim_, ocplug::ControlId::Dim);
    wireButton(min_, ocplug::ControlId::Min);
    wireButton(maj_, ocplug::ControlId::Maj);
    wireButton(sus_, ocplug::ControlId::Sus);
    wireButton(e6_, ocplug::ControlId::Ext6);
    wireButton(em7_, ocplug::ControlId::Extm7);
    wireButton(eM7_, ocplug::ControlId::ExtM7);
    wireButton(e9_, ocplug::ControlId::Ext9);
    wireButton(key_btn_, ocplug::ControlId::Key);
    wireButton(panic_btn_, ocplug::ControlId::Panic);
    wireButton(shift_btn_, ocplug::ControlId::Shift);

    auto setupStick = [this](juce::Slider& s, juce::TextButton& learnBtn, ocplug::ControlId id) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 18);
        s.setRange(-1.0, 1.0, 0.01);
        s.setValue(0.0);
        addAndMakeVisible(s);
        addAndMakeVisible(learnBtn);
        learnBtn.onClick = [this, id] {
            if (proc_.learnArmed() == static_cast<int>(id))
                proc_.cancelLearn();
            else
                proc_.armLearn(id);
            updateButtons();
        };
        s.onValueChange = [this] {
            proc_.uiSetStick(static_cast<float>(stick_x_.getValue()),
                             static_cast<float>(stick_y_.getValue()));
        };
    };
    setupStick(stick_x_, stick_x_learn_, ocplug::ControlId::StickX);
    setupStick(stick_y_, stick_y_learn_, ocplug::ControlId::StickY);

    updateButtons();
    startTimerHz(30);
}

OpenChordMCoreEditor::~OpenChordMCoreEditor() { stopTimer(); }

void OpenChordMCoreEditor::wireButton(HoldButton& b, ocplug::ControlId id) {
    addAndMakeVisible(b);
    b.setClickingTogglesState(false);
    b.onRightClick = [this, id] {
        proc_.clearBinding(id);
        updateButtons();
    };
    b.onArmLearn = [this, id] {
        if (proc_.learnArmed() == static_cast<int>(id))
            proc_.cancelLearn();
        else
            proc_.armLearn(id);
        updateButtons();
    };
    b.onHold = [this, id](bool held) { proc_.uiHoldControl(id, held); };
}

void OpenChordMCoreEditor::setPadLit(HoldButton& b, bool lit, bool armed) {
    b.setToggleState(lit || armed, juce::dontSendNotification);
    if (armed) {
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::gold);
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        b.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgoldenrod);
    } else if (lit) {
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3ecf8e));
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a9d6a));
        b.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    } else {
        b.removeColour(juce::TextButton::buttonOnColourId);
        b.removeColour(juce::TextButton::textColourOnId);
        b.removeColour(juce::TextButton::buttonColourId);
        b.removeColour(juce::TextButton::textColourOffId);
    }
}

void OpenChordMCoreEditor::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void OpenChordMCoreEditor::resized() {
    auto r = getLocalBounds().reduced(10);

    auto top = r.removeFromTop(28);
    title_.setBounds(top.removeFromLeft(160));
    preset_.setBounds(top.removeFromLeft(160));
    reset_map_.setBounds(top.removeFromLeft(100).reduced(0, 2));

    r.removeFromTop(6);
    auto status = r.removeFromTop(24);
    key_label_.setBounds(status.removeFromLeft(140));
    chord_label_.setBounds(status.removeFromLeft(160));
    mode_label_.setBounds(status.removeFromLeft(60));
    degree_disabled_.setBounds(status);

    r.removeFromTop(10);
    auto row1 = r.removeFromTop(48);
    const int bw = row1.getWidth() / 4;
    dim_.setBounds(row1.removeFromLeft(bw).reduced(2));
    min_.setBounds(row1.removeFromLeft(bw).reduced(2));
    maj_.setBounds(row1.removeFromLeft(bw).reduced(2));
    sus_.setBounds(row1.reduced(2));

    auto row2 = r.removeFromTop(48);
    e6_.setBounds(row2.removeFromLeft(bw).reduced(2));
    em7_.setBounds(row2.removeFromLeft(bw).reduced(2));
    eM7_.setBounds(row2.removeFromLeft(bw).reduced(2));
    e9_.setBounds(row2.reduced(2));

    r.removeFromTop(8);
    auto row3 = r.removeFromTop(36);
    key_btn_.setBounds(row3.removeFromLeft(90).reduced(2));
    panic_btn_.setBounds(row3.removeFromLeft(90).reduced(2));
    shift_btn_.setBounds(row3.removeFromLeft(90).reduced(2));

    r.removeFromTop(12);
    auto sx = r.removeFromTop(28);
    stick_x_learn_.setBounds(sx.removeFromLeft(70).reduced(0, 2));
    stick_x_.setBounds(sx);
    auto sy = r.removeFromTop(28);
    stick_y_learn_.setBounds(sy.removeFromLeft(70).reduced(0, 2));
    stick_y_.setBounds(sy);

    r.removeFromTop(12);
    learn_hint_.setBounds(r.removeFromTop(40));
}

void OpenChordMCoreEditor::timerCallback() { refreshLabels(); }

void OpenChordMCoreEditor::refreshLabels() {
    const auto s = proc_.snapshot();
    key_label_.setText("Key: " + juce::String(s.key_name), juce::dontSendNotification);
    chord_label_.setText("Chord: " + juce::String(s.chord), juce::dontSendNotification);

    if (!stick_x_.isMouseButtonDown())
        stick_x_.setValue(s.stick_x, juce::dontSendNotification);
    if (!stick_y_.isMouseButtonDown())
        stick_y_.setValue(s.stick_y, juce::dontSendNotification);

    updateButtons();
}

void OpenChordMCoreEditor::updateButtons() {
    const auto s = proc_.snapshot();
    const int armed = proc_.learnArmed();

    auto setCap = [this, &s, armed](HoldButton& b, ocplug::ControlId id, bool lit) {
        char buf[48];
        proc_.map().formatBinding(id, buf, sizeof(buf));
        juce::String t(buf);
        const bool isArmed = armed == static_cast<int>(id);
        if (isArmed) t = "* " + t;
        b.setButtonText(t);
        setPadLit(b, lit, isArmed);
    };

    setCap(dim_, ocplug::ControlId::Dim, (s.type_mask & 1) != 0);
    setCap(min_, ocplug::ControlId::Min, (s.type_mask & 2) != 0);
    setCap(maj_, ocplug::ControlId::Maj, (s.type_mask & 4) != 0);
    setCap(sus_, ocplug::ControlId::Sus, (s.type_mask & 8) != 0);
    setCap(e6_, ocplug::ControlId::Ext6, (s.ext & oc::Ext6) != 0);
    setCap(em7_, ocplug::ControlId::Extm7, (s.ext & oc::Extm7) != 0);
    setCap(eM7_, ocplug::ControlId::ExtM7, (s.ext & oc::ExtM7) != 0);
    setCap(e9_, ocplug::ControlId::Ext9, (s.ext & oc::Ext9) != 0);
    setCap(key_btn_, ocplug::ControlId::Key, s.key_held);
    setCap(panic_btn_, ocplug::ControlId::Panic, s.panic_held);
    setCap(shift_btn_, ocplug::ControlId::Shift, s.shift_held);

    auto stickCap = [this, armed](juce::TextButton& b, ocplug::ControlId id) {
        char buf[48];
        proc_.map().formatBinding(id, buf, sizeof(buf));
        juce::String t(buf);
        if (armed == static_cast<int>(id)) t = "* " + t;
        b.setButtonText(t);
        b.setToggleState(armed == static_cast<int>(id), juce::dontSendNotification);
    };
    stickCap(stick_x_learn_, ocplug::ControlId::StickX);
    stickCap(stick_y_learn_, ocplug::ControlId::StickY);
}

bool OpenChordMCoreEditor::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::escapeKey) {
        proc_.cancelLearn();
        updateButtons();
        return true;
    }
    return false;
}
