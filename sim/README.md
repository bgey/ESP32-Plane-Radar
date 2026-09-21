# Desktop radar simulator

Renders the real UI code (`src/ui`, `src/hardware`) at 480x320 on a PC through
LovyanGFX's SDL panel, with fake aircraft instead of Wi-Fi/ADS-B. Useful for
iterating on layout without flashing hardware.

## Setup (Linux or WSL2 Ubuntu)

```bash
sudo apt install -y g++ make libsdl2-dev
pio run -e s3mini          # once, so LovyanGFX is in .pio/libdeps
make -C sim -j8
```

Native Windows compilers are not supported here (unsigned GCC is blocked by
Smart App Control); use WSL2.

## Run

```bash
# Headless (default): write a PNG and exit
sim/build/radar_sim --out sim/out/radar.png --planes 12

# Headless with scripted taps on the range control ([-] at ~x345, [+] at ~x455, y~297)
sim/build/radar_sim --out sim/out/tap.png --tap 345,297 --tap 455,297

# More tap targets (window pixels): SETUP button (400,257), alert banner (400,170).
# Inside settings: ALERTS tab (170,22), DONE (440,22), stepper [-] x~310 / [+] x~446
# on rows at y = 66 + 38*row. Example: open settings, go to ALERTS, raise the altitude
sim/build/radar_sim --out sim/out/settings.png --tap 400,257 --tap 170,22 --tap 446,104

# LOCATION tab at (276,22): SAVE the current position (439,64), name it on the keyboard
# (name field 300,76), then SAVE & USE (240,258). Favourites live in the simulated
# NVS for one run only.
sim/build/radar_sim --out sim/out/location.png --tap 400,257 --tap 276,22 --tap 439,64

# Live window (WSLg on Windows 11): aircraft drift at 20x speed, and mouse
# clicks act as touch, so the range control works
sim/build/radar_sim --window
```

Options: `--range N` (number of range-button presses from the default 10 km preset),
`--planes N`, `--seed N`, `--weather TEXT`, `--time TEXT` (firmware formats, e.g.
`'PARTLY CLOUDY 16C RH62%'`, `'8:05A 1 JAN'`), `--textscale 80-130`,
`--tap X,Y` (repeatable; window pixel coordinates), `--inbound` (adds a low aircraft
flying straight at the radar position to trigger the low-flyer alert) with
`--inbound-alt FT` (default 2600), `--lat DEG` / `--lon DEG` (radar centre; distances
are scaled for that latitude, as on the device), `--rotation 0|1`
(0 = portrait 320x480, 1 = landscape 480x320, the firmware default; the window is
sized to match; 180-degree flips are not simulated), `--out FILE`, `--window`.

The sim exercises the app's touch logic (hit-testing, redraws) but not the
XPT2046 itself: calibration, axis mapping and noise only show up on the board.

Stubs live in `sim/stubs` (Arduino `Serial`/`millis`, `Preferences`, `driver/gpio.h`);
fake services (aircraft, weather, settings) are in `sim/sim_services.cpp`.
