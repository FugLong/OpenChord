#pragma once

#include "daisy_seed.h"
#include <cstdint>

// Include the actual manager headers
#include "digital_manager.h"
#include "analog_manager.h"
#include "serial_manager.h"
#include "display_manager.h"
#include "storage_manager.h"

// System status structure
struct SystemStatus {
    bool digital_healthy;
    bool analog_healthy;
    bool serial_healthy;
    bool display_healthy;
    bool storage_healthy;
    uint32_t error_count;
    uint32_t last_error_time;
};

/**
 * Main IO Manager - Coordinates all hardware I/O operations
 * 
 * This class serves as the central coordinator for all input/output operations
 * in the OpenChord system. It manages specialized managers for different
 * types of I/O and provides a unified interface for the rest of the system.
 */
class IOManager {
public:
    // Constructor and destructor
    IOManager();
    ~IOManager();
    
    // Core lifecycle
    void Init(daisy::DaisySeed* hw);
    void Update();
    void Shutdown();
    
    // Manager access - provides access to specialized IO managers
    DigitalManager* GetDigital() { return digital_; }
    AnalogManager* GetAnalog() { return analog_; }
    SerialManager* GetSerial() { return serial_; }
    DisplayManager* GetDisplay() { return display_; }
    StorageManager* GetStorage() { return storage_; }
    
    // System status and health monitoring
    bool IsHealthy() const;
    const SystemStatus& GetStatus() const;
    
    // Error handling
    void ReportError(const char* component, const char* error);
    void ClearErrors();
    
    // System information
    uint32_t GetUpdateCount() const { return update_count_; }
    uint32_t GetLastUpdateTime() const { return last_update_time_; }

private:
    // Hardware reference
    daisy::DaisySeed* hw_;
    
    // Sub-managers for different IO types
    DigitalManager* digital_;
    AnalogManager* analog_;
    SerialManager* serial_;
    DisplayManager* display_;
    StorageManager* storage_;
    
    // System status and monitoring
    SystemStatus status_;
    uint32_t update_count_;
    uint32_t last_update_time_;
    
    // Internal methods
    void UpdateSystemStatus();
    void HandleErrors();
}; 