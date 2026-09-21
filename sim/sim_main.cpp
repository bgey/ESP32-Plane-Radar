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
#include "hardware/display.h"
#include "services/radar_location.h"
#include "sim_api.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"

namespace {

struct Options {
  const char* out = "sim/out/radar.png";
  int range = 1;
  int planes = 12;
  unsigned seed = 1;
  bool window = false;
};

Options g_opts;
services::adsb::Aircraft g_planes[services::adsb::kMaxAircraft];
size_t g_plane_count = 0;

constexpr double kKmPerDegLat = 111.32;
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
  const double km_per_deg_lon = kKmPerDegLat * std::cos(lat0 * kPi / 180.0);

  g_plane_count = static_cast<size_t>(g_opts.planes);
  if (g_plane_count > services::adsb::kMaxAircraft) {
    g_plane_count = services::adsb::kMaxAircraft;
  }
  for (size_t i = 0; i < g_plane_count; ++i) {
    services::adsb::Aircraft& ac = g_planes[i];
    std::memset(&ac, 0, sizeof(ac));
    const float r = outer_km * 1.25f * std::sqrt(uni(rng));
    const float a = uni(rng) * 2.0f * kPi;
    ac.lat = static_cast<float>(lat0 + (r * std::cos(a)) / kKmPerDegLat);
    ac.lon = static_cast<float>(lon0 + (r * std::sin(a)) / km_per_deg_lon);
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
  }
  sim::setAircraft(g_planes, g_plane_count);
}

void advanceAircraft(float dt_s) {
  const double lat0 = services::location::lat();
  const double km_per_deg_lon = kKmPerDegLat * std::cos(lat0 * kPi / 180.0);
  for (size_t i = 0; i < g_plane_count; ++i) {
    services::adsb::Aircraft& ac = g_planes[i];
    const float km = ac.gs_knots * 1.852f / 3600.0f * dt_s * 20.0f;  // 20x speed-up
    const float t = ac.track_deg * kPi / 180.0f;
    ac.lat += static_cast<float>(km * std::cos(t) / kKmPerDegLat);
    ac.lon += static_cast<float>(km * std::sin(t) / km_per_deg_lon);
  }
  sim::setAircraft(g_planes, g_plane_count);
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

int userFunc(bool* running) {
  displayInit();
  ui::radar::rangeInit();
  for (int i = 0; i < g_opts.range; ++i) {
    ui::radar::rangeNext();
  }
  makeAircraft(ui::radar::rangeCurrent().outer_km);
  ui::radarDisplayDraw();
  ui::radarDisplayRefreshAircraft();
  savePng(g_opts.out);

  while (g_opts.window && *running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    advanceAircraft(0.5f);
    ui::radarDisplayRefreshAircraft();
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    if (std::strcmp(a, "--window") == 0) {
      g_opts.window = true;
    } else if (std::strcmp(a, "--out") == 0 && i + 1 < argc) {
      g_opts.out = argv[++i];
    } else if (std::strcmp(a, "--range") == 0 && i + 1 < argc) {
      g_opts.range = std::atoi(argv[++i]);
    } else if (std::strcmp(a, "--planes") == 0 && i + 1 < argc) {
      g_opts.planes = std::atoi(argv[++i]);
    } else if (std::strcmp(a, "--seed") == 0 && i + 1 < argc) {
      g_opts.seed = static_cast<unsigned>(std::atoi(argv[++i]));
    } else {
      std::fprintf(stderr,
                   "usage: %s [--out file.png] [--range 0-3] [--planes N] "
                   "[--seed N] [--window]\n",
                   argv[0]);
      return 2;
    }
  }
  return lgfx::Panel_sdl::main(userFunc);
}

#endif  // SDL_h_
