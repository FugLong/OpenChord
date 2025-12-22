#include "display_manager.h"

DisplayManager::DisplayManager() : hw_(nullptr), healthy_(false) {
}

DisplayManager::~DisplayManager() {
    Shutdown();
}

void DisplayManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    healthy_ = false;
    
    InitDisplay();
    
    healthy_ = true;
}

void DisplayManager::Update() {
    // No-op - display updates happen after drawing operations via Update() calls
}

void DisplayManager::Shutdown() {
    healthy_ = false;
    hw_ = nullptr;
}

void DisplayManager::InitDisplay() {
    if (!hw_) return;
    
    // Configuration matching official Daisy Seed example exactly
    // Only difference: pin assignments for our hardware
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>::Config display_cfg;
    
    // SPI Configuration (matching official example)
    display_cfg.driver_config.transport_config.spi_config.periph
        = daisy::SpiHandle::Config::Peripheral::SPI_1;
    display_cfg.driver_config.transport_config.spi_config.baud_prescaler
        = daisy::SpiHandle::Config::BaudPrescaler::PS_8;
    
    // SPI Pins (matching official example pattern)
    display_cfg.driver_config.transport_config.spi_config.pin_config.sclk
        = daisy::seed::D8;   // Pin 9 = SPI1_SCK
    display_cfg.driver_config.transport_config.spi_config.pin_config.miso
        = daisy::Pin();      // Not used for TX_ONLY
    display_cfg.driver_config.transport_config.spi_config.pin_config.mosi
        = daisy::seed::D10;  // Pin 11 = SPI1_MOSI
    display_cfg.driver_config.transport_config.spi_config.pin_config.nss
        = daisy::seed::D7;   // Pin 8 = SPI1_CS
    
    // Control pins (our pinout - different from example)
    display_cfg.driver_config.transport_config.pin_config.dc
        = daisy::seed::D13;  // Pin 14 = Data/Command
    display_cfg.driver_config.transport_config.pin_config.reset
        = daisy::seed::D14;  // Pin 15 = Reset
    
    // Initialize display (driver handles reset sequence automatically)
    display_.Init(display_cfg);
    
    // Clear display and update (matching official example)
    display_.Fill(false);
    display_.Update();
}

void DisplayManager::Clear() {
    if (!hw_ || !healthy_) return;
    
    display_.Fill(false);
    display_.Update();
}

void DisplayManager::TestDisplay() {
    if (!hw_ || !healthy_) return;
    
    // Simple test pattern matching official example style
    display_.Fill(false);  // Clear to black
    display_.SetCursor(4, 16);
    display_.WriteString("OpenChord", Font_11x18, true);  // White text on black
    display_.SetCursor(4, 32);
    display_.WriteString("Display OK", Font_6x8, true);
    display_.Update();
}

void DisplayManager::SetCursor(uint8_t x, uint8_t y) {
    if (!hw_ || !healthy_) return;
    display_.SetCursor(x, y);
}

void DisplayManager::PrintText(uint8_t x, uint8_t y, const char* text) {
    if (!hw_ || !healthy_ || !text) return;
    
    display_.SetCursor(x, y);
    display_.WriteString(text, Font_6x8, true);  // White text on black
    display_.Update();
}
