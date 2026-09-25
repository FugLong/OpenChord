# M1 interaction plan

Working behavior spec. Hardware intent lives in [goals.md](goals.md). Status lives in [build.md](build.md). **How the box plays is this file.** [chord-engine.md](chord-engine.md) and [plugin.md](plugin.md) are the old Pro/Smart lab notes. Do not put other products’ names in UI, firmware, or this doc.

Engine stays one portable core (`m1/engine`). Firmware and **OpenChord M Core** both host it. If a real M1 is connected, the plugin mirrors and edits; it does not voice the same notes.

---

## What is locked vs open

| Locked | Open, decided here |
|--------|--------------------|
| 8 Gateron keyswitches, direct GPIO | What a keyswitch means in each mode |
| Trackpad (XY) | What XY means per mode |
| Strip (0–255 slider) | When the strip is allowed to sound notes |
| 3 tactile buttons | Middle = menu. Left / right = move |
| 128×32 OLED | Two screens: perform, menu |
| USB MIDI device + TRS in/out | Who is allowed to start a note |
| No pots, no panic key | Notes cannot get stuck |

---

## Names

Screen words are short on purpose. The sentence next to each one is what it means. Use these in UI, firmware, and the engine. Do not revive Pro, Smart, Follow, Play, or Jazz.

The box has four kinds of control. Those words are reserved.

| Part | Count | Call it |
|------|------:|---------|
| Gateron Low Profile keyswitch | 8 | **keyswitch** |
| Edge tactile (EVQ) | 3 | **button** |
| IQS572 XY | 1 | **trackpad** |
| QT2120 slider | 1 | **strip** |

Do not call a keyswitch a pad. A pad is something on a controller that is not this box.

| Word | Where it shows up | What it means |
|------|-------------------|---------------|
| **Keys** | Mode name | An incoming MIDI note is the root. The keyswitches choose the chord type. One hand on a keyboard, the other on the box. |
| **Scale** | Mode name | The eight keyswitches are degrees of the current key. The box plays the chord. No keyboard required. |
| **Drums** | Mode name | The eight keyswitches are drum hits. The box plays the kit. See Drums. |
| **Thru** | Keys, no keyswitch held | The incoming note is echoed. It is not turned into a chord. |
| **Bass** | Keys trackpad X | Which chord tone sits on the bottom: root, 3rd, 5th, or 7th if the chord already has a 7th. |
| **Spread** | Keys trackpad Y | Tight stack versus open voicing. Never changes major into minor. Finger up returns to the home voicing. |
| **Color** | Scale trackpad | Nine positions. Center is that degree’s correct triad. Each of the other eight is its own chord, not a mix of the two axes. |
| **Harmony** | Menu, Scale only | How far those eight chords may leave the key. Rebuilt when you leave the menu, not while you play. |
| **In key** | Harmony 0 | Default. Every slot stays in the key and in the degree’s quality. ii cannot become major. |
| **Tensions** | Harmony 1 | Same chords as In key. Slots that were only inversions become 9 / 11 / 13 when that note is in the key and does not fight the 3rd. |
| **Borrowed** | Harmony 2 | The center and the four straight directions stay the Tensions chords. The four diagonals may borrow. |
| **Free** | Harmony 3 | The ring may change quality and leave the key. The center is still the correct triad. Opt-in. |
| **Strum** | Menu | How the strip starts notes. Not a mode. |
| **Optional** | Strum, default | The chord or note sounds on its own. The strip adds plucks if you touch it. Ignore the strip and nothing changes. |
| **Only** | Strum | Keys and Scale stay quiet until the strip. The strip is the only way a note starts. |
| **Key** | Menu | The tonal center, major for now. The next incoming note can set it, and that note is not performed. |
| **Prev / Menu / Next** | The 3 buttons | SW11 GPIO3 / SW9 GPIO4 / SW10 GPIO5. PCB +X is Next. |
| **I+** | Scale keyswitch 8 | Scale degree I, one octave up. |
| **Vary** | Menu, generated notes | How much velocity jitters around the base. 0 = fixed. Keys Optional leaves an incoming note’s velocity alone. |

Code enum: `oc::PlayMode::Keys` and `oc::PlayMode::Scale` (`m1/engine/chord_engine.h`). Saved plugin state still uses 0 and 1, same as the old Pro / Smart numbers.

---

## Modes

Prev / Next **outside the menu** cycle the mode. Changing mode releases every note we sounded, then switches.

