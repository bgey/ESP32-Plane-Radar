#pragma once

#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

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
