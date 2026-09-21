#pragma once

#include <cstddef>

#include "services/adsb_client.h"

namespace sim {

/** Replace the aircraft list the UI reads (copied, clamped to kMaxAircraft). */
void setAircraft(const services::adsb::Aircraft* list, size_t count);

/** Radar centre (defaults to config::kDefaultRadarLat/Lon). */
void setCenter(double lat, double lon);

/** Fake footer data; the strings use the firmware's formats. */
void setWeatherLine(const char* text);
void setDateTimeLine(const char* text);
void setTextScalePercent(int percent);

}  // namespace sim
