#pragma once

#include <cstdint>

namespace ui::theme {

/**
 * Colour theme. Night is the original light-on-dark look; Day is dark-on-white with
 * stronger colours, for reading the screen in direct sunlight. Every UI colour comes
 * from the ui::radar::kColor* roles declared in radar_theme.h; this module sets them.
 */
enum class Mode : uint8_t { kNight = 0, kDay = 1 };

/** Load the saved mode (Night when none) and apply it. Call once at boot after displayInit(). */
void init();

Mode mode();

/** Switch the mode, save it and apply it. Screens must be redrawn by the caller. */
void setMode(Mode mode);

/** Recompute every colour role for the current mode. Cheap; safe to call on every draw. */
void apply();

}  // namespace ui::theme
