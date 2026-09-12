#include "track_settings.h"
#include "../tracks/track_interface.h"
#include <cstring>

namespace OpenChord {

// Enum option strings
static const char* note_names[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", nullptr
};

static const char* mode_names[] = {
    "Ionian", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian", "Locrian", nullptr
};

TrackSettings::TrackSettings()
    : track_(nullptr)
    , key_root_value_(0)  // C
    , key_mode_value_(0)  // Ionian
{
    InitializeSettings();
}

TrackSettings::~TrackSettings() {
}

void TrackSettings::InitializeSettings() {
    // Setting 0: Key Root (C through B)
    settings_[0].name = "Key";
    settings_[0].type = SettingType::ENUM;
    settings_[0].value_ptr = &key_root_value_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = 11.0f;
    settings_[0].step_size = 1.0f;
    settings_[0].enum_options = note_names;
    settings_[0].enum_count = 12;
    settings_[0].on_change_callback = nullptr;
    
    // Setting 1: Mode (Ionian through Locrian)
    settings_[1].name = "Mode";
    settings_[1].type = SettingType::ENUM;
    settings_[1].value_ptr = &key_mode_value_;
    settings_[1].min_value = 0.0f;
    settings_[1].max_value = 6.0f;
    settings_[1].step_size = 1.0f;
    settings_[1].enum_options = mode_names;
    settings_[1].enum_count = 7;
    settings_[1].on_change_callback = nullptr;
}

int TrackSettings::GetSettingCount() const {
    return SETTING_COUNT;
}

const PluginSetting* TrackSettings::GetSetting(int index) const {
    if (index < 0 || index >= SETTING_COUNT) {
        return nullptr;
    }
    
    // Sync values from track before returning
    SyncFromTrack();
    
    return &settings_[index];
}

void TrackSettings::OnSettingChanged(int setting_index) {
    if (setting_index == 0 || setting_index == 1) {
        // Key root or mode changed - sync to track
        SyncToTrack();
    }
}

void TrackSettings::SyncFromTrack() const {
    if (!track_) return;
    
    MusicalKey key = track_->GetKey();
    key_root_value_ = key.root_note;
    key_mode_value_ = static_cast<int>(key.mode);
}

void TrackSettings::SyncToTrack() {
    if (!track_) return;
    
    // Clamp values to valid ranges
    if (key_root_value_ < 0) key_root_value_ = 0;
    if (key_root_value_ > 11) key_root_value_ = 11;
    if (key_mode_value_ < 0) key_mode_value_ = 0;
    if (key_mode_value_ > 6) key_mode_value_ = 6;
    
    MusicalKey new_key(
        static_cast<uint8_t>(key_root_value_),
        static_cast<MusicalMode>(key_mode_value_)
    );
    
    track_->SetKey(new_key);
}

} // namespace OpenChord

