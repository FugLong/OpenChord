# M1 build plan

How we get from the lab plugin to firmware that matches [interaction.md](interaction.md), and a plugin that plays the same instrument with a mouse or whatever MIDI gear someone already owns.

Behavior names and rules live in interaction.md. This file is the order of work, and the status.

## Where we are

The plugin plays steps 1–7. Firmware does not talk to the board yet. **Next is step 8, the board.**

| Step | Status |
|------|--------|
| 1 Surface | Done |
| 2 Modes | Done |
| 3 Menu | Done. Rows, in order: Key, Octave, Vary, Harmony, Strum |
| 4 Vary | Done |
| 5 Drums | Done. Two banks. Labels follow trackpad X. Velocity follows Y, then Vary |
| 6 Color and the strip | Done for Harmony **In key**, and for Strum **Optional** / **Only** in both Keys and Scale |
| 7 Screen | Done in the plugin. Two lines, three bottom slots. Keys with no keyswitch names the chord you played. Scale shows and sounds the newest keyswitch. Firmware prints the same strings in step 8 |
| 8 Firmware | After the screen. USB device, keys, buttons, OLED, then trackpad, strip, and TRS |
| 9 Box connected | After a chord works on the desk. Plugin mirrors the box and stops voicing |

Also in the plugin, beside those steps:

- Octave −3 to +3. Scale moves the key. Keys moves a generated chord. A note with no keyswitch stays where it was played.
- Keys, no keyswitch: one note shows as `C4`. Two or more notes show the chord they form, slash bass included.
- Scale: the newest keyswitch is the chord on the screen and in the audio. An older one can stay held and comes back when you let go.
- Latch sticks keyswitches only. Prev, Menu, and Next stay momentary.

Not started on purpose: Harmony past In key (the tables are written, the sound is still In key), other scales, arp, a photo skin, drum rebinding, shortcuts for menu rows. Next work is step 8, not those.

---

## One surface

The engine grows a single input surface. Both hosts call it. Neither host invents a second chord model.

| Control | Engine call | Firmware source | Plugin source |
|---------|-------------|-----------------|---------------|
| Keyswitch 1–8 | down / up | GPIO in `firmware/board.h` | Mouse click toggles, or any learned note or CC |
| Prev, Menu, Next | down / up | SW11 GPIO3, SW9 GPIO4, SW10 GPIO5 | Mouse click toggles, or any learned note or CC |
| Trackpad X, Y, finger | −1..+1, touching or not | IQS572 | Mouse drag, value stays, or two learned CCs (joystick axes count) |
| Strip position, finger | 0–255, touching or not | QT2120 | Mouse drag, value stays, or one learned CC (mod wheel, ribbon, fader) |

What a keyswitch *means* depends on the mode (Keys type, Scale degree, Drums hit). The binding does not. Learn once, switch mode, same physical control.

GPIO, I2C, and TinyUSB stay in `m1/firmware`. JUCE stays in `m1/plugin`. `m1/engine` stays free of both.

---

## Plugin, specifically

Someone with other MIDI gear should get the same gestures without our box.

**Play and bind are different.** A plugin-only **Bind** button enters bind mode. It does not exist on the box. Outside bind mode, the picture of the device is an instrument.

- A click is momentary: mouse down is held, mouse up is released. A plugin-only **Latch** control, or holding Option or Control, sticks keyswitches for testing without hardware. Prev, Menu, and Next never stick.
- Drag the trackpad, then let go. The position stays, and the finger stays down, so Keys voicing does not snap home. The strip plays while the mouse is down and lifts on release, which stops its notes.
- **Bind** (plugin only): click it, then click the control to assign (a keyswitch, a button, trackpad X, trackpad Y, or the strip). The next MIDI message becomes that binding. Clicks in this mode do not play. Click Bind again to cancel. A successful assign leaves bind mode.
- Every keyswitch and every button learns **one** message: a note, or a CC. Channel can be any, or one channel. Value ≥ 64 means held. A piano key, a pad, or a button on their controller are all legal.
- The trackpad learns **two** CCs, X and Y, as two separate assigns. A joystick that sends two axes is the normal case.
- The strip learns **one** CC. Mod wheel is a good default to offer, not a requirement.
- One message binds to one control. Binding it again moves it.
- Right-click, while Bind is on, clears that control. A reset restores a named Launchkey preset (their pads → keyswitches 1–8, two knobs → trackpad). That preset is a convenience, not the product model.
- The on-screen labels follow the mode: Dim…9, or I…I+, or the drum names. In Drums the names follow trackpad X (left kit, or the right kit when X is positive). The learned MIDI stays put underneath. Labels are drawn from surface state, so a later USB link can light the same controls.

