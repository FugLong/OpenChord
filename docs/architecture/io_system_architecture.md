# OpenChord IO System Architecture

[← Back to Documentation](../README.md)

## Overview

This document outlines the IO system architecture for OpenChord, designed to handle all hardware components in a clean, modular, and maintainable way. The system follows Daisy Seed best practices and provides a unified interface for all input/output operations.

## Design Principles

1. **Modularity**: Each IO component type is handled by a dedicated manager class
2. **Unified Interface**: Common patterns for all input types (digital, analog, serial)
3. **Daisy-Native**: Leverages Daisy Seed's built-in peripherals and patterns
4. **Performance**: Efficient polling and interrupt-driven approaches where appropriate
5. **Extensibility**: Easy to add new IO components without breaking existing code
6. **Error Handling**: Robust error detection and recovery for hardware issues
7. **Centralized Configuration**: All pin assignments defined in one place (`pin_config.h`)

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    IO Manager (Main Coordinator)            │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────┐ │
│  │   Digital   │ │   Analog    │ │   Serial    │ │ Display │ │
│  │   Manager   │ │   Manager   │ │   Manager   │ │ Manager │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────┘ │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────┐ │
│  │   Storage   │ │   Audio     │ │   MIDI      │ │   UI    │ │
│  │   Manager   │ │   Manager   │ │   Manager   │ │ Manager │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Component Managers

### 1. Digital IO Manager
Handles all digital inputs and outputs including buttons, encoder, and control signals.

**Components:**
- **Key Matrix (4x3)**: 11 keys using 7 pins (4 rows, 3 columns)
- **Rotary Encoder**: 2 pins for quadrature, 1 for button (optional)
- **Joystick Button**: 1 pin for digital input
- **Audio Input Switch**: 1 pin for TRS/line input selection
- **Status LEDs**: 4 pins for visual feedback (if implemented)

**Features:**
- Matrix scanning with debouncing
- Encoder quadrature decoding with acceleration
- Button state tracking (pressed, held, released)
- LED brightness control via PWM

### 2. Analog IO Manager
Handles all analog inputs including potentiometers, joystick axes, and sensors.

**Components:**
- **Volume Potentiometer**: ADC0 (Pin 22)
- **Joystick X-Axis**: ADC2 (Pin 24)
- **Joystick Y-Axis**: ADC3 (Pin 25)
- **Microphone Input**: ADC1 (Pin 23)
- **Battery Monitor**: ADC4 (Pin 26)

**Features:**
- Automatic ADC configuration and calibration
- Noise filtering and smoothing
- Calibration for joystick dead zones
- Battery voltage monitoring with low-battery detection
- Self-initialization using centralized pin configuration

### 3. Serial IO Manager
Handles all serial communication including MIDI, display, and storage.

**Components:**
- **SPI Display**: Pins 8-11 (SPI1)
- **SD Card**: Pins 2-7 (SDMMC1)
- **UART MIDI**: Pins 12-13 (UART4)

**Features:**
- SPI display driver with double buffering
- SD card file system management
- MIDI UART with proper timing
- Error handling and recovery

### 4. Display Manager
Specialized manager for the OLED display with advanced UI features.

**Features:**
- Double buffering for smooth animations
- Font rendering and text layout
- Graphics primitives (lines, circles, rectangles)
- UI state management
- Plugin-driven custom displays

### 5. Storage Manager
Handles SD card operations and file management.

**Features:**
- File system abstraction
- Audio file I/O
- Configuration storage
- Project file management
- Error recovery and corruption detection

## Implementation Strategy

### Phase 1: Core Infrastructure
1. **IO Manager Base Class**: Abstract interface for all managers
2. **Digital Manager**: Button matrix, encoder, basic digital I/O
3. **Analog Manager**: ADC setup, basic analog reading
4. **Error Handling**: Centralized error reporting and recovery

### Phase 2: Communication Systems
1. **Serial Manager**: SPI, UART, and SDMMC initialization
2. **Display Manager**: Basic OLED driver and text rendering
3. **Storage Manager**: SD card initialization and basic file operations

### Phase 3: Advanced Features
1. **Enhanced Digital**: LED control, advanced button features
2. **Enhanced Analog**: Calibration, filtering, advanced processing
3. **UI Integration**: Plugin-driven display system
4. **Performance Optimization**: Interrupt-driven updates where possible

## Class Structure

### IO Manager (Main Coordinator)
```cpp
class IOManager {
public:
    // Core lifecycle
    void Init(daisy::DaisySeed* hw);
    void Update();
    void Shutdown();
    
    // Manager access
    DigitalManager* GetDigital() { return digital_; }
    AnalogManager* GetAnalog() { return analog_; }
    SerialManager* GetSerial() { return serial_; }
    DisplayManager* GetDisplay() { return display_; }
    StorageManager* GetStorage() { return storage_; }
    
    // System status
    bool IsHealthy() const;
    const SystemStatus& GetStatus() const;
    
private:
    DigitalManager* digital_;
    AnalogManager* analog_;
    SerialManager* serial_;
    DisplayManager* display_;
    StorageManager* storage_;
    SystemStatus status_;
};
```

