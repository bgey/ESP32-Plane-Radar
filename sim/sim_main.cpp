// Desktop simulator: draws the radar UI with fake aircraft through LovyanGFX's
// SDL panel. Headless by default (writes a PNG); --window keeps it open.

#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>

#if defined(SDL_h_)

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <thread>

#include "config.h"
#include "geo.h"
#include "hardware/display.h"
#include "services/radar_location.h"
#include "sim_api.h"
#include "ui/radar_display.h"
#include "services/traffic_alert.h"
#include "ui/alert_banner.h"
#include "ui/radar_range.h"
#include "ui/touch_controls.h"

namespace {

struct Options {
  const char* out = "sim/out/radar.png";
  int range = 0;
  int planes = 12;
  unsigned seed = 1;
  int rotation = -1;  // -1 = use config::kDisplayRotation
  bool inbound = false;
  int inbound_alt_ft = 2600;
  bool window = false;
};

Options g_opts;
services::adsb::Aircraft g_planes[services::adsb::kMaxAircraft];
size_t g_plane_count = 0;

constexpr float kPi = 3.14159265f;

void makeAircraft(float outer_km) {
  static const char* kCallsigns[] = {"KLM1234", "EZY97CB", "RYR4XY", "DLH7GA",
                                     "BAW431", "TRA6501", "VLG9GD", "AFR1240"};
  static const char* kRoutes[] = {"AMS-LHR", "AMS-BCN", "EIN-STN", "AMS-FRA",
                                  "", "RTM-MAD", "AGP-BRU", "AMS-CDG"};
  static const char* kTypes[] = {"A320", "B737-800", "E190", "A321", "B77W",
                                 "A20N", "B738", "DH8D"};
  std::mt19937 rng(g_opts.seed);
  std::uniform_real_distribution<float> uni(0.0f, 1.0f);

  const double lat0 = services::location::lat();
  const double lon0 = services::location::lon();
  const geo::KmPerDeg deg = geo::kmPerDegAt(lat0);

  g_plane_count = static_cast<size_t>(g_opts.planes);
  if (g_plane_count > services::adsb::kMaxAircraft) {
    g_plane_count = services::adsb::kMaxAircraft;
  }
  for (size_t i = 0; i < g_plane_count; ++i) {
    services::adsb::Aircraft& ac = g_planes[i];
    std::memset(&ac, 0, sizeof(ac));
    const float r = outer_km * 1.25f * std::sqrt(uni(rng));
    const float a = uni(rng) * 2.0f * kPi;
    ac.lat = static_cast<float>(lat0 + (r * std::cos(a)) / deg.lat);
    ac.lon = static_cast<float>(lon0 + (r * std::sin(a)) / deg.lon);
    ac.track_deg = uni(rng) * 360.0f;
    ac.nose_deg = ac.track_deg;
    ac.gs_knots = 180.0f + uni(rng) * 320.0f;
    std::snprintf(ac.hex, sizeof(ac.hex), "%06x", static_cast<unsigned>(rng() & 0xFFFFFF));
    const size_t k = i % 8;
    std::snprintf(ac.callsign, sizeof(ac.callsign), "%s", kCallsigns[k]);
    std::snprintf(ac.route, sizeof(ac.route), "%s", kRoutes[k]);
    std::snprintf(ac.type, sizeof(ac.type), "%s", kTypes[k]);
    const int alt_ft = 3000 + static_cast<int>(uni(rng) * 36000) % 40000;
    std::snprintf(ac.alt, sizeof(ac.alt), "%d ft", alt_ft);
    ac.has_alt = true;
    ac.alt_ft = static_cast<float>(alt_ft);
    ac.has_vrate = true;
    ac.vrate_fpm = 0.0f;
  }

  if (g_opts.inbound && g_plane_count < services::adsb::kMaxAircraft) {
    // Low aircraft 4.5 km out (bearing 250) flying straight at the radar position.
    services::adsb::Aircraft& ac = g_planes[g_plane_count++];
    std::memset(&ac, 0, sizeof(ac));
    const float bearing = 250.0f * kPi / 180.0f;
    ac.lat = static_cast<float>(lat0 + (4.5f * std::cos(bearing)) / deg.lat);
    ac.lon = static_cast<float>(lon0 + (4.5f * std::sin(bearing)) / deg.lon);
    ac.track_deg = 70.0f;
    ac.nose_deg = 70.0f;
    ac.gs_knots = 180.0f;
    std::snprintf(ac.hex, sizeof(ac.hex), "%s", "48LOW1");
    std::snprintf(ac.callsign, sizeof(ac.callsign), "%s", "TRA6LOW");
    std::snprintf(ac.type, sizeof(ac.type), "%s", "B738");
    std::snprintf(ac.alt, sizeof(ac.alt), "%d ft", g_opts.inbound_alt_ft);
    ac.has_alt = true;
    ac.alt_ft = static_cast<float>(g_opts.inbound_alt_ft);
    ac.has_vrate = true;
    ac.vrate_fpm = -500.0f;
  }
  sim::setAircraft(g_planes, g_plane_count);
  services::alert::update(lat0, lon0);
  std::printf("planes=%zu alerts=%zu\n", g_plane_count, services::alert::count());
}

void advanceAircraft(float dt_s) {
  const double lat0 = services::location::lat();
  const geo::KmPerDeg deg = geo::kmPerDegAt(lat0);
  for (size_t i = 0; i < g_plane_count; ++i) {
    services::adsb::Aircraft& ac = g_planes[i];
    const float km = ac.gs_knots * 1.852f / 3600.0f * dt_s * 20.0f;  // 20x speed-up
    const float t = ac.track_deg * kPi / 180.0f;
    ac.lat += static_cast<float>(km * std::cos(t) / deg.lat);
    ac.lon += static_cast<float>(km * std::sin(t) / deg.lon);
    if (ac.has_vrate) {
      ac.alt_ft += ac.vrate_fpm * dt_s * 20.0f / 60.0f;
      std::snprintf(ac.alt, sizeof(ac.alt), "%d ft", static_cast<int>(ac.alt_ft));
    }
  }
  sim::setAircraft(g_planes, g_plane_count);
  services::alert::update(lat0, services::location::lon());
}

bool savePng(const char* path) {
  size_t len = 0;
  void* png = tft.createPng(&len, 0, 0, tft.width(), tft.height());
  if (png == nullptr || len == 0) {
    std::fprintf(stderr, "createPng failed (display %dx%d)\n", tft.width(), tft.height());
    return false;
  }
  std::FILE* f = std::fopen(path, "wb");
  if (f == nullptr) {
    std::fprintf(stderr, "cannot write %s\n", path);
    return false;
  }
  std::fwrite(png, 1, len, f);
  std::fclose(f);
  std::printf("wrote %s (%dx%d, %d planes, range index %d)\n", path, tft.width(),
              tft.height(), g_opts.planes, g_opts.range);
  return true;
}

struct Tap {
  int x;
  int y;
};
constexpr size_t kMaxTaps = 16;
Tap g_taps[kMaxTaps];
size_t g_tap_count = 0;

/** Poll the touch controls for `ms` milliseconds, redrawing when the range changes. */
void pumpTouch(int ms) {
  for (int t = 0; t < ms; t += 10) {
    if (ui::touchControlsPoll()) {
      char label[12];
      ui::radar::formatCurrentRing3Label(label, sizeof(label));
      std::printf("range -> %s\n", label);
      ui::radarDisplayDraw();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

/** Synthesise a left click at window pixel (x, y), as the SDL panel reads the mouse. */
void injectTap(int x, int y) {
  SDL_Window* window = SDL_GetWindowFromID(1);
  if (window == nullptr) {
    return;
  }
  SDL_WarpMouseInWindow(window, x, y);
  pumpTouch(200);
  SDL_Event ev{};
  ev.type = SDL_MOUSEBUTTONDOWN;
  ev.button.windowID = 1;
  ev.button.button = SDL_BUTTON_LEFT;
  ev.button.state = SDL_PRESSED;
  SDL_PushEvent(&ev);
  pumpTouch(300);
  ev.type = SDL_MOUSEBUTTONUP;
  ev.button.state = SDL_RELEASED;
  SDL_PushEvent(&ev);
  pumpTouch(300);
}

int userFunc(bool* running) {
  // The SDL window is sized once at init, so size it for the final orientation
  // and keep the panel itself at rotation 0 (the SDL panel does not flip 180 degrees).
  const int rotation = g_opts.rotation >= 0 ? g_opts.rotation : config::kDisplayRotation;
  const bool landscape = (rotation & 1) != 0;
  tft.simConfigure(landscape ? 480 : 320, landscape ? 320 : 480);
  displayInit();
  tft.setRotation(0);
  ui::radar::rangeInit();
  for (int i = 0; i < g_opts.range; ++i) {
    ui::radar::rangeNext();
  }
  makeAircraft(ui::radar::rangeCurrent().outer_km);
  ui::radarDisplayDraw();
  ui::radarDisplayRefreshAircraft();
  for (size_t i = 0; i < g_tap_count; ++i) {
    injectTap(g_taps[i].x, g_taps[i].y);
  }
  const bool saved = savePng(g_opts.out);

  if (!g_opts.window) {
    // Panel_sdl::main() would otherwise wait for a window-close event forever.
    std::fflush(stdout);
    std::_Exit(saved ? 0 : 1);
  }

  if (SDL_Window* w = SDL_GetWindowFromID(1)) {
    int ww = 0;
    int wh = 0;
    SDL_GetWindowSize(w, &ww, &wh);
    std::printf("window %dx%d, display %dx%d\n", ww, wh, tft.width(), tft.height());
  }

  unsigned long last_move_ms = millis();
  size_t last_alerts = services::alert::count();
  while (*running) {
    pumpTouch(20);
    ui::alertBannerTick();
    if (millis() - last_move_ms >= 500) {
      last_move_ms = millis();
      advanceAircraft(0.5f);
      ui::radarDisplayRefreshAircraft();
      if (services::alert::count() != last_alerts) {
        last_alerts = services::alert::count();
        std::printf("alerts -> %zu\n", last_alerts);
      }
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  double center_lat = config::kDefaultRadarLat;
  double center_lon = config::kDefaultRadarLon;
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    if (std::strcmp(a, "--window") == 0) {
      g_opts.window = true;
    } else if (std::strcmp(a, "--lat") == 0 && i + 1 < argc) {
      center_lat = std::atof(argv[++i]);
    } else if (std::strcmp(a, "--lon") == 0 && i + 1 < argc) {
      center_lon = std::atof(argv[++i]);
    } else if (std::strcmp(a, "--inbound") == 0) {
      g_opts.inbound = true;
    } else if (std::strcmp(a, "--inbound-alt") == 0 && i + 1 < argc) {
      g_opts.inbound_alt_ft = std::atoi(argv[++i]);
    } else if (std::strcmp(a, "--out") == 0 && i + 1 < argc) {
      g_opts.out = argv[++i];
    } else if (std::strcmp(a, "--range") == 0 && i + 1 < argc) {
      g_opts.range = std::atoi(argv[++i]);
    } else if (std::strcmp(a, "--planes") == 0 && i + 1 < argc) {
      g_opts.planes = std::atoi(argv[++i]);
    } else if (std::strcmp(a, "--seed") == 0 && i + 1 < argc) {
      g_opts.seed = static_cast<unsigned>(std::atoi(argv[++i]));
    } else if (std::strcmp(a, "--tap") == 0 && i + 1 < argc) {
      int tx = 0;
      int ty = 0;
      if (std::sscanf(argv[++i], "%d,%d", &tx, &ty) != 2 || g_tap_count >= kMaxTaps) {
        std::fprintf(stderr, "bad --tap (use X,Y; max %zu)\n", kMaxTaps);
        return 2;
      }
      g_taps[g_tap_count++] = {tx, ty};
    } else if (std::strcmp(a, "--weather") == 0 && i + 1 < argc) {
      sim::setWeatherLine(argv[++i]);
    } else if (std::strcmp(a, "--time") == 0 && i + 1 < argc) {
      sim::setDateTimeLine(argv[++i]);
    } else if (std::strcmp(a, "--textscale") == 0 && i + 1 < argc) {
      sim::setTextScalePercent(std::atoi(argv[++i]));
    } else if (std::strcmp(a, "--rotation") == 0 && i + 1 < argc) {
      g_opts.rotation = std::atoi(argv[++i]) & 1;
    } else {
      std::fprintf(stderr,
                   "usage: %s [--out file.png] [--range 0-3] [--planes N] "
                   "[--seed N] [--rotation 0|1] [--weather TEXT] [--time TEXT] "
                   "[--textscale 80-130] [--tap X,Y]... [--inbound] [--inbound-alt FT] [--lat DEG] [--lon DEG] [--window]\n",
                   argv[0]);
      return 2;
    }
  }
  sim::setCenter(center_lat, center_lon);
  if (!g_opts.window) {
    // The dummy video driver cannot create an accelerated renderer, which makes the
    // panel open a new window on every update; the software renderer avoids that.
    setenv("SDL_VIDEODRIVER", "dummy", 0);
    setenv("SDL_RENDER_DRIVER", "software", 0);
  }
  return lgfx::Panel_sdl::main(userFunc);
}

#endif  // SDL_h_
