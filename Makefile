MAKE = make
CONAN = conan
CMAKE = cmake

BUILD_TYPE ?= Debug
CONAN_PRESET = conan-$(shell echo "$(BUILD_TYPE)" | tr '[:upper:]' '[:lower:]')
PROFILE ?= armv8-macos
ASAN ?= OFF
TSAN ?= OFF
UBSAN ?= OFF
TESTS ?= ON

TARGET = target
TARGET_DIR = ./$(TARGET)/$(PROFILE)/$(BUILD_TYPE)

_all:
	:

_prepare_target_dirs:
	mkdir -p $(TARGET_DIR)

_prepare_output_dir:
	mkdir -p output

_require_docker_linux_profile:
	@case "$(PROFILE)" in \
		*-linux.docker) ;; \
		*) echo "PROFILE must name a Linux Docker profile, for example amd64-linux.docker"; exit 2 ;; \
	esac

clean:
	rm -rf $(TARGET_DIR)

install_dependencies: _prepare_target_dirs
	$(CONAN) install .                             \
		--output-folder=$(TARGET_DIR)              \
		--build=missing                            \
		--profile:host=./conan/$(PROFILE).profile  \
		--profile:build=./conan/$(PROFILE).profile \
		--settings:host=build_type=$(BUILD_TYPE)   \
		--settings:build=build_type=$(BUILD_TYPE)

configure: install_dependencies
	cmake --preset $(CONAN_PRESET) \
		-DASAN=$(ASAN)             \
		-DTSAN=$(TSAN)             \
		-DUBSAN=$(UBSAN)           \
		-DLOGSQL_BUILD_TESTS=$(TESTS)

build: configure
	cmake --build --preset $(CONAN_PRESET) && \
	cp $(TARGET_DIR)/src/cli/lsql output/lsql-$(BUILD_TYPE) && \
	cp $(TARGET_DIR)/src/cli/lpipe output/lpipe-$(BUILD_TYPE)

build-docker-linux-builder:
	docker build                    \
		--tag logsql-builder:latest \
		--build-arg LLVM_VERSION=20 \
		-f ./build/images/Dockerfile.ubuntu2004 .

build-docker-linux-%: _require_docker_linux_profile _prepare_output_dir
	docker run --rm                         \
		-v $(shell pwd):/build/lsql           \
		-v $(shell pwd)/output:/output         \
		logsql-builder:latest $(PROFILE) $*

gen-doc: install_dependencies
	cd $(TARGET_DIR) && $(CMAKE) --build . --target documentation

test: build
	cd $(TARGET_DIR) && ctest --output-on-failure -V $(args)

check-format:
	find src/ tests/ -type f -name '*.h' -o -name '*.cpp' \
		| xargs clang-format --dry-run --Werror

apply-format:
	find src/ tests/ -type f -name '*.h' -o -name '*.cpp' \
		| xargs clang-format -i
