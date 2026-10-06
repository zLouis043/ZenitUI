OUT_FOLDER = ./bin
SRC_FOLDER = ./src
DEPS_FOLDER = ./deps/raylib
DEPS_HEADER_FOLDER = $(DEPS_FOLDER)/include
DEPS_LIBS_FOLDER = $(DEPS_FOLDER)/lib
APP_NAME = testui

PCH_SRC = src/pch.hpp
PCH_OUT = src/pch.hpp.gch

CXX = g++
CXXFLAGS = -ggdb -Wall -Wextra -Wno-missing-field-initializers -DZENITUI_DEBUG

SRC_FILES = $(shell find $(SRC_FOLDER) -name "*.cpp")
INCLUDE_DIRS = -I./src/backend -I./src/ui

RAYLIB_LINK = -I$(DEPS_HEADER_FOLDER) -L$(DEPS_LIBS_FOLDER) -lraylibdll -lopengl32 -lgdi32 -lwinmm

TESTS_SRCS = \
    ./src/ui/layout/Layout.cpp \
    ./src/ui/layout/Measure.cpp \
    ./src/ui/layout/Render.cpp \
    ./src/ui/layout/StyleResolver.cpp \
    ./src/ui/layout/AnimationPlayer.cpp \
    ./src/ui/layout/Input.cpp \
    ./src/ui/layout/Scroll.cpp \
    tests/test_main.cpp \
    tests/test_unit.cpp \
    tests/test_easing.cpp \
    tests/test_style.cpp \
    tests/test_coretypes.cpp \
    tests/test_animations.cpp \
    tests/test_theme.cpp \
    tests/test_parser.cpp \
    tests/test_parser_utils.cpp \
    tests/test_value_parsers.cpp \
    tests/test_zmarkup.cpp \
    tests/test_layout.cpp \
    tests/test_interaction.cpp \
    tests/test_zmarkup_build.cpp

TESTS_INCLUDES = -I./src/ui -I./tests

$(OUT_FOLDER)/$(APP_NAME): $(SRC_FILES) $(PCH_OUT)
	@mkdir -p $(OUT_FOLDER)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -include $(PCH_SRC) \
	    $(SRC_FILES) $(RAYLIB_LINK) -o $@

$(PCH_OUT): $(PCH_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -x c++-header $(PCH_SRC) -o $(PCH_OUT)

main: $(OUT_FOLDER)/$(APP_NAME)

test:
	@mkdir -p $(OUT_FOLDER)
	$(CXX) $(CXXFLAGS) $(TESTS_INCLUDES) $(TESTS_SRCS) -o $(OUT_FOLDER)/tests
	./$(OUT_FOLDER)/tests

clean:
	rm -f $(OUT_FOLDER)/$(APP_NAME) $(OUT_FOLDER)/tests $(PCH_OUT) build_main.log build_test.log

all: main test

.PHONY: main test all clean