When a real M1 is on USB, the plugin stops calling the engine and mirrors the device. That is later in this plan. Until that detection exists, the plugin is the instrument.

---

## Order

Each step is done when the plugin can play it with the mouse and with learned MIDI, before any of it depends on the new board.

### 1. Surface — done

Replace the Launchkey-shaped control list (`Dim`, `Panic`, `Shift`, stick) with the table above. Keep Keys Live behavior working through the new calls: keyswitches still select Dim / Min / Maj / Sus / 6 / m7 / M7 / 9, trackpad X/Y still voice, no keyswitch held still means Thru.

Done when the existing Logic path still works: learned note + C on the keyboard = C major, no keyswitch = the raw note, and a mouse click latches Maj without holding the button. Bind is the only way to learn.

### 2. Modes and the three buttons — done

Prev / Next cycle **Keys → Scale → Drums → Keys**. Menu opens and closes. Changing mode or opening the menu releases every note we sounded.

Scale keyswitches are I–vii and I+, hold to sound, as the lab plugin already does. Drums can be a stub that only changes the labels until step 5.

Done when the mouse can change mode and the eight labels change with it, and a stuck note does not survive the change.

### 3. Menu settings the engine actually reads — done

On-screen menu, same settings as the box: **Key**, **Octave**, **Vary**, **Harmony**, **Strum** (Optional / Only). Trackpad horizontal drag changes the value while the menu is open. Prev / Next move between settings. Menu closes and applies.

Key can also be set by the next incoming note while that row is open, and that note is not performed.

Harmony and Strum may be stored before the music uses them. Vary can be a no-op until step 4. The point of this step is that firmware and plugin will share this state later.

### 4. Generated velocity — done

Keys Optional keeps the incoming velocity. Vary does not touch it.

Scale and Strum use base velocity 100, then Vary. Drums, once it exists, takes base velocity from trackpad Y at the keyswitch press (top 127, bottom 1, center 100), then Vary.

### 5. Drums — done

Channel 10. Default notes from interaction.md. Trackpad X at press picks the bank (left default kit, right the second kit). Press = note-on, release = note-off. Mouse and learned notes both work.

Per-keyswitch rebinding waits. The default kit has to be worth playing first.

### 6. Color, then the strip — done

Scale trackpad becomes Color, using the eight-chord tables in interaction.md. Each direction is one chord. The center is always that degree’s triad. This step ships Harmony `In key` only. Tensions, Borrowed, and Free are already specified there and wait until In key feels right. The table rebuilds when the menu closes, not while a keyswitch is held. The axis mix that is in the plugin now (7th on Y, inversion on X) is not the map.

Then Strum. Optional is the default in Keys and Scale: the chord sounds, and the strip adds plucks. Only keeps Keys and Scale quiet until the strip. A dwell is one chord tone, a swipe follows the finger, and entering a zone plucks again. Finger up is note-off of the strip notes only.

### 7. Screen model — done in the plugin

The two lines in [interaction.md](interaction.md) (Screen). The engine builds the strings. The top line is centered. The bottom line is key, mode, and status, with a divider between them. The plugin draws that in a 128×32 box. Firmware will print those same strings. No picture of the enclosure until a photo exists.

### 8. Firmware on the rev A board

Pins are `m1/firmware/board.h`, checked against the PCB nets.

1. USB MIDI device named `OpenChord M1`. It enumerates and can send a note.
2. Eight keyswitches and three buttons into the surface. OLED shows the two lines over I2C1 (`0x3C`).
3. IQS572 trackpad (`0x74`, RDY GPIO8, NRST GPIO9) and QT2120 strip (`0x1C`) into trackpad and strip.
4. UART0 MIDI at 31250. GPIO0 TX out, GPIO1 RX in. Same surface as USB. Notes we emit go out both ports.

Play a chord on the desk before worrying about the plugin connection.

### 9. Box connected

Plugin sees `OpenChord M1` on USB. It stops voicing. It still shows the two lines and the same controls. The box is the only thing calling the engine. A press on the box lights the matching control. A click or menu edit in the plugin is the same surface event the box applies. One copy of mode, key, Vary, Harmony, Strum, and trackpad position. Unplug and the plugin voices again on its own.

---

## Not in this plan

Notes-row mode, arp, scales other than major, Harmony past `In key`, a photo skin, drum rebinding, the circular trackpad wheel. They wait until Keys, Scale at `In key`, Drums, and Strum are boring on the plugin and then on the board. Scale uses the same strip as Keys: the newest held degree is the recipe.
