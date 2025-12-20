#pragma once

#include "daisy_seed.h"
#include "fatfs.h"
#include <cstdint>

/**
 * Storage Manager - Handles SD card operations and file management
 * 
 * Manages:
 * - SD card initialization and mounting (SDMMC interface)
 * - File system operations (FatFS)
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
    bool IsCardPresent() const { return card_present_; }
    bool IsMounted() const { return mounted_; }
    
    // Test SD card functionality
    bool TestCard();
    
    // File system access
    FATFS* GetFileSystem() { return mounted_ ? &fsi_.GetSDFileSystem() : nullptr; }
    
    // TODO: Implement storage methods
    // - File operations
    // - Audio file handling
    // - Configuration storage
    // - Error recovery
    
private:
    daisy::DaisySeed* hw_;
    bool healthy_;
    bool card_present_;
    bool mounted_;
    
    // SDMMC handler for hardware interface
    daisy::SdmmcHandler sdmmc_;
    
    // FatFS filesystem interface (manages FATFS object internally)
    daisy::FatFSInterface fsi_;
};
