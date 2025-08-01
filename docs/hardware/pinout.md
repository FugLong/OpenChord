# OpenChord Pinout Documentation (WIP)

## Overview

This document tracks the detailed pin assignments for the OpenChord custom hardware setup. The Daisy Seed has limited pins, so every pin must be carefully allocated.

<style>
.pinout-image {
  display: block;
  margin: 0 auto;
  max-width: 100%;
  height: auto;
}

@media (prefers-color-scheme: dark) {
  .pinout-light { display: none; }
  .pinout-dark { display: block; }
}

@media (prefers-color-scheme: light) {
  .pinout-light { display: block; }
  .pinout-dark { display: none; }
}
</style>

<img class="pinout-image pinout-dark" src="https://daisy.nyc3.cdn.digitaloceanspaces.com/products/seed/Seed_pinout_dark.png" alt="Daisy Seed Pinout (Dark Mode)">
<img class="pinout-image pinout-light" src="https://daisy.nyc3.cdn.digitaloceanspaces.com/products/seed/Seed_pinout_light.png" alt="Daisy Seed Pinout (Light Mode)">

## Pin Usage Summary

| Category | Pins Used | Pin Numbers | Notes |
|----------|-----------|-------------|-------|
| Unused | 1 | 1 | USB ID pin |
| SDMMC (SD Card) | 6 | 2-7 | SD card interface |
| SPI (Display) | 4 | 8-11 | Display interface |
| MIDI (UART) | 2 | 12-13 | UART4 for MIDI I/O |
| User Interface | 8 | 14-15, 22-26, 34-35 | Joystick, volume, mic, encoder, battery |
| Audio I/O | 5 | 16-20 | Audio in/out + ground |
| Key Matrix | 7 | 27-33 | 11 keys in 4x3 matrix |
| USB | 2 | 36-37 | USB interface |
| Power & Ground | 4 | 21, 38-40 | 3V3A, 3V3D, VIN, GND |
| **Total** | **39** | **1-40** | **39 pins allocated, 1 unused** |


## Component Connection Details

| Component | Pin | Daisy Pin | Daisy Pin Name | Notes |
|-----------|-----|-----------|----------------|-------|
| **OLED Display** | VCC | 38 | +3V3D | Power |
| | GND | 40 | GND | Ground |
| | SCK | 9 | SPI1_SCK | Clock |
| | MOSI | 11 | SPI1_MOSI | Data out |
| | MISO | 10 | SPI1_MISO | Data in |
| | CS | 8 | SPI1_NSS | Chip select |
| | DC | - | Software | Controlled via SPI |
| **MicroSD Card** | VCC | 38 | +3V3D | Power |
| | GND | 40 | GND | Ground |
| | CLK | 7 | SDMMC1_CK | Clock |
| | CMD | 6 | SDMMC1_CMD | Command |
| | DAT0 | 5 | SDMMC1_D0 | Data 0 |
| | DAT1 | 4 | SDMMC1_D1 | Data 1 |
| | DAT2 | 3 | SDMMC1_D2 | Data 2 |
| | DAT3 | 2 | SDMMC1_D3 | Data 3 |
| **Joystick** | VCC | 21 | +3V3A | Power |
| | GND | 40 | GND | Ground |
| | VRX | 24 | ADC2 | X axis |
| | VRY | 25 | ADC3 | Y axis |
| | SW | 14 | GPIO | Button |
| **Encoder** | A | 34 | GPIO | Encoder A |
| | B | 35 | GPIO | Encoder B |
| | SW | - | - | Not connected |
| **Key Matrix** | Row 0 | 27 | GPIO | Matrix row |
| | Row 1 | 28 | GPIO | Matrix row |
| | Row 2 | 29 | GPIO | Matrix row |
| | Row 3 | 30 | GPIO | Matrix row |
| | Col 0 | 31 | GPIO | Matrix column |
| | Col 1 | 32 | GPIO | Matrix column |
| | Col 2 | 33 | GPIO | Matrix column |
| **Audio I/O** | Line Out L | 18 | AUDIO_OUT_L | Left output |
| | Line Out R | 19 | AUDIO_OUT_R | Right output |
| | Line In L | 16 | AUDIO_IN_L | Left input |
| | Line In R | 17 | AUDIO_IN_R | Right input |
| | AGND | 20 | AGND | Audio ground |
| **MIDI DIN** | TX | 13 | UART4_TX | MIDI out |
| | RX | 12 | UART4_RX | MIDI in |
| **Volume** | Pot | 22 | ADC0 | Volume control |
| **Microphone** | Mic In | 23 | ADC1 | Microphone input |
| **Battery** | Monitor | 26 | ADC4 | Voltage divider |
| **Audio Switch** | TRS Switch | 15 | GPIO | Input select |
| **USB** | D- | 36 | USB_HS_D- | USB data - |
| | D+ | 37 | USB_HS_D+ | USB data + |

## Notes

- **Key matrix** uses 7 pins for 11 keys (4x3 matrix with one unused position)
- **SPI and SDMMC** use hardware peripherals for optimal performance
- **Analog inputs** are used for joystick, volume, microphone, and battery monitoring
- **Digital I/O** used for buttons, encoder, and control signals
- **Audio I/O** uses dedicated audio pins for best performance

## Future Considerations

- If additional features are needed, some pins may need to be multiplexed
- Consider using I2C expanders for additional I/O if needed
- USB MIDI can reduce need for DIN MIDI pins if space is critical
- Some functions could be combined (e.g., encoder button could serve multiple purposes) 

