#pragma once

#include <cstddef>

#include "services/adsb_client.h"

namespace services::alert {

/**
 * Low-flyer alert: an aircraft is flagged when it is heading towards the radar
 * position and its projected closest approach is near, soon, and low. The
 * thresholds are user settings (stored in flash).
 */
struct Settings {
  bool enabled;
  /** Baro altitude (ft), projected to the closest approach using the vertical rate. */
  float max_altitude_ft;
  /** Horizontal distance at the closest approach (km). */
  float max_pass_km;
  /** Seconds until the closest approach. */
  float max_time_s;
  /**
   * Skip aircraft that identify as gliders (they circle low and slow, which gives
   * unreliable tracks and many false alerts). They are still shown on the radar.
   */
  bool ignore_gliders;
};

constexpr bool kDefaultIgnoreGliders = true;
constexpr float kDefaultMaxAltitudeFt = 3000.0f;
constexpr float kDefaultMaxPassKm = 1.5f;
constexpr float kDefaultMaxTimeS = 120.0f;

constexpr float kMinAltitudeFt = 250.0f;
constexpr float kMaxAltitudeFt = 10000.0f;
constexpr float kAltitudeStepFt = 250.0f;
constexpr float kMinPassKm = 0.5f;
constexpr float kMaxPassKm = 5.0f;
constexpr float kPassStepKm = 0.5f;
constexpr float kMinTimeS = 30.0f;
constexpr float kMaxTimeS = 300.0f;
constexpr float kTimeStepS = 30.0f;

/** Ignore aircraft slower than this; their track is not meaningful. */
constexpr float kMinGroundSpeedKnots = 30.0f;

struct Info {
  /** Horizontal distance at the closest approach (km). */
  float pass_km;
  /** Seconds from the last update() until that approach. */
  float time_s;
  /** Projected altitude at the closest approach (ft). */
  float alt_ft;
};

/** Load the saved thresholds (defaults when none). Call once at boot. */
void init();
const Settings& settings();
void setEnabled(bool enabled);
void setIgnoreGliders(bool ignore);
void setMaxAltitudeFt(float ft);
void setMaxPassKm(float km);
void setMaxTimeS(float seconds);
void resetSettings();

/** Re-evaluate every aircraft in the current list against the radar position. */
void update(double center_lat, double center_lon);

/** Number of aircraft currently alerting (dismissed ones included). */
size_t count();
/** Alerting aircraft that have not been dismissed. */
size_t pendingCount();
bool isAlerting(const adsb::Aircraft& plane);
bool isDismissed(const adsb::Aircraft& plane);

/** The pending alert with the soonest closest approach; false when none. */
bool mostUrgent(const adsb::Aircraft** plane, Info* info);

/** Silence the most urgent alert until that aircraft has left the alert zone. */
void dismissMostUrgent();

/** millis() at the last update(); lets the UI count the remaining time down. */
unsigned long updatedMs();

}  // namespace services::alert
