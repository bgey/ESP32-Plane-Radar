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

/**
 * Colour roles (RGB565). The values are set by ui::theme::apply() for the current
 * Day/Night theme; always draw with these instead of literal colours.
 */
// Radar and text
extern uint16_t kColorBackground;      // screen background
extern uint16_t kColorGrid;            // rings, crosshairs, outlines, separators
extern uint16_t kColorLabel;           // primary text and glyphs
extern uint16_t kColorCenter;
extern uint16_t kColorAircraft;
extern uint16_t kColorTrackVector;
extern uint16_t kColorTagType;         // aircraft type text; also secondary text
extern uint16_t kColorTagAltitude;     // altitude, times and values
extern uint16_t kColorRunway;
extern uint16_t kColorRunwayLabel;
extern uint16_t kColorFooterBackground;  // info panel, buttons and header bars
// Low-flyer alert (the banner flashes between dark and bright red)
extern uint16_t kColorAlertRing;
extern uint16_t kColorAlertDark;
extern uint16_t kColorAlertBright;
// Controls
extern uint16_t kColorPressed;    // fill of a pressed or selected button/tab (text: kColorLabel)
extern uint16_t kColorAccent;     // primary action fill, e.g. DONE (text: kColorOnAccent)
extern uint16_t kColorOnAccent;   // text and knobs on accent, toggle and alert fills
extern uint16_t kColorToggleOn;
extern uint16_t kColorToggleOff;
extern uint16_t kColorDisabled;   // glyph of a button that cannot be used right now
// Status text
extern uint16_t kColorGood;
extern uint16_t kColorBad;
extern uint16_t kColorDim;        // hints and secondary text

}  // namespace ui::radar
