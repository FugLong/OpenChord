#pragma once

#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include <cstdint>

/**
 * Display Manager - Simple wrapper for Daisy's OLED display driver
 * 
 * Uses Daisy's built-in OledDisplay with SSD130x4WireSpi128x64Driver
 * Follows official Daisy Seed example pattern exactly
 */
class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();
    
    // Core lifecycle
    void Init(daisy::DaisySeed* hw);
    void Update();  // Called periodically (no-op, updates happen after drawing)
    void Shutdown();
    
    // Health check
    bool IsHealthy() const { return healthy_; }
    
    // Basic display operations
    void Clear();
    void TestDisplay();  // Simple test pattern
    
    // Text rendering
    void PrintText(uint8_t x, uint8_t y, const char* text);
    void SetCursor(uint8_t x, uint8_t y);
    
    // Direct access to display for advanced operations
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* GetDisplay() {
        return healthy_ ? &display_ : nullptr;
    }
    
private:
    daisy::DaisySeed* hw_;
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver> display_;
    bool healthy_;
    
    void InitDisplay();
};
