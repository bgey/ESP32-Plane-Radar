#pragma once

#include <LovyanGFX.hpp>

bool displayFontInit();
bool displayFontIsSmooth();

/** Load embedded VLW font on gfx if smooth fonts are enabled and not already active. */
bool displayFontEnsureLoaded(lgfx::LGFXBase& gfx);

/** VLW text size whose line height is target_px on gfx (font must be loaded). */
float displayFontSmoothSizeForHeight(lgfx::LGFXBase& gfx, int target_px);

/** VLW: setTextSize scale (1.0 = font point size). Bitmap: no-op — use displayFontSetBitmap. */
void displayFontSetSmoothSize(lgfx::LGFXBase& gfx, float size);

/** Bitmap GFXfont fallback; clears any runtime VLW font on this instance. */
void displayFontSetBitmap(lgfx::LGFXBase& gfx, const lgfx::GFXfont* font);
