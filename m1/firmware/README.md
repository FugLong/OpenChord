# M1 product firmware

Product RP2040 + TinyUSB MIDI. Not started.

Lab bring-up is [`../proto-rp2040`](../proto-rp2040) on a Zero. The chord logic belongs in `m1/engine`, not here. This folder is the sellable board: USB-C device, UART TRS, Gateron pads, **IQS572** trackpad + **QT2120** strip/Key/Shift, edge **EVQ-PUA02K** mode, OLED. No USB host. No pots. Works with no plugin; the plugin may later push settings over SysEx.
