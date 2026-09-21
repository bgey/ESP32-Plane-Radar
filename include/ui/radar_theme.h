#pragma once

#include <cstdint>

#include "config.h"

namespace ui::radar {

#if defined(PLANE_RADAR_TARGET_S3_ST7796)
/** Radar square (px), centred on the panel; weather/clock use the side bands. */
constexpr int kSize = 320;
constexpr bool kSideBands = true;
#else
constexpr int kSize = 240;
constexpr bool kSideBands = false;
#endif
constexpr int kCenterX = kSize / 2;
constexpr int kCenterY = kSize / 2;

/** Top-left of the radar square on the panel (left edge, full height). */
constexpr int kRadarOriginX = 0;
constexpr int kRadarOriginY = (config::kDisplayHeight - kSize) / 2;

/** Outermost grid ring (inside edge labels). */
#if defined(PLANE_RADAR_TARGET_S3_ST7796)
constexpr int kGridOuterRadius = 143;
#else
constexpr int kGridOuterRadius = 107;
#endif

/** N: offset from top edge (top_center, negative = up). */
constexpr int kCardinalNorthOffsetY = -1;
/** S: offset from bottom edge (bottom_center, positive = down). */
constexpr int kCardinalSouthOffsetY = 3;

/** Gap between scale label right edge and outer ring on the east spoke (px). */
constexpr int kScaleGapFromOuterRing = 6;

/** Target cap height (px) for N/S/E/W. */
#if defined(PLANE_RADAR_TARGET_S3_ST7796)
constexpr int kCardinalLabelHeightPx = 16;
#else
constexpr int kCardinalLabelHeightPx = 14;
#endif
/** Scale label is this many px shorter than cardinals. */
constexpr int kScaleBelowCardinalPx = 3;

constexpr int kRingCount = 4;

/** Shared grid stroke: drawWideLine half-width (~2 px total); rings use the same px count. */
constexpr float kGridStrokeHalfWidth = 1.0f;

constexpr int kCenterDotRadius = 2;

/** Filled aircraft symbol (nose triangle). */
#if defined(PLANE_RADAR_TARGET_S3_ST7796)
constexpr int kAircraftNoseLenPx = 10;
constexpr int kAircraftTailLenPx = 4;
constexpr int kAircraftTailHalfPx = 5;
#else
constexpr int kAircraftNoseLenPx = 8;
constexpr int kAircraftTailLenPx = 3;
constexpr int kAircraftTailHalfPx = 4;
#endif
/** Track vector: ground distance covered in this many seconds at current gs. */
constexpr float kAircraftTrackHorizonSec = 60.0f;
/** Minimum visible vector when gs > 0 (px). */
constexpr int kAircraftSpeedLineMinPx = 2;
/** Track line length uses this outer_km, not the active range preset. */
constexpr float kAircraftTrackRefOuterKm = 13.3f;
/** Shorter than full 60 s horizon at ref scale; ×1.5 length boost applied. */
constexpr float kAircraftTrackLengthScale = 1.5f / 5.0f;
/** drawWideLine half-width for speed vectors (~2 px total). */
constexpr float kAircraftTrackLineHalfWidth = 1.0f;

constexpr float kRunwayLineWidthPx = 2.0f;
constexpr float kRunwayLineHalfWidth = kRunwayLineWidthPx * 0.5f;
constexpr int kRunwayLabelHeightPx = kCardinalLabelHeightPx;
constexpr int kRunwayLabelGapPx = 3;
/** Gap from triangle edge to tag block (px). */
constexpr int kAircraftLabelGapPx = 1;
/** Keep symbol centroid inside outer ring by at least this inset (px). */
constexpr int kAircraftInsideRingInsetPx =
    kAircraftNoseLenPx + kAircraftTailHalfPx + 1;

/** Ring drawn around an alerting aircraft (px radius, 3 px stroke). */
#if defined(PLANE_RADAR_TARGET_S3_ST7796)
constexpr int kAlertRingRadiusPx = 17;
#else
constexpr int kAlertRingRadiusPx = 13;
#endif

/** Beyond-ring traffic: bearing cues on screen rim (correct direction, fixed radius). */
#if defined(PLANE_RADAR_TARGET_S3_ST7796)
constexpr int kBeyondRingDotRadiusPx = 5;
#else
constexpr int kBeyondRingDotRadiusPx = 4;
#endif
constexpr int kBeyondRingScreenMarginPx = 2;
/** Target cap height (px) for aircraft tags (bold, slightly above scale label). */
#if defined(PLANE_RADAR_TARGET_S3_ST7796)
constexpr int kAircraftTagLabelHeightPx = 15;
#else
constexpr int kAircraftTagLabelHeightPx = 13;
#endif

/** Two-row weather/time overlay inside the round panel (kSideBands == false). */
constexpr int kFooterTopY = 194;
constexpr int kFooterBottomY = 233;
constexpr int kFooterWeatherY = 197;
constexpr int kFooterTimeY = 216;
constexpr int kFooterTimeOnlyY = 205;
constexpr int kFooterLabelHeightPx = 13;

/**
 * Info panel right of the radar (kSideBands == true): clock and weather stacked
 * at the top, the rest left free for touch controls.
 */
constexpr int kBandWidthPx = config::kDisplayWidth - kRadarOriginX - kSize;
constexpr int kBandPadPx = 8;
constexpr int kBandTopPx = 10;
constexpr int kBandTimeHeightPx = 18;
constexpr int kBandLineHeightPx = 12;
constexpr int kBandLineGapPx = 3;
constexpr int kBandSectionGapPx = 10;

/** RGB565 palette targets (applied in initPalette). */
constexpr uint8_t kBgR = 4;
constexpr uint8_t kBgG = 10;
constexpr uint8_t kBgB = 28;
constexpr uint8_t kGridR = 16;
constexpr uint8_t kGridG = 100;
constexpr uint8_t kGridB = 32;
constexpr uint8_t kAircraftR = 255;
constexpr uint8_t kAircraftG = 0;
constexpr uint8_t kAircraftB = 0;
constexpr uint8_t kTrackR = 255;
constexpr uint8_t kTrackG = 0;
constexpr uint8_t kTrackB = 255;
constexpr uint8_t kTagTypeR = 255;
constexpr uint8_t kTagTypeG = 200;
constexpr uint8_t kTagTypeB = 0;
constexpr uint8_t kTagAltR = 90;
constexpr uint8_t kTagAltG = 200;
constexpr uint8_t kTagAltB = 255;
constexpr uint8_t kRunwayR = 56;
constexpr uint8_t kRunwayG = 150;
constexpr uint8_t kRunwayB = 170;
/** Lighter teal for ICAO labels (vs runway lines). */
constexpr uint8_t kRunwayLabelR = 110;
constexpr uint8_t kRunwayLabelG = 210;
constexpr uint8_t kRunwayLabelB = 230;
constexpr uint8_t kFooterBgR = 3;
constexpr uint8_t kFooterBgG = 16;
constexpr uint8_t kFooterBgB = 32;
/** Alert ring around a low, inbound aircraft (yellow: aircraft symbols are red). */
constexpr uint8_t kAlertRingR = 255;
constexpr uint8_t kAlertRingG = 220;
constexpr uint8_t kAlertRingB = 0;
/** Alert banner flashes between these two reds. */
constexpr uint8_t kAlertDarkR = 90;
constexpr uint8_t kAlertDarkG = 0;
constexpr uint8_t kAlertDarkB = 0;
constexpr uint8_t kAlertBrightR = 210;
constexpr uint8_t kAlertBrightG = 0;
constexpr uint8_t kAlertBrightB = 0;

extern uint16_t kColorBackground;
extern uint16_t kColorGrid;
extern uint16_t kColorLabel;
extern uint16_t kColorCenter;
extern uint16_t kColorAircraft;
extern uint16_t kColorTrackVector;
extern uint16_t kColorTagType;
extern uint16_t kColorTagAltitude;
extern uint16_t kColorRunway;
extern uint16_t kColorRunwayLabel;
extern uint16_t kColorFooterBackground;
extern uint16_t kColorAlertRing;
extern uint16_t kColorAlertDark;
extern uint16_t kColorAlertBright;

}  // namespace ui::radar
