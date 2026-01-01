#pragma once

#include "daisy_seed.h"
#include <cstdint>

namespace OpenChord {

/**
 * Power Manager - Optimizes system for low power consumption
 * 
 * Tracks system activity and adjusts update rates dynamically:
 * - Reduces update frequencies when idle
 * - Increases frequencies when active
 * - Manages adaptive timing for main loop
 */
class PowerManager {
public:
    // Power modes - determines update rates
    enum class PowerMode {
        IDLE,        // Minimal updates (lowest power)
        LOW,         // Reduced updates (low power)
        NORMAL,      // Standard updates (normal power)
        ACTIVE       // Full updates (highest power, best responsiveness)
    };
    
    PowerManager();
    ~PowerManager();
    
    // Initialization
    void Init(daisy::DaisySeed* hw);
    
    // Activity tracking - call when user interacts or system is active
    void ReportActivity();
    void ReportUserInput();
    void ReportAudioActivity();
    
    // Update - call from main loop, returns recommended delay time
    uint32_t Update();
    
    // Get current power mode
    PowerMode GetPowerMode() const { return current_mode_; }
    
    // Get recommended update intervals (in milliseconds)
    uint32_t GetMainLoopInterval() const;
    uint32_t GetADCInterval() const;
    uint32_t GetDisplayInterval() const;
    // Note: Digital I/O always updates at high frequency (not power-managed) for proper debouncing
    
    // Check if updates should be skipped
    bool ShouldUpdateADC(uint32_t last_update_time) const;
    bool ShouldUpdateDisplay(uint32_t last_update_time) const;
    
    // Activity detection
    bool IsIdle() const { return current_mode_ == PowerMode::IDLE; }
    bool IsActive() const { return current_mode_ == PowerMode::ACTIVE; }
    
private:
    daisy::DaisySeed* hw_;
    
    // Activity tracking
    uint32_t last_activity_time_;
    uint32_t last_user_input_time_;
    uint32_t last_audio_activity_time_;
    uint32_t boot_start_time_;  // Time when power manager was initialized
    
    // Power mode
    PowerMode current_mode_;
    uint32_t mode_change_time_;
    
    // Activity thresholds (in milliseconds)
    static constexpr uint32_t IDLE_THRESHOLD_MS = 5000;      // 5 seconds of no activity = IDLE
    static constexpr uint32_t LOW_THRESHOLD_MS = 2000;      // 2 seconds of no activity = LOW
    static constexpr uint32_t ACTIVE_THRESHOLD_MS = 100;    // Recent activity = ACTIVE
    static constexpr uint32_t BOOT_PERIOD_MS = 3000;        // 3 seconds after init - stay in NORMAL mode
    
    // Update intervals per mode (in milliseconds)
    static constexpr uint32_t MAIN_LOOP_IDLE_MS = 10;       // 100 Hz when idle
    static constexpr uint32_t MAIN_LOOP_LOW_MS = 5;         // 200 Hz when low
    static constexpr uint32_t MAIN_LOOP_NORMAL_MS = 1;      // 1 kHz when normal
    static constexpr uint32_t MAIN_LOOP_ACTIVE_MS = 1;      // 1 kHz when active
    
    static constexpr uint32_t ADC_IDLE_MS = 100;             // 10 Hz when idle
    static constexpr uint32_t ADC_LOW_MS = 50;               // 20 Hz when low
    static constexpr uint32_t ADC_NORMAL_MS = 10;            // 100 Hz when normal
    static constexpr uint32_t ADC_ACTIVE_MS = 5;             // 200 Hz when active
    
    static constexpr uint32_t DISPLAY_IDLE_MS = 1000;        // 1 Hz when idle (power savings)
    static constexpr uint32_t DISPLAY_LOW_MS = 500;         // 2 Hz when low (power savings)
    static constexpr uint32_t DISPLAY_NORMAL_MS = 100;       // 10 Hz when normal (power optimized, sufficient for UI)
    static constexpr uint32_t DISPLAY_ACTIVE_MS = 50;        // 20 Hz when active (power optimized, smooth for interactions)
    
    // Note: Digital I/O update intervals removed - digital always updates at high frequency
    // (not power-managed) for proper debouncing and user responsiveness
    
    // Internal methods
    void UpdatePowerMode();
    PowerMode DeterminePowerMode() const;
};

} // namespace OpenChord
