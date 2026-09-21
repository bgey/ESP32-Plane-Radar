#pragma once

namespace ui {

/**
 * The LOCATION settings tab: saved places you can switch between with one tap,
 * plus an editor (name, latitude, longitude) built on the number pad and keyboard.
 * The settings screen owns the header; this module draws and handles the body, and
 * takes over the whole screen while the editor or an input pad is open.
 */

/** Draw the tab body, whose first row starts at y = top. */
void locationTabDraw(int top);

/** Feed the tab body touches (x, y valid while down). Call only for y >= the body top. */
void locationTabTouch(bool down, int x, int y);

/** True while the editor or an input pad covers the whole screen. */
bool locationModalActive();

/** Poll a covering modal. Once it closes (locationModalActive() turns false) repaint. */
void locationModalPoll();

/** True once after the radar position changed (used or saved a place). */
bool locationTakeChanged();

}  // namespace ui
