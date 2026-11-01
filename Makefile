# =============================================================================
#  ZenitUI — Makefile frontend for CMake
#
#  This Makefile contains NO build logic. It delegates every step to CMake
#  in $(BUILD_DIR). Target names are preserved from the legacy Makefile so
#  the developer workflow is unchanged.
# =============================================================================

BUILD_DIR  ?= build
BUILD_TYPE ?= Release
CMAKE      ?= cmake

ifeq ($(OS),Windows_NT)
  EXE   := .exe
  NPROC := $(NUMBER_OF_PROCESSORS)
else
  EXE   :=
  NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
endif

ifdef BUILD
  BUILD_TYPE := $(if $(filter debug,$(BUILD)),Debug,Release)
endif

# Prefer Ninja: fast, single-config, deterministic output paths.
ifneq ($(shell command -v ninja 2>/dev/null),)
  CMAKE_GEN := -G Ninja
endif

CMAKE_FLAGS := $(CMAKE_GEN) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
ifdef ZENITUI_USE_MINGW
  CMAKE_FLAGS += -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc
endif
BUILD_CMD   := $(CMAKE) --build $(BUILD_DIR) --parallel $(NPROC)

# --- Compiler forwarding ------------------------------------------------
# Allows 'make test CXX=clang++' to drive CMake's compiler selection.
# A marker file tracks the compiler so switching triggers a clean reconfigure
# (CMake refuses to change CMAKE_CXX_COMPILER once the cache exists).
ifdef CXX
  CMAKE_FLAGS += -DCMAKE_CXX_COMPILER=$(CXX)
  ZENITUI_COMPILER := $(CXX)
else
  ZENITUI_COMPILER := default
endif
COMPILER_MARKER := $(BUILD_DIR)/.compiler

# Wipe $(BUILD_DIR) when the compiler changes, then record the new one.
define ensure_compiler
	@mkdir -p $(BUILD_DIR)
	@if [ -f $(COMPILER_MARKER) ] && [ "$$(cat $(COMPILER_MARKER))" != "$(ZENITUI_COMPILER)" ]; then \
	    echo "[make] Compiler changed → wiping $(BUILD_DIR)"; \
	    rm -rf $(BUILD_DIR); \
	    mkdir -p $(BUILD_DIR); \
	fi
	@echo "$(ZENITUI_COMPILER)" > $(COMPILER_MARKER)
endef

# Configure helper: always re-runs cmake, but it's a no-op when cache is fresh.
define configure
	$(call ensure_compiler)
	@$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_FLAGS) $(1)
endef

# --- Targets -----------------------------------------------------------------
all: libzenit tests

libzenit:
	$(call configure,-DZENITUI_BUILD_TESTS=OFF)
	@$(BUILD_CMD) --target ZenitUI

tests:
	$(call configure,-DZENITUI_BUILD_TESTS=ON)
	@$(BUILD_CMD) --target ZenitTests

test: tests
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

examples:
	$(call configure,-DZENITUI_BUILD_EXAMPLES=ON -DZENITUI_BUILD_TESTS=OFF)
	@$(BUILD_CMD)

run-%:
	$(call configure,-DZENITUI_BUILD_EXAMPLES=ON -DZENITUI_BUILD_TESTS=OFF)
	@$(BUILD_CMD) --target $*
	@./$(BUILD_DIR)/examples/bin/$*/main$(EXE)

bench:
	$(call configure,-DZENITUI_BUILD_BENCHMARKS=ON -DZENITUI_BUILD_TESTS=OFF)
	@$(BUILD_CMD) --target ZenitBenchmarks

bench-run: bench
	@./$(BUILD_DIR)/benchmarks/bin/bench$(EXE)

bench-update: bench
	@./$(BUILD_DIR)/benchmarks/bin/bench$(EXE) --update

clean:
	rm -rf $(BUILD_DIR) obj bin lib examples/bin benchmarks/bin dist

rebuild: clean all

dist:
	$(call configure,)
	@$(BUILD_CMD)
	@$(CMAKE) --install $(BUILD_DIR) --prefix $(CURDIR)/dist

.PHONY: all libzenit tests test examples run-% bench bench-run bench-update clean rebuild dist