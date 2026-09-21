// Host-side stand-ins for the network/NVS services the UI reads from.

#include <cstdio>
#include <cstring>

#include "config.h"
#include "services/adsb_client.h"
#include "services/display_settings.h"
#include "services/radar_location.h"
#include "services/weather_time.h"
#include "sim_api.h"

namespace {
services::adsb::Aircraft g_aircraft[services::adsb::kMaxAircraft];
size_t g_aircraft_count = 0;
}  // namespace

namespace sim {
void setAircraft(const services::adsb::Aircraft* list, size_t count) {
  if (count > services::adsb::kMaxAircraft) {
    count = services::adsb::kMaxAircraft;
  }
  std::memcpy(g_aircraft, list, count * sizeof(services::adsb::Aircraft));
  g_aircraft_count = count;
}
}  // namespace sim

namespace services::adsb {
size_t aircraftCount() { return g_aircraft_count; }
const Aircraft* aircraftList() { return g_aircraft; }
}  // namespace services::adsb

namespace services::location {
double lat() { return config::kDefaultRadarLat; }
double lon() { return config::kDefaultRadarLon; }
}  // namespace services::location

namespace services::settings {
bool footerEnabled() { return true; }
bool weatherEnabled() { return true; }
int textScalePercent() { return kTextScaleDefaultPercent; }
}  // namespace services::settings

namespace services::weather {
void formatWeatherLine(char* out, size_t out_len) {
  std::snprintf(out, out_len, "PARTLY CLOUDY 16C RH62%%");
}
void formatDateTimeLine(char* out, size_t out_len) {
  std::snprintf(out, out_len, "20:30 20 SEP");
}
}  // namespace services::weather
