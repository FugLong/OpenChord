#pragma once

#include "daisy_seed.h"
#include "../io/display_manager.h"
#include <cstdint>

namespace OpenChord {

/**
 * Simple callback function type for rendering debug views
 * 
 * Function should render content to the display and call display->Update()
 * The display will be cleared before this function is called.
 */
typedef void (*DebugRenderFunc)(DisplayManager* display);

/**
 * Simple debug view structure
 * Just a name and a render function - that's it!
 */
struct DebugView {
    const char* name;
    DebugRenderFunc render;
    
    DebugView() : name(nullptr), render(nullptr) {}
    DebugView(const char* n, DebugRenderFunc r) : name(n), render(r) {}
};

} // namespace OpenChord