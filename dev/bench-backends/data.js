window.BENCHMARK_DATA = {
  "lastUpdate": 1795731001633,
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
          "id": "0a1cf19e93f5d1a7d02d0958538f6d8056c8ddab",
          "message": "perf(sdl3): revert font atlas white-pixel coalescing for solid shapes",
          "timestamp": "2026-11-25T19:51:33+01:00",
          "tree_id": "71d3b3b29efade32231378277774636bda71145f",
          "url": "https://github.com/zLouis043/ZenitUI/commit/0a1cf19e93f5d1a7d02d0958538f6d8056c8ddab"
        },
        "date": 1795632969645,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 5967.35,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 5867.32,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 3520.01,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 5816.24,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 8911.64,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 5183.81,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 5105.6,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 3233.41,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 5155.23,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 11619.2,
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
          "id": "5a4e720df44e49644e610020da1b1a028c110dbf",
          "message": "perf(render): cache static filtered layers and add renderer BlendMode\n\nCaches off-screen filtered layer outputs in Layout::drawLayer after one stable frame using subtree fingerprinting and TargetPool generation tracking. Adds IRenderer::setBlendMode (Alpha, Premultiplied, Replace) in SDL3 and Raylib backends so off-screen filter passes preserve straight alpha and Silhouette filters (drop-shadow) composite without double blending or clipping.",
          "timestamp": "2026-11-25T22:18:25+01:00",
          "tree_id": "5597bce72514ec233d2ebaefdc389864b25cd178",
          "url": "https://github.com/zLouis043/ZenitUI/commit/5a4e720df44e49644e610020da1b1a028c110dbf"
        },
        "date": 1795641726052,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 3181.32,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 3174.02,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 1840.36,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 3184.45,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 1779.63,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 2871.46,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 2850.33,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 1700.33,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 2827.36,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 1752.37,
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
          "id": "0ef8a54aff3357dd11b4f584ce33434d22fb119b",
          "message": "perf(render): fast-path animated layer cache checks and avoid filter arg allocations\n\nAdds a scalar pre-check and upfront animation check in Layout::drawLayer to skip subtree fingerprinting on animated nodes, defers Silhouette offset parsing to cache-bake frames only, replaces std::string/std::stof length parsing with a zero-allocation parser, and guards registerBuiltinFilters() with a static flag.",
          "timestamp": "2026-11-25T22:45:56+01:00",
          "tree_id": "abb5ae168a2a9db87805e7f0682905f5bab2aa5c",
          "url": "https://github.com/zLouis043/ZenitUI/commit/0ef8a54aff3357dd11b4f584ce33434d22fb119b"
        },
        "date": 1795643508533,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 5977.85,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 5887.89,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 3462.04,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 5740.71,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 3312.44,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 5733.71,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 4996.56,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 3180.78,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 5078.74,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 3124.46,
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
          "id": "3e942dc17508e3fde79a172a0f6131ca05941d4e",
          "message": "refactor(core): replace global Metrics::viewport with UIContext::viewport\n\nRemoves the global Metrics::viewport struct from CoreTypes.hpp and stores the active logical viewport on UIContext. Value::resolve* and ruleMatches now take an explicit Vec2 viewport parameter (fixing a pre-existing vertical AST mode bug in Value::resolveV), Layout exposes viewportSize(), and SDL3/Raylib/Mock backends compute clip bounds without referencing global metrics.",
          "timestamp": "2026-11-26T12:28:21+01:00",
          "tree_id": "c0778172a694aa2a5b1fc72fe540dce3beeb1bde",
          "url": "https://github.com/zLouis043/ZenitUI/commit/3e942dc17508e3fde79a172a0f6131ca05941d4e"
        },
        "date": 1795692771142,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 5905.61,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 5864.99,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 3512.38,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 5888.28,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 3250.8,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 5248.38,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 5098.05,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 3270.37,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 5213.67,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 3187.29,
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
          "id": "1a5072dab9404e14a78248f33be5bf8feb7f999e",
          "message": "refactor(filters): move FilterRegistry into UIContext\n\nReplaces the FilterRegistry::get() singleton and registerBuiltinFilters() free function with a value member UIContext::filters whose constructor pre-registers builtin filters. Passes UIContext* through FilterContext so FilterRegistry.cpp no longer calls UIContext::get(), resets filters in MockBackend Env teardown/reset, and updates filter tests and callers.",
          "timestamp": "2026-11-26T15:02:29+01:00",
          "tree_id": "0156b6d6c881ee07f8293f73f89d2908a0535f2e",
          "url": "https://github.com/zLouis043/ZenitUI/commit/1a5072dab9404e14a78248f33be5bf8feb7f999e"
        },
        "date": 1795702096612,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 6037.85,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 6068.24,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 3558.69,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 5886.49,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 3352.12,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 5246.56,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 5148.76,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 3194.47,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 5214.55,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 3162.4,
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
          "id": "82f4785c9c8463a69b2326dcc3ac7dad6eacfa5b",
          "message": "refactor(style): move Theme into UIContext and remove Theme::get()\n\nReplaces the global Theme::get() singleton with a per-context UIContext::theme member. Adds Layout::theme() accessor, passes const Theme& explicitly through Anim::* helpers, removes the default Theme::get() argument from loadStyleString/loadStyleFile, and updates StyleResolver, Debug, MockBackend, tests, benchmarks, and examples.",
          "timestamp": "2026-11-26T16:01:00+01:00",
          "tree_id": "37333b82c03816833df2595e8de9ec0180bc199e",
          "url": "https://github.com/zLouis043/ZenitUI/commit/82f4785c9c8463a69b2326dcc3ac7dad6eacfa5b"
        },
        "date": 1795705525735,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 4805.29,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 4635.99,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 2725.27,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 4482.57,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 2579.94,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 4019.52,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 3953.23,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 2466.01,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 4118.97,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 2438.38,
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
          "id": "13566cf500ec0f46864c41c4b895c678afc6eefb",
          "message": "refactor(core): make UIContext instantiable and bind Layout trees via ctx_\n\nMakes UIContext non-copyable/non-movable with explicit mount/unmount, runFrame/renderFrame, and safe bidirectional backend teardown (attachBackend/detachBackend, IBackend::bindTo/unbind). Attaches Layout subtrees via non-owning ctx_ propagated across tree mutations, adds Layout::moveChildBefore so keyed Each reconciliation preserves focus and context state, and migrates core subsystems and components to node.context().",
          "timestamp": "2026-11-26T19:59:11+01:00",
          "tree_id": "163088e90b074fa7664a68bb277ca6ebd5e77d5a",
          "url": "https://github.com/zLouis043/ZenitUI/commit/13566cf500ec0f46864c41c4b895c678afc6eefb"
        },
        "date": 1795719823718,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 4812.32,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 4724.35,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 2894.72,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 4648.9,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 2659.21,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 4239.44,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 4241.82,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 2787.71,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 4418.99,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 2650.66,
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
          "id": "d5436b37468de8cfedd6bdfaaf378521698c8a61",
          "message": "refactor(core): remove UIContext::get() singleton and add multi-context tests\n\nRemoves UIContext::get() and temporary IBackend::bindToContext/unbindFromContext wrappers, switches Layout::context() to nullable UIContext* with safe unmounted fallbacks, owns UIContext per Test::Env fixture, migrates all tests/benchmarks/examples/docs to explicit UIContext instances, and adds tests/test_multi_context.cpp covering theme/viewport/input/filter/target-pool isolation, subtree reparenting, and teardown order independence.",
          "timestamp": "2026-11-26T22:25:58+01:00",
          "tree_id": "37877e268467877cfe8492d328306ce92c61f1f7",
          "url": "https://github.com/zLouis043/ZenitUI/commit/d5436b37468de8cfedd6bdfaaf378521698c8a61"
        },
        "date": 1795728579677,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 3547.47,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 3545.07,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 2212.56,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 3538.56,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 1866.14,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 3044.92,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 3013.69,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 1790.59,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 3135.07,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 1775.96,
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
          "id": "24fda5dfafa2eb652cdb6138aa0ffa984af88c46",
          "message": "fix(docs): Fixed README.md",
          "timestamp": "2026-11-26T22:30:51+01:00",
          "tree_id": "35f62dfcd7471d98684500dca3db9be90395a89d",
          "url": "https://github.com/zLouis043/ZenitUI/commit/24fda5dfafa2eb652cdb6138aa0ffa984af88c46"
        },
        "date": 1795728927005,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 6224.34,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 6033.51,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 3543.57,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 5971.25,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 3420.4,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 5132.9,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 5064.24,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 3187.9,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 5097.73,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 3184.12,
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
          "id": "750e7254662918e2bc5528e1567ec23ade821606",
          "message": "feat(core): track primary mounted root for UIContext::runFrame/renderFrame overloads\n\nStores primaryRoot_ on UIContext when mount() is called (auto-cleared on unmount, detach, reparenting, or destruction), adds zero-root runFrame(dt)/renderFrame()/unmount()/mountedRoot() helpers, updates examples and docs to avoid repeating the root handle in the main loop, and adds unit test coverage in test_multi_context.cpp.",
          "timestamp": "2026-11-26T23:05:23+01:00",
          "tree_id": "dd291632280dc65bc1ec08f6a54eeac46063b743",
          "url": "https://github.com/zLouis043/ZenitUI/commit/750e7254662918e2bc5528e1567ec23ade821606"
        },
        "date": 1795730999986,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "raylib.frame.static_dashboard",
            "value": 4889.87,
            "unit": "us"
          },
          {
            "name": "raylib.render.static_dashboard",
            "value": 4732.22,
            "unit": "us"
          },
          {
            "name": "raylib.frame.rounded_cards_200",
            "value": 2915.32,
            "unit": "us"
          },
          {
            "name": "raylib.frame.virtual_scroll_10k",
            "value": 4872.41,
            "unit": "us"
          },
          {
            "name": "raylib.frame.filters_10",
            "value": 2633.26,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.static_dashboard",
            "value": 4125.09,
            "unit": "us"
          },
          {
            "name": "sdl3.render.static_dashboard",
            "value": 3964.17,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.rounded_cards_200",
            "value": 2447.48,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.virtual_scroll_10k",
            "value": 4003.17,
            "unit": "us"
          },
          {
            "name": "sdl3.frame.filters_10",
            "value": 2411.18,
            "unit": "us"
          }
        ]
      }
    ]
  }
}