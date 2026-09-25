#pragma once

#include "screen.h"

namespace ocfw {

bool InitOled();
bool OledReady();
bool PresentOled(const uint8_t bitmap[kScreenBytes]);

} // namespace ocfw
