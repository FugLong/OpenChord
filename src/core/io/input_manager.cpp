#include "input_manager.h"

namespace OpenChord {

InputManager::InputManager()
    : io_manager_(nullptr),
      initialized_(false) {
}

InputManager::~InputManager() {
    io_manager_ = nullptr;
}

void InputManager::Init(IOManager* io_manager) {
    if (!io_manager) return;
    
    io_manager_ = io_manager;
    
    // Initialize button handler
    DigitalManager* digital = io_manager_->GetDigital();
    if (digital) {
        button_handler_.Init(digital);
        button_handler_.SetInputMode(InputMode::MIDI_NOTES);
    }
    
    // Initialize encoder handler
    if (digital) {
        encoder_handler_.Init(digital);
        encoder_handler_.SetMode(EncoderMode::NAVIGATION);
    }
    
    // Initialize joystick handler
    AnalogManager* analog = io_manager_->GetAnalog();
    if (analog) {
        joystick_handler_.Init(analog);
        joystick_handler_.SetMode(JoystickMode::NAVIGATION);
    }
    
    initialized_ = true;
}

void InputManager::Update() {
    if (!initialized_ || !io_manager_) return;
    
    // Update all input handlers
    button_handler_.Update();
    joystick_handler_.Update();
    encoder_handler_.Update();
}

bool InputManager::IsAnySystemButtonPressed() const {
    // Check all system buttons
    return button_handler_.IsSystemButtonPressed(SystemButton::INPUT) ||
           button_handler_.IsSystemButtonPressed(SystemButton::INSTRUMENT) ||
           button_handler_.IsSystemButtonPressed(SystemButton::FX) ||
           button_handler_.IsSystemButtonPressed(SystemButton::RECORD);
}

bool InputManager::IsHealthy() const {
    if (!io_manager_) return false;
    
    // Check that all required managers are available
    bool digital_ok = io_manager_->GetDigital() != nullptr;
    bool analog_ok = io_manager_->GetAnalog() != nullptr;
    
    return digital_ok && analog_ok && initialized_;
}

} // namespace OpenChord

