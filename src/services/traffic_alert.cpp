#include "services/traffic_alert.h"

#include <Arduino.h>
#include <Preferences.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "geo.h"

namespace services::alert {
namespace {

constexpr size_t kMaxActive = 8;
constexpr double kPi = 3.14159265358979;

constexpr char kPrefsNamespace[] = "alerts";
constexpr char kKeyEnabled[] = "on";
constexpr char kKeyAltitude[] = "altFt";
constexpr char kKeyPass[] = "passKm";
constexpr char kKeyTime[] = "timeS";

// An active alert only clears once it is clearly outside its thresholds (no flicker).
constexpr float kExitPassFactor = 1.35f;
constexpr float kExitTimeFactor = 1.25f;
constexpr float kExitAltitudeMarginFt = 500.0f;

struct Active {
  char hex[7];
  Info info;
  size_t index;
  bool dismissed;
};

Settings s_settings = {true, kDefaultMaxAltitudeFt, kDefaultMaxPassKm,
                       kDefaultMaxTimeS};
Active s_active[kMaxActive];
size_t s_count = 0;
unsigned long s_updated_ms = 0;

void save() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) {
    return;
  }
  prefs.putBool(kKeyEnabled, s_settings.enabled);
  prefs.putFloat(kKeyAltitude, s_settings.max_altitude_ft);
  prefs.putFloat(kKeyPass, s_settings.max_pass_km);
  prefs.putFloat(kKeyTime, s_settings.max_time_s);
  prefs.end();
}

float clampf(float value, float lo, float hi) {
  return std::max(lo, std::min(hi, value));
}

const Active* findActive(const Active* list, size_t n, const char* hex) {
  for (size_t i = 0; i < n; ++i) {
    if (strcmp(list[i].hex, hex) == 0) {
      return &list[i];
    }
  }
  return nullptr;
}

/** True when the aircraft qualifies; fills info with its projected closest approach. */
bool evaluate(const adsb::Aircraft& plane, double lat0, double lon0,
              bool was_active, Info* info) {
  if (!plane.has_alt || plane.gs_knots < kMinGroundSpeedKnots) {
    return false;
  }

  const geo::KmPerDeg& km = geo::kmPerDegCached(lat0);
  const double px = (plane.lon - lon0) * km.lon;
  const double py = (plane.lat - lat0) * km.lat;

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

  const float max_pass =
      s_settings.max_pass_km * (was_active ? kExitPassFactor : 1.0f);
  const float max_time =
      s_settings.max_time_s * (was_active ? kExitTimeFactor : 1.0f);
  const float max_alt =
      s_settings.max_altitude_ft + (was_active ? kExitAltitudeMarginFt : 0.0f);
  if (pass_km > max_pass || t > max_time || alt > max_alt) {
    return false;
  }

  info->pass_km = static_cast<float>(pass_km);
  info->time_s = static_cast<float>(t);
  info->alt_ft = static_cast<float>(alt);
  return true;
}

}  // namespace

void init() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, true)) {
    return;
  }
  s_settings.enabled = prefs.getBool(kKeyEnabled, true);
  s_settings.max_altitude_ft = clampf(
      prefs.getFloat(kKeyAltitude, kDefaultMaxAltitudeFt), kMinAltitudeFt,
      kMaxAltitudeFt);
  s_settings.max_pass_km =
      clampf(prefs.getFloat(kKeyPass, kDefaultMaxPassKm), kMinPassKm, kMaxPassKm);
  s_settings.max_time_s =
      clampf(prefs.getFloat(kKeyTime, kDefaultMaxTimeS), kMinTimeS, kMaxTimeS);
  prefs.end();
}

const Settings& settings() { return s_settings; }

void setEnabled(bool enabled) {
  s_settings.enabled = enabled;
  save();
}

void setMaxAltitudeFt(float ft) {
  s_settings.max_altitude_ft = clampf(ft, kMinAltitudeFt, kMaxAltitudeFt);
  save();
}

void setMaxPassKm(float km) {
  s_settings.max_pass_km = clampf(km, kMinPassKm, kMaxPassKm);
  save();
}

void setMaxTimeS(float seconds) {
  s_settings.max_time_s = clampf(seconds, kMinTimeS, kMaxTimeS);
  save();
}

void resetSettings() {
  s_settings = {true, kDefaultMaxAltitudeFt, kDefaultMaxPassKm, kDefaultMaxTimeS};
  save();
}

void update(double center_lat, double center_lon) {
  Active before[kMaxActive];
  const size_t before_count = s_count;
  memcpy(before, s_active, sizeof(before));

  s_count = 0;
  s_updated_ms = millis();
  if (!s_settings.enabled) {
    return;
  }

  const size_t n = adsb::aircraftCount();
  const adsb::Aircraft* planes = adsb::aircraftList();
  for (size_t i = 0; i < n && s_count < kMaxActive; ++i) {
    if (planes[i].hex[0] == '\0') {
      continue;
    }
    Info info{};
    const Active* previous = findActive(before, before_count, planes[i].hex);
    if (!evaluate(planes[i], center_lat, center_lon, previous != nullptr, &info)) {
      continue;
    }
    Active& slot = s_active[s_count++];
    snprintf(slot.hex, sizeof(slot.hex), "%s", planes[i].hex);
    slot.info = info;
    slot.index = i;
    // A dismissal lasts until the aircraft leaves the alert zone.
    slot.dismissed = previous != nullptr && previous->dismissed;
  }
}

size_t count() { return s_count; }

size_t pendingCount() {
  size_t pending = 0;
  for (size_t i = 0; i < s_count; ++i) {
    if (!s_active[i].dismissed) {
      ++pending;
    }
  }
  return pending;
}

bool isAlerting(const adsb::Aircraft& plane) {
  return findActive(s_active, s_count, plane.hex) != nullptr;
}

bool isDismissed(const adsb::Aircraft& plane) {
  const Active* active = findActive(s_active, s_count, plane.hex);
  return active != nullptr && active->dismissed;
}

bool mostUrgent(const adsb::Aircraft** plane, Info* info) {
  const Active* best = nullptr;
  for (size_t i = 0; i < s_count; ++i) {
    if (s_active[i].dismissed) {
      continue;
    }
    if (best == nullptr || s_active[i].info.time_s < best->info.time_s) {
      best = &s_active[i];
    }
  }
  if (best == nullptr) {
    return false;
  }
  *plane = &adsb::aircraftList()[best->index];
  *info = best->info;
  return true;
}

void dismissMostUrgent() {
  Active* best = nullptr;
  for (size_t i = 0; i < s_count; ++i) {
    if (s_active[i].dismissed) {
      continue;
    }
    if (best == nullptr || s_active[i].info.time_s < best->info.time_s) {
      best = &s_active[i];
    }
  }
  if (best != nullptr) {
    best->dismissed = true;
  }
}

unsigned long updatedMs() { return s_updated_ms; }

}  // namespace services::alert
