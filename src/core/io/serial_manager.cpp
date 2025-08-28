#include "serial_manager.h"

SerialManager::SerialManager() : hw_(nullptr), healthy_(true) {
}

SerialManager::~SerialManager() {
    Shutdown();
}

void SerialManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    healthy_ = true;
    
    // TODO: Initialize SPI, UART, and SDMMC
}

void SerialManager::Update() {
    if (!hw_) return;
    
    // TODO: Update serial communication
}

void SerialManager::Shutdown() {
    if (!hw_) return;
    
    // TODO: Shutdown serial communication
    healthy_ = false;
    hw_ = nullptr;
}
