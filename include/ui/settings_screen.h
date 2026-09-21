#pragma once

namespace ui {

/**
 * Full-screen on-device settings (pages: General, Alerts). While it is active
 * the caller should skip drawing the radar and poll only this screen.
 */

/** Draw the screen and start handling touches. */
void settingsScreenOpen();

bool settingsScreenActive();

/**
 * Poll the touch panel and update the screen. Returns true once the user has
 * closed it (DONE); the caller then repaints the radar.
 */
bool settingsScreenPoll();

/**
 * True once after the radar position was changed from the LOCATION tab. The
 * caller should then drop stale aircraft and refetch for the new position.
 */
bool settingsScreenTakeLocationChanged();

}  // namespace ui
