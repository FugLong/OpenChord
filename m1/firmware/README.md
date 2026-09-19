# M1 product firmware

Product RP2040 + TinyUSB MIDI. Not started.

Lab bring-up is [`../proto-rp2040`](../proto-rp2040) on a Zero. The chord logic belongs in `m1/engine`, not here. This folder is the sellable board: USB-C device, UART TRS, Gateron pads, **IQS572** trackpad + **QT2120** strip only, **3× EVQPUC02K** system (roles TBD), OLED. No USB host. No pots. No QT Key/Shift copper. Works with no plugin; the plugin may later push settings over SysEx.

**Match the rev A schematic**, not older docs. Source of truth: [`../hardware/support.md`](../hardware/support.md).

| Bus / pin | As built |
|-----------|----------|
| I²C | **I2C1** on GPIO10 SDA / GPIO11 SCL (not I2C0) |
| OLED / QT / IQS | `0x3C` / `0x1C` / `0x74` on that bus |
| IQS RDY / NRST | GPIO8 / GPIO9 |
| UART0 MIDI | GPIO0 TX / GPIO1 RX @ 31250 |
| Gateron SW1–8 | GPIO 24, 18, 25, 17, 2, 13, 6, 12 |
| EVQ SW9 / SW10 / SW11 | GPIO 4, 5, 3 |
| QT | Slider KEY0–2 only; **CHANGE** unconnected; RESET hard-tied 3V3 |
| BOOTSEL | Not a GPIO — `~QSPI_SS` via R5 1 kΩ; **R30 10 kΩ** SS→3V3 |
| QSPI flash | **W25Q32JVSSIQ** ([C179173](https://www.lcsc.com/product-detail/C179173.html)) 4 MB; `PICO_FLASH_SIZE_BYTES=4*1024*1024`; default `boot2_w25q080` |
