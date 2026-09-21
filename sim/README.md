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
# Headless: write a PNG
SDL_VIDEODRIVER=dummy sim/build/radar_sim --out sim/out/radar.png --range 1 --planes 12

# Live window (WSLg on Windows 11), aircraft drift at 20x speed
sim/build/radar_sim --window
```

Options: `--range 0-3` (preset index), `--planes N`, `--seed N`, `--rotation 0|1`
(0 = portrait 320x480, 1 = landscape 480x320, the firmware default; the window is
sized to match; 180-degree flips are not simulated), `--out FILE`, `--window`.

Stubs live in `sim/stubs` (Arduino `Serial`/`millis`, `Preferences`, `driver/gpio.h`);
fake services (aircraft, weather, settings) are in `sim/sim_services.cpp`.
