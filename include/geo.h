#pragma once

#include <cmath>

namespace geo {

struct KmPerDeg {
  /** Kilometres per degree of latitude at this latitude. */
  float lat;
  /** Kilometres per degree of longitude at this latitude (shrinks towards the poles). */
  float lon;
};

/** WGS-84 length of one degree of latitude/longitude at lat_deg. */
inline KmPerDeg kmPerDegAt(double lat_deg) {
  const double phi = lat_deg * 0.017453292519943295;
  KmPerDeg k;
  k.lat = static_cast<float>(111.13292 - 0.55982 * std::cos(2.0 * phi) +
                             0.001175 * std::cos(4.0 * phi));
  k.lon = static_cast<float>(111.41284 * std::cos(phi) -
                             0.0935 * std::cos(3.0 * phi) +
                             0.000118 * std::cos(5.0 * phi));
  return k;
}

/**
 * Same as kmPerDegAt, remembering the last latitude asked for. The radar centre
 * changes only when the location is edited, while conversions run per aircraft
 * and per airport on every frame.
 */
inline const KmPerDeg& kmPerDegCached(double lat_deg) {
  static double last_lat = 1000.0;  // out of range: forces the first computation
  static KmPerDeg cached = {111.0f, 111.0f};
  if (lat_deg != last_lat) {
    cached = kmPerDegAt(lat_deg);
    last_lat = lat_deg;
  }
  return cached;
}

}  // namespace geo
