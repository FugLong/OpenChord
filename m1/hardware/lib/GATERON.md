# Gateron LP (KS-27 / KS-33) — vendored KiCad bits

## What we ship

| Item | Path | Source |
|------|------|--------|
| Symbol | `gateron.kicad_sym` → **`SW_Gateron_LP`** | OpenChord (2-pin) |
| Hotswap FP (default) | `gateron.pretty/Gateron-KS33-Hotswap-1U.kicad_mod` | [ai03 MX_V2](https://github.com/ai03-2725/MX_V2) (MIT) |
| Solderable FP (backup) | `gateron.pretty/Gateron-KS33-Solderable-1U.kicad_mod` | same |
| 3D socket | `gateron.3dshapes/Gateron-KS33-Socket.step` | ai03 MX_V2 |
| 3D switch | `gateron.3dshapes/Gateron-KS33-Switch.step` | [Gateron official](https://www.gateron.com/pages/3d) KS-33 LP 2.0 |

License: `gateron.pretty/LICENSE-ai03-MX_V2.txt` (footprints + socket). Switch STEP is Gateron’s CAD download for design use.

Project nicknames: **`oc-gateron`** (footprints + symbols).

Hotswap FP attaches **both** models: socket on the footprint component side, switch opposite via **rotation** (not negative scale — that inverts normals). Z offset assumes **1.6 mm** board. Tweak offset/rotate in the footprint 3D properties if stem/pins need a nudge.

## Critical: flip for bottom socket

ai03 hotswap footprints put socket copper on **F.Cu** (top) by design. For our stack-up (socket on **PCB bottom**):

1. Place `oc-gateron:Gateron-KS33-Hotswap-1U`
2. **Flip** the footprint (F in PCB editor) so pads land on **B.Cu**
3. Confirm NPTH stem/holes still align with the plate

Do **not** fab a coupon without checking pad side. Coupon 1–2 keys first.

## Schematic

- Place **`oc-gateron:SW_Gateron_LP`** × 8
- Pin 1 → GPIO (internal pull-up), pin 2 → GND (or reverse consistently)
- Footprint field defaults to Hotswap-1U

## Not MX / not Choc

Wrong socket family = scrap PCB. Only Gateron LP hotswap (KS-27/KS-33 compatible).
