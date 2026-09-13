RACK_DIR ?= ./Rack-SDK
RACK_SDK_VERSION ?= 2.6.6

# Pick the SDK matching the host, so `make sdk` works on Linux, macOS and MSYS2.
SDK_UNAME := $(shell uname -s)
SDK_MACHINE := $(shell uname -m)
ifeq ($(SDK_UNAME),Darwin)
	ifeq ($(SDK_MACHINE),arm64)
		SDK_PLATFORM := mac-arm64
	else
		SDK_PLATFORM := mac-x64
	endif
else ifeq ($(findstring MINGW,$(SDK_UNAME)),MINGW)
	SDK_PLATFORM := win-x64
else ifeq ($(findstring MSYS,$(SDK_UNAME)),MSYS)
	SDK_PLATFORM := win-x64
else
	SDK_PLATFORM := lin-x64
endif
RACK_SDK_URL := https://vcvrack.com/downloads/Rack-SDK-$(RACK_SDK_VERSION)-$(SDK_PLATFORM).zip


FLAGS +=
CFLAGS +=
CXXFLAGS +=

LDFLAGS +=

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/dsp/*.cpp)
SOURCES += $(wildcard src/seq/*.cpp)
SOURCES += $(wildcard src/ui/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

include $(RACK_DIR)/plugin.mk

# Build number. Bumps on any source change so the panel shows which build is loaded.
BUILD_INFO := src/generated/BuildInfo.hpp
BUILD_WATCH := $(wildcard src/*.cpp src/*.hpp src/dsp/*.hpp src/seq/*.cpp src/seq/*.hpp \
	src/ui/*.cpp src/ui/*.hpp res/*.svg) plugin.json Makefile

$(BUILD_INFO): $(BUILD_WATCH)
	@tools/bump-build.sh $@

# Every object needs the header to exist first, even on a parallel build.
$(OBJECTS): $(BUILD_INFO)

build-number: $(BUILD_INFO)
	@grep M32_BUILD_NUMBER $(BUILD_INFO) | awk '{print "M32 build " $$3}'

.PHONY: build-number

# Offline DSP checks. Runs without a Rack installation, linking the SDK's libRack.
test: build/dsp_test build/seq_test build/panel_test build/led_test build/module_test
	@./build/dsp_test
	@./build/seq_test
	@./build/panel_test
	@./build/led_test
	@./build/module_test

build/led_test: test/led_test.cpp $(wildcard src/seq/*.hpp) src/seq/Pattern.cpp src/seq/PanelControl.cpp src/seq/PanelLeds.cpp
	@mkdir -p build
	$(CXX) -std=c++11 -O2 -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include \
		-o $@ $< src/seq/Pattern.cpp src/seq/PanelControl.cpp src/seq/PanelLeds.cpp -L$(RACK_DIR) -lRack -Wl,-rpath,$(RACK_DIR)

build/module_test: test/module_test.cpp $(BUILD_INFO) $(wildcard src/*.hpp src/*/*.hpp) src/M32.cpp src/seq/Pattern.cpp src/seq/PanelControl.cpp src/seq/PanelLeds.cpp
	@mkdir -p build
	$(CXX) -std=c++11 -O2 -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include \
		-o $@ $< src/M32.cpp src/seq/Pattern.cpp src/seq/PanelControl.cpp src/seq/PanelLeds.cpp -L$(RACK_DIR) -lRack -Wl,-rpath,$(RACK_DIR)

build/dsp_test: test/dsp_test.cpp $(wildcard src/dsp/*.hpp)
	@mkdir -p build
	$(CXX) -std=c++11 -O2 -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include \
		-o $@ $< -L$(RACK_DIR) -lRack -Wl,-rpath,$(RACK_DIR)

build/panel_test: test/panel_test.cpp $(wildcard src/seq/*.hpp) src/seq/Pattern.cpp src/seq/PanelControl.cpp
	@mkdir -p build
	$(CXX) -std=c++11 -O2 -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include \
		-o $@ $< src/seq/Pattern.cpp src/seq/PanelControl.cpp -L$(RACK_DIR) -lRack -Wl,-rpath,$(RACK_DIR)

build/seq_test: test/seq_test.cpp $(wildcard src/seq/*.hpp) src/seq/Pattern.cpp
	@mkdir -p build
	$(CXX) -std=c++11 -O2 -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include \
		-o $@ $< src/seq/Pattern.cpp -L$(RACK_DIR) -lRack -Wl,-rpath,$(RACK_DIR)

.PHONY: test

# Fetch the Rack SDK this plugin builds against.
sdk:
	curl -sSL -o Rack-SDK.zip "$(RACK_SDK_URL)"
	unzip -q -o Rack-SDK.zip
	rm -f Rack-SDK.zip

.PHONY: sdk
