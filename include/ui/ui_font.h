#pragma once

namespace ui {

/**
 * Select the UI font on the display at a line height of about height_px pixels
 * (anti-aliased VLW when available, otherwise the nearest bitmap font). Sizes are
 * measured once per height and cached.
 */
void uiApplyFont(int height_px);

}  // namespace ui
