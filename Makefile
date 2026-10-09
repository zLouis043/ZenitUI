CXX      := g++
CXXFLAGS := -ggdb -std=c++20 -Wall -Wextra -Wno-missing-field-initializers
CPPFLAGS := -I./src -I./src/ui -I./tests

OBJ_DIR := obj
BIN_DIR := bin
LIB_DIR := lib

ifeq ($(OS),Windows_NT)
  EXE := .exe
else
  EXE :=
endif

FRAMEWORK_SRCS := $(wildcard src/ui/layout/*.cpp)
TEST_SRCS      := $(wildcard tests/*.cpp)

FRAMEWORK_OBJS := $(FRAMEWORK_SRCS:%.cpp=$(OBJ_DIR)/%.o)
TEST_OBJS      := $(TEST_SRCS:%.cpp=$(OBJ_DIR)/%.o)

DEPS := $(FRAMEWORK_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

LIB_BIN   := $(LIB_DIR)/libzenit.a
TESTS_BIN := $(BIN_DIR)/tests$(EXE)

# --- Target di default ---
all: $(LIB_BIN) $(TESTS_BIN)

libzenit: $(LIB_BIN)

$(LIB_BIN): $(FRAMEWORK_OBJS) | $(LIB_DIR)
	ar rcs $@ $^

$(TESTS_BIN): $(TEST_OBJS) $(LIB_BIN) | $(BIN_DIR)
	$(CXX) $(TEST_OBJS) -L$(LIB_DIR) -lzenit -o $@

$(OBJ_DIR)/src/ui/layout/%.o: src/ui/layout/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -DZENITUI_DEBUG -MMD -MP -c $< -o $@

-include $(DEPS)

$(BIN_DIR):
	mkdir -p $@
$(LIB_DIR):
	mkdir -p $@

# --- Comandi utente ---
test: $(TESTS_BIN)
	./$(TESTS_BIN)

examples: $(LIB_BIN)
	$(MAKE) -C examples -j8

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)
	$(MAKE) -C examples clean

run-%: $(LIB_BIN)
	$(MAKE) -C examples
	./examples/bin/$*/main$(EXE)

rebuild: clean all

.PHONY: all libzenit test examples run-% clean rebuild