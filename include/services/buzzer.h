#pragma once

namespace services::buzzer {

/** Configure the piezo pin. No-op on targets with no buzzer fitted (the ESP32-C3). */
void init();

/**
 * Call every loop while the radar is showing. Drives a repeating beep pattern
 * while a pending, un-dismissed low-flyer alert exists and the alert sound
 * setting is on; silent otherwise.
 */
void tick();

}  // namespace services::buzzer
