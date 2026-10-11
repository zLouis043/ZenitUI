CXX      := g++
CXXFLAGS := -O2 -std=c++20 -Wall -Wextra -Wno-missing-field-initializers

ROOT := ..

CPPFLAGS := -I$(ROOT)/src -I$(ROOT)/src/ui -I$(ROOT)/tests

LIBZENIT := $(ROOT)/lib/libzenit.a

ifeq ($(OS),Windows_NT)
  EXE := .exe
else
  EXE :=
endif

BIN := bin/bench$(EXE)

all: $(BIN)

$(BIN): bench.cpp $(LIBZENIT)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) bench.cpp $(LIBZENIT) -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf bin

.PHONY: all run clean