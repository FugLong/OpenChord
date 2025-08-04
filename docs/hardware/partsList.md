# 🎵 OpenChord Parts List

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
| **OPA1656** (Headphone) Amp (SOIC-8) | $3.00 | [Digikey](https://www.digikey.com/en/products/detail/texas-instruments/OPA1656ID/10434553) | Audio amplification |
| **SOIC-8 Breakout Board** | $2.95 | [Adafruit](https://www.adafruit.com/product/1212?srsltid=AfmBOoropCMMlkGJ10J4FgTWhxWk_dSRIgU53Iwowsno4qMOiqG6-rZA) or [Digikey](https://www.digikey.com/en/products/detail/adafruit-industries-llc/1212/5022800) | OPA1656 mounting |
| **Replacement Mic** (Primo EM272Z1) *optional* | $13.00 | [Micbooster](https://micbooster.com/product/primo-em272/?v=0b3b97fa6688) | Premium microphone upgrade |

### 💰 Cost Summary

- **Core Cost**: ~$85
- **Total Cost**: ~$130 (+$13 for optional premium mic)

---

## 📚 Component Guides

### 🔊 Audio Output – OPA1656 Component Guide

These components are required around the headphone/line out amplifier.

| Function | Qty | Type | Recommended Value | Notes |
|----------|-----|------|-------------------|-------|
| **DC Blocking Caps** | 2 | C0G / Film | 2.2–4.7 µF | Prevent DC offset at output (headphone safe) |
| **Output Resistors** | 2 | 1% metal film | 100–220Ω | Helps stabilize op-amp, protects against shorts |
| **Virtual Ground Resistors** | 2 | 1% | 10kΩ | For biasing signal path (if needed) |
| **Bypass Caps (V+/V-)** | 2 | X7R Ceramic | 100 nF | Place close to op-amp power pins |
| **Bulk Cap (Optional)** | 1 | Electrolytic | 10–47 µF | Helps during transient load swings |

---

### 🎛️ General Audio Path Passives

| Component | Purpose |
|-----------|---------|
| **2.2–4.7 µF C0G/Film** | DC blocking caps for line/headphone out (non-polar recommended) |
| **100 nF Ceramic** | Local decoupling near analog ICs (op-amps, MAX9814) |
| **100–220Ω Resistors** | Output protection (to line/headphones); matches OPA1656 section |
| **1kΩ Resistors** | Use in simple RC low-pass filters or pad circuits (optional) |
| **10kΩ Resistors** | Pull-downs, signal biasing, or analog voltage dividers |
| **1MΩ Resistor** | Needed if using EM272Z1 capsule (input pulldown, matches its datasheet) |

---

### ⚡ Power Filtering & Protection

| Component | Purpose |
|-----------|---------|
| **10 µF Electrolytic or Tantalum** | Bulk rail cap (one per power domain ideally) |
| **100 nF Ceramic (X7R/C0G)** | Standard decoupling cap — use near every IC, analog + digital |
| **TVS or Schottky Diodes** | USB VBUS ESD protection, reverse polarity guard (optional) |

### Interface Passives

| Component | Purpose |
|-----------|---------|
| **10kΩ Potentiometer** | Volume knob (log taper preferred) |
| **10kΩ Pull-downs** | For joystick button, encoder button, and digital inputs |
| **100 nF Ceramic Caps** | Debounce/filtering for buttons or encoder lines |

---

### 🧠 Daisy-Specific Circuitry

| Component | Purpose |
|-----------|---------|
| **100 nF Ceramic Caps** | On all analog input lines (ADC) for noise suppression |
| **10–100kΩ Resistors** | For analog input scaling or voltage dividers (e.g. battery) |
| **Clamping Diodes (Schottky)** | For protecting ADCs from over-voltage (optional) |

### Mic Input (MAX9814 with Electret or EM272Z1 Capsule Replacement)

| Component | Purpose |
|-----------|---------|
| **MAX9814 Module** | Amplifies mic signal with AGC (even if EM272Z1 is swapped in) |
| **100 nF Ceramic** | Decoupling on MAX9814 power line (place near Vcc) |
| **1MΩ Resistor** | Needed if using EM272Z1 (to match high output impedance) |

---

### 🔋 Battery Monitor

| Component | Purpose |
|-----------|---------|
| **100kΩ Resistor** | Top of voltage divider (VBAT to ADC4) |
| **47kΩ Resistor** | Bottom of divider (ADC4 to GND) |
| **100 nF Ceramic** | Filters noise at ADC4 input (across 47k) |

### MIDI Output (TRS MIDI Out)

| Component | Purpose |
|-----------|---------|
| **220Ω Resistor** | In series between Daisy TX and TRS tip |
| **(Optional) TVS Diode** | On TRS output for ESD protection |

---

### 🎹 MIDI Input (TRS MIDI In w/ Opto-Isolator)

| Component | Purpose |
|-----------|---------|
| **6N138 Opto-Isolator** | MIDI In isolation |
| **220Ω Resistor** | Limits current to opto input |
| **10kΩ Resistor** | Pull-up from opto collector to 3.3V |
| **100 nF Ceramic** | Decoupling for opto IC |
| **(Optional) 1N4148 Diode** | Reverse protection across opto input LED |

### Misc – Handy Inventory

| Component | Purpose |
|-----------|---------|
| **Trimpots (10kΩ)** | Optional tuning of gain or voltage divider |
| **Zener Diodes** | Clamping for critical digital/analog rails (optional) |
| **Extra Header Pins** | For all breakout boards, debug access |
| **Debug Jumpers** | Testing power, signal lines, ground loops |
| **Prototyping Wires** | Always useful for quick changes or fixes |

---

## 📝 Notes

- **Core components** provide the essential functionality for basic operation
- **Extended components** add battery power, enhanced audio (headphone drive vs default line out), and premium microphone options
- **Component guides** provide detailed specifications for proper circuit implementation
- **Passive components** are essential for proper circuit operation and noise reduction
- **MIDI components** enable standard MIDI connectivity for external device integration
