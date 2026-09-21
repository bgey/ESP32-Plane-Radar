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

}  // namespace ui
