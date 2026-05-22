/*
 * Fliphetic Debug Screen — ESP32 button + LED test firmware.
 *
 * Classic ESP32. Reads each cabinet button (wired to GND, read with an
 * internal pull-up) and drives the indicator LEDs in a repeating cycle so
 * every LED can be eyeballed in turn. Emits the live state as a JSON line
 * over USB serial at 115200 baud; the `bridge` service reads that and
 * pushes it to the debug screen.
 *
 * Reporting is event-driven: a line is sent the instant a debounced button
 * (or the LED cycle) changes, plus a slow heartbeat so liveness is visible.
 * This keeps press-to-screen latency low.
 *
 * Keep this table in sync with site/config/buttons.json:
 *   buttons.json "gpio"     -> btnPin
 *   buttons.json "led".gpio -> ledPin   (NO_LED when "led" is false)
 *
 * GPIO 34-39 are input-only with NO internal pull-up: a button on those pins
 * needs an external pull-up resistor. The firmware detects them and uses
 * plain INPUT. LEDs are driven active-high (pin HIGH = lit) — flip the writes
 * in applyLeds() if the cabinet wires them the other way.
 */

#include <Arduino.h>

static const uint32_t BAUD         = 115200;
static const uint32_t DEBOUNCE_MS  = 8;     // matches buttons.json debounce_ms
static const uint32_t CYCLE_MS     = 600;   // matches buttons.json led_cycle_ms
static const uint32_t HEARTBEAT_MS = 250;   // max gap between reports when idle

static const int NO_LED = -1;

struct Button {
  const char *id;
  uint8_t     btnPin;
  int         ledPin;     // NO_LED when this button has no LED
  bool        stable;     // debounced state, true = pressed
  bool        lastRead;
  uint32_t    changedAt;
};

// --- edit to match site/config/buttons.json --------------------------------
Button buttons[] = {
  { "black-left",        16, NO_LED, false, false, 0 },
  { "white-left",         4, NO_LED, false, false, 0 },
  { "front-left-green",  17, NO_LED, false, false, 0 },
  { "front-left-yellow", 18, NO_LED, false, false, 0 },
  { "front-left-red",    19, NO_LED, false, false, 0 },
  { "black-right",       13, NO_LED, false, false, 0 },
  { "white-right",       25, NO_LED, false, false, 0 },
  { "front-white",       33, NO_LED, false, false, 0 },
  { "plunger",           32, NO_LED, false, false, 0 },
};
const int N = sizeof(buttons) / sizeof(buttons[0]);

uint32_t lastCycle  = 0;
uint32_t lastReport = 0;
int      litIndex   = -1;   // index of the button whose LED is currently on

// advance the cycle to the next button that actually has an LED
void advanceCycle() {
  for (int step = 1; step <= N; step++) {
    int idx = (litIndex + step) % N;
    if (buttons[idx].ledPin != NO_LED) { litIndex = idx; return; }
  }
  litIndex = -1;   // no button has an LED
}

void applyLeds() {
  for (int i = 0; i < N; i++) {
    if (buttons[i].ledPin != NO_LED) {
      digitalWrite(buttons[i].ledPin, i == litIndex ? HIGH : LOW);
    }
  }
}

// one JSON state line
void report() {
  Serial.print("{\"buttons\":{");
  for (int i = 0; i < N; i++) {
    Serial.print('"');
    Serial.print(buttons[i].id);
    Serial.print("\":");
    Serial.print(buttons[i].stable ? "true" : "false");
    if (i < N - 1) Serial.print(',');
  }
  Serial.print("},\"led\":");
  if (litIndex >= 0) {
    Serial.print('"');
    Serial.print(buttons[litIndex].id);
    Serial.print('"');
  } else {
    Serial.print("null");
  }
  Serial.print(",\"up\":");
  Serial.print(millis());
  Serial.println("}");
}

void setup() {
  Serial.begin(BAUD);
  for (int i = 0; i < N; i++) {
    // GPIO 34-39 are input-only and have no pull resistors
    bool inputOnly = buttons[i].btnPin >= 34;
    pinMode(buttons[i].btnPin, inputOnly ? INPUT : INPUT_PULLUP);
    if (buttons[i].ledPin != NO_LED) {
      pinMode(buttons[i].ledPin, OUTPUT);
      digitalWrite(buttons[i].ledPin, LOW);
    }
  }
  advanceCycle();
  applyLeds();
}

void loop() {
  uint32_t now = millis();
  bool dirty = false;

  // debounced button read (active-low: pressed reads LOW)
  for (int i = 0; i < N; i++) {
    bool pressed = digitalRead(buttons[i].btnPin) == LOW;
    if (pressed != buttons[i].lastRead) {
      buttons[i].lastRead = pressed;
      buttons[i].changedAt = now;
    }
    if (now - buttons[i].changedAt >= DEBOUNCE_MS && buttons[i].stable != pressed) {
      buttons[i].stable = pressed;
      dirty = true;            // a debounced button state changed
    }
  }

  // LED cycle — one lit at a time, advancing every CYCLE_MS
  if (now - lastCycle >= CYCLE_MS) {
    lastCycle = now;
    int prev = litIndex;
    advanceCycle();
    applyLeds();
    if (litIndex != prev) dirty = true;
  }

  // report immediately on any change, otherwise on a slow heartbeat
  if (dirty || now - lastReport >= HEARTBEAT_MS) {
    lastReport = now;
    report();
  }
}
