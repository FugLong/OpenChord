#pragma once

#include "daisy_seed.h"
#include "pin_config.h"
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
    daisy::SpiHandle spi_handle_;
    daisy::GPIO dc_pin_;   // Data/Command pin (Pin 1, D0)
    daisy::GPIO cs_pin_;   // Chip select pin (Pin 8, D7) - manually controlled
    bool healthy_;
    
    void InitSPI();
    void InitDCPin();
    void InitCSPin();
};
