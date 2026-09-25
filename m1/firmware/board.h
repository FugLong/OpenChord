#pragma once

// Rev A netlist, taken from OpenChordM1.kicad_pcb footprint nets (not from memory).
// Pressed = low. Internal pull-up. Other switch pole is GND.
//
// Keyswitch grid, PCB +X to the right, the 3 buttons toward -Y:
//   SW11   SW9    SW10     Prev / Menu / Next
//   SW1    SW3    SW5    SW7
//   SW2    SW4    SW6    SW8
//
// Keys labels, near row then far row: Dim Min Maj Sus / 6 m7 M7 9
// Scale labels: I ii iii IV / V vi vii I+

namespace ocboard {

constexpr int kKeyswitchCount = 8;

// Index 0..7 is SW1..SW8 (schematic reference order).
constexpr int kKeyswitchGpio[kKeyswitchCount] = {
    24, // SW1
    18, // SW2
    25, // SW3
    17, // SW4
    2,  // SW5
    13, // SW6
    6,  // SW7
    12, // SW8
};

// Spatial index: row-major, near row (toward the 3 buttons) first, left to right.
// 0 SW1, 1 SW3, 2 SW5, 3 SW7, 4 SW2, 5 SW4, 6 SW6, 7 SW8
constexpr int kKeyswitchSpatialSw[kKeyswitchCount] = {1, 3, 5, 7, 2, 4, 6, 8};

constexpr int kPrevGpio = 3; // SW11, -X end of the edge row
constexpr int kMenuGpio = 4; // SW9, middle
constexpr int kNextGpio = 5; // SW10, +X end of the edge row

constexpr int kI2cSda = 10; // I2C1
constexpr int kI2cScl = 11;
constexpr int kIqsRdy = 8;
constexpr int kIqsNrst = 9;

// Hardware UART0, not a PIO or bit-banged port. support.md MIDI Type A.
// OUT J2: GPIO0 TX → 10 Ω → ring. Tip is 3V3 through 33 Ω. TX high is idle (no current).
// IN J3: TLP2361 VO → GPIO1 RX. LED on pulls VO low, which is a UART 0. Do not invert.
constexpr int kMidiTx = 0;
constexpr int kMidiRx = 1;

constexpr uint8_t kOledAddr = 0x3C;
constexpr uint8_t kQtAddr = 0x1C;
constexpr uint8_t kIqsAddr = 0x74;

} // namespace ocboard
