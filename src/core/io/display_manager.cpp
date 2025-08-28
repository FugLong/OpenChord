#include "display_manager.h"

DisplayManager::DisplayManager() : hw_(nullptr), healthy_(true) {
}

DisplayManager::~DisplayManager() {
    Shutdown();
}

void DisplayManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    healthy_ = true;
    
    // TODO: Initialize OLED display
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
