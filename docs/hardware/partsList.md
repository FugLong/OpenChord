# 🎵 OpenChord Parts List

[← Back to Documentation](../README.md)

### 📋 Overview

This document contains the complete parts list for building the OpenChord hardware.

---

## 🔧 Hardware Components

### 💻 Core Components

| Component | Price | Link | Notes |
|-----------|-------|------|-------|
| **Daisy Seed** (64mb) | $24.00 | [electro-smith.com](https://electro-smith.com/products/daisy-seed?variant=45234245108004) | Main microcontroller with audio processing |
| **1.3" OLED Display** (128x64) - Adafruit #938 | $19.95 | [Adafruit](https://www.adafruit.com/product/938?srsltid=AfmBOoqArotUO8oXRElwZ378uK7_5B4PLuVrQ6lUtXmzo5qDvlWPG97c) or [Digikey](https://www.digikey.com/en/products/detail/adafruit-industries-llc/938/5774238) | User interface display |
| **Mini Joystick** - Adafruit #5628 | $4.50 | [Adafruit](https://www.adafruit.com/product/5628) or [Digikey](https://www.digikey.com/en/products/detail/adafruit-industries-llc/5628/21839788) | Navigation control |
| **USB-C Breakout Board** - Adafruit #4090 | $2.95 | [Adafruit](https://www.adafruit.com/product/4090) or [Digikey](https://www.digikey.com/en/products/detail/adafruit-industries-llc/4090/9951930?gclsrc=aw.ds&gad_source=1&gad_campaignid=20232005509&gbraid=0AAAAADrbLlgv5G918gXCw7Mjs2-GOwfEn&gclid=CjwKCAjw7fzDBhA7EiwAOqJkh7rJU_q2HvSHWckA-OhgfuaNkxFl6pKy-FL9vPUBsJYN4sl-CAQn4BoCMyEQAvD_BwE) | USB connectivity |
| **MicroSD Card Breakout** - Adafruit #4682 | $3.50 | [Adafruit](https://www.adafruit.com/product/4682) or [Digikey](https://www.digikey.com/en/products/detail/adafruit-industries-llc/4682/12822319?gclsrc=aw.ds&gad_source=1&gad_campaignid=20243136172&gbraid=0AAAAADrbLljgtgGMP-Hf9vXbbCgmAhdnL&gclid=Cj0KCQjw4qHEBhCDARIsALYKFNOMCrdoUGhWjUQsv7L68_WBWqd5tObZsJBwlpf_pgUr4mqvSdI4JGUaAmazEALw_wcB) | Storage expansion |
| **3.5mm TRS Female Jack** (Audio out) | $0.85 | [Adafruit](https://www.adafruit.com/product/1699) or [Digikey](https://www.digikey.com/en/products/detail/kycon-inc/STX-3120-5B/9990114) | Audio output |
| **3.5mm TRS Female Jack** (Line in, MIDI in/out) × 3 | $1.73/ea | [Digikey](https://www.digikey.com/en/products/detail/kycon-inc/STX-3790-5N/9990125) | Audio input and MIDI I/O |
| **Wheel Potentiometer** (Volume) | $2.49 | [Digikey](https://www.digikey.com/en/products/detail/alps-alpine/RK10J12E0A0A/21721435) | Volume control |
| **Rotary Encoder** (EC12E2420301 or EC12E2440301) | $2.81 | [Digikey](https://www.digikey.com/en/products/detail/alps-alpine/EC12E2440301/21721630) | Menu navigation |
| **GATERON Low-Profile Key Switches** (35 pcs) | $15.99 | [Amazon](https://a.co/d/7Rx4Xij) | Only 11 used in final design |

---

### 🔋 Extended Components

| Component | Price | Link | Notes |
|-----------|-------|------|-------|
| **Electret Microphone Amplifier** - MAX9814 with Auto Gain Control | $7.95 | [Adafruit #1713](https://www.adafruit.com/product/1713) | Microphone amplification |
| **PowerBoost 1000C** (Battery charging + boost) | $19.95 | [Adafruit #2465](https://www.adafruit.com/product/2465) | Battery management |
| **LiPo Battery** (2000mAh) | $12.50 | [Adafruit #2011](https://www.adafruit.com/product/2011) | Portable power |

| **Replacement Mic** (Primo EM272Z1) *optional* | $13.00 | [Micbooster](https://micbooster.com/product/primo-em272/?v=0b3b97fa6688) | Premium microphone upgrade |

### 💰 Cost Summary

- **Core Cost**: ~$85
- **Total Cost**: ~$121 (+$13 for optional premium mic)

---

## 📚 Component Guides

### 🔊 Audio Output – Simple Voltage Divider

The audio output uses a simple voltage divider to reduce the Daisy Seed's output level so it doesn't blow your ears out.

| Function | Qty | Type | Recommended Value | Notes |
|----------|-----|------|-------------------|-------|
| **Output Resistors** | 2 | 1% metal film | 4.7kΩ | In series with audio output (targets ~3x reduction) |
| **Ground Resistors** | 2 | 1% metal film | 1kΩ | From audio channels to ground |

---

### 🎛️ Essential Passives

| Component | Qty | Purpose |
|-----------|-----|---------|
| **4.7kΩ Resistors** | 2 | Audio output voltage divider |
| **1kΩ Resistors** | 2 | Audio output to ground |
| **100kΩ Resistor** | 1 | Battery voltage divider (top) |
| **47kΩ Resistor** | 1 | Battery voltage divider (bottom) |
| **220Ω Resistor** | 1 | MIDI Out current limiting (Daisy TX to TRS tip) |

---

### 🎹 MIDI TRS Components

| Component | Qty | Purpose |
|-----------|-----|---------|
| **6N138/LTV-817S-TA1/PC817 Opto-Isolator** | 1 | MIDI In isolation |

---

### ⌨️ Key Matrix Components

| Component | Qty | Purpose |
|-----------|-----|---------|
| **1N4148 Diodes** | 11 | Key matrix diodes (one per key) |
| **10kΩ Resistors** | 2-3 | Strategic pull-downs (if needed) |

---


