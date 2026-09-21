#include "hardware/touch_calibration.h"

#if defined(PLANE_RADAR_TARGET_S3_ST7796) && !defined(PLANE_RADAR_SIM)

#include <Preferences.h>

#include <cstdint>

#include <lgfx/v1/lgfx_fonts.hpp>

#include "hardware/display.h"

namespace lgfx_fonts = lgfx::v1::fonts;

namespace {

constexpr char kPrefsNamespace[] = "planeradar";
constexpr char kPrefsKey[] = "touchCal";
constexpr size_t kParamBytes = 8 * sizeof(uint16_t);
constexpr unsigned long kOfferMs = 10000;

bool loadParams(uint16_t* params) {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, true)) {
    return false;
  }
  const size_t n = prefs.getBytes(kPrefsKey, params, kParamBytes);
  prefs.end();
  return n == kParamBytes;
}

void saveParams(const uint16_t* params) {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) {
    return;
  }
  prefs.putBytes(kPrefsKey, params, kParamBytes);
  prefs.end();
}

void showMessage(const char* line1, const char* line2) {
  tft.fillScreen(0x0000);
  tft.setFont(&lgfx_fonts::FreeSansBold12pt7b);
  tft.setTextSize(1);
  tft.setTextDatum(textdatum_t::middle_center);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.drawString(line1, tft.width() / 2, tft.height() / 2 - 16);
  tft.setFont(&lgfx_fonts::FreeSansBold9pt7b);
  tft.drawString(line2, tft.width() / 2, tft.height() / 2 + 20);
  tft.setTextDatum(textdatum_t::top_left);
}

void waitForRelease() {
  int32_t x = 0;
  int32_t y = 0;
  while (tft.getTouchRaw(&x, &y)) {
    delay(20);
  }
  delay(300);
}

}  // namespace

void touchCalibrationRun() {
  waitForRelease();
  showMessage("TOUCH CALIBRATION", "Press each crosshair firmly");
  delay(1500);

  uint16_t params[8] = {};
  tft.calibrateTouch(params, static_cast<uint32_t>(0xFFFFFF),
                     static_cast<uint32_t>(0x000000), 15);
  saveParams(params);
  Serial.println("Touch calibration saved");

  showMessage("CALIBRATION SAVED", "");
  delay(800);
}

void touchCalibrationInit() {
  uint16_t params[8] = {};
  if (loadParams(params)) {
    tft.setTouchCalibrate(params);
    return;
  }

  showMessage("TOUCH CALIBRATION", "Touch the screen to start");
  const unsigned long started = millis();
  int32_t x = 0;
  int32_t y = 0;
  while (millis() - started < kOfferMs) {
    if (tft.getTouch(&x, &y)) {
      touchCalibrationRun();
      return;
    }
    delay(20);
  }
  tft.fillScreen(0x0000);
}

#else

void touchCalibrationInit() {}
void touchCalibrationRun() {}

#endif
