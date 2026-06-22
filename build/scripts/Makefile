MAKE = make
CONAN = conan
CMAKE = cmake

BUILD_TYPE ?= $(error BUILD_TYPE is required)
PROFILE ?= $(error PROFILE is required)

CONAN_PRESET = conan-$(shell echo "$(BUILD_TYPE)" | tr '[:upper:]' '[:lower:]')
ASAN ?= OFF
TSAN ?= OFF
UBSAN ?= OFF
TESTS ?= ON

TARGET = target
TARGET_DIR = ./$(TARGET)/$(PROFILE)/$(BUILD_TYPE)

_prepare_target_dirs:
	mkdir -p $(TARGET_DIR)

deps: _prepare_target_dirs
	$(CONAN) install .                             \
		--output-folder=$(TARGET_DIR)              \
		--build=missing                            \
		--profile:host=./conan/$(PROFILE).profile  \
		--profile:build=./conan/$(PROFILE).profile \
		--settings:host=build_type=$(BUILD_TYPE)   \
		--settings:build=build_type=$(BUILD_TYPE)

configure: deps
	cmake --preset $(CONAN_PRESET) \
		-DASAN=$(ASAN)             \
		-DTSAN=$(TSAN)             \
		-DUBSAN=$(UBSAN)           \
		-DLOGSQL_BUILD_TESTS=$(TESTS)

build: configure
	cmake --build --preset $(CONAN_PRESET)

test: build
	ctest --test-dir $(TARGET_DIR) --output-on-failure
