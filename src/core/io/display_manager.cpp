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
    
    // Initialize OLED display
    InitDisplay();
    
    // Test display with "Hello World"
    TestDisplay();
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

// Low-level SPI communication functions
void DisplayManager::SendCommand(uint8_t cmd) {
    if (!hw_) return;
    
    // Set DC pin low for command mode
    dc_pin_.Write(false);
    
    // Pull CS low to select display
    cs_pin_.Write(false);
    
    // Send command byte
    spi_handle_.BlockingTransmit(&cmd, 1);
    
    // Pull CS high to deselect display
    cs_pin_.Write(true);
    
    // Small delay for display to process
    hw_->DelayMs(1);
}

void DisplayManager::SendData(uint8_t data) {
    if (!hw_) return;
    
    // Set DC pin high for data mode
    dc_pin_.Write(true);
    
    // Pull CS low to select display
    cs_pin_.Write(false);
    
    // Send data byte
    spi_handle_.BlockingTransmit(&data, 1);
    
    // Pull CS high to deselect display
    cs_pin_.Write(true);
}

void DisplayManager::SendDataBuffer(const uint8_t* data, size_t length) {
    if (!hw_ || !data || length == 0) return;
    
    // Set DC pin high for data mode
    dc_pin_.Write(true);
    
    // Pull CS low to select display
    cs_pin_.Write(false);
    
    // Send data buffer (cast away const since BlockingTransmit doesn't modify the data)
    spi_handle_.BlockingTransmit(const_cast<uint8_t*>(data), length);
    
    // Pull CS high to deselect display
    cs_pin_.Write(true);
}

void DisplayManager::InitDisplay() {
    if (!hw_) return;
    
    // Wait for display to power up and auto-reset circuit to complete
    // Adafruit #938 has auto-reset, so no manual reset needed
    // Ensure display is configured for SPI mode (not I2C) via jumpers
    hw_->DelayMs(150);  // Give auto-reset time to complete
    
    // SSD1306 initialization sequence for 128x64 display
    // Turn off display during initialization
    SendCommand(0xAE);  // Display OFF
    
    // Clock divide ratio and oscillator frequency
    SendCommand(0xD5);  // Set display clock divide ratio
    SendCommand(0x80);  // Default value
    
    // Multiplex ratio (64 for 128x64 display)
    SendCommand(0xA8);  // Set multiplex ratio
    SendCommand(0x3F);  // 64-1 = 63 = 0x3F
    
    // Display offset
    SendCommand(0xD3);  // Set display offset
    SendCommand(0x00);  // No offset
    
    // Start line address
    SendCommand(0x40);  // Set start line address (0)
    
    // Charge pump setting
    SendCommand(0x8D);  // Charge pump setting
    SendCommand(0x14);  // Enable charge pump (required for 3.3V operation)
    
    // Memory addressing mode
    SendCommand(0x20);  // Set memory addressing mode
    SendCommand(0x00);  // Horizontal addressing mode
    
    // Segment remap (flip horizontally)
    SendCommand(0xA1);  // Segment remap (column 127 mapped to SEG0)
    
    // COM scan direction (flip vertically)
    SendCommand(0xC8);  // COM output scan direction (scan from COM63 to COM0)
    
    // COM pins hardware configuration
    SendCommand(0xDA);  // Set COM pins hardware configuration
    SendCommand(0x12);  // Alternative COM pin configuration, disable COM left/right remap
    
    // Contrast control
    SendCommand(0x81);  // Set contrast control
    SendCommand(0xCF);  // Contrast value (0-255, default 0xCF)
    
    // Precharge period
    SendCommand(0xD9);  // Set precharge period
    SendCommand(0xF1);  // Phase 1: 15 DCLKs, Phase 2: 1 DCLK
    
    // VCOMH deselect level
    SendCommand(0xDB);  // Set VCOMH deselect level
    SendCommand(0x40);  // 0.77 * VCC
    
    // Display on
    SendCommand(0xA4);  // Display all on resume (use RAM contents)
    SendCommand(0xA6);  // Normal display (not inverted)
    SendCommand(0xAF);  // Display ON
    
    hw_->DelayMs(10);
}

void DisplayManager::Clear() {
    if (!hw_) return;
    
    // Set column address range (0-127)
    SendCommand(0x21);  // Set column address
    SendCommand(0x00);  // Start column
    SendCommand(0x7F);  // End column (127)
    
    // Set page address range (0-7)
    SendCommand(0x22);  // Set page address
    SendCommand(0x00);  // Start page
    SendCommand(0x07);  // End page (7)
    
    // Clear entire display (1024 bytes = 128 * 64 / 8)
    uint8_t zeros[128];
    for (int i = 0; i < 128; i++) {
        zeros[i] = 0x00;
    }
    
    // Send 8 pages of zeros (8 pages * 128 bytes = 1024 bytes = full display)
    for (int page = 0; page < 8; page++) {
        SendDataBuffer(zeros, 128);
    }
}

// Simple 5x7 font for basic text rendering (ASCII 32-127)
// Each character is 5 pixels wide, 7 pixels tall, stored column-wise
static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space (32)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x00, 0xA0, 0x60, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x59, 0x51, 0x3E}, // @
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};

void DisplayManager::TestDisplay() {
    if (!hw_) return;
    
    // Clear display first
    Clear();
    
    hw_->DelayMs(10);
    
    // Set cursor to top-left (column 0, page 0)
    SendCommand(0x21);  // Set column address
    SendCommand(0x00);  // Start column
    SendCommand(0x7F);  // End column
    
    SendCommand(0x22);  // Set page address
    SendCommand(0x00);  // Start page
    SendCommand(0x07);  // End page
    
    // Write "HELLO WORLD" at position (10, 0) - approximately
    const char* text = "HELLO WORLD";
    uint8_t start_col = 10;
    
    // Set starting column
    SendCommand(0x21);  // Set column address
    SendCommand(start_col);
    SendCommand(0x7F);
    
    SendCommand(0x22);  // Set page address (page 0 for top row)
    SendCommand(0x00);
    SendCommand(0x00);
    
    // Send each character
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        // Convert lowercase to uppercase for display
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
        if (c >= 32 && c <= 90) {  // Printable ASCII range (space to Z)
            const uint8_t* char_data = font_5x7[c - 32];
            SendDataBuffer(char_data, 5);
            // Add 1 pixel spacing between characters
            SendData(0x00);
        }
    }
}
