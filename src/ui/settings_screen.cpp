#include "ui/settings_screen.h"

#include <lgfx/v1/lgfx_fonts.hpp>

#include <cstdint>
#include <cstdio>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"
#include "services/display_settings.h"
#include "services/traffic_alert.h"
#include "ui/location_settings.h"
#include "ui/radar_range.h"
#include "ui/wifi_settings.h"
#include "ui/radar_theme.h"
#include "ui/theme.h"

namespace lgfx_fonts = lgfx::v1::fonts;

namespace ui {
namespace {

constexpr int kScreenW = config::kDisplayWidth;
constexpr int kHeaderH = 44;
constexpr int kRowsTop = kHeaderH + 3;
constexpr int kRowH = 38;
constexpr int kMargin = 12;

constexpr int kTabW = 96;
constexpr int kTabH = 34;
constexpr int kDoneW = 78;

constexpr int kToggleW = 60;
constexpr int kToggleH = 26;
constexpr int kStepBoxW = 44;
constexpr int kStepBoxH = 30;
constexpr int kStepValueW = 92;

constexpr int kBodyHeightPx = 16;
constexpr unsigned long kRepeatDelayMs = 500;
constexpr unsigned long kRepeatIntervalMs = 130;

constexpr float kKmPerMile = 1.609344f;

enum class Kind { kToggle, kStepper, kAction };

struct Row {
  const char* label;
  Kind kind;
  bool (*get)();
  void (*toggle)();
  void (*value)(char*, size_t);
  void (*step)(int);
  void (*run)();
  /** Repaint the whole screen after a toggle (it changes how everything looks). */
  bool full_redraw;  // omitted in a row's initializer means false
};

/** Pages that are drawn and handled by their own module instead of generic rows. */
enum class Special { kNone, kLocation, kWifi };

struct Page {
  const char* title;
  const Row* rows;
  size_t count;
  Special special;
};

// ---- Setting accessors ----------------------------------------------------

int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void formatDistance(char* out, size_t n, float km) {
  if (radar::useMiles()) {
    snprintf(out, n, "%.1f mi", km / kKmPerMile);
  } else {
    snprintf(out, n, "%.1f km", km);
  }
}

// The 24-hour clock, temperature unit and distance unit are only editable on the
// web setup page; they keep their defaults (24 h, Celsius, km) unless changed there.
const Row kGeneralRows[] = {
    {"Day theme (sunlight)", Kind::kToggle,
     [] { return theme::mode() == theme::Mode::kDay; },
     [] {
       theme::setMode(theme::mode() == theme::Mode::kDay ? theme::Mode::kNight
                                                          : theme::Mode::kDay);
     },
     nullptr, nullptr, nullptr, true},
    {"Show runways", Kind::kToggle, [] { return radar::showRunways(); },
     [] { radar::setShowRunways(!radar::showRunways()); }, nullptr, nullptr, nullptr},
    {"Clock and weather panel", Kind::kToggle,
     [] { return services::settings::footerEnabled(); },
     [] { services::settings::setFooterEnabled(!services::settings::footerEnabled()); },
     nullptr, nullptr, nullptr},
    {"Show weather", Kind::kToggle,
     [] { return services::settings::weatherEnabled(); },
     [] { services::settings::setWeatherEnabled(!services::settings::weatherEnabled()); },
     nullptr, nullptr, nullptr},
    {"Text size", Kind::kStepper, nullptr, nullptr,
     [](char* out, size_t n) {
       snprintf(out, n, "%d%%", services::settings::textScalePercent());
     },
     [](int dir) {
       services::settings::setTextScalePercent(clampInt(
           services::settings::textScalePercent() + 10 * dir,
           services::settings::kTextScaleMinPercent,
           services::settings::kTextScaleMaxPercent));
     },
     nullptr},
};

const Row kAlertRows[] = {
    {"Low-flyer alerts", Kind::kToggle,
     [] { return services::alert::settings().enabled; },
     [] { services::alert::setEnabled(!services::alert::settings().enabled); },
     nullptr, nullptr, nullptr},
    {"Alert sound", Kind::kToggle,
     [] { return services::alert::settings().sound_enabled; },
     [] { services::alert::setSoundEnabled(!services::alert::settings().sound_enabled); },
     nullptr, nullptr, nullptr},
    {"Ignore gliders", Kind::kToggle,
     [] { return services::alert::settings().ignore_gliders; },
     [] { services::alert::setIgnoreGliders(!services::alert::settings().ignore_gliders); },
     nullptr, nullptr, nullptr},
    {"Alert below altitude", Kind::kStepper, nullptr, nullptr,
     [](char* out, size_t n) {
       snprintf(out, n, "%d ft",
                static_cast<int>(services::alert::settings().max_altitude_ft));
     },
     [](int dir) {
       services::alert::setMaxAltitudeFt(services::alert::settings().max_altitude_ft +
                                         services::alert::kAltitudeStepFt * dir);
     },
     nullptr},
    {"Passing within", Kind::kStepper, nullptr, nullptr,
     [](char* out, size_t n) {
       formatDistance(out, n, services::alert::settings().max_pass_km);
     },
     [](int dir) {
       services::alert::setMaxPassKm(services::alert::settings().max_pass_km +
                                     services::alert::kPassStepKm * dir);
     },
     nullptr},
    {"Arriving within", Kind::kStepper, nullptr, nullptr,
     [](char* out, size_t n) {
       snprintf(out, n, "%d s",
                static_cast<int>(services::alert::settings().max_time_s));
     },
     [](int dir) {
       services::alert::setMaxTimeS(services::alert::settings().max_time_s +
                                    services::alert::kTimeStepS * dir);
     },
     nullptr},
    {"Reset alert defaults", Kind::kAction, nullptr, nullptr, nullptr, nullptr,
     [] { services::alert::resetSettings(); }},
};

const Page kPages[] = {
    {"GENERAL", kGeneralRows, sizeof(kGeneralRows) / sizeof(kGeneralRows[0]),
     Special::kNone},
    {"ALERTS", kAlertRows, sizeof(kAlertRows) / sizeof(kAlertRows[0]), Special::kNone},
    {"LOCATION", nullptr, 0, Special::kLocation},
    {"WI-FI", nullptr, 0, Special::kWifi},
};
constexpr int kPageCount = sizeof(kPages) / sizeof(kPages[0]);

// ---- State ----------------------------------------------------------------

enum class Target { kNone, kDone, kTab, kToggle, kMinus, kPlus, kAction };

struct Hit {
  Target target = Target::kNone;
  int index = 0;  // tab number or row number
  bool operator==(const Hit& o) const {
    return target == o.target && (target == Target::kNone || index == o.index);
  }
};

bool s_active = false;
int s_page = 0;
bool s_down = false;
Hit s_pressed;
unsigned long s_press_ms = 0;
unsigned long s_repeat_ms = 0;

float s_body_vlw_size = 0.4f;
bool s_metrics_ready = false;

uint16_t colorOn() { return radar::kColorToggleOn; }
uint16_t colorOff() { return radar::kColorToggleOff; }
uint16_t colorDone() { return radar::kColorAccent; }

void applyBodyFont() {
  displayFontEnsureLoaded(tft);
  if (displayFontIsSmooth()) {
    if (!s_metrics_ready) {
      s_body_vlw_size = displayFontSmoothSizeForHeight(tft, kBodyHeightPx);
      s_metrics_ready = true;
    }
    displayFontSetSmoothSize(tft, s_body_vlw_size);
  } else {
    displayFontSetBitmap(tft, &lgfx_fonts::FreeSansBold9pt7b);
  }
}

// ---- Geometry -------------------------------------------------------------

int rowY(int row) { return kRowsTop + row * kRowH; }
int tabX(int tab) { return 6 + tab * kTabW; }
int doneX() { return kScreenW - kDoneW - 6; }
int toggleX() { return kScreenW - kMargin - kToggleW; }
int plusX() { return kScreenW - kMargin - kStepBoxW; }
int minusX() { return plusX() - kStepValueW - kStepBoxW; }
int valueCenterX() { return minusX() + kStepBoxW + kStepValueW / 2; }

Hit hitTest(int x, int y) {
  Hit hit;
  if (y < kHeaderH) {
    if (x >= doneX() && y >= 5 && y < 5 + kTabH) {
      hit.target = Target::kDone;
      return hit;
    }
    for (int i = 0; i < kPageCount; ++i) {
      if (x >= tabX(i) && x < tabX(i) + kTabW - 4 && y >= 5 && y < 5 + kTabH) {
        hit.target = Target::kTab;
        hit.index = i;
        return hit;
      }
    }
    return hit;
  }

  const Page& page = kPages[s_page];
  for (size_t i = 0; i < page.count; ++i) {
    const int top = rowY(static_cast<int>(i));
    if (y < top || y >= top + kRowH) {
      continue;
    }
    hit.index = static_cast<int>(i);
    switch (page.rows[i].kind) {
      case Kind::kToggle:
        hit.target = Target::kToggle;
        break;
      case Kind::kAction:
        hit.target = Target::kAction;
        break;
      case Kind::kStepper:
        if (x >= minusX() && x < minusX() + kStepBoxW) {
          hit.target = Target::kMinus;
        } else if (x >= plusX() && x < plusX() + kStepBoxW) {
          hit.target = Target::kPlus;
        }
        break;
    }
    return hit;
  }
  return hit;
}

// ---- Drawing --------------------------------------------------------------

void drawStepBox(int x, int cy, bool plus, bool pressed) {
  const int y = cy - kStepBoxH / 2;
  const uint16_t fill = pressed ? radar::kColorPressed : radar::kColorFooterBackground;
  tft.fillRoundRect(x, y, kStepBoxW, kStepBoxH, 5, fill);
  tft.drawRoundRect(x, y, kStepBoxW, kStepBoxH, 5, radar::kColorGrid);
  const int cx = x + kStepBoxW / 2;
  tft.fillRect(cx - 9, cy - 2, 18, 4, radar::kColorLabel);
  if (plus) {
    tft.fillRect(cx - 2, cy - 9, 4, 18, radar::kColorLabel);
  }
}

void drawRow(int row) {
  const Page& page = kPages[s_page];
  const Row& r = page.rows[row];
  const int top = rowY(row);
  const int cy = top + kRowH / 2;

  tft.startWrite();
  tft.fillRect(0, top, kScreenW, kRowH, radar::kColorBackground);
  tft.drawFastHLine(kMargin, top + kRowH - 1, kScreenW - 2 * kMargin,
                    radar::kColorGrid);

  const bool row_pressed =
      s_down && s_pressed.index == row &&
      (s_pressed.target == Target::kToggle || s_pressed.target == Target::kAction);
  if (row_pressed) {
    tft.fillRect(0, top, kScreenW, kRowH - 1, radar::kColorFooterBackground);
  }

  applyBodyFont();
  tft.setTextDatum(textdatum_t::middle_left);
  tft.setTextColor(radar::kColorLabel,
                   row_pressed ? radar::kColorFooterBackground : radar::kColorBackground);
  tft.drawString(r.label, kMargin, cy);

  if (r.kind == Kind::kToggle) {
    const bool on = r.get();
    const int x = toggleX();
    tft.fillRoundRect(x, cy - kToggleH / 2, kToggleW, kToggleH, kToggleH / 2,
                      on ? colorOn() : colorOff());
    const int knob_x = on ? x + kToggleW - kToggleH / 2 : x + kToggleH / 2;
    tft.fillCircle(knob_x, cy, kToggleH / 2 - 3, radar::kColorOnAccent);
  } else if (r.kind == Kind::kStepper) {
    drawStepBox(minusX(), cy, false,
                s_down && s_pressed.target == Target::kMinus && s_pressed.index == row);
    drawStepBox(plusX(), cy, true,
                s_down && s_pressed.target == Target::kPlus && s_pressed.index == row);
    char value[16];
    r.value(value, sizeof(value));
    applyBodyFont();
    tft.setTextDatum(textdatum_t::middle_center);
    tft.setTextColor(radar::kColorTagAltitude, radar::kColorBackground);
    tft.drawString(value, valueCenterX(), cy);
  } else {
    // Action rows show a chevron-style hint on the right.
    tft.setTextDatum(textdatum_t::middle_right);
    tft.setTextColor(radar::kColorTagType,
                     row_pressed ? radar::kColorFooterBackground : radar::kColorBackground);
    tft.drawString("TAP", kScreenW - kMargin, cy);
  }
  tft.setTextDatum(textdatum_t::top_left);
  tft.endWrite();
}

void drawHeader() {
  tft.startWrite();
  tft.fillRect(0, 0, kScreenW, kHeaderH, radar::kColorFooterBackground);
  tft.drawFastHLine(0, kHeaderH - 1, kScreenW, radar::kColorGrid);

  applyBodyFont();
  tft.setTextDatum(textdatum_t::middle_center);
  for (int i = 0; i < kPageCount; ++i) {
    const bool selected = i == s_page;
    const bool pressed =
        s_down && s_pressed.target == Target::kTab && s_pressed.index == i;
    const uint16_t fill =
        selected ? radar::kColorPressed
                 : (pressed ? radar::kColorBackground : radar::kColorFooterBackground);
    tft.fillRoundRect(tabX(i), 5, kTabW - 4, kTabH, 6, fill);
    tft.drawRoundRect(tabX(i), 5, kTabW - 4, kTabH, 6, radar::kColorGrid);
    tft.setTextColor(radar::kColorLabel, fill);
    tft.drawString(kPages[i].title, tabX(i) + (kTabW - 4) / 2, 5 + kTabH / 2);
  }

  const bool done_pressed = s_down && s_pressed.target == Target::kDone;
  const uint16_t done_fill = done_pressed ? radar::kColorPressed : colorDone();
  tft.fillRoundRect(doneX(), 5, kDoneW, kTabH, 6, done_fill);
  tft.setTextColor(done_pressed ? radar::kColorLabel : radar::kColorOnAccent, done_fill);
  tft.drawString("DONE", doneX() + kDoneW / 2, 5 + kTabH / 2);
  tft.setTextDatum(textdatum_t::top_left);
  tft.endWrite();
}

void drawAll() {
  tft.startWrite();
  tft.fillScreen(radar::kColorBackground);
  drawHeader();
  for (size_t i = 0; i < kPages[s_page].count; ++i) {
    drawRow(static_cast<int>(i));
  }
  if (kPages[s_page].special == Special::kLocation) {
    locationTabDraw(kRowsTop);
  } else if (kPages[s_page].special == Special::kWifi) {
    wifiTabDraw(kRowsTop);
  }
  tft.endWrite();
}

/** Redraw whatever a change of target press state touches. */
void redrawFor(const Hit& hit) {
  switch (hit.target) {
    case Target::kDone:
    case Target::kTab:
      drawHeader();
      break;
    case Target::kToggle:
    case Target::kMinus:
    case Target::kPlus:
    case Target::kAction:
      drawRow(hit.index);
      break;
    case Target::kNone:
      break;
  }
}

void applyStep(const Hit& hit) {
  const Row& r = kPages[s_page].rows[hit.index];
  r.step(hit.target == Target::kPlus ? +1 : -1);
  drawRow(hit.index);
}

void activate(const Hit& hit) {
  switch (hit.target) {
    case Target::kTab:
      s_page = hit.index;
      drawAll();
      break;
    case Target::kToggle: {
      const Row& row = kPages[s_page].rows[hit.index];
      row.toggle();
      if (row.full_redraw) {
        drawAll();
      } else {
        drawRow(hit.index);
      }
      break;
    }
    case Target::kAction:
      kPages[s_page].rows[hit.index].run();
      // Values may have changed anywhere on the page.
      drawAll();
      break;
    default:
      break;
  }
}

}  // namespace

void settingsScreenOpen() {
  s_active = true;
  s_page = 0;
  s_down = false;
  s_pressed = Hit{};
  drawAll();
}

bool settingsScreenActive() { return s_active; }

bool settingsScreenTakeLocationChanged() { return locationTakeChanged(); }

bool settingsScreenPoll() {
  if (!s_active) {
    return false;
  }

  // The place editor, Wi-Fi connect screen and their input pads cover the whole screen.
  if (locationModalActive()) {
    locationModalPoll();
    if (!locationModalActive()) {
      s_down = false;
      s_pressed = Hit{};
      drawAll();
    }
    return false;
  }
  if (wifiModalActive()) {
    wifiModalPoll();
    if (!wifiModalActive()) {
      s_down = false;
      s_pressed = Hit{};
      drawAll();
    }
    return false;
  }

  int32_t x = 0;
  int32_t y = 0;
  const bool down = tft.getTouch(&x, &y);
  const unsigned long now = millis();
  bool closed = false;

  if (kPages[s_page].special == Special::kLocation) {
    locationTabTouch(down && y >= kHeaderH, x, y);
    if (locationModalActive()) {
      return false;  // a touch on the tab just opened the editor
    }
  } else if (kPages[s_page].special == Special::kWifi) {
    wifiTabTouch(down && y >= kHeaderH, x, y);  // also advances scans and status
    if (wifiModalActive()) {
      return false;
    }
  }

  if (down) {
    const Hit hit = hitTest(x, y);
    if (!s_down) {
      s_down = true;
      s_pressed = hit;
      s_press_ms = now;
      s_repeat_ms = now;
      if (hit.target == Target::kMinus || hit.target == Target::kPlus) {
        applyStep(hit);  // steppers act on press, and repeat while held
      } else {
        redrawFor(hit);
      }
    } else if (!(hit == s_pressed)) {
      const Hit previous = s_pressed;
      s_pressed = Hit{};  // finger slid off the target: cancel
      redrawFor(previous);
    } else if ((s_pressed.target == Target::kMinus ||
                s_pressed.target == Target::kPlus) &&
               now - s_press_ms >= kRepeatDelayMs &&
               now - s_repeat_ms >= kRepeatIntervalMs) {
      s_repeat_ms = now;
      applyStep(s_pressed);
    }
  } else if (s_down) {
    s_down = false;
    const Hit released = s_pressed;
    s_pressed = Hit{};
    if (released.target == Target::kDone) {
      s_active = false;
      closed = true;
    } else if (released.target == Target::kMinus || released.target == Target::kPlus) {
      redrawFor(released);  // clear the pressed highlight
    } else if (released.target != Target::kNone) {
      activate(released);  // redraws without the pressed highlight
    }
  }
  return closed;
}

}  // namespace ui
