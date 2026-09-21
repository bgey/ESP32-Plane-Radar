#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"

#if defined(PLANE_RADAR_SIM)

/** Desktop simulator: SDL window (or headless dummy video driver). */
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_sdl _panel;

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
