# ESP32 button + LED tester firmware

Firmware for the cabinet's classic **ESP32**. It exists so you can verify the
physical wiring: that every arcade button registers, and that every indicator
LED lights.

## What it does

- Reads each button (wired between a GPIO and GND, internal pull-up).
- Drives the indicator LEDs in a repeating **cycle** — one lit at a time,
  600 ms each — so you can watch each LED come on in turn.
- Emits a JSON state line over USB serial (115200 baud) the instant a
  debounced button or the LED cycle changes, plus a heartbeat every 250 ms:

  ```json
  {"buttons":{"black-left":true,"white-left":false,"...":false},"led":null,"up":48213}
  ```

  `buttons` is each button id → pressed; `led` is the id of the button whose
  LED is currently lit in the cycle (or `null` if no button has an LED).

The `bridge` service reads this serial stream and pushes each line to the
debug screen over SSE, so a press shows up with no polling delay.

## The shared contract: `buttons.json`

[`site/config/buttons.json`](../site/config/buttons.json) is the single source
of truth. Each button has:

| Field | Used by | Meaning |
|-------|---------|---------|
| `gpio` | firmware | ESP32 input pin the button is wired to. |
| `active_low` | firmware | `true` = wired to GND, read with an internal pull-up. |
| `led` | firmware + screen | `false` if the button has no addressable LED, or `{ "gpio": <pin> }` once one is wired. |
| `id` / `label` | screen | Identifier and human label. |

If you rewire, edit `buttons.json` **and** the `buttons[]` table in
`src/main.cpp` to match. LEDs are driven active-high (pin HIGH = lit); flip the
writes in `applyLeds()` if the cabinet wires them the other way.

## Wiring

- **Buttons:** each button between its `gpio` pin and `GND`. No external
  resistor — the firmware enables internal pull-ups.
- **LEDs:** each LED's `led.gpio` pin → current-limiting resistor → LED → GND
  (or via a driver/transistor for high-current LEDs).

## Building

The cabinet flashes a prebuilt binary; it does not build firmware. The binary
at `firmware/build/firmware.bin` is built and committed automatically by
[`.github/workflows/firmware.yml`](../.github/workflows/firmware.yml) (a
PlatformIO build) whenever anything under `firmware/` changes.

To build locally:

```sh
pio run --project-dir firmware
```

The manifest's `[esp32.esp32]` block points the cabinet at the committed
`firmware/build/firmware.bin` (a merged image, flashed at `0x0`).
