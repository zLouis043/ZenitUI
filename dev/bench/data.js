window.BENCHMARK_DATA = {
  "lastUpdate": 1791807071866,
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
      }
    ]
  }
}