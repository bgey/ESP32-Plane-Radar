#pragma once

/**
 * Touch calibration for the XPT2046 (S3 target only; no-ops elsewhere).
 * The four-corner result is stored in flash and applied at every boot.
 */

/**
 * Apply the saved calibration. When none is saved, offer the calibration screen
 * for a few seconds (skipped if nobody touches, so a board without a working
 * touch panel still boots).
 */
void touchCalibrationInit();

/** Show the four-crosshair calibration screen and save the result. Blocks until done. */
void touchCalibrationRun();
