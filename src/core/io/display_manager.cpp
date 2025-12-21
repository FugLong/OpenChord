#include "display_manager.h"
#include "pin_config.h"

DisplayManager::DisplayManager() : hw_(nullptr), healthy_(true) {
}

DisplayManager::~DisplayManager() {
    Shutdown();
}

void DisplayManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    healthy_ = true;
    
    // Initialize control pins first (DC and CS)
    InitDCPin();
    InitCSPin();
    
    // Initialize SPI interface
    InitSPI();
    
    // TODO: Initialize OLED display driver
    // TODO: Send initialization sequence to display
}

void DisplayManager::Update() {
    if (!hw_) return;
    
    // TODO: Update display
}

void DisplayManager::Shutdown() {
    if (!hw_) return;
    
    // TODO: Shutdown display
    healthy_ = false;
    hw_ = nullptr;
}

void DisplayManager::InitDCPin() {
    if (!hw_) return;
    
    // Configure DC pin (Pin 1, D0) as GPIO output
    // Using pin directly to avoid linker issues with static constexpr
    dc_pin_.Init(daisy::seed::D0, daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL);
    dc_pin_.Write(false);  // Start with command mode
}

void DisplayManager::InitCSPin() {
    if (!hw_) return;
    
    // Configure CS pin (Pin 8, D7) as GPIO output for manual chip select control
    // Even though we use SOFT NSS, we need to manually control CS for the display
    // Using pin directly to avoid linker issues with static constexpr
    cs_pin_.Init(daisy::seed::D7, daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL);
    cs_pin_.Write(true);  // CS is active low, so start with display deselected (high)
}

void DisplayManager::InitSPI() {
    if (!hw_) return;
    
    // Configure SPI1 for display - TWO_LINES_TX_ONLY (transmit only, per Daisy docs)
    // Pins: CS=8 (D7, manually controlled GPIO), SCK=9 (D8), MOSI=11 (D10)
    // Note: Physical pin numbers don't match Daisy pin names!
    // Following Daisy Seed SPI documentation for transmit-only communication
    daisy::SpiHandle::Config spi_config;
    spi_config.periph = daisy::SpiHandle::Config::Peripheral::SPI_1;
    spi_config.mode = daisy::SpiHandle::Config::Mode::MASTER;
    spi_config.direction = daisy::SpiHandle::Config::Direction::TWO_LINES_TX_ONLY;  // Transmit only (simplex TX)
    spi_config.datasize = 8;  // 8 bits per transmission
    spi_config.clock_polarity = daisy::SpiHandle::Config::ClockPolarity::LOW;  // Clock active low (default)
    spi_config.clock_phase = daisy::SpiHandle::Config::ClockPhase::ONE_EDGE;  // Data read on first edge (default)
    spi_config.nss = daisy::SpiHandle::Config::NSS::SOFT;  // Software chip select (CS handled manually via GPIO)
    spi_config.baud_prescaler = daisy::SpiHandle::Config::BaudPrescaler::PS_8;  // 25MHz / 8 = 3.125MHz (default)
    // Pin configuration - per Daisy docs, unused pins can be set to Pin()
    // CRITICAL: Use correct Daisy pin names (D8 for Pin 9, D10 for Pin 11)
    // Using pins directly to avoid linker issues with static constexpr
    spi_config.pin_config.sclk = daisy::seed::D8;   // Pin 9 = D8 = SPI1_SCK (clock)
    spi_config.pin_config.mosi = daisy::seed::D10;  // Pin 11 = D10 = SPI1_MOSI (data out)
    spi_config.pin_config.miso = daisy::Pin();      // Not used for TX_ONLY (set to Pin() per docs)
    spi_config.pin_config.nss = daisy::Pin();       // Not used for SOFT NSS (set to Pin() per docs)
    
    spi_handle_.Init(spi_config);
}
