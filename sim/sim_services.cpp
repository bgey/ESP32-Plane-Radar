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
char g_weather[32] = "PARTLY CLOUDY 16C RH62%";
char g_date_time[20] = "20:30 20 SEP";
int g_text_scale = 110;
}  // namespace

namespace sim {
void setCenter(double lat, double lon) { services::location::setPosition(lat, lon); }
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
void clearAircraft() { g_aircraft_count = 0; }
}  // namespace services::adsb

namespace services::settings {
namespace {
bool g_footer = true;
bool g_weather = true;
bool g_fahrenheit = false;
bool g_clock24 = true;
}  // namespace

bool footerEnabled() { return g_footer; }
bool weatherEnabled() { return g_weather; }
bool temperatureFahrenheit() { return g_fahrenheit; }
bool use24HourClock() { return g_clock24; }
int textScalePercent() { return g_text_scale; }

void setFooterEnabled(bool enabled) { g_footer = enabled; }
void setWeatherEnabled(bool enabled) { g_weather = enabled; }
void setTemperatureFahrenheit(bool fahrenheit) { g_fahrenheit = fahrenheit; }
void setUse24HourClock(bool use_24_hour) { g_clock24 = use_24_hour; }
void setTextScalePercent(int percent) {
  g_text_scale = percent < kTextScaleMinPercent
                     ? kTextScaleMinPercent
                     : (percent > kTextScaleMaxPercent ? kTextScaleMaxPercent : percent);
}
}  // namespace services::settings

namespace services::weather {
void formatWeatherLine(char* out, size_t out_len) {
  std::snprintf(out, out_len, "%s", g_weather);
}
void formatDateTimeLine(char* out, size_t out_len) {
  std::snprintf(out, out_len, "%s", g_date_time);
}
}  // namespace services::weather
