#pragma once

#include "session.h"

namespace ocfw {

using TrsNoteFn = void (*)(uint8_t channel, uint8_t pitch, uint8_t velocity);

void InitTrs(TrsNoteFn on, TrsNoteFn off);
void ReadTrs();
void SendTrs(const oc::MidiEvent& ev);

} // namespace ocfw
