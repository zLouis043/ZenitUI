OUT_FOLDER = ./bin
SRC_FOLDER = ./src
DEPS_FOLDER = ./deps/raylib
DEPS_HEADER_FOLDER = $(DEPS_FOLDER)/include
DEPS_LIBS_FOLDER = $(DEPS_FOLDER)/lib
APP_NAME = testui

SRC_FILES = $(shell find $(SRC_FOLDER) -name "*.cpp")
INCLUDE_DIRS = -I./src/backend -I./src/ui

CC = g++
CFLAGS = -ggdb -Wall -Wextra -Wno-missing-field-initializers

$(OUT_FOLDER)/$(APP_NAME): $(SRC_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) $(INCLUDE_DIRS) -I$(DEPS_HEADER_FOLDER) -L$(DEPS_LIBS_FOLDER) -lraylibdll -lopengl32 -lgdi32 -lwinmm -o $(OUT_FOLDER)/$(APP_NAME)

