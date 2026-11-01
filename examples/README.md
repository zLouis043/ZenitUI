# Examples

Self-contained programs that demonstrate ZenitUI's features. Each folder
is a complete example with its own `main.cpp` and can be built and run
independently.

For a guided tour of the framework itself, see the
[user guide](../docs/user/01-getting-started.md). This directory is a
catalogue of runnable code, not a tutorial.

---

## Prerequisites

- A C++20 compiler (`g++`, `clang++`, or MSVC).
- CMake ≥ 3.20 and Ninja (recommended).
- `git` (required by `FetchContent` to pull Raylib).
- Optional: `make`, for the convenience targets documented below.

Raylib is fetched and built automatically via CMake's `FetchContent`.
There is no `deps/raylib/` folder — the build is self-contained.

---

## Building the library

Every example links the framework library. Build it once from the
repository root before compiling any example:

```bash
make            # builds libzenit.a + unit tests
make libzenit   # builds only the library
```

Or, without the `make` frontend:

```bash
cmake -S . -B build -G Ninja
cmake --build build --target ZenitUI
```

Either way, the library ends up at `build/libzenit.a`.

The library contains the core framework (`src/layout/*.cpp` and
`src/internal/*.cpp`). The Raylib backend (`backends/raylib/`) is a
separate static target (`ZenitUI::RaylibBackend`) linked into the
examples by CMake. It is compiled **once**, not per-example — a custom
backend can be swapped in without touching the library.

---

## Building and running an example

From the repository root:

```bash
make examples        # compiles every example
make run-00_demo     # compiles (if needed) and runs 00_demo
```

Or with CMake directly:

```bash
cmake -S . -B build -G Ninja -DZENITUI_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/bin/00_demo/main
```

Each example produces `build/examples/bin/<name>/main` (`.exe` on
Windows). Raylib is linked statically, so no DLL is required at
runtime.

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
2. Run `make examples` (or `cmake --build build`).

The CMake build discovers every subdirectory that contains a
`main.cpp` and produces a matching executable under
`build/examples/bin/`. No `CMakeLists.txt` edit is required.

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
| `06_zmarkup` | Declarative UI: `ZMarkup::build`, `UINode`, callback binding |
| `07_effects` | `filter:` (blur, drop-shadow, sepia custom) and `effect:` (single shader) |
| `08_portals` | Tooltip, Popup, ContextMenu, Modal |
| `09_custom_widget` | Writing a widget from scratch: `TLayout`, `::part`, `onUpdate` |
| `10_canvas` | `CanvasLayout`: render a subtree into an offscreen target, apply a shader |
| `11_ref_list` | `@ref` and `refAll`: wire every item of a dynamic list from one loop |
| `12_bindable` | `Bindable<T>`: one model drives many widgets, lifetime-safe subscriptions |
| `13_prefabs` | `ZMarkup::registerPrefab`: register once a prefab with a Slot and reuse it in the app |
