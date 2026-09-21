#pragma once

namespace ui {

/**
 * On-device Wi-Fi setup: connection status, a scan list of nearby networks, and
 * screens for the password, the connection attempt and its result. Used as the
 * WI-FI tab of the settings screen, and full-screen at boot when there is no
 * working network. Same conventions as the location tab.
 */

/** Draw the tab body, whose first row starts at y = top (starts a scan on first use). */
void wifiTabDraw(int top);

/**
 * Feed the tab body touches (x, y valid while down). Call every poll, even without a
 * touch: it also advances the scan and refreshes the status.
 */
void wifiTabTouch(bool down, int x, int y);

/** True while the password, connecting or result screen covers the whole screen. */
bool wifiModalActive();

/** Poll a covering screen. Once it closes (wifiModalActive() turns false) repaint. */
void wifiModalPoll();

/**
 * Boot-time full-screen setup, blocking until the device is connected or the user
 * asks for phone setup. Returns true when connected. phone_requested is set when the
 * user chose the phone/web portal instead (also always set on devices without a
 * touch screen, which return false at once). idle is called every loop iteration.
 */
bool wifiBootSetup(bool* phone_requested, void (*idle)());

}  // namespace ui
