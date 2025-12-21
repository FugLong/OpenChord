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
    
    // Display operations
    void Clear();
    void TestDisplay();  // Display "Hello World" test message
    
private:
    daisy::DaisySeed* hw_;
    daisy::SpiHandle spi_handle_;
    daisy::GPIO dc_pin_;   // Data/Command pin (Pin 1, D0)
    daisy::GPIO cs_pin_;   // Chip select pin (Pin 8, D7) - manually controlled
    bool healthy_;
    
    void InitSPI();
    void InitDCPin();
    void InitCSPin();
    void InitDisplay();
    
    // Low-level SPI communication
    void SendCommand(uint8_t cmd);
    void SendData(uint8_t data);
    void SendDataBuffer(const uint8_t* data, size_t length);
    
    // Display dimensions
    static constexpr uint8_t DISPLAY_WIDTH = 128;
    static constexpr uint8_t DISPLAY_HEIGHT = 64;
    static constexpr uint8_t DISPLAY_PAGES = 8;  // 64 pixels / 8 bits per page
};
