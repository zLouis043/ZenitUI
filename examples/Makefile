CXX      := g++
CXXFLAGS := -ggdb -std=c++20 -Wall -Wextra -Wno-missing-field-initializers

ROOT := ..

CPPFLAGS := -I$(ROOT)/src -I$(ROOT)/src/ui -I$(ROOT)/src/backend/raylib
CPPFLAGS += -I$(ROOT)/deps/raylib/include

LDFLAGS  := -L$(ROOT)/deps/raylib/lib
LIBS     := -lraylibdll -lopengl32 -lgdi32 -lwinmm

LIBZENIT    := $(ROOT)/lib/libzenit.a
BACKEND_SRC := $(ROOT)/src/backend/raylib/RaylibBackend.cpp
RAYLIB_DLL  := $(ROOT)/deps/raylib/lib/raylib.dll

ifeq ($(OS),Windows_NT)
  EXE := .exe
else
  EXE :=
endif

# Scoperta: ogni sottocartella con un main.cpp è un esempio.
EXAMPLE_DIRS := $(patsubst %/,%,$(dir $(wildcard */main.cpp)))

all: $(EXAMPLE_DIRS:%=bin/%/main$(EXE))

# Template: genera una regola esplicita per un esempio.
# $(1) è il nome della cartella (es. 00_demo).
define EXAMPLE_RULE
bin/$(1)/main$(EXE): $(wildcard $(1)/*.cpp) $(LIBZENIT) $(BACKEND_SRC)
	@mkdir -p $$(dir $$@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I$(1) \
	    $(wildcard $(1)/*.cpp) $(BACKEND_SRC) \
	    $(LIBZENIT) \
	    $(LDFLAGS) $(LIBS) -o $$@
	@cp $(RAYLIB_DLL) $$(dir $$@) 2>/dev/null || true
endef

$(foreach d,$(EXAMPLE_DIRS),$(eval $(call EXAMPLE_RULE,$(d))))

clean:
	rm -rf bin

.PHONY: all clean