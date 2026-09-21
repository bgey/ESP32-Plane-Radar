#include "ui/touch_controls.h"

#include <lgfx/v1/lgfx_fonts.hpp>

#include <cstdint>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"
#include "hardware/touch_calibration.h"
#include "services/traffic_alert.h"
#include "ui/alert_banner.h"
#include "ui/radar_range.h"
#include "ui/radar_theme.h"

namespace lgfx_fonts = lgfx::v1::fonts;

namespace ui {
namespace {

constexpr int kRowHeightPx = 46;
constexpr int kSetupHeightPx = 34;
constexpr int kBoxCount = 3;
constexpr int kLabelHeightPx = 20;
constexpr int kSetupLabelHeightPx = 16;
constexpr int kGlyphHalfLenPx = 11;
constexpr int kGlyphThicknessPx = 4;

enum Box { kNone = -1, kMinus = 0, kRange = 1, kPlus = 2, kSetup = 3, kBanner = 4 };

constexpr unsigned long kRecalibrateHoldMs = 3000;

bool s_down = false;
int s_pressed = kNone;
unsigned long s_range_hold_start_ms = 0;

bool s_drawn = false;
uint8_t s_drawn_index = 0;
int s_drawn_pressed = kNone;
bool s_drawn_miles = false;

float s_label_vlw_size = 0.5f;
float s_setup_vlw_size = 0.4f;
bool s_label_metrics_ready = false;

int rowX() { return radar::kRadarOriginX + radar::kSize; }
int rowY() { return config::kDisplayHeight - kRowHeightPx; }
int setupY() { return rowY() - kSetupHeightPx; }

/** Box edges: three boxes side by side with no gaps, filling the panel width. */
int boxEdge(int i) { return rowX() + i * radar::kBandWidthPx / kBoxCount; }

int hitTest(int x, int y) {
  if (x < rowX()) {
    return kNone;
  }
  if (y >= rowY() && y < rowY() + kRowHeightPx) {
    for (int i = 0; i < kBoxCount; ++i) {
      if (x >= boxEdge(i) && x < boxEdge(i + 1)) {
        return i;
      }
    }
    return kNone;
  }
  if (y >= setupY() && y < setupY() + kSetupHeightPx) {
    return kSetup;
  }
  if (alertBannerContains(x, y)) {
    return kBanner;
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

void applyLabelStyle(bool setup_button) {
  displayFontEnsureLoaded(tft);
  if (displayFontIsSmooth()) {
    if (!s_label_metrics_ready) {
      s_label_vlw_size = displayFontSmoothSizeForHeight(tft, kLabelHeightPx);
      s_setup_vlw_size = displayFontSmoothSizeForHeight(tft, kSetupLabelHeightPx);
      s_label_metrics_ready = true;
    }
    displayFontSetSmoothSize(tft, setup_button ? s_setup_vlw_size : s_label_vlw_size);
  } else {
    displayFontSetBitmap(tft, setup_button ? &lgfx_fonts::FreeSansBold9pt7b
                                           : &lgfx_fonts::FreeSansBold12pt7b);
  }
}

void drawSetupButton() {
  const bool pressed = s_pressed == kSetup;
  const uint16_t fill =
      pressed ? radar::kColorGrid : radar::kColorFooterBackground;
  tft.fillRect(rowX(), setupY(), radar::kBandWidthPx, kSetupHeightPx, fill);
  tft.drawRect(rowX(), setupY(), radar::kBandWidthPx, kSetupHeightPx,
               radar::kColorGrid);
  applyLabelStyle(true);
  tft.setTextDatum(textdatum_t::middle_center);
  tft.setTextColor(radar::kColorLabel, fill);
  tft.drawString("SETUP", rowX() + radar::kBandWidthPx / 2,
                 setupY() + kSetupHeightPx / 2);
  tft.setTextDatum(textdatum_t::top_left);
}

void drawRow() {
  const int y = rowY();
  const int cy = y + kRowHeightPx / 2;

  tft.startWrite();
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
      applyLabelStyle(false);
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

  drawSetupButton();
  tft.endWrite();

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

TouchAction touchControlsPoll() {
  if (!radar::kSideBands) {
    return TouchAction::kNone;
  }

  int32_t x = 0;
  int32_t y = 0;
  const bool down = tft.getTouch(&x, &y);
  TouchAction action = TouchAction::kNone;

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
      return TouchAction::kRedraw;  // the caller repaints everything
    }
    if (!s_down) {
      // A press only counts on an enabled button; the range box is inert.
      s_pressed = (box != kNone && box != kRange && boxEnabled(box)) ? box : kNone;
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
      action = radar::rangeStep(+1) ? TouchAction::kRedraw : TouchAction::kNone;
    } else if (box == kPlus) {
      action = radar::rangeStep(-1) ? TouchAction::kRedraw : TouchAction::kNone;
    } else if (box == kSetup) {
      action = TouchAction::kOpenSettings;
    } else if (box == kBanner) {
      services::alert::dismissMostUrgent();
      action = TouchAction::kRedraw;
    }
    if (action == TouchAction::kNone) {
      touchControlsDraw(false);  // clear the pressed highlight
    }
  }
  return action;
}

}  // namespace ui