| Mode | Hands | Who picks the root | What the 8 keyswitches are | What the trackpad is |
|------|--------|--------------------|----------------------------|----------------------|
| **Keys** | Piano (or DAW / TRS / USB) plus one hand on the box | Incoming MIDI note | Chord type + extras, combinable | **Bass** (X) and **Spread** (Y). Lift = home voicing |
| **Scale** | Both hands on the box | The keyswitch, from the current key | I–vii, 8th = **I+**. Hold = sound | **Color**. Center = the correct triad. See Harmony |
| **Drums** | Both hands on the box | The hit itself | One drum sound each, two banks | X = bank. Y = velocity at the moment the keyswitch closes |

No keyswitch held in Keys = **Thru**.

Scale layout, near row (toward the edge buttons) first:

| I | ii | iii | IV |
|---|----|-----|----|
| V | vi | vii | I↑ |

Keys layout, same near-row-first order:

| Dim | Min | Maj | Sus |
|-----|-----|-----|-----|
| 6 | m7 | M7 | 9 |

Maj means maj. Extras do nothing without a type. Type is captured at note-on and stays with that key until release.

---

## Notes cannot stick

There is no panic button. Every note-on we emit has one owner. The owner’s release is the note-off.

| Owner | Starts the note | Ends it |
|-------|-----------------|---------|
| Incoming MIDI note | Keys, trigger = Live | That note’s note-off |
| Keyswitch | Scale, held | Keyswitch up |
| Strip finger | Strum articulation | Finger up, or the armed chord is replaced |

Also flush our notes (not a user-facing control):

- Mode change
- Menu open
- USB suspend / disconnect

A firmware bug can still wedge a note. Hold middle **inside the menu** sends a flush. That is a recovery, not an instrument control. Do not label it on the case.

---

## Keys

Recipe = root (incoming note) + held type/extras + trackpad voicing.

**Trackpad.** Same in every key, including mid-song. X picks bass/inversion (root, 3rd, 5th, 7th if a 7th is already in the chord). Y opens the voicing or pulls it tight. Corners do both. Center and finger-up return to the home voicing of that recipe. Y never changes major into minor.

**Strum** is a menu setting, not a third mode. It applies in Keys and in Scale. There is no off. Optional is the default, so the strip can be ignored.

- **Optional.** Keys: note-on sounds the voiced chord, note-off releases it. Scale: the degree sounds while it is held. Touching the strip adds plucks of that chord. Lift the finger and only those strip notes stop. The chord that was already playing stays until its own release.
- **Only.** Nothing sounds until the strip. Keys: the note and keyswitches arm the chord, the screen shows the name, and a new note replaces that arm. Scale: the held degree is the recipe and does not sound on its own. Letting the degree go clears it. The strip is the same either way.

Strip position 0–255 is split into equal zones, one per note in the current voicing, lowest pitch at the low end. A strum is the order you cross those zones: low to high one way, high to low the other. Entering a zone plucks that note again, so you can strum back and forth without lifting. Staying inside a zone does not repeat it. Lift stops the notes.

---

## Scale

Hold a degree. The trackpad is eight chord buttons around a dead zone, the same gesture in every key. **I+** uses I’s eight chords, one octave higher. Keys mode does not use this map. There the pad only voices.

The newest keyswitch is the chord. An older one can stay held, and it does not keep sounding or keep the screen. Let the newer one go and the one still held comes back.

**Center**, including finger-up, is always that degree’s correct triad in root position. On every Harmony step, including Free. You can always come home.

**A direction is one chord.** Up-right is not “up’s chord plus right’s chord.” Past the dead zone, the nearest of the eight wedges wins. Nothing blends.

**Stable.** Performing does not rewrite the map. Voice leading may slide a note by octaves toward the last chord. It keeps every pitch class in the recipe, including which one is the bass. It does not turn an inversion back into root position, and it does not drop the note that makes that slot a different chord.

**Rebuilt only when a setting changes.** Key, Harmony, or (later) a scale other than major. Rebuild on menu exit, before the next note. The finger can stay where it is. That is the new chord, on purpose.

**Strip.** The newest held degree is the recipe, in both Strum settings. Optional plays that degree and lets the strip add plucks. Only keeps it quiet until you drag. Zones, order, and retrigger are the same as Keys.

Shown in C. Other keys are the same shapes, transposed. “In the key” means the note is in the major scale of the Key setting.

### How a slot is chosen

The four straight directions try for a real color, in this order. A candidate is skipped if its extra note is outside the key, if it would change the degree’s quality (ii must not become major), or if that chord is already sitting on another slot.

