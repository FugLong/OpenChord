#pragma once

#include "daisy_seed.h"
#include <cstdint>

/**
 * Serial Manager - Handles all serial communication
 * 
 * Manages:
 * - SPI Display interface
 * - SD Card (SDMMC)
 * - UART MIDI
 * - Error handling and recovery
 */
class SerialManager {
public:
    SerialManager();
    ~SerialManager();
    
    // Core lifecycle
    void Init(daisy::DaisySeed* hw);
    void Update();
    void Shutdown();
    
    // Health monitoring
    bool IsHealthy() const { return healthy_; }
    
    // TODO: Implement serial communication methods
    // - SPI display methods
    // - SD card methods  
    // - MIDI UART methods
    
private:
    daisy::DaisySeed* hw_;
    bool healthy_;
    
    // TODO: Add serial communication members
};
