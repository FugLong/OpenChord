#include "PluginEditor.h"

#include <cstring>

void TrackpadControl::setReading(float x, float y, bool finger) {
    if (dragging_) return;
    x_ = x;
    y_ = y;
    finger_ = finger;
    repaint();
}

void TrackpadControl::setFrom(juce::Point<float> p) {
    const float w = juce::jmax(1.0f, static_cast<float>(getWidth()));
    const float h = juce::jmax(1.0f, static_cast<float>(getHeight()));
    x_ = juce::jlimit(-1.0f, 1.0f, (p.x / w) * 2.0f - 1.0f);
    y_ = juce::jlimit(-1.0f, 1.0f, 1.0f - (p.y / h) * 2.0f);
}

void TrackpadControl::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff1c1c1c));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.drawLine(r.getCentreX(), r.getY(), r.getCentreX(), r.getBottom());
    g.drawLine(r.getX(), r.getCentreY(), r.getRight(), r.getCentreY());
    const float px = r.getX() + (x_ * 0.5f + 0.5f) * r.getWidth();
    const float py = r.getY() + (1.0f - (y_ * 0.5f + 0.5f)) * r.getHeight();
    g.setColour(finger_ ? juce::Colour(0xff3ecf8e) : juce::Colours::white.withAlpha(0.35f));
    g.fillEllipse(px - 7.0f, py - 7.0f, 14.0f, 14.0f);
}

void TrackpadControl::mouseDown(const juce::MouseEvent& e) {
    if (bindOn && bindOn()) return;
    dragging_ = true;
    finger_ = true;
    setFrom(e.position);
    if (onChange) onChange(x_, y_, true);
    repaint();
}

void TrackpadControl::mouseDrag(const juce::MouseEvent& e) {
    if (!dragging_) return;
    finger_ = true;
    setFrom(e.position);
    if (onChange) onChange(x_, y_, true);
    repaint();
}

void TrackpadControl::mouseUp(const juce::MouseEvent&) {
    if (!dragging_) return;
    dragging_ = false;
    finger_ = true;
    repaint();
}

uint8_t StripControl::positionFrom(float x) const {
    const float w = juce::jmax(1.0f, static_cast<float>(getWidth()));
    const int pos = static_cast<int>(juce::jlimit(0.0f, 1.0f, x / w) * 255.0f);
    return static_cast<uint8_t>(pos);
}

void StripControl::setReading(uint8_t position, bool finger) {
    if (dragging_) return;
    pos_ = position;
    finger_ = finger;
    repaint();
}

void StripControl::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xff1c1c1c));
    g.fillRoundedRectangle(r, 4.0f);
    const float t = static_cast<float>(pos_) / 255.0f;
    auto fill = r.withWidth(juce::jmax(4.0f, r.getWidth() * t));
    g.setColour(finger_ ? juce::Colour(0xff3ecf8e) : juce::Colours::white.withAlpha(0.28f));
    g.fillRoundedRectangle(fill, 4.0f);
}

void StripControl::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu()) {
        if (bindOn && bindOn() && onClear) onClear();
        return;
    }
    if (bindOn && bindOn()) {
        if (onBind) onBind();
        return;
    }
    dragging_ = true;
    finger_ = true;
    pos_ = positionFrom(e.position.x);
    if (onChange) onChange(pos_, true);
    repaint();
}

void StripControl::mouseDrag(const juce::MouseEvent& e) {
    if (!dragging_) return;
    finger_ = true;
    pos_ = positionFrom(e.position.x);
    if (onChange) onChange(pos_, true);
    repaint();
}

void StripControl::mouseUp(const juce::MouseEvent&) {
    if (!dragging_) return;
    dragging_ = false;
    finger_ = false;
    if (onChange) onChange(pos_, false);
    repaint();
}

