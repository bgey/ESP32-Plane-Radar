#include "services/buzzer.h"

#include "config.h"
#include "services/traffic_alert.h"

#if defined(PLANE_RADAR_TARGET_S3_ST7796)

#include <Arduino.h>

namespace services::buzzer {
namespace {

constexpr unsigned kToneHz = 4000;  // resonant frequency of the piezo capsule
constexpr unsigned long kBeepOnMs = 110;
constexpr unsigned long kBeepGapMs = 90;
constexpr unsigned kBeepsPerBurst = 2;
constexpr unsigned long kPauseMs = 650;

enum class Phase { kOn, kGap, kPause };
Phase s_phase = Phase::kPause;
unsigned long s_phase_start_ms = 0;
unsigned s_beep_index = 0;
bool s_sounding = false;

void startBeep(unsigned long now) {
  tone(config::kBuzzerPin, kToneHz, kBeepOnMs);
  s_phase = Phase::kOn;
  s_phase_start_ms = now;
}

}  // namespace

void init() {
  pinMode(config::kBuzzerPin, OUTPUT);
  digitalWrite(config::kBuzzerPin, LOW);
}

void tick() {
  const bool should_sound =
      services::alert::settings().sound_enabled && services::alert::pendingCount() > 0;
  const unsigned long now = millis();

  if (!should_sound) {
    if (s_sounding) {
      noTone(config::kBuzzerPin);
      s_sounding = false;
    }
    s_phase = Phase::kPause;
    s_beep_index = 0;
    return;
  }

  if (!s_sounding) {
    // A new alert: start the pattern right away rather than waiting out a pause.
    s_sounding = true;
    s_beep_index = 0;
    startBeep(now);
    return;
  }

  switch (s_phase) {
    case Phase::kOn:
      // tone() already stops itself after kBeepOnMs; just track the phase.
      if (now - s_phase_start_ms >= kBeepOnMs) {
        s_phase = Phase::kGap;
        s_phase_start_ms = now;
      }
      break;
    case Phase::kGap:
      if (now - s_phase_start_ms >= kBeepGapMs) {
        ++s_beep_index;
        if (s_beep_index < kBeepsPerBurst) {
          startBeep(now);
        } else {
          s_phase = Phase::kPause;
          s_phase_start_ms = now;
        }
      }
      break;
    case Phase::kPause:
      if (now - s_phase_start_ms >= kPauseMs) {
        s_beep_index = 0;
        startBeep(now);
      }
      break;
  }
}

}  // namespace services::buzzer

#else

namespace services::buzzer {
void init() {}
void tick() {}
}  // namespace services::buzzer

#endif  // PLANE_RADAR_TARGET_S3_ST7796