### Digital Manager
```cpp
class DigitalManager {
public:
    // Key matrix
    bool IsKeyPressed(int row, int col) const;
    bool WasKeyPressed(int row, int col) const;
    const KeyMatrixState& GetKeyMatrix() const;
    
    // Encoder
    int GetEncoderValue() const;
    float GetEncoderDelta() const;
    bool IsEncoderButtonPressed() const;
    
    // Joystick button
    bool IsJoystickButtonPressed() const;
    bool WasJoystickButtonPressed() const;
    
    // Audio switch
    bool IsAudioInputSwitched() const;
    
    // LEDs
    void SetLED(int led, bool state);
    void SetLEDBrightness(int led, float brightness);
    
private:
    KeyMatrix key_matrix_;
    Encoder encoder_;
    Button joystick_button_;
    Button audio_switch_;
    LEDArray leds_;
};
```

### Analog Manager
```cpp
class AnalogManager {
public:
    // Volume control
    float GetVolume() const;
    float GetVolumeNormalized() const;
    
    // Joystick
    void GetJoystick(float* x, float* y) const;
    float GetJoystickX() const;
    float GetJoystickY() const;
    
    // Microphone
    float GetMicrophoneLevel() const;
    bool IsMicrophoneClipping() const;
    
    // Battery
    float GetBatteryVoltage() const;
    float GetBatteryPercentage() const;
    bool IsLowBattery() const;
    
    // Calibration
    void CalibrateJoystick();
    void CalibrateVolume();
    
private:
    daisy::AdcChannelConfig adc_configs_[5];
    AnalogInput volume_;
    AnalogInput joystick_x_;
    AnalogInput joystick_y_;
    AnalogInput microphone_;
    AnalogInput battery_;
};
```

## Pin Assignment Summary

| Function | Daisy Pin | STM32 Pin | Manager | Notes |
|----------|-----------|-----------|---------|-------|
| **Key Matrix** | 27-33 | Various | Digital | 4x3 matrix, 7 pins |
| **Encoder** | 34-35 | Various | Digital | Quadrature A/B |
| **Joystick Button** | 14 | PB6 | Digital | GPIO input |
| **Audio Switch** | 15 | PB7 | Digital | GPIO input |
| **Volume Pot** | 22 | PC0 | Analog | ADC0 |
| **Joystick X** | 24 | PB1 | Analog | ADC2 |
| **Joystick Y** | 25 | PA7 | Analog | ADC3 |
| **Microphone** | 23 | PA3 | Analog | ADC1 |
| **Battery** | 26 | PA6 | Analog | ADC4 |
| **Display SPI** | 8-11 | Various | Serial | SPI1 |
| **SD Card** | 2-7 | Various | Serial | SDMMC1 |
| **MIDI UART** | 12-13 | PB8-PB9 | Serial | UART4 |

## Error Handling Strategy

### Hardware Fault Detection
- **ADC Failures**: Detect out-of-range values, implement fallback values
- **Digital Glitches**: Debouncing, state validation, error counters
- **Communication Errors**: Timeout detection, retry mechanisms
- **Storage Issues**: Corruption detection, backup/restore mechanisms

### Recovery Mechanisms
- **Automatic Retry**: Configurable retry counts for transient failures
- **Fallback Modes**: Degraded operation when components fail
- **User Notification**: Clear error messages and status indicators
- **Logging**: Persistent error logs for debugging

## Performance Considerations

### Update Frequencies
- **Digital I/O**: 1kHz (1ms) for responsive button handling
- **Analog I/O**: 100Hz (10ms) for smooth control response
- **Display**: 60Hz (16.7ms) for smooth animations
- **Storage**: On-demand for file operations
- **MIDI**: Real-time for low-latency performance

### Memory Management
- **Static Allocation**: Pre-allocate buffers to avoid fragmentation
- **Pool Management**: Efficient memory pools for dynamic objects
- **Cache Optimization**: Minimize memory access patterns
- **DMA Usage**: Use DMA for SPI and audio operations where possible

## Integration with Other Systems

### Audio System
The IO system provides volume control and other analog inputs to the audio engine. The `VolumeManager` coordinates between the `AnalogManager` and `AudioEngine` to provide sophisticated volume scaling.

### MIDI System
Digital inputs (key matrix, encoder) and analog inputs (joystick) can be mapped to MIDI control changes, notes, and other musical events.

### Plugin System
The IO system is designed to integrate with a plugin architecture, allowing plugins to:
- Register custom input handlers
- Define custom control mappings
- Create custom UI displays
- Handle specialized hardware

## Conclusion

This IO system architecture provides a solid foundation for OpenChord's hardware interface needs. The modular design ensures maintainability and extensibility while following Daisy Seed best practices. The phased implementation approach allows for incremental development and testing, ensuring each component is robust before moving to the next phase.

The system is designed to handle all current hardware requirements while providing clear paths for future enhancements and plugin integration.
