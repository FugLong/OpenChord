#include "daisy_seed.h"
#include "daisysp.h"
#include "../include/system_interface.h"
#include "../include/io.h"

using namespace daisy;
using namespace daisysp;

// Global system instance
OpenChord::OpenChordSystem openchord_system;

// Hardware objects
DaisySeed hw;
IO io;

// Audio callback
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    // Process the system
    openchord_system.Process(const_cast<float*>(in[0]), out[0], size);
    openchord_system.Process(const_cast<float*>(in[1]), out[1], size);
    
    // Update system (non-audio processing)
    openchord_system.Update();
}

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.SetAudioBlockSize(4);
    
    // Initialize IO
    io.Init(&hw);
    
    // Initialize OpenChord system
    openchord_system.Init();
    openchord_system.SetSampleRate(hw.AudioSampleRate());
    openchord_system.SetBufferSize(hw.AudioBlockSize());
    
    // Set up audio callback
    hw.StartAudio(AudioCallback);
    
    // Main loop
    while (1) {
        // Update IO and handle controls
        io.Update();
        
        // Handle encoder
        float encoder_delta = io.GetEncoderDelta();
        if (encoder_delta != 0.0f) {
            openchord_system.HandleEncoder(0, encoder_delta);
        }
        
        // Handle buttons
        for (int i = 0; i < IO::NUM_BUTTONS; i++) {
            bool pressed = io.IsButtonPressed(i);
            bool was_pressed = io.WasButtonPressed(i);
            
            if (pressed != was_pressed) {
                openchord_system.HandleButton(i, pressed);
            }
        }
        
        // Handle joystick
        float joystick_x, joystick_y;
        io.GetJoystick(&joystick_x, &joystick_y);
        openchord_system.HandleJoystick(joystick_x, joystick_y);
        
        // Update UI
        openchord_system.UpdateUI();
        
        // Small delay to prevent overwhelming the system
        System::Delay(1);
    }
} 