| Direction | First choice |
|-----------|----------------|
| N | Diatonic 7th. I and IV are major 7. ii, iii, and vi are minor 7. V is dominant 7. vii is half-diminished. |
| E | Sus4. |
| S | 6th, still the degree’s quality (C6, Dm6). |
| W | Sus2. |

Anything still empty, including the four diagonals, is filled in this order until all eight differ:

1. First inversion (3rd in the bass), then second inversion (5th), then the 7th in the bass, of a chord already chosen for this degree.
2. If those run out, an open voicing of the center triad: same notes, the middle voice up an octave.

Diagonals are inversions on purpose when the key has no eighth color. They add bass motion and do not add a wrong note. vii in C only has two in-key chords (Bdim and Bm7b5), so most of its ring is inversions plus one open voicing. iii has three (Em, Em7, Esus). That is the locked-down case, not a hole to fill with something ugly.

### In key

Default. No borrowed chords. No 9, 11, or 13 yet (a 6th or a sus2 that is already in the list above is the chord tone, not a tension).

| Degree | Center | N | E | S | W | NE | SE | SW | NW |
|--------|--------|---|---|---|---|----|----|----|----|
| I | C | CM7 | Csus | C6 | Csus2 | CM7/E | C/E | C/G | CM7/G |
| ii | Dm | Dm7 | Dsus | Dm6 | Dsus2 | Dm7/F | Dm/F | Dm/A | Dm7/A |
| iii | Em | Em7 | Esus | Em/B | Em/G | Em7/G | Esus/A | Em7/D | Em7/B |
| IV | F | FM7 | F/A | F6 | Fsus2 | FM7/A | F/C | Fsus2/C | FM7/C |
| V | G | G7 | Gsus | G6 | Gsus2 | G7/B | G/B | G/D | G7/D |
| vi | Am | Am7 | Asus | Am/E | Asus2 | Am7/C | Asus/D | Am7/E | Am7/G |
| vii | Bdim | Bm7b5 | Bdim/D | Bdim/F | Bm7b5/A | Bm7b5/D | Bm7b5/F | open Bdim | Bm7b5 open |

IV has no sus4, because that 4th is Bb. iii has no 6th or sus2, because those notes are C# and F#. vi has no 6th, because that note is F#. The table already shows what replaced them.

### Tensions

Every In key chord stays where it was. A slot that was only an inversion, or the open voicing, becomes a tension when all of these are true:

- The added note is in the key.
- 9 is allowed on I, ii, IV, V, and vi. Not on iii or vii.
- 11 is allowed on ii, iii, and vi, where it sits a whole step above the minor 3rd. Not on I or IV, where it fights the major 3rd. Not on V, where it muddies the leading tone. Not on vii, where it crowds the flat 5.
- 13 is allowed on I, ii, IV, and V. Not on iii, vi, or vii.

The new chord keeps the degree’s quality. Corners that are still inversions invert one of these tension chords, so the color is reachable with a different bass.

| Degree | What changes | The new slots |
|--------|----------------|---------------|
| I | Three corners | NE CM9, SE C6/9, SW CM9/E. NW stays CM7/G |
| ii | Three corners | NE Dm9, SE Dm11, SW Dm13. NW stays Dm7/A |
| iii | One new color | NE Em11, SE Em11/G. The other corners stay |
| IV | No sus4 to replace | NE FM9, SE F6/9, SW FM9/A. NW stays FM7/C |
| V | G7 on N stays | NE G9, SE G13, SW G9/B. NW stays G7/D |
| vi | Two new colors | NE Am9, SE Am11, SW Am9/C. NW stays Am7/G |
| vii | Nothing legal | Same eight as In key |

Straight up, down, left, and right do not get riskier at this step. The new notes live on the diagonals, except iii’s Em11, which takes the NE diagonal because iii had no spare cardinal.

### Borrowed

The center and N, E, S, and W stay exactly the Tensions chords. A thumb that misses toward a side is still safe. Only the diagonals borrow.

| Corner | Chord |
|--------|--------|
| NE | Dominant 7 on this degree. On V, which is already a dominant 7, this is 7b9. |
| SE | Quality flip. Major degrees become the minor triad. Minor degrees become the major triad. vii becomes the minor triad. |
| SW | First inversion of that NE chord. |
| NW | First inversion of that SE chord. |

In C that is:

