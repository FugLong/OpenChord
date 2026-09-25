#pragma once

#include "input.h"

namespace ocfw {

void InitSwitches();
// Pressed is low. Spatial order matches the engine keyswitch index.
void ReadSwitches(Input& in);

} // namespace ocfw
