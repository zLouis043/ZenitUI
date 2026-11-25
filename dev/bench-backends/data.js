window.BENCHMARK_DATA = {
  "lastUpdate": 1795625494417,
  "repoUrl": "https://github.com/zLouis043/ZenitUI",
  "entries": {
    "ZenitUI backend benchmarks (Raylib & SDL3)": [
      {
        "commit": {
          "author": {
            "email": "louis.003.lt@gmail.com",
            "name": "zLouis043",
            "username": "zLouis043"
          },
          "committer": {
            "email": "louis.003.lt@gmail.com",
            "name": "zLouis043",
            "username": "zLouis043"
          },
          "distinct": true,
          "id": "fb2f2129c88edbc4dacc85a4937cb58a52cbedf6",
          "message": "fix(bench): Fixing benchmarks CI for backend performance data collections",
          "timestamp": "2026-11-25T13:01:08+01:00",
          "tree_id": "129ccaf40c9e03e865de25b0aff36b1c49742860",
          "url": "https://github.com/zLouis043/ZenitUI/commit/fb2f2129c88edbc4dacc85a4937cb58a52cbedf6"
        },
        "date": 1795608272229,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 5839.73,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 5845.6,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 3405.42,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 5679.62,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 7896.1,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 5106.56,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 4999.21,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 3198.33,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 5203.65,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 9896.91,
            "unit": "us"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "louis.003.lt@gmail.com",
            "name": "zLouis043",
            "username": "zLouis043"
          },
          "committer": {
            "email": "louis.003.lt@gmail.com",
            "name": "zLouis043",
            "username": "zLouis043"
          },
          "distinct": true,
          "id": "826b0e8308635e2ea0dbba284eaa5842c9f380b3",
          "message": "feat(geometry): Moving rendering of primitives to a centrilized static mesh generation for every backend",
          "timestamp": "2026-11-25T13:43:24+01:00",
          "tree_id": "cd0993d8139c066abe86e043cef1bbae9596a898",
          "url": "https://github.com/zLouis043/ZenitUI/commit/826b0e8308635e2ea0dbba284eaa5842c9f380b3"
        },
        "date": 1795610856439,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 7112.24,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 5881.61,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 3483.19,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 5701.49,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 8942.49,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 5106.92,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 5000.88,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 3173.01,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 5069.36,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 11569.1,
            "unit": "us"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "louis.003.lt@gmail.com",
            "name": "zLouis043",
            "username": "zLouis043"
          },
          "committer": {
            "email": "louis.003.lt@gmail.com",
            "name": "zLouis043",
            "username": "zLouis043"
          },
          "distinct": true,
          "id": "3a2f62d4be7442f2783323e203d84d88f2b1a156",
          "message": "perf(sdl3): use indexed VBO/EBO ring-buffer, GL state cache and font atlas white pixel",
          "timestamp": "2026-11-25T17:47:41+01:00",
          "tree_id": "0b033277bcf0bf8e58a5516d81ba802779e2cb39",
          "url": "https://github.com/zLouis043/ZenitUI/commit/3a2f62d4be7442f2783323e203d84d88f2b1a156"
        },
        "date": 1795625493115,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 3449.67,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 3456.48,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 2123.84,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 3579.51,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 5180.94,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 4191.26,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 4135.31,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 2480.7,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 4144.85,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 7715.59,
            "unit": "us"
          }
        ]
      }
    ]
  }
}