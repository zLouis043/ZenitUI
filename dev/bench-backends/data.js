window.BENCHMARK_DATA = {
  "lastUpdate": 1795608273811,
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
      }
    ]
  }
}