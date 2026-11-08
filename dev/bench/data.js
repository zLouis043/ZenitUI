window.BENCHMARK_DATA = {
  "lastUpdate": 1794145219147,
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
          "id": "2f00a3884d2f5bc9cd1a0d4cb540f829c8a0de89",
          "message": "Fixed docs.yml that was previously forcing itself onto the benchmarks workflow deleting its history",
          "timestamp": "2026-10-11T22:52:50+02:00",
          "tree_id": "e5a2ea74b6fdc05327f107363c385ec30e165bd1",
          "url": "https://github.com/zLouis043/ZenitUI/commit/2f00a3884d2f5bc9cd1a0d4cb540f829c8a0de89"
        },
        "date": 1791752022618,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Value x1 term (Px)",
            "value": 0.0084,
            "unit": "us"
          },
          {
            "name": "Value x2 terms (calc)",
            "value": 0.0425,
            "unit": "us"
          },
          {
            "name": "ComputedStyle default + assign",
            "value": 0.2896,
            "unit": "us"
          },
          {
            "name": "ComputedStyle copy",
            "value": 0.1101,
            "unit": "us"
          },
          {
            "name": "frame loop 100 nodes",
            "value": 6.9473,
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
          "id": "cf13e87f3e1b3e945e05f655a89d1331ed763de5",
          "message": "Still trying to make benchmarks and docs live together",
          "timestamp": "2026-10-11T22:57:15+02:00",
          "tree_id": "1a448c193f1076ba9a2ba8ae9a003c2a87d9b3ff",
          "url": "https://github.com/zLouis043/ZenitUI/commit/cf13e87f3e1b3e945e05f655a89d1331ed763de5"
        },
        "date": 1791752314894,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Value x1 term (Px)",
            "value": 0.0084,
            "unit": "us"
          },
          {
            "name": "Value x2 terms (calc)",
            "value": 0.0425,
            "unit": "us"
          },
          {
            "name": "ComputedStyle default + assign",
            "value": 0.2878,
            "unit": "us"
          },
          {
            "name": "ComputedStyle copy",
            "value": 0.1097,
            "unit": "us"
          },
          {
            "name": "frame loop 100 nodes",
            "value": 6.9391,
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
          "id": "b637452435441915f15a39ec972bfc427373b332",
          "message": "Completed 4.3 of the ROADMPA.md: index Theme rules by tag; rebuild benchmark suite",
          "timestamp": "2026-10-12T11:52:45+02:00",
          "tree_id": "14aaab1c0e50c60706ea5f3573d3261f92248b58",
          "url": "https://github.com/zLouis043/ZenitUI/commit/b637452435441915f15a39ec972bfc427373b332"
        },
        "date": 1791798843963,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 348.526,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.76357,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0124511,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.051901,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0383898,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.790141,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.971209,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.390528,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 11.3055,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 267.945,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.2087,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 9.4323,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934119,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 233.635,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 2015.79,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 236.368,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 338.218,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 485.744,
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
          "id": "b3248fcdde8dac777eaf778d807b1fd0225d990a",
          "message": "Added mor e benchmarks to cover more realistic cases",
          "timestamp": "2026-10-12T12:23:17+02:00",
          "tree_id": "99e13ed9d1715fb74e47a3ceb97eff326c24742b",
          "url": "https://github.com/zLouis043/ZenitUI/commit/b3248fcdde8dac777eaf778d807b1fd0225d990a"
        },
        "date": 1791800649610,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 170.481,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 2.65132,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00812138,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0345372,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0295534,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.437039,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.511106,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.192563,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 6.53281,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 142.711,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 7.89382,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 5.58496,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00456752,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 114.261,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 1123.55,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 119.64,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 176.61,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 241.566,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 127.614,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 334.299,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 1141.21,
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
          "id": "cf17e3dc76235aa3fc5f926a754cfc2b21c55feb",
          "message": "Completed 4.4 of the ROADMAP.md, added another benchmark and test and updated docs",
          "timestamp": "2026-10-12T14:10:03+02:00",
          "tree_id": "2d32198fe35041bd46563edd9ac95ed2149700ee",
          "url": "https://github.com/zLouis043/ZenitUI/commit/cf17e3dc76235aa3fc5f926a754cfc2b21c55feb"
        },
        "date": 1791807071321,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 346.145,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.81264,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0124782,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0519025,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0438496,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.80556,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.972605,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.404388,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 14.832,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 64.1141,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 276.74,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.5043,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.15888,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00935161,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 236.856,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 2099.31,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 241.107,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 342.967,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 487.688,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 263.494,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 680.933,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 2368.25,
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
          "id": "c0a7823f8a7c34157e6abe2cb27f96701341d7df",
          "message": "Build defaults to -O2, so that the benchmarks now measure an optimized build of the framework.",
          "timestamp": "2026-10-12T14:51:34+02:00",
          "tree_id": "99f6e2befd40dec6c4a1a32903c334001b195375",
          "url": "https://github.com/zLouis043/ZenitUI/commit/c0a7823f8a7c34157e6abe2cb27f96701341d7df"
        },
        "date": 1791809604909,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 333.082,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.6593,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0117084,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0506721,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0399619,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.737261,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.905633,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.340483,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.12332,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 35.6401,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 45.2112,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 14.3804,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.94686,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0066705,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 40.9149,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 314.641,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 42.542,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 103.848,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 84.1184,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 46.0615,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 113.259,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 448.603,
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
          "id": "88bb8910d15e031994b02c7c2e3a14e126c8ab25",
          "message": "Completed 4.5 of the ROADMAP.md. Now isInheritedProp is optimized. Added a new benchmark and test, updated docs",
          "timestamp": "2026-10-12T15:07:23+02:00",
          "tree_id": "d6691a58960e18fc59cc1fc77306b6e4c0c2ae88",
          "url": "https://github.com/zLouis043/ZenitUI/commit/88bb8910d15e031994b02c7c2e3a14e126c8ab25"
        },
        "date": 1791810507974,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 170.138,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 2.69636,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00861414,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0347796,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0291457,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.438325,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.522408,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.482189,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.193877,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.836219,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 19.8367,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 24.7958,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 7.84,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.02533,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00459678,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 19.2313,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 184.559,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 20.3135,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 55.4819,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 44.6985,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 23.6915,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 58.1349,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 220.507,
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
          "id": "6deb44535f6d19c8044302e9b2d06241ded24f1c",
          "message": "added benchmark for 4.6, completed phase 4 of the ROADMAP.md, updated docs.",
          "timestamp": "2026-10-12T15:15:59+02:00",
          "tree_id": "faae354d5715d2d78ed1d2153b5a8d61c345d02b",
          "url": "https://github.com/zLouis043/ZenitUI/commit/6deb44535f6d19c8044302e9b2d06241ded24f1c"
        },
        "date": 1791811075388,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 378.007,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.81669,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0140604,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0622875,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0637385,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.833154,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.995668,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.898367,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.341619,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.949878,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 36.691,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 48.4058,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 14.6021,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.61436,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 32.145,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0105445,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 40.6092,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 330.104,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 42.5712,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 107.227,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 84.3031,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 45.3951,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 117.66,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 446.544,
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
          "id": "d8f84330580b08f289ea858de6b1c36583a5e101",
          "message": "Fixed the 5.4a of the ROADMAP.md. Now chained filters are enabled. Updated src, test, example and docs",
          "timestamp": "2026-10-12T20:15:17+02:00",
          "tree_id": "a62c5231e6ce4877667eb20c9da058ba078c399c",
          "url": "https://github.com/zLouis043/ZenitUI/commit/d8f84330580b08f289ea858de6b1c36583a5e101"
        },
        "date": 1791828990132,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 217.301,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.10957,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00875175,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0388793,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0297356,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.533453,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.650913,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.557391,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.25284,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.942001,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 24.0668,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 29.6938,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 10.0313,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.80681,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 20.5234,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00480347,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 26.9254,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 192.031,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 28.0243,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 67.3068,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 54.6843,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 30.2762,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 72.2322,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 328.629,
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
          "id": "dc046a6c7e1d259851df2525a9a6874d7e1e2a66",
          "message": "Closed the 5.4b closing the 5.4 in the ROADMAP.md, it is just a change in docs, left a single check just as a reminder for the future",
          "timestamp": "2026-10-12T21:27:20+02:00",
          "tree_id": "3e76eeb3f0c5b7004fc17d62fd3aa4adf67dcd02",
          "url": "https://github.com/zLouis043/ZenitUI/commit/dc046a6c7e1d259851df2525a9a6874d7e1e2a66"
        },
        "date": 1791833307562,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 344.55,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.78222,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0124494,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0520682,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0438854,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.805805,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.950117,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.88705,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.372852,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.03337,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.1459,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 52.2027,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.6602,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 9.76853,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 33.9959,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00933987,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 43.6117,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 325.537,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 47.2497,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 115.158,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 93.8194,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 49.8461,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 124.546,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 480.204,
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
          "id": "0b8af1762040f45ac9181c7cf3905448c08b1d98",
          "message": "Done 5.8 of the ROADMAP.md",
          "timestamp": "2026-10-12T22:57:02+02:00",
          "tree_id": "e384f1a5031cda5493e9bb9d953efbf950287200",
          "url": "https://github.com/zLouis043/ZenitUI/commit/0b8af1762040f45ac9181c7cf3905448c08b1d98"
        },
        "date": 1791838720472,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 347.106,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.81692,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0124576,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0519954,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0441622,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.807823,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.966329,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.885056,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.369944,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.02569,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.3971,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 52.2742,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.7576,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 8.55492,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 33.4826,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00936587,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 44.2648,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 327.763,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 48.498,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 113.796,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 94.6653,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 49.8912,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 123.851,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 485.335,
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
          "id": "df352916be9285a4e5305757a3ca25e495ca8426",
          "message": "Closed 5.7 of the ROADMAP.md, modified Popup, Dropdown, Modal widgets, updated docs added tests",
          "timestamp": "2026-10-12T23:07:22+02:00",
          "tree_id": "869445f4a72f1ed304b61c57c2e2d2f43e8df071",
          "url": "https://github.com/zLouis043/ZenitUI/commit/df352916be9285a4e5305757a3ca25e495ca8426"
        },
        "date": 1791839308877,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 345.574,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.77028,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.014092,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0622013,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0642304,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.838427,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.00378,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.894193,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.348035,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.935778,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 36.0293,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 50.2857,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 14.9817,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 9.72198,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 31.5106,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0105488,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 43.0194,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 322.915,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 45.1155,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 107.195,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 89.3756,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 48.0993,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 120.328,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 451.754,
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
          "id": "2ef3be258cb75f033d4dc0c2cb0bd9adfe6f97ad",
          "message": "Completed 5.6 of ROADMAP.md, added tests and updated docs",
          "timestamp": "2026-10-13T12:13:05+02:00",
          "tree_id": "cc38dc5ef9b3a05229fa430466139cd42fea7da1",
          "url": "https://github.com/zLouis043/ZenitUI/commit/2ef3be258cb75f033d4dc0c2cb0bd9adfe6f97ad"
        },
        "date": 1791886450190,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 298.114,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.1666,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0102351,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0451157,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0357597,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.656391,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.79582,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.747543,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.325968,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.1137,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 32.7702,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 39.7121,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 13.6488,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.38713,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 23.4213,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00611659,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 36.1972,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 257.145,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 38.1373,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 92.3922,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 74.5702,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 40.6084,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 96.9741,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 405.988,
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
          "id": "1434272fa97a545b8ff8bc4158c248e1a1dfaf92",
          "message": "Closed new points in 5th chapther of the ROADMAP, the 5.12a-d. Fixed some missing documentations. Added tests",
          "timestamp": "2026-10-13T13:12:25+02:00",
          "tree_id": "1789ce0b92d70ec1dbf4caade9bcd9daf0f9ab48",
          "url": "https://github.com/zLouis043/ZenitUI/commit/1434272fa97a545b8ff8bc4158c248e1a1dfaf92"
        },
        "date": 1791890031363,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 345.92,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.82063,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0124833,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0526155,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.04378,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.803095,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.950051,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.894548,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.366581,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.04235,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.7648,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 50.5968,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.7422,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 8.66418,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 33.9727,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934298,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 42.7406,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 324.114,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 45.6893,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 113.412,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 92.5774,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 48.0847,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 120.43,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 459.848,
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
          "id": "ad5c7443c39a1acbec5e0e8566358bf4f4504dc9",
          "message": "Completed 5.13 of the ROADMAP.md, added tests, updated docs",
          "timestamp": "2026-10-13T14:41:22+02:00",
          "tree_id": "52ad240329ad371f11e2807ca79b0bf55f3f586e",
          "url": "https://github.com/zLouis043/ZenitUI/commit/ad5c7443c39a1acbec5e0e8566358bf4f4504dc9"
        },
        "date": 1791895381674,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 349.653,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.87381,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.012564,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0521838,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0476705,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.797261,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.94233,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.90738,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.392968,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.04043,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.2475,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 50.1355,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.4802,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 9.23301,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 34.0334,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00933935,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 41.1647,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 329.491,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 44.2238,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 110.963,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 89.0236,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 46.7751,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 119.795,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 450.693,
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
          "id": "fb6bc9d3d6fde5e9825b1bd834ee7bc8745af017",
          "message": "Added and completed 5.14 of the ROADMAP.md",
          "timestamp": "2026-10-13T15:12:52+02:00",
          "tree_id": "e0c9c8ef03f59d6b6ca355bcbff8140920edbaf4",
          "url": "https://github.com/zLouis043/ZenitUI/commit/fb6bc9d3d6fde5e9825b1bd834ee7bc8745af017"
        },
        "date": 1791897276200,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 343.278,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.79405,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0125858,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.052381,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0438326,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.796685,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.972733,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.875115,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.365117,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.02837,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 40.9045,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 51.1586,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.2021,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 8.71862,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 34.2076,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934055,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 42.807,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 331.218,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 45.7088,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 117.029,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 92.8258,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 48.2581,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 122.17,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 477.296,
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
          "id": "3b2b48f0e66b573e8dc2c76a31a5c1fb2e03f48b",
          "message": "Closed 5.10 and 5.10a, updated docs",
          "timestamp": "2026-10-13T16:15:42+02:00",
          "tree_id": "14df85c9a72eadc2f2f41952ee1944a1ecefc32e",
          "url": "https://github.com/zLouis043/ZenitUI/commit/3b2b48f0e66b573e8dc2c76a31a5c1fb2e03f48b"
        },
        "date": 1791901018962,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 163.945,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 2.58591,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00787438,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0348956,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0292525,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.425073,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.518159,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.464483,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.187615,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.832106,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 19.891,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 24.0046,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 7.86667,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 4.12778,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 15.4034,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00443663,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 19.3663,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 183.261,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 20.2173,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 51.2933,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 39.8692,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 21.8122,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 57.7113,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 220.392,
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
          "id": "c0743f9b16357c4a8c55ad45260cef4e264de5aa",
          "message": "Replaced shiftHeld with ModifierState struct",
          "timestamp": "2026-10-13T21:49:03+02:00",
          "tree_id": "afd904eafa53909d97962243d517782d5c544261",
          "url": "https://github.com/zLouis043/ZenitUI/commit/c0743f9b16357c4a8c55ad45260cef4e264de5aa"
        },
        "date": 1791921033246,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 350.77,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.87276,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0121423,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0530597,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0421977,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.757238,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.925205,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.842581,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.362839,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.17072,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.7481,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 46.4371,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 14.5841,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 8.95994,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 28.3607,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00714202,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 42.3004,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 298.825,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 44.1145,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 104.745,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 86.509,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 47.6821,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 114.993,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 454.916,
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
          "id": "8436bdd49c4a16f02e4133612e354bf8cb27bedf",
          "message": "Replaced 19 bool fields with three bitfield structs",
          "timestamp": "2026-10-16T16:52:51+02:00",
          "tree_id": "313d7dc6a0d248b3fc4814bb701535bc265a60ff",
          "url": "https://github.com/zLouis043/ZenitUI/commit/8436bdd49c4a16f02e4133612e354bf8cb27bedf"
        },
        "date": 1792162435204,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 343.487,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.80506,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0127985,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0523072,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0438061,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.790574,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.954905,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.871809,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.361619,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.02874,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.7005,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 49.8848,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.4127,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 6.49704,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 33.5177,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00933331,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 41.8688,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 317.476,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 45.2496,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 110.856,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 87.9991,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 47.2203,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 119.578,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 455.092,
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
          "id": "83a5a4ecb585724136e879dfeaa65e8f325fd84a",
          "message": "Updated docs, now they follow the bitfield flags structure",
          "timestamp": "2026-10-16T21:47:06+02:00",
          "tree_id": "6779497342bf12b6f0a3636b685e71972a9c1404",
          "url": "https://github.com/zLouis043/ZenitUI/commit/83a5a4ecb585724136e879dfeaa65e8f325fd84a"
        },
        "date": 1792180107556,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 347.672,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.84361,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.012486,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0520703,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0438353,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.83729,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.971673,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.909932,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.373799,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.01824,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.4625,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 49.5174,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.9956,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 8.17631,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 33.7778,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00933855,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 41.5906,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 324.898,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 45.1024,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 111.869,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 89.8413,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 46.6296,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 118.94,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 448.679,
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
          "id": "db5060a0a7b9eb188dcdcdb9ec6b19b55cd92d8a",
          "message": "Working on input, expanding PointerState and UIContext to per-button arrays",
          "timestamp": "2026-10-16T23:34:11+02:00",
          "tree_id": "f12b6363c11c2771e16ef0c1bfef74740daea2ef",
          "url": "https://github.com/zLouis043/ZenitUI/commit/db5060a0a7b9eb188dcdcdb9ec6b19b55cd92d8a"
        },
        "date": 1792186526102,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 256.812,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.77734,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0109036,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0483567,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0497028,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.640147,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.775993,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.68042,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.272053,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.14608,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 27.8844,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 37.5373,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 11.4591,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 5.44367,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 24.1061,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00817512,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 30.6253,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 254.444,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 32.1986,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 78.834,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 64.0025,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 34.7896,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 88.1871,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 337.997,
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
          "id": "b3bf5f4ba2d11c00a5ced120d3cfb85a37368883",
          "message": "Updated README.md and ROADMAP.md, preparing for the docs and example commit of the 5.15 of the ROADMAP.md",
          "timestamp": "2026-10-18T11:34:08+02:00",
          "tree_id": "907a2784007e22408c3e4218eba52c56b20300f6",
          "url": "https://github.com/zLouis043/ZenitUI/commit/b3bf5f4ba2d11c00a5ced120d3cfb85a37368883"
        },
        "date": 1792316116672,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 346.344,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.80874,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0124434,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0518614,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0438156,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.792821,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.947892,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.883033,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.372789,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.0425,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.4395,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 50.5939,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.4709,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 6.04155,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 33.5042,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0093574,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 42.769,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 326.739,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 46.57,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 115.118,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 90.9709,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 48.3513,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 121.195,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 465.54,
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
          "id": "756c9c0600e74b4479c3ac1fc87a9d5212f73971",
          "message": "Added examples to CI so we can catch api drift",
          "timestamp": "2026-10-18T11:39:24+02:00",
          "tree_id": "0e64ce3cdbf851a2346481944cc0d29c81c64551",
          "url": "https://github.com/zLouis043/ZenitUI/commit/756c9c0600e74b4479c3ac1fc87a9d5212f73971"
        },
        "date": 1792316445339,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 333.471,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.6913,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0120525,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0513308,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0399103,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.724418,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.916112,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.808263,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.34901,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.14119,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 37.1993,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 47.2418,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 14.3372,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.95224,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 27.5616,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00691119,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 43.5264,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 295.73,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 44.7386,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 105.152,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 88.4632,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 48.3325,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 115.237,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 452.349,
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
          "id": "e0f13bde54c1e9d46a6ac7237ecc7df7810f0c2d",
          "message": "Addeded docs/architecture/PROPOSALS.md, so we can keep track not only with what we need to do on the ROADMAP.md but also future ideas",
          "timestamp": "2026-10-18T12:48:02+02:00",
          "tree_id": "b9f263948a07b1b2c5f0e5b0411efce700407f70",
          "url": "https://github.com/zLouis043/ZenitUI/commit/e0f13bde54c1e9d46a6ac7237ecc7df7810f0c2d"
        },
        "date": 1792320533072,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 179.993,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 2.7357,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00830013,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0361791,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.029919,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.451885,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.517755,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.466176,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.193015,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.845743,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 19.9429,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 24.3896,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 7.78641,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.9459,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 15.0548,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0044456,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 19.7552,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 182.843,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 20.9268,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 55.5244,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 41.8337,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 23.0729,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 58.7192,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 224.686,
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
          "id": "b274f7d5ee6a518ca5fe8f725eef25cacf2255e9",
          "message": "Added ref logic adding and completing the 5.16 from the ROADMAP.md, come from the PROPOSALS.md. Updated docs and tests",
          "timestamp": "2026-10-18T18:06:15+02:00",
          "tree_id": "ffa8ae6becf9856038d3c54f4e82c3bfede695ec",
          "url": "https://github.com/zLouis043/ZenitUI/commit/b274f7d5ee6a518ca5fe8f725eef25cacf2255e9"
        },
        "date": 1792339634267,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 345.47,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.90695,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0124509,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0519404,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0436315,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.795947,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.943285,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.907517,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.367043,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.04208,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 37.681,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 54.1773,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.8769,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.63279,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 35.5279,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934844,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 45.5618,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 336.236,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 48.2368,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 113.118,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 94.739,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 50.6325,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 125.768,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 491.851,
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
          "id": "73462f42adb368595764f36d7fffe8ccfc31fb85",
          "message": "Fix ROADMAP.md 1.10, clear context pointers in ~Layout",
          "timestamp": "2026-10-19T12:22:29+02:00",
          "tree_id": "349119ac2da62fcb8b97a458d4154c46e987ec06",
          "url": "https://github.com/zLouis043/ZenitUI/commit/73462f42adb368595764f36d7fffe8ccfc31fb85"
        },
        "date": 1792405432141,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 346.023,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.93477,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0124914,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0521664,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0439273,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.800988,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.94914,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.889734,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.383512,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.04147,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.6681,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 53.2141,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.4736,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 7.16786,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 35.625,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00933061,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 45.0968,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 330.278,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 49.1688,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 116.559,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 96.2374,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 51.1748,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 126.39,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 487.027,
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
          "id": "add8f5aceec47d97510e38ffdd2183877b8042a9",
          "message": "Fixed test_interaction.cpp fail, pushing to check the CI",
          "timestamp": "2026-10-19T12:38:50+02:00",
          "tree_id": "2d4a3eadb6245ae1fdb932fdd5b157b1c03b33fd",
          "url": "https://github.com/zLouis043/ZenitUI/commit/add8f5aceec47d97510e38ffdd2183877b8042a9"
        },
        "date": 1792406417507,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 343.542,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 5.07582,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0120692,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0521302,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0410275,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.758863,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.957828,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.828029,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.360333,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.18536,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.5903,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 49.2928,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 14.6702,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 10.5828,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 27.4975,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0067679,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 44.4414,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 299.533,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 46.4526,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 108.359,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 91.252,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 49.5557,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 117.243,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 465.851,
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
          "id": "5a8ace81d0e35b476196e008f23a992fe827ed65",
          "message": "Added second proposal Bindable<T> for lightweight data binding, added tests",
          "timestamp": "2026-10-19T12:44:10+02:00",
          "tree_id": "342f1b1e93603cb29a288f348abb5a4ff036a870",
          "url": "https://github.com/zLouis043/ZenitUI/commit/5a8ace81d0e35b476196e008f23a992fe827ed65"
        },
        "date": 1792406730997,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 339.055,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.9012,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0125801,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0524534,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0437422,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.795528,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.973729,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.883972,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.380286,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.04133,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.9518,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 52.6766,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.5448,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 8.7108,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 35.5459,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934578,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 45.4418,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 331.956,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 48.0949,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 116.985,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 94.5422,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 51.6097,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 125.842,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 486.96,
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
          "id": "858116b819b2eeaf379a796c0a8f47282d400ade",
          "message": "Added and updated docs to introduce the new Bindable<T> feature",
          "timestamp": "2026-10-19T12:55:33+02:00",
          "tree_id": "c85994eab265c8f81f0867e850f30b259fbd8869",
          "url": "https://github.com/zLouis043/ZenitUI/commit/858116b819b2eeaf379a796c0a8f47282d400ade"
        },
        "date": 1792407410726,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 327.879,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.81686,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0115565,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0479812,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0395939,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.757148,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.907725,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.832263,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.35759,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.07872,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 37.7207,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 47.8887,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 14.5562,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 9.8459,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 28.563,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0067963,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 41.4154,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 316.81,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 43.853,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 104.941,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 83.7187,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 45.9771,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 112.548,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 503.56,
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
          "id": "bae9faff87777eb80a3861afb4cfb41e638b2a5a",
          "message": "Fix, shrink containers after child removal ROADMAP.md 1.11",
          "timestamp": "2026-10-19T13:25:34+02:00",
          "tree_id": "d7ef53c8650a023724fcb501bf84744803f442b6",
          "url": "https://github.com/zLouis043/ZenitUI/commit/bae9faff87777eb80a3861afb4cfb41e638b2a5a"
        },
        "date": 1792409192459,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 252.173,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.81624,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0109026,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0482762,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0492499,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.62961,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.741875,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.661427,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.271281,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.13836,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 28.194,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 37.5231,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 11.8588,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 8.77004,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 24.6695,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00818125,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 31.3196,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 244.534,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 33.474,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 82.319,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 65.271,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 35.649,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 89.5561,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 341.236,
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
          "id": "3b40ae0ab27ff9fcad7d258880dfcb1f26314e27",
          "message": "Fix Bindable snippets in docs",
          "timestamp": "2026-10-19T13:26:14+02:00",
          "tree_id": "6e71544ca8d8468ee74daf8eac4448f122b1c334",
          "url": "https://github.com/zLouis043/ZenitUI/commit/3b40ae0ab27ff9fcad7d258880dfcb1f26314e27"
        },
        "date": 1792409248803,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 346.561,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.89457,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0124537,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0524664,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0439049,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.810128,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.962841,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.885788,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.380877,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.03195,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 38.8831,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 51.397,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.8367,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 9.41508,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 36.0647,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00935512,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 43.5769,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 317.792,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 47.9577,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 115.484,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 90.69,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 49.1154,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 121.274,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 471.475,
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
          "id": "251e7dd28a87ad69eb084bcc73d91a5f750103e5",
          "message": "Added new example 12 for Bindable<T>",
          "timestamp": "2026-10-19T13:26:51+02:00",
          "tree_id": "54a6712b00e94441460276e5c18421c21bd22e57",
          "url": "https://github.com/zLouis043/ZenitUI/commit/251e7dd28a87ad69eb084bcc73d91a5f750103e5"
        },
        "date": 1792409320388,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 329.79,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.80518,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0112306,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0477583,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0418372,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.756799,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.917531,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.833418,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.355175,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.11141,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 37.3743,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 47.2394,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.2415,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 10.5674,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 28.4333,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00664249,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 40.8086,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 301.949,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 43.4476,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 105.684,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 82.9949,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 45.232,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 110.227,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 494.276,
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
          "id": "4f4d2654c6c3fe6efa965d34dec785d5c251fedd",
          "message": "Completed 5.3 of the ROADMAP.md, added tests and updated docs",
          "timestamp": "2026-10-27T16:46:57+01:00",
          "tree_id": "a9ffe4c7f926f7041827f595c5636aa6f7d5c033",
          "url": "https://github.com/zLouis043/ZenitUI/commit/4f4d2654c6c3fe6efa965d34dec785d5c251fedd"
        },
        "date": 1793116121705,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 194.371,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.29863,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00854733,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0406945,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0324052,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.665261,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.675378,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.68018,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.242016,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.8682,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 25.6301,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 31.0016,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 8.36287,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 12.9191,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 15.9331,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00455675,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 25.1657,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 211.57,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 26.1781,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 65.7904,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 51.6202,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 28.5958,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 72.9399,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 279.161,
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
          "id": "029b8c0fa55224c6103c7c96acf987a8c790394f",
          "message": "Started 5.1 of the roadmap.md. Bug fixes for interaction between clip and effects. Update test and docs",
          "timestamp": "2026-10-27T22:28:44+01:00",
          "tree_id": "d01d9151f904954d6b21c00792aa4357454b40e6",
          "url": "https://github.com/zLouis043/ZenitUI/commit/029b8c0fa55224c6103c7c96acf987a8c790394f"
        },
        "date": 1793136598778,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 290.106,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.9077,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0114919,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0572083,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0503782,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.780326,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.949627,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.818269,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.361396,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.12507,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 37.8704,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 46.5923,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 12.1177,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 10.7658,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 25.6545,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00822271,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 37.5731,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 305.872,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 39.3751,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 99.5207,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 79.206,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 43.2684,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 110.474,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 418.283,
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
          "id": "558ba3a0bdbbd6abbded256a829d9f34eca31321",
          "message": "Completed 5.5 of the ROADMAP.md, added tests and updated docs.",
          "timestamp": "2026-10-28T12:25:00+01:00",
          "tree_id": "0a4b01aabe0feeab3f8078b4959627fdf968e43c",
          "url": "https://github.com/zLouis043/ZenitUI/commit/558ba3a0bdbbd6abbded256a829d9f34eca31321"
        },
        "date": 1793186804113,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 175.869,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.09932,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00801071,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0390123,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.030472,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.50132,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.601029,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.531466,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.239061,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.805845,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 24.0555,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 29.2543,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 8.12476,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 10.1453,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 19.7395,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00465665,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 22.9811,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 200.309,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 24.5563,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 61.3564,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 48.4395,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 25.8989,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 66.6496,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 251.715,
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
          "id": "3413b657cd3f32bdff179ee190a51e52d032501c",
          "message": "Completed 5.1 of the ROADMAP.md, added tests and updated docs",
          "timestamp": "2026-10-28T12:39:57+01:00",
          "tree_id": "4c259d272531f225cf7ce08e75da7f2d35d94302",
          "url": "https://github.com/zLouis043/ZenitUI/commit/3413b657cd3f32bdff179ee190a51e52d032501c"
        },
        "date": 1793187687683,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 175.64,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.11694,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00797543,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0368222,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0299933,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.494255,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.602429,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.528203,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.240829,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.810139,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 25.2562,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 28.941,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 7.77345,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 6.479,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 14.57,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00423747,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 22.8299,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 203.724,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 23.5613,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 60.6185,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 48.286,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 26.8581,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 69.041,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 254.622,
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
          "id": "a504b7d92b22b0a489ba159613e75d02f7b6925b",
          "message": "Fixed bug in Tooltip.hpp that made the tooltip go always on top of the relative widget, even if the widget was a y=0. Now it should work fine and flip.",
          "timestamp": "2026-10-28T12:47:51+01:00",
          "tree_id": "346ad1bd3135663953080073a814ae586b803b47",
          "url": "https://github.com/zLouis043/ZenitUI/commit/a504b7d92b22b0a489ba159613e75d02f7b6925b"
        },
        "date": 1793188160365,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 385.562,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.2551,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0127746,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0601339,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0465955,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.979755,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.2045,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.09587,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.49149,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.03401,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 51.3655,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 60.6089,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 16.3106,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 11.1065,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 37.4424,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00935832,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 50.8452,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 392.767,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 54.3293,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 134.793,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 105.595,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 58.4183,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 143.08,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 552.689,
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
          "id": "53359e679ec218e8e12c0834d99980b9c4d28eb3",
          "message": "Fixed drowdown navigation, text selection and disabled focus. Added separated styling options for selection box in the Textinput and for dropdown items.",
          "timestamp": "2026-10-28T13:25:31+01:00",
          "tree_id": "eabfb900550b0da4f95a2a50d5261b15240dc25e",
          "url": "https://github.com/zLouis043/ZenitUI/commit/53359e679ec218e8e12c0834d99980b9c4d28eb3"
        },
        "date": 1793190406992,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 390.467,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.27948,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0129263,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0598219,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0463681,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.991597,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.21185,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.09777,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.501376,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.03133,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 52.3349,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 59.9866,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.9553,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 13.0636,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 37.9745,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00941415,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 50.8252,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 385.051,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 55.3598,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 141.276,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 106.25,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 58.3664,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 144.265,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 542.225,
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
          "id": "864f5896d1cee838854acf531d9a3d503f7ed5c1",
          "message": "Completed proposals 3, added and updated to ROADMAP.md as 5.18,5.19. Added tests and examples, updated docs.",
          "timestamp": "2026-10-30T16:20:05+01:00",
          "tree_id": "fddabe998f54953773cba5944320c67b0b38732d",
          "url": "https://github.com/zLouis043/ZenitUI/commit/864f5896d1cee838854acf531d9a3d503f7ed5c1"
        },
        "date": 1793373723796,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 364.462,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.38818,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0146376,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0751586,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.065387,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 1.01833,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.25569,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.07317,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.475959,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.947638,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 46.505,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 59.7901,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.3487,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 12.1731,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 34.172,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0106238,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 48.2557,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 385.616,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 50.162,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 127.584,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 100.301,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 55.4461,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 141.075,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 540.076,
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
          "id": "2b0fd8741a537440e0d9eaa871d89b3627511e31",
          "message": "Completed proposal 4 and 5.20 of the ROADMAP, now we have proper diagnostics system and warnings",
          "timestamp": "2026-11-01T15:53:35+01:00",
          "tree_id": "3d0dc942c6f918c00c4bbaccc5f6d8d0a78031f1",
          "url": "https://github.com/zLouis043/ZenitUI/commit/2b0fd8741a537440e0d9eaa871d89b3627511e31"
        },
        "date": 1793544890531,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 370.792,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 5.9125,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0118561,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0532439,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0417039,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.87508,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.06566,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.970427,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.433577,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.08291,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 44.9425,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 53.1914,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.0398,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 15.7491,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 29.1383,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00664781,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 45.0632,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 352.979,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 47.2464,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 116.943,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 90.2799,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 50.3722,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 126.667,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 596.212,
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
          "id": "7fbae45e8f3f17b074007c3a70fd3395c8db5cae",
          "message": "Added wrap to the layout. COmpleted 5.21 of the roadmap, updated examples: 02_layout, added tests ,updated docs",
          "timestamp": "2026-11-01T18:56:38+01:00",
          "tree_id": "0efad31c906c6c2300e061a918fcebe306cb1ca5",
          "url": "https://github.com/zLouis043/ZenitUI/commit/7fbae45e8f3f17b074007c3a70fd3395c8db5cae"
        },
        "date": 1793555905099,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 401.753,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.00659,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0131302,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0580744,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0464799,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.939911,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.13798,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.07087,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.482438,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.04225,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 51.147,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 61.2785,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.9299,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 13.318,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 35.9811,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00935814,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 52.3919,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 380.009,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 54.9841,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 136.708,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 106.95,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 58.9604,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 147.029,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 560.108,
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
          "id": "04d592e507d683548937035f9a18d232a7fe3279",
          "message": "Cleaned files",
          "timestamp": "2026-11-01T19:36:13+01:00",
          "tree_id": "55b57624b8d52e4e46200be83233a67f981d009b",
          "url": "https://github.com/zLouis043/ZenitUI/commit/04d592e507d683548937035f9a18d232a7fe3279"
        },
        "date": 1793558238017,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 293.215,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.68716,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0113711,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0568207,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0505415,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.748331,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.897116,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.798493,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.352087,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.12989,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 35.7699,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 44.9807,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 11.5381,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 9.51199,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 24.829,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00820169,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 37.5543,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 318.803,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 39.0542,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 97.5089,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 78.8008,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 42.4828,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 112.428,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 417.346,
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
          "id": "9e8b084cedb53371a32b12502f97c2d02cdac7cd",
          "message": "Updated error messages",
          "timestamp": "2026-11-01T19:46:29+01:00",
          "tree_id": "6f1d6ca16c27410ac771e77590eb353153bd0851",
          "url": "https://github.com/zLouis043/ZenitUI/commit/9e8b084cedb53371a32b12502f97c2d02cdac7cd"
        },
        "date": 1793558862212,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 387.511,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 5.98445,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0131105,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0727218,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0606487,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.945936,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.15056,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.07975,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.475826,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.04475,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 48.8309,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 62.1413,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 16.0061,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 12.439,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 34.3894,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934262,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 52.8859,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 378.206,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 56.1727,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 134.454,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 108.551,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 59.0382,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 147.857,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 567.801,
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
          "id": "0694bf548f7f252a1a74ed704078b5706c8a6acc",
          "message": "Moved from pure Makefiles to CMake (i know i fell for this trap too). Removed vendored raylib, improved and added msvc to the matrix for the tests and examples.",
          "timestamp": "2026-11-01T21:24:49+01:00",
          "tree_id": "7901d1f3c088cbaa1dd0ae4848850e9141b0dbbf",
          "url": "https://github.com/zLouis043/ZenitUI/commit/0694bf548f7f252a1a74ed704078b5706c8a6acc"
        },
        "date": 1793564756279,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 395.928,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.28483,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0128214,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0577416,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0463059,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.937536,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.12509,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.04419,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.467784,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.21463,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 48.5456,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 61.981,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.9777,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 12.7299,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 36.9399,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0093385,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 52.9376,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 381.459,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 56.1553,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 137.675,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 108.955,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 61.2995,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 147.935,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 567.235,
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
          "id": "f6cdae4ef0e4f39d19243919a016aef10a71e9cf",
          "message": "Fix linux x11 problem for the examples build",
          "timestamp": "2026-11-01T21:31:47+01:00",
          "tree_id": "8e7e815f098f127bea979d88019d8179d37dfd99",
          "url": "https://github.com/zLouis043/ZenitUI/commit/f6cdae4ef0e4f39d19243919a016aef10a71e9cf"
        },
        "date": 1793565205878,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 397.939,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.03676,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.012788,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0576703,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0463184,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.938291,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.11614,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.03688,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.465876,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.21247,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 48.8759,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 61.4608,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 17.7701,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 16.7225,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 37.241,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934644,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 52.57,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 379.704,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 55.3595,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 138.026,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 109.535,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 60.4849,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 147.289,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 565.517,
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
          "id": "0f878fafb4ec8896b844cce00dec8eb7bc3a0c08",
          "message": "Yeah I cleaned some autogenerated s**t from the new Cmake files. I did not want to put my hands directly onto that things",
          "timestamp": "2026-11-01T21:39:15+01:00",
          "tree_id": "0dc0db494c21c51ea8c22cb62dd87af57dc9d6f7",
          "url": "https://github.com/zLouis043/ZenitUI/commit/0f878fafb4ec8896b844cce00dec8eb7bc3a0c08"
        },
        "date": 1793565626890,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 395.649,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.89177,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0127692,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0577935,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0463071,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.948794,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.12584,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.06777,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.473414,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.2129,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 49.2023,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 61.6664,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 16.1468,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 11.06,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 35.4225,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934249,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 53.1173,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 376.49,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 56.7705,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 134.423,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 111.04,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 59.1029,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 146.194,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 552.045,
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
          "id": "97f6efa2022c3695a638b257615a0a731c2b4f82",
          "message": "Updated docs",
          "timestamp": "2026-11-01T21:44:03+01:00",
          "tree_id": "5c534a938eed7a5a35434aac1acf34d68acf42f1",
          "url": "https://github.com/zLouis043/ZenitUI/commit/97f6efa2022c3695a638b257615a0a731c2b4f82"
        },
        "date": 1793565909637,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 394.804,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.10337,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0131039,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0591793,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0464064,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.951583,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.14209,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.05133,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.470444,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.21374,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 48.2165,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 61.4607,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.8513,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 11.2343,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 36.6138,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00935667,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 52.7572,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 376.308,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 56.3,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 137.096,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 109.441,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 58.9725,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 144.72,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 555.628,
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
          "id": "68e8a734d82357bc3021d2aa0f508d3b7710e55c",
          "message": "Updated examples/README.md. Now it is coherent with the new cmake usage",
          "timestamp": "2026-11-01T21:57:34+01:00",
          "tree_id": "75420a9381edfef5a8ab8fa60d620155032606d1",
          "url": "https://github.com/zLouis043/ZenitUI/commit/68e8a734d82357bc3021d2aa0f508d3b7710e55c"
        },
        "date": 1793566719784,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 395.459,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 5.92814,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0127661,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.05755,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0461449,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.938194,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.12814,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.04138,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.478395,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.21268,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 47.9849,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 61.7779,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.9117,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 10.5758,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 36.0173,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00936001,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 52.5138,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 378.263,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 56.2175,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 136.496,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 109.183,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 59.1649,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 145.485,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 555.21,
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
          "id": "8f8ac74b5ab46b4f34577a7919a078488c6a6d3e",
          "message": "Fix benchmarks fail problem",
          "timestamp": "2026-11-01T22:20:58+01:00",
          "tree_id": "cdb6815c022a3a8ab3775874ba4b78ba6cfb94b1",
          "url": "https://github.com/zLouis043/ZenitUI/commit/8f8ac74b5ab46b4f34577a7919a078488c6a6d3e"
        },
        "date": 1793568145113,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 192.796,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.27463,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00847152,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0367909,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0309372,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.49682,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.603984,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.545279,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.23142,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.878241,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 24.6269,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 30.4525,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 8.40605,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 11.7026,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 16.7262,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00447471,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 23.9988,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 207.755,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 25.348,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 65.8939,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 50.0569,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 27.1315,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 69.7882,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 266.471,
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
          "id": "02281f9b58eb73e6af3470f51721994fbdcfc4c8",
          "message": "Updated root README.md",
          "timestamp": "2026-11-01T22:23:47+01:00",
          "tree_id": "1c725bebbb6be031944716db9c0ab9b819f4ed8a",
          "url": "https://github.com/zLouis043/ZenitUI/commit/02281f9b58eb73e6af3470f51721994fbdcfc4c8"
        },
        "date": 1793568291413,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 370.07,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 5.91164,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0148237,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0713648,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0659046,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.95717,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.21102,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.03077,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.440323,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.15151,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 46.3545,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 58.7983,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.5927,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 10.2289,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 33.776,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.010573,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 47.488,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 377.969,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 50.0455,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 128.11,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 101.101,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 55.3948,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 138.219,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 525.519,
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
          "id": "8527bdc89c9b8e259917646b2c8e062e9c599b16",
          "message": "Added cache resolveFor theme, dirty mask for style transitions, invalidated descendant style cache on class/tag change. Added test",
          "timestamp": "2026-11-02T19:21:21+01:00",
          "tree_id": "300533816db070bb627f5556341f3f42ccc066f6",
          "url": "https://github.com/zLouis043/ZenitUI/commit/8527bdc89c9b8e259917646b2c8e062e9c599b16"
        },
        "date": 1793643796134,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 341.989,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.8615,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0131002,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.058727,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0460594,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.972317,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.1782,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.0846,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.491479,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.25276,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 49.5181,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 49.5459,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 16.8519,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 14.5199,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 35.8481,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00936933,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 51.3625,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 179.852,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 54.679,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 136.829,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 103.612,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 54.9899,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 124.147,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 550.679,
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
          "id": "26f5b02b3dc9c7619517b8bac27a7c98556870a9",
          "message": "perf(style): cache resolveFor + transition dirty mask + reverse dependency for class/tag invalidation",
          "timestamp": "2026-11-03T13:09:21+01:00",
          "tree_id": "36ebd797e0ba5193ee75fa850b177be5d69420a8",
          "url": "https://github.com/zLouis043/ZenitUI/commit/26f5b02b3dc9c7619517b8bac27a7c98556870a9"
        },
        "date": 1793707873067,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 368.249,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 6.89214,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0130697,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0600351,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0460432,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.969172,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.19329,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.07065,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.479997,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.3106,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 49.1626,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 50.0536,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 17.0361,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 13.9655,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 35.6253,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934068,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 50.2448,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 177.867,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 53.779,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 136.866,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 104.149,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 55.471,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 122.677,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 535.615,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 561.44,
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
          "id": "68473ca0792889464a91188857b06d97910f0344",
          "message": "Updated docs",
          "timestamp": "2026-11-06T10:48:12+01:00",
          "tree_id": "b080b935b55811c375d748a9d07ad12b8d166ef3",
          "url": "https://github.com/zLouis043/ZenitUI/commit/68473ca0792889464a91188857b06d97910f0344"
        },
        "date": 1793958611621,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 341.664,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 7.04787,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0146602,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0747509,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.065177,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 1.03173,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.20482,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.08915,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.477387,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.21613,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 49.3558,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 47.9538,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 15.8845,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 14.4089,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 33.4038,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0105767,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 48.1436,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 173.24,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 50.8195,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 125.873,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 100.557,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 51.4042,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 118.884,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 535.729,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 562.681,
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
          "id": "9d09bd765c17f4300f9ffda63f5959b007bd6b94",
          "message": "Done 5.2a of the ROADMAP.md, font fallback chain. Added tests, updated docs and mock backend.",
          "timestamp": "2026-11-06T12:42:06+01:00",
          "tree_id": "f58df144651043660c7fe58244da16531745634e",
          "url": "https://github.com/zLouis043/ZenitUI/commit/9d09bd765c17f4300f9ffda63f5959b007bd6b94"
        },
        "date": 1793965829007,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 187.54,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.88247,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00839228,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0402478,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0317201,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.547573,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.657839,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.575439,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.266528,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.93249,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 26.6106,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 24.2084,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 8.33411,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 17.8977,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 15.7516,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00445534,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 24.575,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 97.4244,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 25.9697,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 64.8107,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 49.6453,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 25.7682,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 57.7978,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 266.34,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 267.634,
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
          "id": "b6f815631685dd18039c46d90c8d96855cf5f711",
          "message": "Completed 5.2 of the ROADMAP.md. Added tests and updated docs",
          "timestamp": "2026-11-06T16:16:52+01:00",
          "tree_id": "70ef7a036db8c68adcc17bfca0d24e2a6fe86e39",
          "url": "https://github.com/zLouis043/ZenitUI/commit/b6f815631685dd18039c46d90c8d96855cf5f711"
        },
        "date": 1793978310791,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 370.615,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 7.15874,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0127784,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0600202,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0461206,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.998487,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.24943,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.0893,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.516064,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.3645,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 51.6182,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 53.3031,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 17.052,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 22.1758,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 36.9135,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00937273,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 53.0064,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 186.457,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 55.8228,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 142.745,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 106.512,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 58.0311,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 126.087,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 567.902,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 573.344,
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
          "id": "14a87238dd45ba9999745c29ae8641964120f465",
          "message": "Docs updated",
          "timestamp": "2026-11-06T16:23:35+01:00",
          "tree_id": "8f02af1ad3ce4ddd88159f64d984437f7839a035",
          "url": "https://github.com/zLouis043/ZenitUI/commit/14a87238dd45ba9999745c29ae8641964120f465"
        },
        "date": 1793978698045,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 368.942,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 7.11273,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0128258,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0600004,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0464018,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 1.00022,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.26109,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.07811,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.49842,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.33849,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 52.0241,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 51.7636,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 17.7441,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 19.6462,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 35.7706,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00934263,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 51.9651,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 187.296,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 54.3029,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 139.415,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 104.876,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 56.4777,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 121.61,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 546.251,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 561.865,
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
          "id": "5cdfa2ee4c31312e04ea1c1f320eca3e2f2cfa58",
          "message": "Docs updated",
          "timestamp": "2026-11-06T16:27:40+01:00",
          "tree_id": "38af5c41ee13d0267d9dabb03d0b678faf05ae06",
          "url": "https://github.com/zLouis043/ZenitUI/commit/5cdfa2ee4c31312e04ea1c1f320eca3e2f2cfa58"
        },
        "date": 1793979003523,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 367.639,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 7.17617,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0131748,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0591247,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0465563,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 1.00504,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.28311,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.10246,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.500143,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.3642,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 51.4934,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 51.6205,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 17.6048,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 17.8294,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 35.8393,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.0093366,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 52.6547,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 185.445,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 54.8147,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 139.226,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 104.031,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 56.348,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 121.178,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 560.612,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 560.625,
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
          "id": "257a9962a4d036d670dee4297e483c4d1cea0ef4",
          "message": "Updated PROPOSALS.md",
          "timestamp": "2026-11-06T16:38:17+01:00",
          "tree_id": "76edf04a300aeab3ab6f48816aa364de9d193446",
          "url": "https://github.com/zLouis043/ZenitUI/commit/257a9962a4d036d670dee4297e483c4d1cea0ef4"
        },
        "date": 1793979607724,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 189.776,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.98851,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0085422,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0404531,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0324228,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.551111,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.664929,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.577457,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.265199,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.922209,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 27.875,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 24.6825,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 8.67151,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 18.6931,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 15.5483,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00453393,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 23.7606,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 111.743,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 25.0964,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 66.9445,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 49.325,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 26.2575,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 58.3342,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 269.794,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 278.488,
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
          "id": "d0eddd1624816377b0e4034a8217496d4ba3d42d",
          "message": "Reworking testing infrastructure",
          "timestamp": "2026-11-06T20:19:59+01:00",
          "tree_id": "3ed132e5cdf0986360b19e2e2fdb00a1ab0f8f37",
          "url": "https://github.com/zLouis043/ZenitUI/commit/d0eddd1624816377b0e4034a8217496d4ba3d42d"
        },
        "date": 1793992864328,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 179.392,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.70874,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00799089,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0380573,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0297828,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.528089,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.639783,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.557647,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.270911,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.894528,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 27.2611,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 24.2789,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 8.84623,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 19.0979,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 15.7058,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00455096,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 23.4743,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 91.0642,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 24.377,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 65.7885,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 47.9424,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 25.7975,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 58.3151,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 264.414,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 275.597,
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
          "id": "ba94ad0b2780ee5733a01f126677b64c7430cf6b",
          "message": "Cleaned build files",
          "timestamp": "2026-11-06T21:15:19+01:00",
          "tree_id": "829f4f80581eeb0c540e275f6d08f2f31824f0fd",
          "url": "https://github.com/zLouis043/ZenitUI/commit/ba94ad0b2780ee5733a01f126677b64c7430cf6b"
        },
        "date": 1793996216260,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 176.635,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.67094,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00799682,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0376889,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0305298,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.518347,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.625598,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.567269,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.26061,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.917604,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 27.176,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 22.779,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 7.94915,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 14.6075,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 15.0514,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00451891,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 22.8293,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 86.6862,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 23.6225,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 63.5566,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 47.579,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 25.0279,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 58.019,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 261.774,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 267.802,
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
          "id": "4f8eb0591380733e1c36051b1f1a9b972e185b07",
          "message": "Added virtual view",
          "timestamp": "2026-11-06T21:49:30+01:00",
          "tree_id": "7f0dc0c14bafdd693b4dfdaac916f634b2f087eb",
          "url": "https://github.com/zLouis043/ZenitUI/commit/4f8eb0591380733e1c36051b1f1a9b972e185b07"
        },
        "date": 1793998255389,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 377.954,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 7.47791,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0130997,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0593934,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0460447,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 1.04759,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.27555,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 1.13737,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.531844,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.37373,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 54.0558,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 53.6487,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 16.8029,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 19.3109,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 36.5406,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00920809,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 53.4599,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 194.904,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 57.9399,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 147.27,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 108.649,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 56.3334,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 126.969,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 563.475,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 581.152,
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
          "id": "212b9ecca268c621a15c976b5583c30981f51627",
          "message": "Updated docs for 5.24 of the ROADMAP.md",
          "timestamp": "2026-11-06T22:23:47+01:00",
          "tree_id": "8a6609259ed9e557888e179bcd2d10a9cfd88398",
          "url": "https://github.com/zLouis043/ZenitUI/commit/212b9ecca268c621a15c976b5583c30981f51627"
        },
        "date": 1794000641422,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 275.182,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 5.79488,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.0113227,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0578962,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0501856,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.832735,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 1.01658,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.872005,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.39563,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 1.21458,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 41.0352,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 39.5411,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 12.3998,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 18.3467,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 24.9813,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00819454,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 38.033,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 143.906,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 39.5448,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 106.118,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 79.3166,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 41.4853,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 95.9111,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 426.873,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 443.184,
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
          "id": "aa8d29c584ec60573c766e88a10449e7efafbf1f",
          "message": "Fix: renderChrome before pushCLip in the overflow nodes. The border of th nodes with overflow:auto/hidden/scroll was clipped away by pushClip, called in drawInline before renderChrome, making visible only a portion of the rounded angles.",
          "timestamp": "2026-11-08T13:07:41+01:00",
          "tree_id": "3a822bd861878942618ee1669fccba1993544bca",
          "url": "https://github.com/zLouis043/ZenitUI/commit/aa8d29c584ec60573c766e88a10449e7efafbf1f"
        },
        "date": 1794139779888,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 346.747,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.89349,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00435958,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0369476,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0260943,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.69723,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.885784,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.813713,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.421068,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.769528,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 41.8366,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 47.0402,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 14.4494,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 12.7976,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 36.5382,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00373235,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 49.6507,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 156.966,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 51.6471,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 127.528,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 102.212,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 52.6159,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 114.112,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 529.319,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 549.904,
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
          "id": "65752c71e441f199dbd580f2d34945e3614271b6",
          "message": "Fix: subtract border-width from the content box in arrange and measure.",
          "timestamp": "2026-11-08T13:14:33+01:00",
          "tree_id": "1f03a67f2466925d335ae1cf27390013c570f057",
          "url": "https://github.com/zLouis043/ZenitUI/commit/65752c71e441f199dbd580f2d34945e3614271b6"
        },
        "date": 1794140192699,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 351.274,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 4.89703,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00435916,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0369695,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.02615,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.700101,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.872359,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.820323,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.423174,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.769193,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 40.9399,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 48.5655,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 16.5907,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 15.7097,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 37.3183,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00373093,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 48.6704,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 153.879,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 53.0261,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 127.7,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 99.1552,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 52.8021,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 119.28,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 539.272,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 546.713,
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
          "id": "d9c4398f145d2e4fd2826f496114d4c5682f61de",
          "message": "Add todo for css semantics migration",
          "timestamp": "2026-11-08T14:38:28+01:00",
          "tree_id": "c410d564a9948b96c90605f9075716ffa659124a",
          "url": "https://github.com/zLouis043/ZenitUI/commit/d9c4398f145d2e4fd2826f496114d4c5682f61de"
        },
        "date": 1794145217591,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "parse.zstyle.200rules",
            "value": 256,
            "unit": "us"
          },
          {
            "name": "parse.zmarkup.100elements",
            "value": 3.67015,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_1term",
            "value": 0.00354404,
            "unit": "us"
          },
          {
            "name": "micro.Value.construct_2term",
            "value": 0.0355228,
            "unit": "us"
          },
          {
            "name": "micro.Value.lerp",
            "value": 0.0262984,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from",
            "value": 0.526092,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.copy",
            "value": 0.691566,
            "unit": "us"
          },
          {
            "name": "micro.ComputedStyle.from_with_parent",
            "value": 0.592862,
            "unit": "us"
          },
          {
            "name": "micro.evaluateKeyframes",
            "value": 0.301851,
            "unit": "us"
          },
          {
            "name": "micro.tickCss.100",
            "value": 0.996896,
            "unit": "us"
          },
          {
            "name": "micro.tickCssAndOverlay.100",
            "value": 29.5664,
            "unit": "us"
          },
          {
            "name": "micro.tickImperative.100",
            "value": 32.3867,
            "unit": "us"
          },
          {
            "name": "micro.tree.addChild.1000",
            "value": 11.8921,
            "unit": "us"
          },
          {
            "name": "micro.tree.removeAllChildren.1000",
            "value": 10.8208,
            "unit": "us"
          },
          {
            "name": "micro.tree.drawChildren.500",
            "value": 25.7805,
            "unit": "us"
          },
          {
            "name": "micro.SmallVector.push_back_inline",
            "value": 0.00326865,
            "unit": "us"
          },
          {
            "name": "system.frame.static100",
            "value": 31.7226,
            "unit": "us"
          },
          {
            "name": "system.frame.200rules",
            "value": 108.244,
            "unit": "us"
          },
          {
            "name": "system.frame.transitions100",
            "value": 34.1543,
            "unit": "us"
          },
          {
            "name": "system.frame.animated100",
            "value": 84.5458,
            "unit": "us"
          },
          {
            "name": "system.frame.wide200",
            "value": 69.0005,
            "unit": "us"
          },
          {
            "name": "system.frame.layout_stress",
            "value": 34.5803,
            "unit": "us"
          },
          {
            "name": "system.frame.mixed200",
            "value": 80.4476,
            "unit": "us"
          },
          {
            "name": "system.frame.large1000",
            "value": 364.879,
            "unit": "us"
          },
          {
            "name": "system.frame.classChange.1000",
            "value": 392.122,
            "unit": "us"
          }
        ]
      }
    ]
  }
}