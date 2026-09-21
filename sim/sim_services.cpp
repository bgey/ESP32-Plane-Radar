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
double g_center_lat = config::kDefaultRadarLat;
double g_center_lon = config::kDefaultRadarLon;
char g_weather[32] = "PARTLY CLOUDY 16C RH62%";
char g_date_time[20] = "20:30 20 SEP";
int g_text_scale = 110;
}  // namespace

namespace sim {
void setCenter(double lat, double lon) {
  g_center_lat = lat;
  g_center_lon = lon;
}
void setWeatherLine(const char* text) { std::snprintf(g_weather, sizeof(g_weather), "%s", text); }
void setDateTimeLine(const char* text) {
  std::snprintf(g_date_time, sizeof(g_date_time), "%s", text);
}
void setTextScalePercent(int percent) { g_text_scale = percent; }

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
double lat() { return g_center_lat; }
double lon() { return g_center_lon; }
}  // namespace services::location

namespace services::settings {
bool footerEnabled() { return true; }
bool weatherEnabled() { return true; }
int textScalePercent() { return g_text_scale; }
}  // namespace services::settings

namespace services::weather {
void formatWeatherLine(char* out, size_t out_len) {
  std::snprintf(out, out_len, "%s", g_weather);
}
void formatDateTimeLine(char* out, size_t out_len) {
  std::snprintf(out, out_len, "%s", g_date_time);
}
}  // namespace services::weather
