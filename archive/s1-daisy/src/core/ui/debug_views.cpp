#include "debug_views.h"
#include "../io/io_manager.h"
#include "../io/input_manager.h"
#include "../io/analog_manager.h"
#include "../io/digital_manager.h"
#include "../io/encoder_input_handler.h"
#include "../io/joystick_input_handler.h"
#include "../audio/audio_engine.h"
#include "../audio/volume_manager.h"
#include "../audio/volume_interface.h"
#include "../midi/midi_handler.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include <cstdio>
#include <cstring>

namespace OpenChord {

void RenderSystemStatus(DisplayManager* display, IOManager* io_manager) {
    if (!display || !display->IsHealthy() || !io_manager) return;
    
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* disp = display->GetDisplay();
    if (!disp) return;
    
    SystemStatus status = io_manager->GetStatus();
    char buffer[64];
    int y = 10;  // Offset by 10 pixels for system bar (content area starts at y=10)
    
    disp->SetCursor(0, y);
    disp->WriteString("System Status", Font_6x8, true);
    y += 10;
    
    snprintf(buffer, sizeof(buffer), "Digital: %s", status.digital_healthy ? "OK" : "FAIL");
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    snprintf(buffer, sizeof(buffer), "Analog:  %s", status.analog_healthy ? "OK" : "FAIL");
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    snprintf(buffer, sizeof(buffer), "Serial:  %s", status.serial_healthy ? "OK" : "FAIL");
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    snprintf(buffer, sizeof(buffer), "Display: %s", status.display_healthy ? "OK" : "FAIL");
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    snprintf(buffer, sizeof(buffer), "Storage: %s", status.storage_healthy ? "OK" : "FAIL");
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    snprintf(buffer, sizeof(buffer), "Errors: %lu", status.error_count);
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    
    // Note: Display Update() is handled by UIManager
}

void RenderInputStatus(DisplayManager* display, InputManager* input_manager, IOManager* io_manager) {
    if (!display || !display->IsHealthy() || !input_manager || !io_manager) return;
    
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* disp = display->GetDisplay();
    if (!disp) return;
    
    DigitalManager* digital = io_manager->GetDigital();
    AnalogManager* analog = io_manager->GetAnalog();
    
    EncoderInputHandler& encoder = input_manager->GetEncoder();
    JoystickInputHandler& joystick = input_manager->GetJoystick();
    
    char buffer[64];
    int y = 10;  // Offset by 10 pixels for system bar (content area starts at y=10)
    
    disp->SetCursor(0, y);
    disp->WriteString("Inputs", Font_6x8, true);
    y += 10;
    
    // Show encoder value and delta
    int enc_val = 0;
    float enc_delta = 0.0f;
    if (digital) {
        enc_val = digital->GetEncoderValue();
        enc_delta = digital->GetEncoderDelta();
    } else {
        enc_val = encoder.GetValue();
        enc_delta = encoder.GetDelta();
    }
    snprintf(buffer, sizeof(buffer), "Enc: %d (%.1f)", enc_val, enc_delta);
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    // Show raw joystick ADC values from AnalogManager (Y axis inverted for display)
    if (analog) {
        float joy_x_raw = analog->GetJoystickXRaw();
        float joy_y_raw = analog->GetJoystickYRaw();
        // Invert Y axis for display (typical: up should show as positive)
        snprintf(buffer, sizeof(buffer), "Joy Raw: %.3f,%.3f", joy_x_raw, 1.0f - joy_y_raw);
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
        y += 8;
    }
    
    // Show processed joystick values from handler (should be -1.0 to 1.0)
    float joy_x = joystick.GetX();
    float joy_y = joystick.GetY();
    snprintf(buffer, sizeof(buffer), "Joy: %.2f,%.2f", joy_x, joy_y);
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    // Show joystick button state
    if (digital) {
        bool joy_btn = digital->IsJoystickButtonPressed();
        snprintf(buffer, sizeof(buffer), "Joy BTN: %s", joy_btn ? "ON" : "OFF");
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
    }
    
    // Note: Display Update() is handled by UIManager
}

void RenderAnalogStatus(DisplayManager* display, IOManager* io_manager) {
    if (!display || !display->IsHealthy() || !io_manager) return;
    
    AnalogManager* analog = io_manager->GetAnalog();
    if (!analog) return;
    
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* disp = display->GetDisplay();
    if (!disp) return;
    
    char buffer[64];
    int y = 10;  // Offset by 10 pixels for system bar (content area starts at y=10)
    
    disp->SetCursor(0, y);
    disp->WriteString("Analog", Font_6x8, true);
    y += 10;
    
    // Try GetVolume() instead of GetVolumeNormalized() - show raw ADC too
    float vol = analog->GetVolume();
    snprintf(buffer, sizeof(buffer), "Vol: %.3f", vol);
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    // Show raw joystick values from AnalogManager (0.0 to 1.0)
    float joy_x = analog->GetJoystickX();
    float joy_y = analog->GetJoystickY();
    snprintf(buffer, sizeof(buffer), "Joy: %.3f,%.3f", joy_x, joy_y);
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    snprintf(buffer, sizeof(buffer), "Mic: %.3f", analog->GetMicrophoneLevel());
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    snprintf(buffer, sizeof(buffer), "Bat: %.2fV", analog->GetBatteryVoltage());
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    y += 8;
    
    snprintf(buffer, sizeof(buffer), "Bat%%: %.0f%%", analog->GetBatteryPercentage());
    disp->SetCursor(0, y);
    disp->WriteString(buffer, Font_6x8, true);
    
    // Note: Display Update() is handled by UIManager
}

void RenderAudioStatus(DisplayManager* display, AudioEngine* audio_engine, VolumeManager* volume_manager) {
    if (!display || !display->IsHealthy()) return;
    
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* disp = display->GetDisplay();
    if (!disp) return;
    
    char buffer[64];
    int y = 10;  // Offset by 10 pixels for system bar (content area starts at y=10)
    
    disp->SetCursor(0, y);
    disp->WriteString("Audio", Font_6x8, true);
    y += 10;
    
    if (audio_engine) {
        snprintf(buffer, sizeof(buffer), "Note: %s", audio_engine->IsNoteOn() ? "ON" : "OFF");
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
        y += 8;
        
        snprintf(buffer, sizeof(buffer), "Freq: N/A");
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
        y += 8;
        
        snprintf(buffer, sizeof(buffer), "Mic: %s", audio_engine->IsMicPassthroughEnabled() ? "ON" : "OFF");
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
        y += 8;
    }
    
    if (volume_manager) {
        const VolumeData& vol_data = volume_manager->GetVolumeData();
        snprintf(buffer, sizeof(buffer), "Amp: %.3f", vol_data.amplitude);
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
        y += 8;
        
        snprintf(buffer, sizeof(buffer), "Line: %.3f", vol_data.line_level);
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
    }
    
    // Note: Display Update() is handled by UIManager
}

void RenderMIDIStatus(DisplayManager* display, OpenChordMidiHandler* midi_handler) {
    if (!display || !display->IsHealthy()) return;
    
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* disp = display->GetDisplay();
    if (!disp) return;
    
    char buffer[64];
    int y = 10;  // Offset by 10 pixels for system bar (content area starts at y=10)
    
    disp->SetCursor(0, y);
    disp->WriteString("MIDI", Font_6x8, true);
    y += 10;
    
    if (midi_handler) {
        snprintf(buffer, sizeof(buffer), "TRS: %s", midi_handler->IsTrsInitialized() ? "ON" : "OFF");
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
        y += 8;
        
        snprintf(buffer, sizeof(buffer), "USB: %s", midi_handler->IsUsbInitialized() ? "ON" : "OFF");
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
    }
    
    // Note: Display Update() is handled by UIManager
}

} // namespace OpenChord