| Degree | NE | SE | SW | NW |
|--------|----|----|----|----|
| I | C7 | Cm | C7/E | Cm/Eb |
| ii | D7 | D | D7/F# | D/F# |
| iii | E7 | E | E7/G# | E/G# |
| IV | F7 | Fm | F7/A | Fm/Ab |
| V | G7b9 | Gm | G7b9/B | Gm/Bb |
| vi | A7 | A | A7/C# | A/C# |
| vii | B7 | Bm | B7/D# | Bm/D |

ii as a major chord exists here, and only on a diagonal. The straight directions still cannot make ii major.

### Free

Opt-in. The center stays the correct triad. Every ring slot may leave the key. The roles stay the same on every degree, so the hand still knows where the wild one is.

| Direction | Chord |
|-----------|--------|
| N | Dominant 7#9 on this degree. |
| E | The quality flip from Borrowed (I becomes minor, ii becomes major, vii becomes minor). |
| S | Diminished 7 on this degree. |
| W | Tritone substitute: a dominant 7 whose root is a tritone away. |
| NE | Major degrees: major 7#11. Minor degrees, and vii: half-diminished on this degree. |
| SE | Major triad on the note a minor 3rd below this degree (bVI of the degree). |
| SW | First inversion of N. |
| NW | First inversion of W. |

In C, W is F#7 on I, Ab7 on ii, Bb7 on iii, B7 on IV, Db7 on V, Eb7 on vi, and F7 on vii. SE is Ab on I, Bb on ii, C on iii, Db on IV, Eb on V, F on vi, and G on vii.

“Wrong” is a menu setting. A missed wedge at In key is still a chord from the key. The diagonals are where each higher setting starts to allow more.

---

## Drums

The eight keyswitches send drum notes. Nothing is held as a chord. Press sounds the hit, release sends note-off.

**Default kit**, channel 10, General MIDI note numbers. Near row first, left to right. This matches a lot of basic kits without any binding.

| | left | | | right |
|--|------|--|--|-------|
| Near the buttons | Kick 36 | Snare 38 | Closed hat 42 | Open hat 46 |
| Far row | Low tom 41 | Mid tom 47 | Clap 39 | Crash 49 |

**Banks.** Trackpad X at the moment the keyswitch closes picks the bank. Left half, including center, is the kit above. Right half is a second kit: rim 37, pedal hat 44, ride 51, tambourine 54, cowbell 56, high tom 50, crash 2 57, ride bell 53. Same grid order. X is not used for Color or Bass in this mode, so it does not fight velocity.

The plugin labels follow X live, so the names match the next hit. A note already sounding stays the bank captured at press. The screen names that bank the same way: `Left` or `Right` when nothing is held, and the hit name while one is down.

Each keyswitch can be rebound later (plugin, then the menu). The default has to be useful with zero setup.

---

## Velocity

Keyswitches are digital. They do not measure how hard they were hit. Incoming MIDI in **Keys** with Strum **Optional** already has a velocity, and we keep it. **Vary** does not touch those notes.

Anything the box generates needs a velocity we invent:

| Source | Base velocity | Then |
|--------|----------------|------|
| Keys Optional | The incoming note | Unchanged |
| Keys Only | Fixed base (100 until a setting exists) | **Vary** |
| Scale | Fixed base | **Vary**. The trackpad is already Color, so it does not also set velocity |
| Drums | Trackpad Y when the keyswitch closes. Top = 127, bottom = 1, center = 100 | **Vary** |

**Vary** is one menu setting for every generated note. 0 is fixed. Above that, each note-on jitters a little around the base so a machine-gun chord is not eight identical velocities. It is not a performance control.

---

## Edge buttons and menu

Three mechanical buttons. Middle is the menu button. Left and right move.

**Performing**

- Prev / Next: previous / next mode. Wrap. Flush notes, then switch.
- Middle: open the menu. Flush notes. Keyswitches do not sound while the menu is open.

**Menu**

One setting on screen. Left / right move between settings. Trackpad horizontal drag changes the value. Middle exits and applies (flavor tables rebuild here).

v1 settings:

| Setting | Control | Notes |
|---------|---------|--------|
| Key | Trackpad drag, or the next incoming note | That note sets the key and is not performed. Major only until other scales exist |
| Octave | −3 to +3, default 0 | Scale moves the whole key. Keys moves a generated chord off the played note. A note with no keyswitch stays where it was played. Shortcuts for this can come later |
| Vary | 0 = off, then a small range | Generated notes only. Keys Optional keeps the incoming velocity |
| Harmony | In key / Tensions / Borrowed / Free | Scale only |
| Strum | Optional / Only | Keys and Scale. Optional is the default. Only waits for the strip |

