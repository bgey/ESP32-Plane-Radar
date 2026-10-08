#include "ui/ui_font.h"

#include <lgfx/v1/lgfx_fonts.hpp>

#include "hardware/display.h"
#include "hardware/display_font.h"
#include "ui/radar_theme.h"

namespace lgfx_fonts = lgfx::v1::fonts;

namespace ui {
namespace {

struct Cached {
  int height;
  float size;
};
constexpr int kMaxCached = 8;
Cached s_cache[kMaxCached];
int s_cached = 0;

float sizeFor(int height_px) {
  for (int i = 0; i < s_cached; ++i) {
    if (s_cache[i].height == height_px) {
      return s_cache[i].size;
    }
  }
  const float size = displayFontSmoothSizeForHeight(tft, height_px);
  if (s_cached < kMaxCached) {
    s_cache[s_cached++] = {height_px, size};
  }
  return size;
}

}  // namespace

void uiDrawButton(int x, int y, int w, int h, const char* label, int font_px,
                  bool pressed, bool accent) {
  const uint16_t fill = pressed ? radar::kColorPressed
                                : (accent ? radar::kColorAccent
                                          : radar::kColorFooterBackground);
  tft.fillRoundRect(x, y, w, h, 6, fill);
  tft.drawRoundRect(x, y, w, h, 6, radar::kColorGrid);
  uiApplyFont(font_px);
  tft.setTextDatum(textdatum_t::middle_center);
  tft.setTextColor(accent && !pressed ? radar::kColorOnAccent : radar::kColorLabel, fill);
  tft.drawString(label, x + w / 2, y + h / 2);
  tft.setTextDatum(textdatum_t::top_left);
}

void uiApplyFont(int height_px) {
  displayFontEnsureLoaded(tft);
  if (displayFontIsSmooth()) {
    displayFontSetSmoothSize(tft, sizeFor(height_px));
  } else {
    displayFontSetBitmap(tft, height_px >= 20 ? &lgfx_fonts::FreeSansBold12pt7b
                                              : &lgfx_fonts::FreeSansBold9pt7b);
  }
}

}  // namespace ui
