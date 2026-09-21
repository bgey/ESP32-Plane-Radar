#include "ui/alert_banner.h"

#include <lgfx/v1/lgfx_fonts.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"
#include "services/traffic_alert.h"
#include "ui/radar_range.h"
#include "ui/radar_theme.h"

namespace lgfx_fonts = lgfx::v1::fonts;

namespace ui {
namespace {

constexpr int kBannerTopPx = 110;
constexpr int kBannerHeightPx = 126;
constexpr int kBannerInsetPx = 6;
constexpr int kTitleHeightPx = 12;
constexpr int kCallsignHeightPx = 18;
constexpr int kDetailHeightPx = 12;
constexpr int kLineGapPx = 4;
constexpr unsigned long kFlashHalfPeriodMs = 500;

bool s_drawn_active = false;
int s_drawn_phase = -1;
int s_drawn_seconds = -1;
size_t s_drawn_count = 0;
char s_drawn_hex[7] = {};

float s_size_small = 0.36f;
float s_size_large = 0.55f;
bool s_metrics_ready = false;

int bannerX() { return radar::kRadarOriginX + radar::kSize + kBannerInsetPx; }
int bannerWidth() { return radar::kBandWidthPx - 2 * kBannerInsetPx; }

void applyStyle(bool large) {
  displayFontEnsureLoaded(tft);
  if (displayFontIsSmooth()) {
    if (!s_metrics_ready) {
      s_size_small = displayFontSmoothSizeForHeight(tft, kDetailHeightPx);
      s_size_large = displayFontSmoothSizeForHeight(tft, kCallsignHeightPx);
      s_metrics_ready = true;
    }
    displayFontSetSmoothSize(tft, large ? s_size_large : s_size_small);
  } else {
    displayFontSetBitmap(tft, large ? &lgfx_fonts::FreeSansBold12pt7b
                                    : &lgfx_fonts::FreeSansBold9pt7b);
  }
}

void clearBanner() {
  tft.fillRect(radar::kRadarOriginX + radar::kSize + 1, kBannerTopPx,
               radar::kBandWidthPx - 1, kBannerHeightPx + 2,
               radar::kColorFooterBackground);
}

void drawLine(const char* text, bool large, int* y, uint16_t fill) {
  applyStyle(large);
  tft.setTextDatum(textdatum_t::top_center);
  tft.setTextColor(radar::kColorLabel, fill);
  tft.drawString(text, bannerX() + bannerWidth() / 2, *y);
  *y += tft.fontHeight() + kLineGapPx;
}

void drawBanner(const services::adsb::Aircraft& plane,
                const services::alert::Info& info,
                int phase, int seconds, size_t count) {
  const uint16_t fill =
      phase == 0 ? radar::kColorAlertBright : radar::kColorAlertDark;
  tft.fillRect(bannerX(), kBannerTopPx, bannerWidth(), kBannerHeightPx, fill);
  tft.drawRect(bannerX(), kBannerTopPx, bannerWidth(), kBannerHeightPx,
               radar::kColorLabel);

  int y = kBannerTopPx + 8;
  drawLine("LOW INBOUND", false, &y, fill);
  drawLine(plane.callsign[0] != '\0' ? plane.callsign : plane.hex, true, &y, fill);
  if (plane.type[0] != '\0') {
    drawLine(plane.type, false, &y, fill);
  }

  char text[24];
  if (plane.has_alt) {
    snprintf(text, sizeof(text), "%d ft", static_cast<int>(plane.alt_ft));
    drawLine(text, false, &y, fill);
  }
  if (radar::useMiles()) {
    snprintf(text, sizeof(text), "PASS %.1f mi", info.pass_km / 1.609344f);
  } else {
    snprintf(text, sizeof(text), "PASS %.1f km", info.pass_km);
  }
  drawLine(text, false, &y, fill);
  snprintf(text, sizeof(text), "IN %d:%02d", seconds / 60, seconds % 60);
  drawLine(text, false, &y, fill);
  if (count > 1) {
    snprintf(text, sizeof(text), "+%u more", static_cast<unsigned>(count - 1));
    drawLine(text, false, &y, fill);
  }
  tft.setTextDatum(textdatum_t::top_left);
}

}  // namespace

void alertBannerDraw(bool force) {
  if (!radar::kSideBands) {
    return;
  }

  const services::adsb::Aircraft* plane = nullptr;
  services::alert::Info info{};
  if (!services::alert::mostUrgent(&plane, &info)) {
    if (s_drawn_active && !force) {
      clearBanner();
    }
    s_drawn_active = false;
    return;
  }

  const float elapsed_s =
      static_cast<float>(millis() - services::alert::updatedMs()) / 1000.0f;
  const int seconds =
      static_cast<int>(std::max(0.0f, info.time_s - elapsed_s));
  const int phase = static_cast<int>((millis() / kFlashHalfPeriodMs) % 2);
  const size_t count = services::alert::pendingCount();

  if (!force && s_drawn_active && phase == s_drawn_phase &&
      seconds == s_drawn_seconds && count == s_drawn_count &&
      strcmp(plane->hex, s_drawn_hex) == 0) {
    return;
  }

  drawBanner(*plane, info, phase, seconds, count);
  s_drawn_active = true;
  s_drawn_phase = phase;
  s_drawn_seconds = seconds;
  s_drawn_count = count;
  snprintf(s_drawn_hex, sizeof(s_drawn_hex), "%s", plane->hex);
}

void alertBannerTick() { alertBannerDraw(false); }

bool alertBannerContains(int x, int y) {
  if (!radar::kSideBands || !s_drawn_active) {
    return false;
  }
  return x >= bannerX() && x < bannerX() + bannerWidth() && y >= kBannerTopPx &&
         y < kBannerTopPx + kBannerHeightPx;
}

}  // namespace ui
