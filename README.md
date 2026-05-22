# Fliphetic Debug Screen

A calibration & diagnostics app for the [fliphetic](https://github.com/PANDORMedia/fliphetic)
virtual pinball cab. Load it to verify each screen is positioned and sized
correctly, read off cab info at a glance, and test the cabinet's buttons and
indicator LEDs.

Each screen shows:

- **Positioning grid** — 50 px minor / 200 px major lines with coordinate
  ruler labels along the top and left edges.
- **Registration marks** — L-brackets in all four corners + corner pixel
  coordinates. If a bracket is clipped, that screen's kiosk window geometry is
  wrong.
- **Center crosshair** with the centre pixel coordinate.
- **Diagnostics panel** — screen role, live viewport vs. physical display
  size, aspect ratio, device pixel ratio, the cab's Tailscale IP + dashboard
  URL, the kiosk source URL, network state, a live clock and page uptime.

The **playfield** screen additionally shows a **button / LED test panel** down
the side (see below).

Matrix-themed to match the cab's idle/waiting app.

## Button / LED tester

An ESP32 reads the cabinet buttons and drives the indicator LEDs. The playfield
screen shows, per button: a dot that lights when the button is pressed, the
button's GPIO pin, and an LED indicator that lights when the firmware's LED
cycle reaches that button. The firmware lights each LED in turn so you can
confirm every LED physically works.

```
ESP32  --USB serial JSON-->  bridge service  --GET /state-->  playfield screen
```

- **`site/config/buttons.json`** — the single source of truth: each button's
  GPIO, and `led` (`false`, or `{ gpio }` once addressable LEDs are wired).
  Shared by firmware + screen.
- **`firmware/`** — the classic-ESP32 tester firmware (PlatformIO). Built and
  committed automatically by `.github/workflows/firmware.yml`; the cabinet
  flashes the committed `firmware/build/firmware.bin` on load.
- **`bridge/`** — a dependency-free Node service that reads the ESP serial
  device and serves the latest state at `/state`.

If you rewire, edit `buttons.json` **and** the table in
`firmware/src/main.cpp`. See `firmware/README.md`.

## Topology

Two services in `deploy/docker-compose.yml`:

- `debug` — nginx, serves all three screens (`/?screen=<role>`) and proxies
  `/state` to the bridge.
- `bridge` — relays the ESP32 serial stream to HTTP.

The `[screens]` block in `fliphetic.toml` points each screen at
`/?screen=<role>`; the page reads that query param to label and tint itself.

## Run locally

    docker compose -f deploy/docker-compose.yml up -d
    docker compose -f deploy/docker-compose.yml port debug 80
    # open  http://localhost:<printed-port>/?screen=playfield

The `bridge` service expects the ESP32 at the serial device mapped in
`docker-compose.yml`; without it the panel just shows `WAITING FOR ESP`.
