#pragma once

namespace ui {

/**
 * Touch controls in the info panel (only when the layout has one,
 * radar::kSideBands; otherwise everything here does nothing):
 *   - Range row [-] [range] [+]: "-" zooms out (larger range), "+" zooms in.
 *   - SETUP button above the range row: opens the settings screen.
 *   - Tapping the alert banner dismisses that alert.
 *   - Holding the range box for 3 s recalibrates the touch panel.
 */
enum class TouchAction {
  kNone,
  /** State changed; the caller must repaint the radar. */
  kRedraw,
  /** The SETUP button was tapped. */
  kOpenSettings,
};

/** Paint the controls. Repaints only when their state changed unless force is set. */
void touchControlsDraw(bool force);

/** Poll the touch panel and handle taps on the controls. */
TouchAction touchControlsPoll();

}  // namespace ui
