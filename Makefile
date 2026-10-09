CXX      := g++
CXXFLAGS := -ggdb -std=c++20 -Wall -Wextra -Wno-missing-field-initializers

ifeq ($(OS),Windows_NT)
  EXE := .exe
else
  EXE :=
endif

CPPFLAGS := -I./src -I./src/ui -I./src/backend -I./src/backend/raylib -I./tests
CPPFLAGS += -Ideps/raylib/include

LDFLAGS  := -Ldeps/raylib/lib
LIBS     := -lraylibdll -lopengl32 -lgdi32 -lwinmm
RAYLIB_DLL := deps/raylib/lib/raylib.dll

FRAMEWORK_SRCS := $(wildcard src/ui/layout/*.cpp)
TEST_SRCS      := $(wildcard tests/*.cpp)
BACKEND_SRCS   := src/backend/raylib/RaylibBackend.cpp
DEMO_SRCS      := src/main.cpp

OBJ_DIR := obj
BIN_DIR := bin

FRAMEWORK_OBJS := $(FRAMEWORK_SRCS:%.cpp=$(OBJ_DIR)/%.o)
BACKEND_OBJS   := $(BACKEND_SRCS:%.cpp=$(OBJ_DIR)/%.o)
TEST_OBJS      := $(TEST_SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEMO_OBJS      := $(DEMO_SRCS:%.cpp=$(OBJ_DIR)/%.o)

DEPS := $(FRAMEWORK_OBJS:.o=.d) $(BACKEND_OBJS:.o=.d) \
        $(TEST_OBJS:.o=.d) $(DEMO_OBJS:.o=.d)

TESTS_BIN := $(BIN_DIR)/tests$(EXE)
DEMO_BIN  := $(BIN_DIR)/demo$(EXE)

all: $(TESTS_BIN) $(DEMO_BIN)

$(TESTS_BIN): $(FRAMEWORK_OBJS) $(TEST_OBJS) | $(BIN_DIR)
	$(CXX) $^ -o $@

$(DEMO_BIN): $(FRAMEWORK_OBJS) $(BACKEND_OBJS) $(DEMO_OBJS) $(RAYLIB_DLL) | $(BIN_DIR)
	$(CXX) $(FRAMEWORK_OBJS) $(BACKEND_OBJS) $(DEMO_OBJS) \
	    $(LDFLAGS) $(LIBS) -o $@
	@cp $(RAYLIB_DLL) $(BIN_DIR)/

$(RAYLIB_DLL):
	@echo "ERROR: raylib.dll not found in $(RAYLIB_DLL)"
	@echo "Try copying manually raylib.dll in bin/ or check the path."
	@exit 1

$(OBJ_DIR)/src/ui/layout/%.o: src/ui/layout/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/src/backend/%.o: src/backend/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -DZENITUI_DEBUG -MMD -MP -c $< -o $@

-include $(DEPS)

$(BIN_DIR):
	mkdir -p $@

test: $(TESTS_BIN)
	./$(TESTS_BIN)

demo: $(DEMO_BIN)
	./$(DEMO_BIN)

run: demo

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

rebuild: clean all

.PHONY: all test demo run clean rebuild