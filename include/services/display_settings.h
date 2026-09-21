#pragma once

#include <cstddef>

namespace services::settings {

constexpr size_t kOtaPasswordMaxLen = 32;
constexpr int kTextScaleMinPercent = 80;
constexpr int kTextScaleMaxPercent = 130;
constexpr int kTextScaleDefaultPercent = 110;

/** Load persistent display and OTA settings from NVS. */
void init();

bool footerEnabled();
bool weatherEnabled();
bool temperatureFahrenheit();
bool use24HourClock();
int textScalePercent();
const char* otaPassword();

/** Direct setters for the on-device settings screen; each persists immediately. */
void setFooterEnabled(bool enabled);
void setWeatherEnabled(bool enabled);
void setTemperatureFahrenheit(bool fahrenheit);
void setUse24HourClock(bool use_24_hour);
/** Clamped to [kTextScaleMinPercent, kTextScaleMaxPercent]. */
void setTextScalePercent(int percent);

/**
 * Store web-portal values. An empty OTA password keeps the current password so
 * the portal never needs to echo the stored secret into its HTML.
 */
void saveFromPortal(const char* footer_checkbox, const char* weather_checkbox,
                    const char* fahrenheit_checkbox,
                    const char* clock24_checkbox,
                    const char* text_scale_percent_value,
                    const char* ota_password_value);

/** Restore defaults during a full BOOT-button reset. */
void clear();

}  // namespace services::settings
