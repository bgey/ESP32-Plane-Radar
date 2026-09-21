#pragma once

#include <cstddef>

namespace ui {

/**
 * Full-screen modal touch input: a number pad (for coordinates) and a QWERTY
 * keyboard (for names). Open one, then call its poll function every loop; each
 * poll reads the touch panel itself. When it returns something other than
 * kOpen the modal has closed and the caller repaints its own screen.
 */
enum class InputResult { kOpen, kAccepted, kCancelled };

/**
 * Number pad for a decimal value in [min, max]. OK is rejected with an on-screen
 * hint while the text is not a number in range.
 */
void numberPadOpen(const char* title, const char* initial_text, double min, double max);
InputResult numberPadPoll();
/** The entered text; valid after kAccepted. */
const char* numberPadText();

/**
 * Keyboard for free text up to max_len characters (at most 64), with letters, digits
 * and a symbols layer covering every printable ASCII character. With secret set the
 * text is masked (a SHOW/HIDE button reveals it) and nothing is auto-capitalised.
 */
void keyboardOpen(const char* title, const char* initial_text, size_t max_len,
                  bool secret = false);
InputResult keyboardPoll();
/** The entered text; valid after kAccepted. */
const char* keyboardText();

}  // namespace ui
