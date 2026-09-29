#pragma once

#include <cstdint>

#include <driver/gpio.h>

namespace config {

// --- Wi-Fi portal ---
constexpr char kPortalApName[] = "PlaneRadar-Setup";
constexpr char kPortalIp[] = "192.168.4.1";
/** mDNS host (no ".local" suffix); browser: http://plane-radar.local */
constexpr char kPortalHostname[] = "plane-radar";
constexpr char kPortalHostUrl[] = "plane-radar.local";

/** Per-attempt STA connect wait (ms); retried kWifiConnectAttempts times. */
constexpr unsigned long kWifiConnectAttemptMs = 15000;
constexpr uint8_t kWifiConnectAttempts = 3;
constexpr unsigned long kWifiPortalTimeoutSec = 0;  // 0 = no timeout while configuring
constexpr unsigned long kWifiConnectingFrameMs = 50;
/** Wait after disconnect before reconnecting (avoids portal on brief drops). */
constexpr unsigned long kWifiDownGraceMs = 4000;
/** Minimum interval between background reconnect tries. */
constexpr unsigned long kWifiReconnectIntervalMs = 15000;

#if defined(PLANE_RADAR_TARGET_S3_ST7796)

// --- BOOT button (Wemos S3 Mini onboard button, active LOW) ---
constexpr gpio_num_t kBootPin = GPIO_NUM_0;
constexpr unsigned long kBootResetHoldMs = 3000UL;
/** Ignore BOOT taps shorter than this (debounce). */
constexpr unsigned long kBootTapMinMs = 40UL;

// --- Display: ST7796S 4" 480×320 (SPI) ---
// Placeholder pin plan for the Wemos S3 Mini — confirm against your board's
// silkscreen before wiring; only GPIO0 (onboard BOOT button) is fixed.
// Board header labels (11-pin: CLK/MOS/MIS shared with touch) in comments.
// One GPIO per header row (rows are "a/b" pairs); BLK and CS2 are on the left header.
constexpr gpio_num_t kDisplayPinRst = GPIO_NUM_2;    // RES
constexpr gpio_num_t kDisplayPinCs = GPIO_NUM_10;    // CS1
constexpr gpio_num_t kDisplayPinDc = GPIO_NUM_4;     // DC
constexpr gpio_num_t kDisplayPinMosi = GPIO_NUM_11;  // MOS
constexpr gpio_num_t kDisplayPinSclk = GPIO_NUM_12;  // CLK
constexpr gpio_num_t kDisplayPinMiso = GPIO_NUM_13;  // MIS
constexpr gpio_num_t kDisplayPinBacklight = GPIO_NUM_16;  // BLK

// --- Touch: XPT2046 resistive, shares the display SPI bus (CLK/MOS/MIS) ---
constexpr gpio_num_t kTouchPinCs = GPIO_NUM_18;   // CS2
constexpr gpio_num_t kTouchPinIrq = GPIO_NUM_35;  // PEN
/** false = poll the XPT2046 over SPI and ignore PEN (works even if PEN is not wired). */
constexpr bool kTouchUseIrq = false;

// --- Alert buzzer: passive piezo, one leg to GND ---
constexpr gpio_num_t kBuzzerPin = GPIO_NUM_17;

constexpr int kDisplayWidth = 480;
constexpr int kDisplayHeight = 320;

constexpr uint32_t kDisplaySpiWriteHz = 40000000;
constexpr uint32_t kTouchSpiHz = 1000000;
constexpr bool kDisplayInvert = false;
constexpr bool kDisplayRgbOrder = false;
/** Panel is native 320x480 (portrait); rotation 1 yields 480x320 (landscape). */
constexpr int kDisplayRotation = 1;

#else

// --- BOOT button (ESP32-C3 Super Mini, active LOW) ---
constexpr gpio_num_t kBootPin = GPIO_NUM_9;
constexpr unsigned long kBootResetHoldMs = 3000UL;
/** Ignore BOOT taps shorter than this (debounce). */
constexpr unsigned long kBootTapMinMs = 40UL;

// --- Display: GC9A01 1.28" round 240×240 (SPI) ---
constexpr gpio_num_t kDisplayPinRst = GPIO_NUM_0;
constexpr gpio_num_t kDisplayPinCs = GPIO_NUM_1;
constexpr gpio_num_t kDisplayPinDc = GPIO_NUM_10;
constexpr gpio_num_t kDisplayPinMosi = GPIO_NUM_3;  // display SDA
constexpr gpio_num_t kDisplayPinSclk = GPIO_NUM_4;  // display SCL

constexpr int kDisplayWidth = 240;
constexpr int kDisplayHeight = 240;

constexpr uint32_t kDisplaySpiWriteHz = 40000000;
// GC9A01 modules often need invert + BGR for correct black/green output
constexpr bool kDisplayInvert = true;
constexpr bool kDisplayRgbOrder = true;
constexpr int kDisplayRotation = 0;

#endif  // PLANE_RADAR_TARGET_S3_ST7796

// --- Radar center defaults (overridden via WiFi setup portal) ---
constexpr double kDefaultRadarLat = 52.3676;
constexpr double kDefaultRadarLon = 4.9041;

/** Poll adsb.fi (API public limit: 1 req/s). */
constexpr unsigned long kAdsbFetchIntervalMs = 3000;
/** Legacy scale unused — fetch uses radar::fetchRadiusKm() to screen edge. */
constexpr float kAdsbFetchRadiusScale = 1.0f;
/** false = hide aircraft with alt_baro "ground"; true = show them too. */
constexpr bool kAdsbShowGroundAircraft = false;

// --- Flight enrichment (origin/destination and detailed aircraft type) ---
constexpr char kFlightDataApiBase[] = "https://api.adsbdb.com/v0/";
/** One lookup at a time; successful results remain cached for six hours. */
constexpr unsigned long kFlightLookupMinIntervalMs = 750UL;
constexpr unsigned long kFlightLookupTimeoutMs = 5000UL;
constexpr unsigned long kFlightLookupFailureBackoffMs = 30000UL;
constexpr unsigned long kFlightCacheSuccessMs = 6UL * 60UL * 60UL * 1000UL;
constexpr unsigned long kFlightCacheMissMs = 10UL * 60UL * 1000UL;

// --- Weather and local time ---
constexpr char kWeatherApiBase[] = "https://api.open-meteo.com/v1/forecast";
constexpr unsigned long kWeatherFetchIntervalMs = 15UL * 60UL * 1000UL;
constexpr unsigned long kWeatherRequestTimeoutMs = 6000UL;

// --- User-facing defaults ---
constexpr char kOtaUsername[] = "admin";
/** Change this in the web settings before exposing the device to other users. */
constexpr char kDefaultOtaPassword[] = "plane-radar";

// --- UI colors (RGB565) — status screens ---
constexpr uint16_t kColorBlack = 0x0000;
constexpr uint16_t kColorYellow = 0xFFE0;
constexpr uint16_t kTextOnYellow = kColorBlack;
constexpr uint16_t kTextOnBlack = 0xFFFF;

}  // namespace config
