window.BENCHMARK_DATA = {
  "lastUpdate": 1792406732249,
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
      }
    ]
  }
}