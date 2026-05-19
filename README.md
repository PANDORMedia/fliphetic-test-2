# fliphetic-test-2

Second sample app for the [fliphetic](https://github.com/PANDORMedia/fliphetic)
virtual pinball cab. Built to test app switching: deliberately loud animated
visuals + classic DMD-style scrolling logo so a switch from any other app is
obvious at a glance.

## Screens

| Role        | Look                                                       |
|-------------|------------------------------------------------------------|
| `playfield` | Vertical placeholder image, slow hue cycling, gradient sweep, big "TEST #2" mark |
| `backglass` | Landscape placeholder, animated conic glow, color-stripe marquees top + bottom |
| `dmd`       | Orange dot-matrix scrolling logo + sub-marquee, pulsing emblem, scanlines      |

## Run locally

    docker compose -f deploy/docker-compose.yml up -d
    docker compose -f deploy/docker-compose.yml port playfield 80
