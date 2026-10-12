CXX      := g++
CXXFLAGS := -O2 -std=c++20 -Wall -Wextra -Wno-missing-field-initializers

ROOT := ..

CPPFLAGS := -I$(ROOT)/src -I$(ROOT)/src/ui -I$(ROOT)/tests -I.

LIBZENIT := $(ROOT)/lib/libzenit.a

ifeq ($(OS),Windows_NT)
  EXE := .exe
else
  EXE :=
endif

BIN  := bin/bench$(EXE)
SRCS := main.cpp benches.cpp

all: $(BIN)

$(BIN): $(SRCS) bench_framework.hpp $(LIBZENIT)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(SRCS) $(LIBZENIT) -o $@

run: $(BIN)
	./$(BIN)

update-baseline: $(BIN)
	./$(BIN) --update

clean:
	rm -rf bin

.PHONY: all run update-baseline clean