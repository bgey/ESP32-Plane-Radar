#pragma once

namespace ui {

/**
 * Select the UI font on the display at a line height of about height_px pixels
 * (anti-aliased VLW when available, otherwise the nearest bitmap font). Sizes are
 * measured once per height and cached.
 */
void uiApplyFont(int height_px);

/**
 * Rounded button with a centred label (accent = green call-to-action fill). The
 * caller wraps drawing in startWrite()/endWrite() when batching.
 */
void uiDrawButton(int x, int y, int w, int h, const char* label, int font_px,
                  bool pressed, bool accent);

}  // namespace ui
