#include "storage_manager.h"

StorageManager::StorageManager() 
    : hw_(nullptr), healthy_(true), card_present_(false), mounted_(false) {
}

StorageManager::~StorageManager() {
    Shutdown();
}

void StorageManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    healthy_ = true;
    card_present_ = false;
    mounted_ = false;
    
    if (hw_) {
        hw_->PrintLine("SD card: Initializing...");
    }
    
    // Initialize SDMMC handler with default configuration
    // Uses pins 2-7 (SDMMC1_D3, D2, D1, D0, CMD, CK)
    daisy::SdmmcHandler::Config sdmmc_cfg;
    sdmmc_cfg.Defaults();
    sdmmc_cfg.speed = daisy::SdmmcHandler::Speed::STANDARD; // Good balance of speed and reliability
    
    // Initialize SDMMC hardware
    daisy::SdmmcHandler::Result sdmmc_result = sdmmc_.Init(sdmmc_cfg);
    if (sdmmc_result == daisy::SdmmcHandler::Result::OK) {
        card_present_ = true;
        if (hw_) {
            hw_->PrintLine("SD card: Hardware detected");
        }
        
        // Initialize FatFS interface for SD card
        daisy::FatFSInterface::Config fsi_cfg;
        fsi_cfg.media = daisy::FatFSInterface::Config::MEDIA_SD;
        fsi_.Init(fsi_cfg);
        
        // Get reference to the FATFS filesystem object (required per Daisy example)
        // The FatFSInterface manages the FATFS object internally
        FATFS& fs = fsi_.GetSDFileSystem();
        const char* sd_path = fsi_.GetSDPath();  // Get SD card path (e.g., "0:")
        
        // Try to mount the filesystem
        FRESULT result = f_mount(&fs, sd_path, 0);
        
        // If mount fails due to no filesystem, format the card to FAT32
        if (result == FR_NO_FILESYSTEM) {
            if (hw_) {
                hw_->PrintLine("SD card: Not formatted, formatting to FAT32...");
            }
            
            // Work area buffer for formatting (512 bytes is standard sector size)
            // FatFS needs a buffer for formatting operations
            BYTE work[512];
            
            // Format the SD card to FAT32
            // f_mkfs(path, opt, au, work, len)
            // opt: FM_FAT32 = format as FAT32
            // au: 0 = auto-determine allocation unit size
            result = f_mkfs(sd_path, FM_FAT32, 0, work, sizeof(work));
            if (result == FR_OK) {
                if (hw_) {
                    hw_->PrintLine("SD card: Format complete");
                }
                // Try mounting again after format
                result = f_mount(&fs, sd_path, 1);  // Force remount
            } else {
                if (hw_) {
                    hw_->PrintLine("SD card: Format failed (error %d)", result);
                }
            }
        }
        
        if (result == FR_OK) {
            mounted_ = true;
            healthy_ = true;
            if (hw_) {
                hw_->PrintLine("SD card: Mounted successfully");
            }
        } else {
            // Mount failed for other reasons (hardware issue, etc.)
            if (hw_) {
                hw_->PrintLine("SD card: Mount failed (error %d)", result);
            }
            healthy_ = false;
            // Keep card_present_ = true since hardware init succeeded
        }
    } else {
        // SDMMC initialization failed (no card or hardware issue)
        if (hw_) {
            hw_->PrintLine("SD card: Hardware not detected (no card or wiring issue)");
        }
        healthy_ = false;
        card_present_ = false;
    }
}

void StorageManager::Update() {
    if (!hw_) return;
    
    // TODO: Update storage operations
    // - Check card presence
    // - Handle card removal/insertion
    // - Monitor filesystem health
}

bool StorageManager::TestCard() {
    if (!hw_ || !mounted_) {
        return false;
    }
    
    // Try to open and read the root directory to verify filesystem is working
    DIR dir;
    FRESULT result = f_opendir(&dir, "/");
    
    if (result == FR_OK) {
        f_closedir(&dir);
        return true;
    }
    
    return false;
}

void StorageManager::Shutdown() {
    if (!hw_) return;
    
    // Unmount filesystem if mounted
    if (mounted_) {
        f_mount(nullptr, "/", 0);  // Unmount
        mounted_ = false;
    }
    
    healthy_ = false;
    card_present_ = false;
    hw_ = nullptr;
}
