# Examples

Self-contained programs that demonstrate ZenitUI's features. Each folder
is a complete example with its own `main.cpp` and can be built and run
independently.

For a guided tour of the framework itself, see the
[user guide](../docs/user/01-getting-started.md). This directory is a
catalogue of runnable code, not a tutorial.

---

## Prerequisites

- A C++20 compiler (`g++` or `clang++`).
- `make`.
- The Raylib backend in `deps/raylib/` (only needed at runtime and for
  the demo executables).
- `lib/libzenit.a`, the framework library (see below).

---

## Building the library

Every example links `lib/libzenit.a`. Build it once from the repository
root before compiling any example:

```bash
make            # builds lib/libzenit.a and bin/tests
make libzenit   # builds only the library
```

The library contains the core framework (`src/ui/layout/*.cpp`). The
Raylib backend (`src/backend/raylib/RaylibBackend.cpp`) is compiled
directly into each example, so a custom backend can be swapped in
without rebuilding the library.

---

## Building and running an example

From the repository root:

```bash
make examples        # compiles every example
make run-00_demo     # compiles (if needed) and runs 00_demo
```

Each example produces `examples/bin/<name>/main` (`.exe` on Windows).
On Windows, `raylib.dll` is copied next to the executable automatically.

**Important:** examples must be run **from the repository root**,
because their asset paths (`assets/...`) are relative to the root. The
`run-<name>` target already does this for you:

```bash
make run-00_demo
```

Running `examples/bin/00_demo/main` directly from a different working
directory will make the asset lookups fail.

---

## Adding a new example

1. Create `examples/NN_name/main.cpp`.
2. Run `make examples`.

The Makefile discovers every subdirectory that contains a `main.cpp`
and produces a matching target under `examples/bin/`. No Makefile edit
is required.

If the example needs more than one source file, put them all in the
same folder — every `.cpp` in the example's directory is compiled and
linked together.

If the example needs assets (fonts, textures, shaders, stylesheets),
add them to the shared `assets/` directory at the repository root
rather than to the example's own folder. This keeps the examples
small and the assets reusable.

---

## Index

| Folder | What it shows |
|--------|---------------|
| `00_demo` | Full demo: every widget, ZMarkup, ZStyle, animations, filters |
| `01_hello_world` | Minimal setup: window, root, one clickable button |
| `02_layouts` | Vertical, Horizontal, Stack containers; `grow`, `gap`, `justify` |
| `03_widgets` | Built-in widgets: Button, Toggle, Checkbox, Slider, TextInput, Dropdown |
| `04_animations` | Transitions, `@keyframes`, and `::part` animations |
| `04b_imperative_animations` | `UIAnimation` in C++: play/reverse, custom math, chaining |
| `05_zstyle` | External `.zstyle` file: `var()`, `calc()`, `@media`, states, `::part` |