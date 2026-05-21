# Fliphetic Debug Screen

A calibration & diagnostics app for the [fliphetic](https://github.com/PANDORMedia/fliphetic)
virtual pinball cab. Load it to verify each screen is positioned and sized
correctly and to read off cab info at a glance.

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

Matrix-themed to match the cab's idle/waiting app.

## Topology

One nginx service (`debug`) backs all three screens. The `[screens]` block in
`fliphetic.toml` points each screen at `/?screen=<role>`; the page reads that
query param to label and tint itself.

## Run locally

    docker compose -f deploy/docker-compose.yml up -d
    docker compose -f deploy/docker-compose.yml port debug 80
    # open  http://localhost:<printed-port>/?screen=playfield
