# Changelog

All notable changes to this fork of
[ironicbadger/ESP32-Plane-Radar](https://github.com/ironicbadger/ESP32-Plane-Radar) are
documented here, in the style of [Keep a Changelog](https://keepachangelog.com/).
Changes made upstream before this fork (up to "Improve radar text settings flow") are in
the upstream project's history.

To publish a release, rename `## [Unreleased]` to the version and date, for example
`## [1.0.0] - 2026-10-15`, add a fresh empty `## [Unreleased]` above it, commit, then tag
(see the README, "CI and releases"). The release workflow puts the matching section on
the GitHub Release page.

## [Unreleased]

First release of the ESP32-S3 version. The original ESP32-C3 + round-display build is
unchanged apart from the fixes listed below.

### Added

- **ESP32-S3 target** (`pio run -e s3mini`): Wemos LOLIN S3 Mini with a 4" 480x320
  ST7796S SPI display, XPT2046 resistive touch and PWM backlight.
- **480x320 layout**: a 320 px radar at the left edge, and a panel on the right with the
  clock, weather, alert banner and touch controls. Text and aircraft symbols are scaled
  up for the larger radar.
- **Touch controls**: `[-] [range] [+]` zoom buttons, a **SETUP** button, and tap-to-dismiss
  on the alert banner. A four-crosshair calibration runs on first boot and is stored in
  flash; hold the range box for 3 seconds to recalibrate.
- **On-device settings** with **General**, **Alerts**, **Location** and **Wi-Fi** tabs.
  Changes are saved immediately; stepper buttons repeat while held.
- **Saved places**: up to six named locations, switched with one tap. New positions are
  typed on an on-screen number pad and can be saved under a name entered on an on-screen
  keyboard. Switching clears stale aircraft and refetches traffic and weather.
- **Wi-Fi setup on the screen**: scan for networks, pick one and type its password
  (masked, with SHOW/HIDE and a symbols layer). The same screen appears at boot when no
  network works, with the phone portal kept as a fallback button. A failed attempt
  restores the previously saved network.
- **Low-flyer alerts**: an aircraft heading towards your position whose projected closest
  approach is near, soon and low (altitude projected with its climb or descent rate) gets a
  yellow ring on the radar and a flashing banner with callsign, type, altitude, pass
  distance and a countdown. Distance, time and altitude thresholds, **Alert sound** and
  **Ignore gliders** are settings in the Alerts tab; alerts do not flicker at the limits.
- **Buzzer**: an optional passive piezo on GPIO 17 beeps while an alert is pending
  (not yet verified on hardware).
- **Day / Night theme**: a white, high-contrast Day theme for reading the screen in
  sunlight, switched in **SETUP -> General**. The choice is saved and also applies to the
  Wi-Fi screen at boot. Night is the original look.
- **Desktop simulator** (`sim/`): runs the real UI code on a PC with fake aircraft,
  scripted taps and screenshots, for working on the layout without the hardware.
- **GitHub Actions** build both boards, and a release attaches firmware for both
  (`plane-radar-<version>-<board>-full.bin` and `-ota.bin`). `scripts/merge-firmware.sh`
  gained `--env`.

### Changed

- The on-device General tab no longer offers the 24-hour clock, Fahrenheit and miles
  options; they keep their defaults (24 h, Celsius, km) and remain on the web setup page.
- Holding BOOT for 3 seconds (factory reset) now also clears the saved places. Alert
  thresholds and the touch calibration are kept.
- Display pins, size and rotation are chosen per target in `include/config.h`.
- The README documents the S3 build, wiring (including the buzzer), prebuilt firmware and
  flashing.

### Fixed

- **Radar never updated** on adsb.fi: its replies are chunked, which the body reader did
  not decode, so every fetch failed to parse. Requests now use HTTP/1.0.
- **Garbled weather line**: the rounded temperature was printed with the wrong format, so
  the temperature, unit and humidity came out as junk.
- **East-west distances were stretched** by 1/cos(latitude) (about 64% at 52 N) for
  aircraft, runways and alert maths. Distances now use the length of a degree of longitude
  at the radar's own latitude, wherever it is set.
- **Wi-Fi scan found nothing** when the saved network was out of reach: the radio cannot
  scan while it keeps retrying a connection. Scanning now stops the retries first, tries a
  few times, and reports "Scan failed" instead of an empty list.
