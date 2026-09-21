# Desktop radar simulator. Run from the repo root inside Linux/WSL:
#   sudo apt install g++ make libsdl2-dev     (once)
#   make -C sim
#   SDL_VIDEODRIVER=dummy sim/build/radar_sim --out sim/out/radar.png
# LovyanGFX is taken from the PlatformIO cache; run `pio run -e s3mini` once first.

ROOT  := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
LGFX  := $(ROOT)/.pio/libdeps/s3mini/LovyanGFX/src
BUILD := $(ROOT)/sim/build
BIN   := $(BUILD)/radar_sim

CXXFLAGS := -std=gnu++17 -O1 -g -pthread -Wno-psabi -MMD -MP \
  -DPLANE_RADAR_SIM -DPLANE_RADAR_TARGET_S3_ST7796 -DLGFX_SDL \
  -I$(ROOT)/sim/stubs -I$(ROOT)/include -I$(ROOT)/sim -I$(LGFX) \
  $(shell sdl2-config --cflags)
CFLAGS   := -O1 -g -I$(LGFX)
LDLIBS   := $(shell sdl2-config --libs) -lpthread

APP_SRC := $(wildcard $(ROOT)/src/ui/*.cpp) \
  $(ROOT)/src/hardware/display.cpp $(ROOT)/src/hardware/display_font.cpp \
  $(ROOT)/src/data/large_airports_data.cpp $(ROOT)/src/services/traffic_alert.cpp \
  $(ROOT)/sim/sim_main.cpp $(ROOT)/sim/sim_services.cpp

LGFX_CPP := $(wildcard $(LGFX)/lgfx/v1/*.cpp) \
  $(wildcard $(LGFX)/lgfx/v1/misc/*.cpp) \
  $(LGFX)/lgfx/v1/panel/Panel_Device.cpp \
  $(LGFX)/lgfx/v1/panel/Panel_FrameBufferBase.cpp \
  $(wildcard $(LGFX)/lgfx/v1/platforms/sdl/*.cpp)
LGFX_C := $(wildcard $(LGFX)/lgfx/utility/*.c) \
  $(wildcard $(LGFX)/lgfx/v1/lv_font/*.c) \
  $(wildcard $(LGFX)/lgfx/Fonts/efont/*.c) \
  $(wildcard $(LGFX)/lgfx/Fonts/IPA/*.c) \
  $(wildcard $(LGFX)/lgfx/Fonts/lvgl/*.c)

OBJS := $(patsubst $(ROOT)/%.cpp,$(BUILD)/%.o,$(APP_SRC)) \
  $(patsubst $(LGFX)/%.cpp,$(BUILD)/lgfx/%.o,$(LGFX_CPP)) \
  $(patsubst $(LGFX)/%.c,$(BUILD)/lgfx/%.o,$(LGFX_C)) \
  $(BUILD)/ui_font.o

all: $(BIN)

# Rebuild objects when a header they include changes.
-include $(OBJS:.o=.d)

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDLIBS)

# App sources get the Arduino stub implicitly, as they do through the ESP32 framework.
$(BUILD)/%.o: $(ROOT)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -include Arduino.h -c $< -o $@

$(BUILD)/lgfx/%.o: $(LGFX)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/lgfx/%.o: $(LGFX)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Embeds data/ui_font.vlw with the same _binary_data_ui_font_vlw_* symbols as the ESP32 build.
$(BUILD)/ui_font.o: $(ROOT)/data/ui_font.vlw
	@mkdir -p $(BUILD)
	cd $(ROOT) && ld -r -b binary -o $@ data/ui_font.vlw

clean:
	rm -rf $(BUILD)

.PHONY: all clean
