#include "services/traffic_alert.h"

#include <Arduino.h>

#include <cmath>
#include <cstdio>
#include <cstring>

namespace services::alert {
namespace {

constexpr size_t kMaxActive = 8;
constexpr double kKmPerDegLat = 111.32;
constexpr double kPi = 3.14159265358979;

struct Active {
  char hex[7];
  Info info;
  size_t index;
};

Active s_active[kMaxActive];
size_t s_count = 0;
unsigned long s_updated_ms = 0;

bool wasActive(const Active* previous, size_t previous_count, const char* hex) {
  for (size_t i = 0; i < previous_count; ++i) {
    if (strcmp(previous[i].hex, hex) == 0) {
      return true;
    }
  }
  return false;
}

/** True when the aircraft qualifies; fills info with its projected closest approach. */
bool evaluate(const adsb::Aircraft& plane, double lat0, double lon0,
              bool was_active, Info* info) {
  if (!plane.has_alt || plane.gs_knots < kMinGroundSpeedKnots) {
    return false;
  }

  const double km_per_deg_lon = kKmPerDegLat * std::cos(lat0 * kPi / 180.0);
  const double px = (plane.lon - lon0) * km_per_deg_lon;
  const double py = (plane.lat - lat0) * kKmPerDegLat;

  const double speed_km_s = plane.gs_knots * 1.852 / 3600.0;
  const double track = plane.track_deg * kPi / 180.0;
  const double vx = speed_km_s * std::sin(track);
  const double vy = speed_km_s * std::cos(track);
  const double v2 = vx * vx + vy * vy;

  const double along = px * vx + py * vy;  // < 0 while approaching
  const bool approaching = along < 0.0;
  if (!approaching && !was_active) {
    return false;
  }

  const double t = approaching ? -along / v2 : 0.0;
  const double cx = px + vx * t;
  const double cy = py + vy * t;
  const double pass_km = std::sqrt(cx * cx + cy * cy);

  double alt = plane.alt_ft;
  if (plane.has_vrate) {
    alt += plane.vrate_fpm * (t / 60.0);
  }
  if (alt < 0.0) {
    alt = 0.0;
  }

  const float max_pass = was_active ? kExitPassKm : kMaxPassKm;
  const float max_time = was_active ? kExitTimeToPassS : kMaxTimeToPassS;
  const float max_alt = was_active ? kExitAltitudeFt : kMaxAltitudeFt;
  if (pass_km > max_pass || t > max_time || alt > max_alt) {
    return false;
  }

  info->pass_km = static_cast<float>(pass_km);
  info->time_s = static_cast<float>(t);
  info->alt_ft = static_cast<float>(alt);
  return true;
}

}  // namespace

void update(double center_lat, double center_lon) {
  Active before[kMaxActive];
  const size_t before_count = s_count;
  memcpy(before, s_active, sizeof(before));

  s_count = 0;
  s_updated_ms = millis();

  const size_t n = adsb::aircraftCount();
  const adsb::Aircraft* planes = adsb::aircraftList();
  for (size_t i = 0; i < n && s_count < kMaxActive; ++i) {
    if (planes[i].hex[0] == '\0') {
      continue;
    }
    Info info{};
    const bool was = wasActive(before, before_count, planes[i].hex);
    if (!evaluate(planes[i], center_lat, center_lon, was, &info)) {
      continue;
    }
    Active& slot = s_active[s_count++];
    snprintf(slot.hex, sizeof(slot.hex), "%s", planes[i].hex);
    slot.info = info;
    slot.index = i;
  }
}

size_t count() { return s_count; }

bool isAlerting(const adsb::Aircraft& plane) {
  for (size_t i = 0; i < s_count; ++i) {
    if (strcmp(s_active[i].hex, plane.hex) == 0) {
      return true;
    }
  }
  return false;
}

bool mostUrgent(const adsb::Aircraft** plane, Info* info) {
  if (s_count == 0) {
    return false;
  }
  size_t best = 0;
  for (size_t i = 1; i < s_count; ++i) {
    if (s_active[i].info.time_s < s_active[best].info.time_s) {
      best = i;
    }
  }
  *plane = &adsb::aircraftList()[s_active[best].index];
  *info = s_active[best].info;
  return true;
}

unsigned long updatedMs() { return s_updated_ms; }

}  // namespace services::alert
