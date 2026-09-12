#include "global_settings.h"
#include <cstring>

namespace OpenChord {

// Enum option strings for transport routing
static const char* transport_routing_options[] = {
    "Internal Only",
    "DAW Only",
    "Both",
    nullptr
};

GlobalSettings::GlobalSettings()
    : transport_routing_(TransportRouting::DAW_ONLY)  // Default: DAW only (for now)
    , transport_routing_value_(static_cast<int>(TransportRouting::DAW_ONLY))
{
    InitializeSettings();
    SyncTransportRoutingValue();  // Initial sync
}

GlobalSettings::~GlobalSettings() {
}

void GlobalSettings::InitializeSettings() {
    // Setting 0: Transport Routing (enum)
    settings_[0].name = "Route";
    settings_[0].type = SettingType::ENUM;
    settings_[0].value_ptr = &transport_routing_value_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = 2.0f;
    settings_[0].step_size = 1.0f;
    settings_[0].enum_options = transport_routing_options;
    settings_[0].enum_count = 3;
    settings_[0].on_change_callback = nullptr;
}

int GlobalSettings::GetSettingCount() const {
    return SETTING_COUNT;
}

const PluginSetting* GlobalSettings::GetSetting(int index) const {
    if (index < 0 || index >= SETTING_COUNT) {
        return nullptr;
    }
    return &settings_[index];
}

void GlobalSettings::OnSettingChanged(int setting_index) {
    if (setting_index == 0) {
        // Transport routing changed - sync enum value
        SyncTransportRoutingValue();
    }
}

void GlobalSettings::SyncTransportRoutingValue() {
    // Sync enum from helper int value
    transport_routing_ = static_cast<TransportRouting>(transport_routing_value_);
    // Clamp to valid range
    if (transport_routing_value_ < 0) {
        transport_routing_value_ = 0;
        transport_routing_ = TransportRouting::INTERNAL_ONLY;
    } else if (transport_routing_value_ > 2) {
        transport_routing_value_ = 2;
        transport_routing_ = TransportRouting::BOTH;
    }
}

} // namespace OpenChord

