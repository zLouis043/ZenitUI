window.BENCHMARK_DATA = {
  "lastUpdate": 1791751422615,
  "repoUrl": "https://github.com/zLouis043/ZenitUI",
  "entries": {
    "ZenitUI benchmarks": [
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
          "id": "03ae0f98ad127650ec4132cabbe9e3a298c8825d",
          "message": "Completed 4.2 of the ROADMAP.md. Now we skip ComputedStyle copy when no css animation in active",
          "timestamp": "2026-10-11T22:42:43+02:00",
          "tree_id": "45f3e393450d29d899482a477b2a3c81f6e79452",
          "url": "https://github.com/zLouis043/ZenitUI/commit/03ae0f98ad127650ec4132cabbe9e3a298c8825d"
        },
        "date": 1791751421364,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Value x1 term (Px)",
            "value": 0.0099,
            "unit": "us"
          },
          {
            "name": "Value x2 terms (calc)",
            "value": 0.0435,
            "unit": "us"
          },
          {
            "name": "ComputedStyle default + assign",
            "value": 0.3693,
            "unit": "us"
          },
          {
            "name": "ComputedStyle copy",
            "value": 0.1349,
            "unit": "us"
          },
          {
            "name": "frame loop 100 nodes",
            "value": 8.3771,
            "unit": "us"
          }
        ]
      }
    ]
  }
}