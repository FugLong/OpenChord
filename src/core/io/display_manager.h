#pragma once

#include "daisy_seed.h"
#include <cstdint>

/**
 * Display Manager - Handles OLED display operations
 * 
 * Manages:
 * - SPI display interface
 * - Double buffering
 * - Text and graphics rendering
 * - UI state management
 */
class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();
    
    // Core lifecycle
    void Init(daisy::DaisySeed* hw);
    void Update();
    void Shutdown();
    
    // Health monitoring
    bool IsHealthy() const { return healthy_; }
    
    // TODO: Implement display methods
    // - Text rendering
    // - Graphics primitives
    // - Double buffering
    // - UI state management
    
private:
    daisy::DaisySeed* hw_;
    bool healthy_;
    
    // TODO: Add display members
};
