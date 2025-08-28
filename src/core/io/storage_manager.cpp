#include "storage_manager.h"

StorageManager::StorageManager() : hw_(nullptr), healthy_(true) {
}

StorageManager::~StorageManager() {
    Shutdown();
}

void StorageManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    healthy_ = true;
    
    // TODO: Initialize SD card
}

void StorageManager::Update() {
    if (!hw_) return;
    
    // TODO: Update storage operations
}

void StorageManager::Shutdown() {
    if (!hw_) return;
    
    // TODO: Shutdown storage
    healthy_ = false;
    hw_ = nullptr;
}
