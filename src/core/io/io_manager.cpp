#include "io_manager.h"
#include "pin_config.h"
#include "digital_manager.h"
#include "analog_manager.h"
#include "serial_manager.h"
#include "display_manager.h"
#include "storage_manager.h"
#include <cstring>

IOManager::IOManager() 
    : hw_(nullptr), digital_(nullptr), analog_(nullptr), serial_(nullptr), 
      display_(nullptr), storage_(nullptr), update_count_(0), last_update_time_(0) {
    
    // Initialize system status
    memset(&status_, 0, sizeof(status_));
    status_.digital_healthy = true;
    status_.analog_healthy = true;
    status_.serial_healthy = true;
    status_.display_healthy = true;
    status_.storage_healthy = true;
}

IOManager::~IOManager() {
    Shutdown();
}

void IOManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    
    // Create and initialize all sub-managers
    digital_ = new DigitalManager();
    analog_ = new AnalogManager();
    serial_ = new SerialManager();
    display_ = new DisplayManager();
    storage_ = new StorageManager();
    
    digital_->Init(hw);
    analog_->Init(hw);
    serial_->Init(hw);
    display_->Init(hw);
    storage_->Init(hw);
    
    // Reset update counters
    update_count_ = 0;
    last_update_time_ = 0;
    
    // Clear any previous errors
    ClearErrors();
}

void IOManager::Update() {
    if (!hw_) return;
    
    // Update all sub-managers
    if (digital_) digital_->Update();
    if (analog_) analog_->Update();
    if (serial_) serial_->Update();
    if (display_) display_->Update();
    if (storage_) storage_->Update();
    
    // Update system status
    UpdateSystemStatus();
    
    // Handle any errors that occurred
    HandleErrors();
    
    // Update counters
    update_count_++;
    last_update_time_ = hw_->system.GetNow();
}

void IOManager::Shutdown() {
    if (!hw_) return;
    
    // Shutdown all sub-managers
    if (digital_) digital_->Shutdown();
    if (analog_) analog_->Shutdown();
    if (serial_) serial_->Shutdown();
    if (display_) display_->Shutdown();
    if (storage_) storage_->Shutdown();
    
    // Delete sub-managers
    delete digital_;
    delete analog_;
    delete serial_;
    delete display_;
    delete storage_;
    
    digital_ = nullptr;
    analog_ = nullptr;
    serial_ = nullptr;
    display_ = nullptr;
    storage_ = nullptr;
    
    hw_ = nullptr;
}

bool IOManager::IsHealthy() const {
    return status_.digital_healthy && 
           status_.analog_healthy && 
           status_.serial_healthy && 
           status_.display_healthy && 
           status_.storage_healthy &&
           status_.error_count == 0;
}

const SystemStatus& IOManager::GetStatus() const {
    return status_;
}

void IOManager::ReportError(const char* component, const char* error) {
    status_.error_count++;
    status_.last_error_time = hw_ ? hw_->system.GetNow() : 0;
    
    // Mark the appropriate component as unhealthy
    if (strcmp(component, "digital") == 0) {
        status_.digital_healthy = false;
    } else if (strcmp(component, "analog") == 0) {
        status_.analog_healthy = false;
    } else if (strcmp(component, "serial") == 0) {
        status_.serial_healthy = false;
    } else if (strcmp(component, "display") == 0) {
        status_.display_healthy = false;
    } else if (strcmp(component, "storage") == 0) {
        status_.storage_healthy = false;
    }
    
    // TODO: Add proper error logging when we implement it
    // For now, we could use the Daisy Seed's serial output for debugging
}

void IOManager::ClearErrors() {
    status_.error_count = 0;
    status_.last_error_time = 0;
    status_.digital_healthy = true;
    status_.analog_healthy = true;
    status_.serial_healthy = true;
    status_.display_healthy = true;
    status_.storage_healthy = true;
}

void IOManager::UpdateSystemStatus() {
    // Check health of each sub-manager
    status_.digital_healthy = digital_ ? digital_->IsHealthy() : false;
    status_.analog_healthy = analog_ ? analog_->IsHealthy() : false;
    status_.serial_healthy = serial_ ? serial_->IsHealthy() : false;
    status_.display_healthy = display_ ? display_->IsHealthy() : false;
    status_.storage_healthy = storage_ ? storage_->IsHealthy() : false;
}

void IOManager::HandleErrors() {
    // TODO: Implement error recovery strategies
    // For now, just track errors in the status
    // Later we can add:
    // - Automatic retry mechanisms
    // - Fallback modes
    // - User notifications
    // - Error logging
} 