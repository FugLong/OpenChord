#include "storage_manager.h"
#include <cstring>
#include <cstdio>

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
    
    // Note: SDMMC Init can hang for 30-60 seconds if the card is in a bad state.
    // There's no way to add a timeout to the blocking Init() call without modifying
    // Daisy's library. If boot hangs here, it's likely due to SD card hardware issues.
    // The best workaround is to remove the SD card if boot hangs.
    
    // Get start time to log if init is slow
    uint32_t start_time = 0;
    if (hw_) {
        start_time = hw_->system.GetNow();
    }
    
    // Initialize SDMMC hardware (this may block for up to 60 seconds if card is problematic)
    daisy::SdmmcHandler::Result sdmmc_result = sdmmc_.Init(sdmmc_cfg);
    
    // Log if initialization took a long time (for debugging)
    if (hw_) {
        uint32_t elapsed = hw_->system.GetNow() - start_time;
        if (elapsed > 5000) {  // More than 5 seconds
            hw_->PrintLine("SD card: Init took %dms (slow)", elapsed);
        }
    }
    
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
            
            // Cleanup any leftover .bin files and bootloader log files from firmware updates
            int deleted_count = CleanupBinFiles();
            // Don't log cleanup - user doesn't need this info unless debugging
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

int StorageManager::CleanupBinFiles() {
    if (!mounted_ || !hw_) {
        return 0;
    }
    
    const char* sd_path = fsi_.GetSDPath();  // Get SD card path (e.g., "0:")
    if (!sd_path) {
        return 0;
    }
    
    DIR dir;
    FILINFO fno;  // File information structure
    int deleted_count = 0;
    
    // Open root directory
    FRESULT result = f_opendir(&dir, sd_path);
    if (result != FR_OK) {
        // Directory open failed, log error and return
        if (hw_) {
            hw_->PrintLine("SD card: Failed to open directory for cleanup (error %d)", result);
        }
        return 0;
    }
    
    // Read through all directory entries
    while (true) {
        result = f_readdir(&dir, &fno);
        if (result != FR_OK || fno.fname[0] == 0) {
            // End of directory or error
            break;
        }
        
        // Skip directories (only process files)
        if (fno.fattrib & AM_DIR) {
            continue;
        }
        
        const char* filename = fno.fname;
        size_t name_len = strlen(filename);
        bool should_delete = false;
        
        // Check if file ends with .bin extension (case-insensitive)
        if (name_len >= 4 && filename[name_len - 4] == '.') {
            char ext[4];
            ext[0] = filename[name_len - 3];
            ext[1] = filename[name_len - 2];
            ext[2] = filename[name_len - 1];
            ext[3] = '\0';
            
            // Case-insensitive comparison for "bin"
            if ((ext[0] == 'b' || ext[0] == 'B') &&
                (ext[1] == 'i' || ext[1] == 'I') &&
                (ext[2] == 'n' || ext[2] == 'N')) {
                should_delete = true;
            }
        }
        
        // Also delete bootloader log files (typically .log or .txt files created by bootloader)
        // Common names: bootloader.log, flash.log, or any .log/.txt file
        if (!should_delete && name_len >= 4) {
            // Check for .log extension
            if (name_len >= 5 && filename[name_len - 4] == '.' &&
                (filename[name_len - 3] == 'l' || filename[name_len - 3] == 'L') &&
                (filename[name_len - 2] == 'o' || filename[name_len - 2] == 'O') &&
                (filename[name_len - 1] == 'g' || filename[name_len - 1] == 'G')) {
                should_delete = true;
            }
            // Check for .txt extension (bootloader might use this)
            else if (name_len >= 5 && filename[name_len - 4] == '.' &&
                     (filename[name_len - 3] == 't' || filename[name_len - 3] == 'T') &&
                     (filename[name_len - 2] == 'x' || filename[name_len - 2] == 'X') &&
                     (filename[name_len - 1] == 't' || filename[name_len - 1] == 'T')) {
                // Only delete .txt files that look like bootloader logs
                // Check if filename contains "boot", "flash", or "log"
                bool is_bootlog = false;
                for (size_t i = 0; i < name_len - 4; i++) {
                    char c = filename[i];
                    if (c >= 'A' && c <= 'Z') c += 32; // Convert to lowercase
                    if ((c == 'b' && i + 4 < name_len && 
                         (filename[i+1] == 'o' || filename[i+1] == 'O') &&
                         (filename[i+2] == 'o' || filename[i+2] == 'O') &&
                         (filename[i+3] == 't' || filename[i+3] == 'T')) ||
                        (c == 'f' && i + 5 < name_len &&
                         (filename[i+1] == 'l' || filename[i+1] == 'L') &&
                         (filename[i+2] == 'a' || filename[i+2] == 'A') &&
                         (filename[i+3] == 's' || filename[i+3] == 'S') &&
                         (filename[i+4] == 'h' || filename[i+4] == 'H')) ||
                        (c == 'l' && i + 3 < name_len &&
                         (filename[i+1] == 'o' || filename[i+1] == 'O') &&
                         (filename[i+2] == 'g' || filename[i+2] == 'G'))) {
                        is_bootlog = true;
                        break;
                    }
                }
                should_delete = is_bootlog;
            }
        }
        
        if (should_delete) {
            // Construct full path for deletion
            // FatFS paths: "0:filename.bin" format
            char full_path[64];  // Increased size for longer paths
            int path_len = snprintf(full_path, sizeof(full_path), "%s/%s", sd_path, filename);
            
            if (path_len < 0 || path_len >= (int)sizeof(full_path)) {
                // Path too long, skip this file
                if (hw_) {
                    hw_->PrintLine("SD card: Path too long for %s, skipping", filename);
                }
                continue;
            }
            
            // Delete the file
            result = f_unlink(full_path);
            if (result == FR_OK) {
                deleted_count++;
                if (hw_) {
                    hw_->PrintLine("SD card: Deleted %s", filename);
                }
            } else {
                // Deletion failed, log but continue
                if (hw_) {
                    hw_->PrintLine("SD card: Failed to delete %s (error %d)", filename, result);
                }
            }
        }
    }
    
    // Close directory
    f_closedir(&dir);
    
    return deleted_count;
}