## Daisy Seed Pinout Table Reference

| PINOUT | DAISY PIN NAME | STM32 PIN NAME | PRIMARY FUNCTION | ALT. FUNCTIONS 1 | ALT. FUNCTIONS 2 |
|--------|----------------|----------------|------------------|------------------|------------------|
| 1 | D0 | PB12 | GPIO | USB_HS_ID/UART5_RX/USART3_CK | TIM1_BKIN |
| 2 | D1 | PC11 | GPIO | SDMMC1_D3/USART3_RX/UART4_RX | SPI3_MISO/I2S3_SDI/HRTIM_FLT2 |
| 3 | D2 | PC10 | GPIO | SDMMC1_D2/USART3_TX/UART4_TX | SPI3_SCK/I2S3_CK/HRTIM_EEV1 |
| 4 | D3 | PC9 | GPIO | SDMMC1_D1/UART5_CTS | I2S_CKIN/MCO2 |
| 5 | D4 | PC8 | GPIO | SDMMC1_D0/UART5_RTS | |
| 6 | D5 | PD2 | GPIO | SDMMC1_CMD/UART5_RX | |
| 7 | D6 | PC12 | GPIO | SDMMC1_CK/UART5_TX/USART3_CK | SPI3_MOSI/I2S3_SDO |
| 8 | D7 | PG10 | GPIO | SPI1_NSS/I2S1_WS | HRTIM_FLT5 |
| 9 | D8 | PG11 | GPIO | SPI1_SCK/I2S1_CK | LPTIM1_IN2/HRTIM_EEV4 |
| 10 | D9 | PB4 | GPIO | SPI1_MISO/UART7_TX | SPI1_MISO/I2S1_SDI/SPI3_MISO/I2S3_SDI/SPI6_MISO |
| 11 | D10 | PB5 | GPIO | SPI1_MOSI/UART5_RX | SPI1_MOSI/I2S1_SDO/SPI3_MOSI/I2S3_SDO/SPI6_MOSI/I2C4_SMBA/TIM17_BKIN |
| 12 | D11 | PB8 | GPIO | I2C1_SCL/UART4_RX | I2C4_SCL/TIM16_CH1/TIM4_CH3 |
| 13 | D12 | PB9 | GPIO | I2C1_SDA/UART4_TX/SPI2_NSS/I2S2_WS | I2C4_SDA/I2C4_SMBA/TIM17_CH1/TIM4_CH4 |
| 14 | D13 | PB6 | GPIO | USART1_TX/LPUART1_TX/UART5_TX | I2C1_SCL/I2C4_SCL/TIM16_CH1N/TIM4_CH1 |
| 15 | D14 | PB7 | GPIO | USART1_RX/LPUART1_RX | I2C1_SDA/I2C4_SDA/TIM17_CH1N/TIM4_CH2 |
| 16 | NC | | AUDIO IN L | | |
| 17 | NC | | AUDIO IN R | | |
| 18 | NC | | AUDIO OUT L | | |
| 19 | NC | | AUDIO OUT R | | |
| 20 | NC | | AGND | | |
| 21 | NC | | +3V3A | | |
| 22 | A0, D15 | PC0 | GPIO | ADC0/SAI2_FS_B | |
| 23 | A1, D16 | PA3 | GPIO | ADC1/USART2_RX | TIM2_CH4/TIM5_CH4 |
| 24 | A2, D17 | PB1 | GPIO | ADC2 | TIM1_CH3N/TIM3_CH4 |
| 25 | A3, D18 | PA7 | GPIO | ADC3/SPI1_MOSI/I2S1_SDO/SPI6_MOSI | TIM1_CH1N/TIM3_CH2 |
| 26 | A4, D19 | PA6 | GPIO | ADC4/SPI1_MISO/I2S1_SDI/SPI6_MISO | TIM1_BKIN/TIM3_CH1 |
| 27 | A5, D20 | PC1 | GPIO | ADC5 | |
| 28 | A6, D21 | PC4 | GPIO | ADC6/I2S1_MCK | |
| 29 | A7, D22 | PA5 | GPIO | ADC7/DAC1_OUT2 | SPI1_SCK/I2S1_CK/SPI6_SCK/D2PWREN/TIM2_CH1 |
| 30 | A8, D23 | PA4 | GPIO | ADC8/DAC1_OUT1 | SPI1_NSS/I2S1_WS/SPI3_NSS/I2S3_WS/SPI6_NSS/D1PWREN |
| 31 | A9, D24 | PA1 | GPIO | ADC9/SAI2_MCLK_B | UART4_RX/TIM2_CH2/TIM5_CH2 |
| 32 | A10, D25 | PA0 | GPIO | ADC10/SAI2_SD_B | UART4_TX/TIM2_CH1/TIM2_ETR/TIM5_CH1 |
| 33 | D26 | PD11 | GPIO | SAI2_SD_A/I2C4_SMBA | LPTIM2_IN2 |
| 34 | D27 | PG9 | GPIO | SAI2_FS_B/USART6_RX | SPI1_MISO/I2S1_SDI |
| 35 | A11, D28 | PA2 | GPIO | ADC11/SAI2_SCK_B | USART2_TX/TIM2_CH3/TIM5_CH3 |
| 36 | D29 | PB14 | GPIO | USB_HS_D-/USART1_TX | TIM1_CH2N |
| 37 | D30 | PB15 | GPIO | USB_HS_D+/USART1_RX | |
| 38 | NC | | +3V3D | | |
| 39 | NC | | VIN | | |
| 40 | PG3 | | GND | | |