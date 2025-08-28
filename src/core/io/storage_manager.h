#pragma once

#include "daisy_seed.h"
#include <cstdint>

/**
 * Storage Manager - Handles SD card operations and file management
 * 
 * Manages:
 * - SD card initialization and mounting
 * - File system operations
 * - Audio file I/O
 * - Configuration storage
 * - Error recovery
 */
class StorageManager {
public:
    StorageManager();
    ~StorageManager();
    
    // Core lifecycle
    void Init(daisy::DaisySeed* hw);
    void Update();
    void Shutdown();
    
    // Health monitoring
    bool IsHealthy() const { return healthy_; }
    
    // TODO: Implement storage methods
    // - File operations
    // - Audio file handling
    // - Configuration storage
    // - Error recovery
    
private:
    daisy::DaisySeed* hw_;
    bool healthy_;
    
    // TODO: Add storage members
};
