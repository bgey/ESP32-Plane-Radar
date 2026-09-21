#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"

#if defined(PLANE_RADAR_SIM)

/**
 * Panel_sdl waits up to 1 ms on the render thread after every low-level draw call,
 * so text and rounded shapes drawn straight to the panel take hundreds of ms. This
 * drops that handshake and just flags the framebuffer as modified; the SDL thread
 * copies it to the texture on its own schedule.
 */
struct SimPanel : public lgfx::Panel_sdl {
  void drawPixelPreclipped(uint_fast16_t x, uint_fast16_t y, uint32_t rawcolor) override {
    lgfx::Panel_FrameBufferBase::drawPixelPreclipped(x, y, rawcolor);
    ++_modified_counter;
  }
  void writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w,
                               uint_fast16_t h, uint32_t rawcolor) override {
    lgfx::Panel_FrameBufferBase::writeFillRectPreclipped(x, y, w, h, rawcolor);
    ++_modified_counter;
  }
  void writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h,
                  lgfx::pixelcopy_t* param, bool use_dma) override {
    lgfx::Panel_FrameBufferBase::writeImage(x, y, w, h, param, use_dma);
    ++_modified_counter;
  }
  void writeImageARGB(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w,
                      uint_fast16_t h, lgfx::pixelcopy_t* param) override {
    lgfx::Panel_FrameBufferBase::writeImageARGB(x, y, w, h, param);
    ++_modified_counter;
  }
  void writePixels(lgfx::pixelcopy_t* param, uint32_t len, bool use_dma) override {
    lgfx::Panel_FrameBufferBase::writePixels(param, len, use_dma);
    ++_modified_counter;
  }
};

/** Desktop simulator: SDL window (or headless dummy video driver). */
class LGFX : public lgfx::LGFX_Device {
  SimPanel _panel;

public:
  LGFX() {
    simConfigure(480, 320);
    _panel.setScaling(1, 1);
    setPanel(&_panel);
  }

  /** Set the SDL window/framebuffer size; call before init(). */
  void simConfigure(int width, int height) {
    auto cfg = _panel.config();
    cfg.memory_width = width;
    cfg.memory_height = height;
    cfg.panel_width = width;
    cfg.panel_height = height;
    _panel.config(cfg);
  }
};

#elif defined(PLANE_RADAR_TARGET_S3_ST7796)

/**
 * LovyanGFX device: ST7796S on SPI, XPT2046 touch on the same bus.
 * Pin values come from config.h.
 */
class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_SPI _bus;
  lgfx::Panel_ST7796 _panel;
  lgfx::Light_PWM _light;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.freq_write = config::kDisplaySpiWriteHz;
      cfg.freq_read = 16000000;
      cfg.pin_sclk = static_cast<int>(config::kDisplayPinSclk);
      cfg.pin_mosi = static_cast<int>(config::kDisplayPinMosi);
      cfg.pin_miso = static_cast<int>(config::kDisplayPinMiso);
      cfg.pin_dc = static_cast<int>(config::kDisplayPinDc);
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = static_cast<int>(config::kDisplayPinCs);
      cfg.pin_rst = static_cast<int>(config::kDisplayPinRst);
      cfg.invert = config::kDisplayInvert;
      cfg.rgb_order = config::kDisplayRgbOrder;
      // Native panel orientation; setRotation() at runtime yields 480x320.
      cfg.panel_width = 320;
      cfg.panel_height = 480;
      _panel.config(cfg);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl = static_cast<int>(config::kDisplayPinBacklight);
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    {
      auto cfg = _touch.config();
      cfg.bus_shared = true;
      cfg.spi_host = SPI2_HOST;
      cfg.freq = config::kTouchSpiHz;
      cfg.pin_sclk = static_cast<int>(config::kDisplayPinSclk);
      cfg.pin_mosi = static_cast<int>(config::kDisplayPinMosi);
      cfg.pin_miso = static_cast<int>(config::kDisplayPinMiso);
      cfg.pin_cs = static_cast<int>(config::kTouchPinCs);
      cfg.pin_int = config::kTouchUseIrq ? static_cast<int>(config::kTouchPinIrq) : -1;
      cfg.x_min = 0;
      cfg.x_max = 4095;
      cfg.y_min = 0;
      cfg.y_max = 4095;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};

#else

/** LovyanGFX device: GC9A01 on SPI. Pin values come from config.h. */
class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_SPI _bus;
  lgfx::Panel_GC9A01 _panel;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.freq_write = config::kDisplaySpiWriteHz;
      cfg.pin_sclk = static_cast<int>(config::kDisplayPinSclk);
      cfg.pin_mosi = static_cast<int>(config::kDisplayPinMosi);
      cfg.pin_miso = -1;
      cfg.pin_dc = static_cast<int>(config::kDisplayPinDc);
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = static_cast<int>(config::kDisplayPinCs);
      cfg.pin_rst = static_cast<int>(config::kDisplayPinRst);
      cfg.invert = config::kDisplayInvert;
      cfg.rgb_order = config::kDisplayRgbOrder;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

#endif  // PLANE_RADAR_TARGET_S3_ST7796