OpenChordMCoreEditor::OpenChordMCoreEditor(OpenChordMCoreProcessor& p)
    : AudioProcessorEditor(&p), proc_(p)
{
    setSize(640, 620);
    setWantsKeyboardFocus(true);

    auto style = [](juce::Label& l, float size, bool bold = false) {
        l.setJustificationType(juce::Justification::centredLeft);
        l.setFont(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
        l.setInterceptsMouseClicks(false, false);
    };

    title_.setText("OpenChord M Core", juce::dontSendNotification);
    style(title_, 18.0f, true);
    addAndMakeVisible(title_);

    bind_btn_.onClick = [this] { proc_.toggleBind(); refresh(); };
    addAndMakeVisible(bind_btn_);

    latch_btn_.onClick = [this] {
        latchOn_ = !latchOn_;
        if (!latchHeld()) releaseLatches();
        refresh();
    };
    addAndMakeVisible(latch_btn_);

    reset_btn_.onClick = [this] { proc_.resetMap(); refresh(); };
    addAndMakeVisible(reset_btn_);

    hint_.setText(
        "Hold a keyswitch or button, or turn on Latch (Option or Control does the same) and click several. "
        "They stay down until Latch is off and those keys are up. "
        "Trackpad stays where you let go. Strip plays while you drag, and lifts when you release.",
        juce::dontSendNotification);
    style(hint_, 12.0f);
    addAndMakeVisible(hint_);

    for (int i = 0; i < 8; ++i) wireKeyswitch(key_[i], i);
    wireButton(prev_, oc::Button::Prev, ocplug::ControlId::Prev);
    wireButton(menu_, oc::Button::Menu, ocplug::ControlId::Menu);
    wireButton(next_, oc::Button::Next, ocplug::ControlId::Next);

    auto wireAxis = [this](ClickButton& b, ocplug::ControlId id) {
        addAndMakeVisible(b);
        b.onClick = [this, id] {
            if (proc_.bindOn()) proc_.chooseBind(id);
        };
        b.onRightClick = [this, id] {
            if (proc_.bindOn()) proc_.clearBinding(id);
            refresh();
        };
    };
    wireAxis(track_x_, ocplug::ControlId::TrackpadX);
    wireAxis(track_y_, ocplug::ControlId::TrackpadY);

    trackpad_.onChange = [this](float x, float y, bool finger) {
        proc_.uiSetTrackpad(x, y, finger);
    };
    trackpad_.bindOn = [this] { return proc_.bindOn(); };
    addAndMakeVisible(trackpad_);

    strip_.onChange = [this](uint8_t pos, bool finger) { proc_.uiSetStrip(pos, finger); };
    strip_.bindOn = [this] { return proc_.bindOn(); };
    strip_.onBind = [this] { proc_.chooseBind(ocplug::ControlId::Strip); };
    strip_.onClear = [this] { proc_.clearBinding(ocplug::ControlId::Strip); };
    addAndMakeVisible(strip_);

    refresh();
    startTimerHz(30);
}

OpenChordMCoreEditor::~OpenChordMCoreEditor() { stopTimer(); }

const char* OpenChordMCoreEditor::keyswitchLabel(oc::PlayMode mode, int index, bool drumRight) {
    static const char* kKeys[8] = {"Dim", "Min", "Maj", "Sus", "6", "m7", "M7", "9"};
    static const char* kScale[8] = {"I", "ii", "iii", "IV", "V", "vi", "vii", "I+"};
    if (index < 0 || index > 7) return "?";
    if (mode == oc::PlayMode::Scale) return kScale[index];
    if (mode == oc::PlayMode::Drums) return oc::drumName(index, drumRight);
    return kKeys[index];
}

void OpenChordMCoreEditor::wireKeyswitch(ClickButton& b, int index) {
    addAndMakeVisible(b);
    b.onDown = [this, index] {
        if (proc_.bindOn()) {
            proc_.chooseBind(static_cast<ocplug::ControlId>(index));
        } else if (latchHeld()) {
            const auto bit = static_cast<uint8_t>(1u << index);
            if (latchedKeys_ & bit) {
                latchedKeys_ = static_cast<uint8_t>(latchedKeys_ & ~bit);
                proc_.uiSetKeyswitch(index, false);
            } else {
                latchedKeys_ = static_cast<uint8_t>(latchedKeys_ | bit);
                proc_.uiSetKeyswitch(index, true);
            }
        } else {
            proc_.uiSetKeyswitch(index, true);
        }
        refresh();
    };
    b.onUp = [this, index] {
        if (proc_.bindOn()) return;
        if (latchedKeys_ & (1u << index)) return;
        proc_.uiSetKeyswitch(index, false);
        refresh();
    };
    b.onRightClick = [this, index] {
        if (!proc_.bindOn()) return;
        proc_.clearBinding(static_cast<ocplug::ControlId>(index));
        refresh();
    };
}

void OpenChordMCoreEditor::wireButton(ClickButton& b, oc::Button button, ocplug::ControlId id) {
    addAndMakeVisible(b);
    b.onDown = [this, button, id] {
        if (proc_.bindOn()) proc_.chooseBind(id);
        else proc_.uiSetButton(button, true);
        refresh();
    };
    b.onUp = [this, button] {
        if (proc_.bindOn()) return;
        proc_.uiSetButton(button, false);
        refresh();
    };
    b.onRightClick = [this, id] {
        if (!proc_.bindOn()) return;
        proc_.clearBinding(id);
        refresh();
    };
}

void OpenChordMCoreEditor::paintLatch(juce::Button& b, bool lit, bool armed) {
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
    auto screen = getLocalBounds().reduced(12).withTrimmedTop(34).removeFromTop(96);
    screen = screen.withSizeKeepingCentre(384, 96);
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(screen.toFloat(), 4.0f);
    auto inner = screen.reduced(10, 6);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    g.drawText(screen_top_, inner.removeFromTop(40), juce::Justification::centred, true);
    g.setFont(juce::FontOptions(16.0f));
    auto bottom = inner.removeFromTop(28);
    const int col = bottom.getWidth() / 3;
    auto left = bottom.removeFromLeft(col);
    auto right = bottom.removeFromRight(col);
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.fillRect(left.getRight(), bottom.getY() + 6, 1, bottom.getHeight() - 12);
    g.fillRect(right.getX(), bottom.getY() + 6, 1, bottom.getHeight() - 12);
    g.setColour(juce::Colours::white);
    g.drawText(screen_left_, left.reduced(4, 0), juce::Justification::centred, true);
    g.drawText(screen_mid_, bottom, juce::Justification::centred, true);
    g.drawText(screen_right_, right.reduced(4, 0), juce::Justification::centred, true);
    if (screen_zones_ > 0) {
        auto marks = inner.removeFromBottom(8);
        const int w = marks.getWidth() / screen_zones_;
        for (int i = 0; i < screen_zones_; ++i) {
            auto cell = marks.removeFromLeft(w).reduced(2, 2);
            g.setColour(i == screen_zone_ ? juce::Colours::white : juce::Colours::white.withAlpha(0.35f));
            g.fillRect(cell);
        }
    }
}

void OpenChordMCoreEditor::resized() {
    auto r = getLocalBounds().reduced(12);

    auto top = r.removeFromTop(28);
    title_.setBounds(top.removeFromLeft(220));
    reset_btn_.setBounds(top.removeFromRight(72).reduced(0, 2));
    latch_btn_.setBounds(top.removeFromRight(72).reduced(0, 2));
    bind_btn_.setBounds(top.removeFromRight(72).reduced(0, 2));

    r.removeFromTop(6);
    r.removeFromTop(96);
    r.removeFromTop(10);
    auto row1 = r.removeFromTop(52);
    const int bw = row1.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        key_[i].setBounds(row1.removeFromLeft(bw).reduced(3));
    auto row2 = r.removeFromTop(52);
    for (int i = 4; i < 8; ++i)
        key_[i].setBounds(row2.removeFromLeft(bw).reduced(3));

    r.removeFromTop(8);
    auto buttons = r.removeFromTop(36);
    const int tw = buttons.getWidth() / 3;
    prev_.setBounds(buttons.removeFromLeft(tw).reduced(3));
    menu_.setBounds(buttons.removeFromLeft(tw).reduced(3));
    next_.setBounds(buttons.reduced(3));

    r.removeFromTop(10);
    auto trackRow = r.removeFromTop(180);
    trackpad_.setBounds(trackRow.removeFromLeft(180).reduced(3));
    auto axes = trackRow.reduced(8, 3);
    track_x_.setBounds(axes.removeFromTop(36));
    axes.removeFromTop(6);
    track_y_.setBounds(axes.removeFromTop(36));

    r.removeFromTop(8);
    strip_.setBounds(r.removeFromTop(36).reduced(3, 4));

    r.removeFromTop(8);
    hint_.setBounds(r.removeFromTop(64));
}

bool OpenChordMCoreEditor::latchHeld() const {
    if (latchOn_) return true;
    const auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
    return mods.isAltDown() || mods.isCtrlDown();
}

void OpenChordMCoreEditor::releaseLatches() {
    const uint8_t keys = latchedKeys_;
    const uint8_t buttons = latchedButtons_;
    latchedKeys_ = 0;
    latchedButtons_ = 0;
    for (int i = 0; i < 8; ++i) {
        if (keys & (1u << i)) proc_.uiSetKeyswitch(i, false);
    }
    if (buttons & 1u) proc_.uiSetButton(oc::Button::Prev, false);
    if (buttons & 2u) proc_.uiSetButton(oc::Button::Menu, false);
    if (buttons & 4u) proc_.uiSetButton(oc::Button::Next, false);
}

void OpenChordMCoreEditor::timerCallback() {
    if (!latchHeld() && (latchedKeys_ != 0 || latchedButtons_ != 0))
        releaseLatches();
    refresh();
}

void OpenChordMCoreEditor::refresh() {
    const auto s = proc_.snapshot();
    std::strncpy(screen_top_, s.screen_top, sizeof(screen_top_) - 1);
    screen_top_[sizeof(screen_top_) - 1] = 0;
    std::strncpy(screen_left_, s.screen_left, sizeof(screen_left_) - 1);
    screen_left_[sizeof(screen_left_) - 1] = 0;
    std::strncpy(screen_mid_, s.screen_mid, sizeof(screen_mid_) - 1);
    screen_mid_[sizeof(screen_mid_) - 1] = 0;
    std::strncpy(screen_right_, s.screen_right, sizeof(screen_right_) - 1);
    screen_right_[sizeof(screen_right_) - 1] = 0;
    screen_zones_ = s.screen_zones;
    screen_zone_ = s.screen_zone;
    repaint();

    trackpad_.setReading(s.track_x, s.track_y, s.track_finger);
    strip_.setReading(s.strip, s.strip_finger);

    const bool bind = proc_.bindOn();
    const int target = proc_.bindTarget();
    paintLatch(bind_btn_, bind, false);
    paintLatch(latch_btn_, latchOn_ || latchHeld(), false);
    bind_btn_.setButtonText(bind ? "Bind…" : "Bind");

    auto caption = [this](ocplug::ControlId id, const char* name) {
        const auto b = proc_.binding(id);
        juce::String t(name);
        if (b.kind == ocplug::Binding::Kind::Cc)
            t += "\nCC " + juce::String(static_cast<int>(b.number));
        else if (b.kind == ocplug::Binding::Kind::Note)
            t += "\nN" + juce::String(static_cast<int>(b.number));
        return t;
    };

    for (int i = 0; i < 8; ++i) {
        const auto id = static_cast<ocplug::ControlId>(i);
        const bool drumRight = s.mode == oc::PlayMode::Drums && s.track_x > 0.f;
        juce::String t = caption(id, keyswitchLabel(s.mode, i, drumRight));
        const bool armed = bind && target == i;
        if (armed) t = "* " + t;
        key_[i].setButtonText(t);
        paintLatch(key_[i], (s.keyswitches & (1u << i)) != 0, armed);
    }

    auto showButton = [&](ClickButton& b, ocplug::ControlId id, const char* name, bool lit) {
        juce::String t = caption(id, name);
        const bool armed = bind && target == static_cast<int>(id);
        if (armed) t = "* " + t;
        b.setButtonText(t);
        paintLatch(b, lit, armed);
    };
    showButton(prev_, ocplug::ControlId::Prev, "Prev", (s.buttons & 1u) != 0);
    showButton(menu_, ocplug::ControlId::Menu, "Menu", (s.buttons & 2u) != 0);
    showButton(next_, ocplug::ControlId::Next, "Next", (s.buttons & 4u) != 0);

    auto showAxis = [&](ClickButton& b, ocplug::ControlId id) {
        const auto binding = proc_.binding(id);
        juce::String t = ocplug::ControlName(id);
        if (binding.kind == ocplug::Binding::Kind::Cc)
            t += "  CC " + juce::String(static_cast<int>(binding.number));
        else if (binding.kind == ocplug::Binding::Kind::Note)
            t += "  N" + juce::String(static_cast<int>(binding.number));
        const bool armed = bind && target == static_cast<int>(id);
        if (armed) t = "* " + t;
        b.setButtonText(t);
        paintLatch(b, false, armed);
    };
    showAxis(track_x_, ocplug::ControlId::TrackpadX);
    showAxis(track_y_, ocplug::ControlId::TrackpadY);
}

bool OpenChordMCoreEditor::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::escapeKey && proc_.bindOn()) {
        proc_.toggleBind();
        refresh();
        return true;
    }
    return false;
}
