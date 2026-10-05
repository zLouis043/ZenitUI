OUT_FOLDER = ./bin
SRC_FOLDER = ./src
DEPS_FOLDER = ./deps/raylib
DEPS_HEADER_FOLDER = $(DEPS_FOLDER)/include
DEPS_LIBS_FOLDER = $(DEPS_FOLDER)/lib
APP_NAME = testui

SRC_FILES = $(shell find $(SRC_FOLDER) -name "*.cpp")
INCLUDE_DIRS = -I./src/backend -I./src/ui

CC = g++
CFLAGS = -ggdb -Wall -Wextra -Wno-missing-field-initializers -DZENITUI_DEBUG

TESTS_SRCS = \
    ./src/ui/Layout.cpp \
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

$(OUT_FOLDER)/$(APP_NAME): $(SRC_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) $(INCLUDE_DIRS) -I$(DEPS_HEADER_FOLDER) -L$(DEPS_LIBS_FOLDER) -lraylibdll -lopengl32 -lgdi32 -lwinmm -o $(OUT_FOLDER)/$(APP_NAME)

test:
	@mkdir -p ./bin
	$(CXX) $(CXXFLAGS) $(TESTS_INCLUDES) $(TESTS_SRCS) -o ./bin/tests
	./bin/tests

main: $(OUT_FOLDER)/$(APP_NAME)

all: main test

.PHONY: test main all