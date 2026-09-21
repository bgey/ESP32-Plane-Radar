#include "ui/touch_controls.h"

#include <lgfx/v1/lgfx_fonts.hpp>

#include <cstdint>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"
#include "hardware/touch_calibration.h"
#include "ui/radar_range.h"
#include "ui/radar_theme.h"

namespace lgfx_fonts = lgfx::v1::fonts;

namespace ui {
namespace {

constexpr int kRowHeightPx = 46;
constexpr int kBoxCount = 3;
constexpr int kLabelHeightPx = 20;
constexpr int kGlyphHalfLenPx = 11;
constexpr int kGlyphThicknessPx = 4;

enum Box { kNone = -1, kMinus = 0, kRange = 1, kPlus = 2 };

constexpr unsigned long kRecalibrateHoldMs = 3000;

bool s_down = false;
int s_pressed = kNone;
unsigned long s_range_hold_start_ms = 0;

bool s_drawn = false;
uint8_t s_drawn_index = 0;
int s_drawn_pressed = kNone;
bool s_drawn_miles = false;

float s_label_vlw_size = 0.5f;
bool s_label_metrics_ready = false;

int rowX() { return radar::kRadarOriginX + radar::kSize; }
int rowY() { return config::kDisplayHeight - kRowHeightPx; }

/** Box edges: three boxes side by side with no gaps, filling the panel width. */
int boxEdge(int i) { return rowX() + i * radar::kBandWidthPx / kBoxCount; }

int hitTest(int x, int y) {
  if (y < rowY() || y >= rowY() + kRowHeightPx) {
    return kNone;
  }
  for (int i = 0; i < kBoxCount; ++i) {
    if (x >= boxEdge(i) && x < boxEdge(i + 1)) {
      return i;
    }
  }
  return kNone;
}

bool boxEnabled(int box) {
  if (box == kMinus) {
    return static_cast<size_t>(radar::rangeIndex()) + 1 < radar::kRangePresetCount;
  }
  if (box == kPlus) {
    return radar::rangeIndex() > 0;
  }
  return true;
}

void drawMinus(int cx, int cy, uint16_t color) {
  tft.fillRect(cx - kGlyphHalfLenPx, cy - kGlyphThicknessPx / 2,
               kGlyphHalfLenPx * 2, kGlyphThicknessPx, color);
}

void drawPlus(int cx, int cy, uint16_t color) {
  drawMinus(cx, cy, color);
  tft.fillRect(cx - kGlyphThicknessPx / 2, cy - kGlyphHalfLenPx,
               kGlyphThicknessPx, kGlyphHalfLenPx * 2, color);
}

void applyLabelStyle() {
  displayFontEnsureLoaded(tft);
  if (displayFontIsSmooth()) {
    if (!s_label_metrics_ready) {
      s_label_vlw_size = displayFontSmoothSizeForHeight(tft, kLabelHeightPx);
      s_label_metrics_ready = true;
    }
    displayFontSetSmoothSize(tft, s_label_vlw_size);
  } else {
    displayFontSetBitmap(tft, &lgfx_fonts::FreeSansBold12pt7b);
  }
}

void drawRow() {
  const int y = rowY();
  const int cy = y + kRowHeightPx / 2;

  for (int i = 0; i < kBoxCount; ++i) {
    const int x0 = boxEdge(i);
    const int w = boxEdge(i + 1) - x0;
    const bool pressed = (i == s_pressed) && boxEnabled(i);
    const uint16_t fill =
        pressed ? radar::kColorGrid : radar::kColorFooterBackground;
    tft.fillRect(x0, y, w, kRowHeightPx, fill);

    const int cx = x0 + w / 2;
    const uint16_t glyph =
        boxEnabled(i) ? radar::kColorLabel : radar::kColorGrid;
    if (i == kMinus) {
      drawMinus(cx, cy, glyph);
    } else if (i == kPlus) {
      drawPlus(cx, cy, glyph);
    } else {
      char label[12];
      radar::formatCurrentRing3Label(label, sizeof(label));
      applyLabelStyle();
      tft.setTextDatum(textdatum_t::middle_center);
      tft.setTextColor(radar::kColorTagAltitude, fill);
      tft.drawString(label, cx, cy);
      tft.setTextDatum(textdatum_t::top_left);
    }
  }

  // Grid-coloured outline and dividers.
  tft.drawRect(rowX(), y, radar::kBandWidthPx, kRowHeightPx, radar::kColorGrid);
  for (int i = 1; i < kBoxCount; ++i) {
    tft.drawFastVLine(boxEdge(i), y, kRowHeightPx, radar::kColorGrid);
  }

  s_drawn = true;
  s_drawn_index = radar::rangeIndex();
  s_drawn_pressed = s_pressed;
  s_drawn_miles = radar::useMiles();
}

}  // namespace

void touchControlsDraw(bool force) {
  if (!radar::kSideBands) {
    return;
  }
  if (!force && s_drawn && s_drawn_index == radar::rangeIndex() &&
      s_drawn_pressed == s_pressed && s_drawn_miles == radar::useMiles()) {
    return;
  }
  drawRow();
}

bool touchControlsPoll() {
  if (!radar::kSideBands) {
    return false;
  }

  int32_t x = 0;
  int32_t y = 0;
  const bool down = tft.getTouch(&x, &y);
  bool changed = false;

  if (down) {
    const int box = hitTest(x, y);
    // Holding the range box recalibrates the touch panel.
    if (!s_down) {
      s_range_hold_start_ms = box == kRange ? millis() : 0;
    } else if (box != kRange) {
      s_range_hold_start_ms = 0;
    }
    if (s_range_hold_start_ms != 0 &&
        millis() - s_range_hold_start_ms >= kRecalibrateHoldMs) {
      s_range_hold_start_ms = 0;
      s_down = false;
      s_pressed = kNone;
      touchCalibrationRun();
      return true;  // the caller repaints everything
    }
    if (!s_down) {
      // A press only counts on an enabled button; the range box is inert.
      s_pressed = (box == kMinus || box == kPlus) && boxEnabled(box) ? box : kNone;
      touchControlsDraw(false);
    } else if (s_pressed != kNone && box != s_pressed) {
      s_pressed = kNone;  // finger slid off: cancel
      touchControlsDraw(false);
    }
    s_down = true;
  } else if (s_down) {
    s_down = false;
    const int box = s_pressed;
    s_pressed = kNone;
    if (box == kMinus) {
      changed = radar::rangeStep(+1);
    } else if (box == kPlus) {
      changed = radar::rangeStep(-1);
    }
    if (!changed) {
      touchControlsDraw(false);  // clear the pressed highlight
    }
  }
  return changed;
}

}  // namespace ui
