#pragma once

namespace ui {

/**
 * Flashing low-flyer banner in the info panel (only when the layout has one,
 * radar::kSideBands). Shows the most urgent alerting aircraft: callsign, type,
 * altitude, closest-approach distance and a countdown.
 */

/** Paint or clear the banner. Repaints only on a state change unless force is set. */
void alertBannerDraw(bool force);

/** Call every loop: advances the flash and the countdown. Cheap when nothing changed. */
void alertBannerTick();

/** True when the banner is showing and (x, y) is inside it (a tap there dismisses it). */
bool alertBannerContains(int x, int y);

}  // namespace ui
