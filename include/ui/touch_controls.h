#pragma once

namespace ui {

/**
 * Range control row in the info panel: [-] [range] [+]. "-" zooms out (larger
 * range), "+" zooms in (smaller range). Only present when the layout has an
 * info panel (radar::kSideBands); otherwise both functions do nothing.
 */

/** Paint the row. Repaints only when its state changed unless force is set. */
void touchControlsDraw(bool force);

/**
 * Poll the touch panel and handle taps on the row. Returns true when the
 * range changed, so the caller must redraw the radar.
 */
bool touchControlsPoll();

}  // namespace ui
