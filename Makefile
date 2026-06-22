LLVM_PATH=/opt/homebrew/opt/llvm@20
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

# Use the specified toolchain (llvm@20)
export PATH := $(LLVM_PATH)/bin:${PATH}

all:
	echo "${PATH}"

clean:
	rm -rf $(TARGET_DIR)

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
	cmake --build --preset $(CONAN_PRESET) && \
	cp $(TARGET_DIR)/src/cli/lsql output/lsql-$(BUILD_TYPE) && \
	cp $(TARGET_DIR)/src/cli/lpipe output/lpipe-$(BUILD_TYPE)

build-docker-linux-builder:
	docker build                    \
		--tag logsql-builder:latest \
		--build-arg LLVM_VERSION=20 \
		-f ./build/images/Dockerfile.ubuntu2004 .

_prepare_output_dir:
	mkdir -p output

_require_docker_linux_profile:
	@case "$(PROFILE)" in \
		*-linux.docker) ;; \
		*) echo "PROFILE must name a Linux Docker profile, for example amd64-linux.docker"; exit 2 ;; \
	esac

build-docker-linux-%: _require_docker_linux_profile _prepare_output_dir
	docker run --rm                         \
		-v $(shell pwd):/build/lsql           \
		-v $(shell pwd)/output:/output         \
		logsql-builder:latest $(PROFILE) $*

gen-doc: deps
	cd $(TARGET_DIR) && $(CMAKE) --build . --target documentation

test: build
	cd $(TARGET_DIR) && ctest --output-on-failure -V $(args)

check-tidy: configure
	run-clang-tidy          \
		-quiet              \
		-use-color          \
		-j `nproc`          \
		-p $(TARGET_DIR)    \
		-header-filter=src/ \
		`find src/ -name '*.cpp' -o -name '*.h'

check-format:
	find src/ tests/ -type f -name '*.h' -o -name '*.cpp' \
		| xargs clang-format --dry-run --Werror

apply-format:
	find src/ tests/ -type f -name '*.h' -o -name '*.cpp' \
		| xargs clang-format -i

sync-dev-vm:
	rsync -av \
		--exclude=/target \
		--exclude=/output \
		--exclude=/_logs  \
		--exclude=/bench/results  \
		--exclude=.git   \
		../logsql/ dev-vm:/home/dzhiblavi/logsql
