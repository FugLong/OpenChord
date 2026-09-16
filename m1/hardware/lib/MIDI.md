# MIDI IN opto — Toshiba TLP2361

| | |
|--|--|
| MPN | **TLP2361(TPL,E** |
| LCSC | **C107626** |
| KiCad | `oc-midi:TLP2361` |
| Files | `lib/easyeda/oc_midi.{kicad_sym,pretty,3dshapes}/` |

SO6 / 5-pad (pin 2 NC). Supply **2.7–5.5 V**. Totem-pole **inverter** output — no pull-up. Replaces obsolete H11L1.

## Pins

| Pin | Name | Net |
|-----|------|-----|
| 1 | AN (anode) | MIDI Tip via 220 Ω |
| 3 | CAT (cathode) | MIDI Ring |
| 4 | GND | GND |
| 5 | VO | UART RX |
| 6 | VCC | 3V3 |

Mandatory **100 nF** VCC↔GND next to the package. **1N4148** across LED (cathode→anode). Full circuit: [`../support.md`](../support.md).
