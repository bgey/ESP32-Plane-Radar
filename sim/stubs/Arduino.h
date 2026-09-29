#pragma once

#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <thread>

inline unsigned long millis() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  return static_cast<unsigned long>(
      duration_cast<milliseconds>(steady_clock::now() - start).count());
}

struct SimSerial {
  void println(const char* s = "") { std::puts(s); }
  void print(const char* s) { std::fputs(s, stdout); }
  int printf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
    va_list args;
    va_start(args, fmt);
    const int n = std::vprintf(fmt, args);
    va_end(args);
    return n;
  }
};

inline SimSerial Serial;

inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// GPIO/tone stand-ins for services that toggle a real pin (e.g. the buzzer). No pin
// actually exists on a desktop, so tone()/noTone() just print what would sound.
constexpr int OUTPUT = 0;
constexpr int INPUT = 1;
constexpr int INPUT_PULLUP = 2;
constexpr int LOW = 0;
constexpr int HIGH = 1;

inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return HIGH; }

inline void tone(int pin, unsigned frequency, unsigned long duration = 0) {
  std::printf("BEEP pin=%d %uHz %lums\n", pin, frequency, duration);
}
inline void noTone(int pin) { std::printf("SILENCE pin=%d\n", pin); }
