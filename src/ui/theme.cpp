#include "ui/theme.h"

#include <Preferences.h>

#include "config.h"
#include "hardware/display.h"
#include "ui/radar_theme.h"

namespace ui::theme {
namespace {

constexpr char kPrefsNamespace[] = "theme";
constexpr char kKeyMode[] = "mode";

struct Rgb {
  uint8_t r, g, b;
};

struct Palette {
  Rgb background;
  Rgb grid;
  Rgb label;
  Rgb center;
  Rgb aircraft;
  Rgb track;
  Rgb tag_type;
  Rgb tag_altitude;
  Rgb runway;
  Rgb runway_label;
  Rgb panel;
  Rgb alert_ring;
  Rgb alert_dark;
  Rgb alert_bright;
  Rgb pressed;
  Rgb accent;
  Rgb on_accent;
  Rgb toggle_on;
  Rgb toggle_off;
  Rgb good;
  Rgb bad;
  Rgb dim;
  Rgb disabled;
};

// The original look.
constexpr Palette kNight = {
    /*background*/ {4, 10, 28},
    /*grid*/ {16, 100, 32},
    /*label*/ {255, 255, 255},
    /*center*/ {255, 255, 255},
    /*aircraft*/ {255, 0, 0},
    /*track*/ {255, 0, 255},
    /*tag_type*/ {255, 200, 0},
    /*tag_altitude*/ {90, 200, 255},
    /*runway*/ {56, 150, 170},
    /*runway_label*/ {110, 210, 230},
    /*panel*/ {3, 16, 32},
    /*alert_ring*/ {255, 220, 0},
    /*alert_dark*/ {90, 0, 0},
    /*alert_bright*/ {210, 0, 0},
    /*pressed*/ {16, 100, 32},
    /*accent*/ {30, 130, 60},
    /*on_accent*/ {255, 255, 255},
    /*toggle_on*/ {40, 170, 70},
    /*toggle_off*/ {70, 80, 95},
    /*good*/ {80, 200, 90},
    /*bad*/ {255, 130, 80},
    /*dim*/ {90, 110, 130},
    /*disabled*/ {16, 100, 32},
};

// White background, black text and darkened colours: a bright screen with dark text
// keeps its contrast in sunlight, where reflections wash out a dark screen.
constexpr Palette kDay = {
    /*background*/ {255, 255, 255},
    /*grid*/ {0, 110, 40},
    /*label*/ {0, 0, 0},
    /*center*/ {0, 0, 0},
    /*aircraft*/ {210, 0, 0},
    /*track*/ {150, 0, 170},
    /*tag_type*/ {150, 85, 0},
    /*tag_altitude*/ {0, 60, 160},
    /*runway*/ {0, 110, 135},
    /*runway_label*/ {0, 85, 110},
    /*panel*/ {228, 234, 241},
    /*alert_ring*/ {255, 110, 0},
    /*alert_dark*/ {110, 0, 0},
    /*alert_bright*/ {215, 0, 0},
    /*pressed*/ {150, 190, 235},
    /*accent*/ {0, 125, 55},
    /*on_accent*/ {255, 255, 255},
    /*toggle_on*/ {0, 140, 60},
    /*toggle_off*/ {120, 130, 145},
    /*good*/ {0, 125, 40},
    /*bad*/ {190, 45, 0},
    /*dim*/ {90, 100, 115},
    /*disabled*/ {175, 180, 190},
};

Mode s_mode = Mode::kNight;

uint16_t rgb(const Rgb& c) { return tft.color565(c.r, c.g, c.b); }

}  // namespace

void init() {
  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, true)) {
    s_mode = prefs.getUChar(kKeyMode, 0) == 1 ? Mode::kDay : Mode::kNight;
    prefs.end();
  }
  apply();
}

Mode mode() { return s_mode; }

void setMode(Mode mode) {
  s_mode = mode;
  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, false)) {
    prefs.putUChar(kKeyMode, mode == Mode::kDay ? 1 : 0);
    prefs.end();
  }
  apply();
}

void apply() {
  const Palette& p = s_mode == Mode::kDay ? kDay : kNight;
  radar::kColorBackground = rgb(p.background);
  radar::kColorGrid = rgb(p.grid);
  radar::kColorLabel = rgb(p.label);
  radar::kColorCenter = rgb(p.center);
  // GC9A01 BGR panel: swap R/B in color565 so logical red renders red on screen.
  radar::kColorAircraft = config::kDisplayRgbOrder
                              ? tft.color565(p.aircraft.b, p.aircraft.g, p.aircraft.r)
                              : rgb(p.aircraft);
  radar::kColorTrackVector = rgb(p.track);
  radar::kColorTagType = rgb(p.tag_type);
  radar::kColorTagAltitude = rgb(p.tag_altitude);
  radar::kColorRunway = rgb(p.runway);
  radar::kColorRunwayLabel = rgb(p.runway_label);
  radar::kColorFooterBackground = rgb(p.panel);
  radar::kColorAlertRing = rgb(p.alert_ring);
  radar::kColorAlertDark = rgb(p.alert_dark);
  radar::kColorAlertBright = rgb(p.alert_bright);
  radar::kColorPressed = rgb(p.pressed);
  radar::kColorAccent = rgb(p.accent);
  radar::kColorOnAccent = rgb(p.on_accent);
  radar::kColorToggleOn = rgb(p.toggle_on);
  radar::kColorToggleOff = rgb(p.toggle_off);
  radar::kColorGood = rgb(p.good);
  radar::kColorBad = rgb(p.bad);
  radar::kColorDim = rgb(p.dim);
  radar::kColorDisabled = rgb(p.disabled);
}

}  // namespace ui::theme
