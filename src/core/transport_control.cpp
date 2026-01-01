#include "transport_control.h"

namespace OpenChord {

TransportControl::TransportControl()
    : midi_handler_(nullptr)
    , global_settings_(nullptr)
    , is_playing_(false)
    , is_recording_(false)
{
}

TransportControl::~TransportControl() {
}

void TransportControl::Init(OpenChordMidiHandler* midi_handler, GlobalSettings* global_settings) {
    midi_handler_ = midi_handler;
    global_settings_ = global_settings;
}

void TransportControl::HandleCombo(int combo_type) {
    if (!midi_handler_ || !global_settings_) {
        return;
    }
    
    if (combo_type == 0) {
        // INPUT button tap (Play/Pause)
        SendPlayPause();
        // Don't track state - DAW handles the toggle
    } else if (combo_type == 1) {
        // RECORD button tap (Record toggle)
        SendRecord();
        // Don't track state - DAW handles the toggle
    }
}

void TransportControl::SendPlayPause() {
    TransportRouting routing = global_settings_->GetTransportRouting();
    
    // For now, only send to DAW (MIDI output)
    // Send CC #115 (Play/Pause) - Logic maps this to play/pause toggle, so we send the same signal every time
    // The DAW handles the toggle logic, we just send a trigger
    // TODO: Add internal looper control when implemented
    if (routing == TransportRouting::DAW_ONLY || routing == TransportRouting::BOTH) {
        if (midi_handler_) {
            // Always send CC #115 with value 127 - DAW handles the toggle
            midi_handler_->SendMidi(daisy::MidiMessageType::ControlChange, 0, 115, 127);
        }
    }
    
    // TODO: Control internal looper when routing includes INTERNAL_ONLY or BOTH
}


void TransportControl::SendRecord() {
    TransportRouting routing = global_settings_->GetTransportRouting();
    
    // For now, only send to DAW (MIDI output)
    // Send CC #117 (Record) - Logic maps this to record toggle, so we send the same signal every time
    // The DAW handles the toggle logic, we just send a trigger
    if (routing == TransportRouting::DAW_ONLY || routing == TransportRouting::BOTH) {
        if (midi_handler_) {
            // Always send CC #117 with value 127 - DAW handles the toggle
            midi_handler_->SendMidi(daisy::MidiMessageType::ControlChange, 0, 117, 127);
        }
    }
    
    // TODO: Control internal looper recording when routing includes INTERNAL_ONLY or BOTH
}


void TransportControl::SendContinue() {
    if (midi_handler_) {
        midi_handler_->SendSystemRealtime(0xFB);  // MIDI CONTINUE (0xFB)
    }
}

} // namespace OpenChord