A circular wheel gesture on the trackpad is a feel experiment after the real trackpad is in hand. v1 does not depend on it. Horizontal drag must work.

---

## Screen

128×32. The plugin draws this same rectangle. Firmware prints the same two strings. Neither side invents a third status line.

Two lines. The top line answers “what will sound?” The bottom line is where you are. Big type. Almost no chrome.

**Perform, Keys and Scale**

| Top | When |
|-----|------|
| Chord name | A chord is armed or sounding. `CM7`, `Dm7/F`, `Csus2` |
| The played note | Keys, no keyswitch, one note. `C4` |
| The chord you played | Keys, no keyswitch, two or more notes. The symbol for those pitches, slash bass included. `Am7`, `C/E`, `G7b9` |
| `---` | Nothing is armed and nothing is sounding |

The top line is centered. The bottom line is three slots with a divider between them.

| Left | Middle | Right |
|------|--------|-------|
| Key, and octave when it is not 0. `C`, `C+1`, `F-2` | `Keys` or `Scale` | `arm` when Strip is waiting. Otherwise empty |

The chord name already shows Color and Harmony. Vary, the Strum name, and the degree number stay off this screen. The keyswitch under the finger is the degree.

**Perform, Drums**

The buttons cannot relabel themselves. The screen says which bank the stick is in, before the hit.

| Top | When |
|-----|------|
| `Left` or `Right` | Nothing held |
| The hit name | A keyswitch is down. Most recent one if several are. `Snare`, `Ride bell` |

The middle slot is `Drums`. While a hit is held, the right slot keeps the bank (`Left` or `Right`) so the top line can be the hit name. Key and octave are not used.

**Menu**

Replaces both lines. The top line is the setting, centered: Key, Octave, Vary, Harmony, Strum. The bottom slots are `<`, the value, and `>`.

`Octave` over `<` `+1` `>` · `Strum` over `<` `Optional` `>` or `<` `Only` `>`

**Strip marks**

While the strip finger is down and a chord recipe exists, one row of pixels shows the zones, with the current zone lit. No marks in the menu, and none in Drums.

**Not on the glass**

Bind, Latch, and Reset are plugin testing tools. They are not on the box and not on this screen. Keyswitch names (Dim, I, Kick) are the caps in the plugin picture. On the board they are learned by touch, except the drum bank, which the screen carries.

---

## Plugin

M Core is the same instrument with a picture of the box on it, including a live copy of the two OLED lines.

| Box on USB | Who runs the engine | Plugin |
|------------|---------------------|--------|
| No | Plugin | Full instrument. Click toggles a keyswitch or button (no mouse-hold). Drag leaves the trackpad and strip where you put them. A plugin-only **Bind** button assigns MIDI; it is not on the box |
| Yes, named `OpenChord M1` | Box | Mirror the screen. Edit the same settings and push them to the device. Do not voice |

One display model (the two lines, plus menu index and values) is shared so the plugin cannot invent a different UI than the firmware.

The picture is a view of that one surface, not a second instrument. When the box is connected, a finger on a keyswitch and a click on its picture are the same event: the plugin lights up from the box, and a click in the plugin is what the box would have sent. Menu values, mode, key, and trackpad position stay one copy. Do not keep a private latch or a drum-bank label that the engine cannot see. Until the box is connected, the plugin runs the engine itself.

---

## Later, without a redesign

These are more modes or settings. They call the same recipe, the same owners, and the same strip articulator.

- **Notes.** One row is scale tones (single notes). Holding one turns the other row into the Keys type/extras for that root.
- **Arp.** Clock (internal or MIDI) walks the current recipe. A keyswitch or the strip gates it.
- **Other scales** (Dorian, etc.). The Key setting grows a scale. Harmony `In key` stays “correct for that scale.”

Do not build these until Keys, Scale at Harmony `In key`, and both Strum settings are boringly good.

---

## Build order

1. Accept this file. Then point goals + chord-engine at it and stop describing panic / Key / Shift as the three buttons.
2. Engine: recipe in, voiced notes out. Color table builder (key × degree × harmony × direction). Strip articulator (voicing + position → note owners). No board required.
3. Plugin hosts that, ugly controls first, OLED lines included. Photo skin when a photo exists.
4. Firmware: USB MIDI, eight keyswitches, three buttons, OLED lines. Touch and TRS after the board plays a chord.
