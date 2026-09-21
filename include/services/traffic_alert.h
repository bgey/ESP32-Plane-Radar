#pragma once

#include <cstddef>

#include "services/adsb_client.h"

namespace services::alert {

/**
 * Low-flyer alert: an aircraft is flagged when it is heading towards the radar
 * position and its projected closest approach is near, soon, and low.
 */
constexpr float kMaxPassKm = 1.5f;
constexpr float kMaxTimeToPassS = 120.0f;
/** Baro altitude (ft), projected to the closest approach using the vertical rate. */
constexpr float kMaxAltitudeFt = 3000.0f;

/** An active alert only clears once it is clearly outside these (no flicker). */
constexpr float kExitPassKm = 2.0f;
constexpr float kExitTimeToPassS = 150.0f;
constexpr float kExitAltitudeFt = 3500.0f;

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

/** Re-evaluate every aircraft in the current list against the radar position. */
void update(double center_lat, double center_lon);

size_t count();
bool isAlerting(const adsb::Aircraft& plane);

/** The alerting aircraft with the soonest closest approach; false when none. */
bool mostUrgent(const adsb::Aircraft** plane, Info* info);

/** millis() at the last update(); lets the UI count the remaining time down. */
unsigned long updatedMs();

}  // namespace services::alert
