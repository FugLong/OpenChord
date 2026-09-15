# OLED 0.91" (Ali 4-pin) — verified in-repo

~12×38 mm white SSD1306. Solder into PCB holes.

**Pinout (locked)** — view screen, pins on right, top→bottom:

1. GND · 2. VCC · 3. SCL · 4. SDA · I2C **0x3C**

| KiCad | Path |
|-------|------|
| Symbol | `oc-oled:SSD1306_0.91_4pin` ← `oled.kicad_sym` |
| Footprint | `oc-oled:SSD1306-0.91-OLED-4pin-128x32` |
| 3D | `3dshapes/OLED_0.91_128x32.step` |

Footprint from [gorbachev](https://github.com/gorbachev/KiCad-SSD1306-0.91-OLED-4pin-128x32.pretty) (MIT — `oled.pretty/LICENSE-gorbachev.txt`). Nudge STEP offset in KiCad if origin is off.
