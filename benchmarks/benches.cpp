#include "bench_framework.hpp"

#include "Mocks.hpp"
#include "ZenitUI.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

namespace Bench
{
    float g_sink = 0.0f;
}

namespace
{

    std::string makeCss(int n)
    {
        std::string css;
        css.reserve(n * 40);
        for (int i = 0; i < n; ++i)
        {
            css += "Tag";
            css += std::to_string(i);
            css += " { color: #000000ff; }\n";
        }
        return css;
    }

    std::shared_ptr<Layout> makeFlatTree(int n)
    {
        auto root = std::make_shared<Layout>(LayoutType::Vertical);
        root->size(Percent(100), Percent(100));

        for (int i = 0; i < n; ++i)
        {
            auto n_ = std::make_shared<Layout>(LayoutType::Stack);
            n_->setStyleTag("Tag" + std::to_string(i % 200));
            n_->size(Px(20), Px(10));
            root->addChild(n_);
        }
        return root;
    }

    std::shared_ptr<Layout> makeFiltered50Tree(bool withDropShadow)
    {
        auto root = std::make_shared<Layout>(LayoutType::Stack);
        root->size(Percent(100), Percent(100));

        std::vector<FilterRef> chain;
        chain.push_back({"blur", {"4px"}});
        if (withDropShadow)
            chain.push_back({"drop-shadow", {"2px", "2px", "#000"}});

        for (int i = 0; i < 50; ++i)
        {
            auto card = std::make_shared<Layout>(LayoutType::Stack);
            card->size(Px(200), Px(200));
            card->getInlineBase().translateX = Px(100.0f);
            card->getInlineBase().translateY = Px(100.0f);
            card->getInlineBase().filters = chain;
            root->addChild(card);
        }
        return root;
    }

    void bindFilterBenchEnv(Env &env)
    {
        env.reset();
        env.renderer.supportsEffects = true;
        env.renderer.supportsTargetSubRect = true;
        env.renderer.currentClip = {0.0f, 0.0f, 800.0f, 600.0f};
        env.assets.registerEffect("blur");
        env.assets.registerEffect("dropShadow");
    }

    // Shared state for benchmarks whose body must operate on pre-built data.
    // The setup() callback rebuilds this state before every run; the body()
    // then measures only the operation of interest, not the construction.

    Theme &sharedBenchTheme()
    {
        static Theme t;
        return t;
    }

    std::vector<ActiveCssAnimation> &sharedTickCssAnims()
    {
        static std::vector<ActiveCssAnimation> v;
        return v;
    }

    std::vector<std::shared_ptr<Layout>> &sharedImperativeNodes()
    {
        static std::vector<std::shared_ptr<Layout>> v;
        return v;
    }

    std::vector<std::shared_ptr<Layout>> &sharedTreeAddNodes()
    {
        static std::vector<std::shared_ptr<Layout>> v;
        return v;
    }

    std::shared_ptr<Layout> &sharedTreeRemoveRoot()
    {
        static std::shared_ptr<Layout> v;
        return v;
    }

} // namespace

namespace Bench
{

    void registerAll(Runner &R)
    {
        // =======================================================================
        //  Parsing
        // =======================================================================

        R.add("parse.zstyle.200rules", "parsing",
              /*iterations=*/50,
              /*setup=*/[] {},
              /*body=*/[]
              {
              auto &theme = sharedBenchTheme();
              ZMarkup::loadStyleString(makeCss(200), theme);
              theme.clear(); });

        R.add("parse.zmarkup.100elements", "parsing",
              /*iterations=*/200,
              /*setup=*/[]
              {
                  // Pre-build the source string once; we measure parsing, not
                  // string concatenation.
              },
              /*body=*/[]
              {
              static const std::string src = []{
                  std::string s;
                  for (int i = 0; i < 100; ++i)
                      s += "Button#b" + std::to_string(i) + " \"Item\"\n";
                  return s;
              }();
              auto ui = ZMarkup::build(src);
              g_sink += (float)(ui.root() ? 1 : 0); });

        // =======================================================================
        //  Value
        // =======================================================================

        R.add("micro.Value.construct_1term", "value",
              /*iterations=*/1'000'000,
              /*setup=*/[] {},
              /*body=*/[]
              {
              static int i = 0;
              Value v = Value::px(100.0f + (float)(i++ & 0xFF));
              g_sink += v.terms[0].coeff; });

        R.add("micro.Value.construct_2term", "value",
              /*iterations=*/1'000'000,
              /*setup=*/[] {},
              /*body=*/[]
              {
              static int i = 0;
              Value v = Value::percent(100.0f) - Value::px(20.0f + (float)(i++ & 0xF));
              g_sink += v.terms[0].coeff; });

        R.add("micro.Value.lerp", "value",
              /*iterations=*/1'000'000,
              /*setup=*/[] {},
              /*body=*/[]
              {
              static int i = 0;
              Value a = Value::px(0.0f);
              Value b = Value::percent(100.0f);
              Value c = lerpProp(a, b, (float)(i++ & 0xFF) / 255.0f);
              g_sink += c.terms[0].coeff; });

        // =======================================================================
        //  ComputedStyle
        // =======================================================================

        R.add("micro.ComputedStyle.from", "computed_style",
              /*iterations=*/200'000,
              /*setup=*/[] {},
              /*body=*/[]
              {
              static int i = 0;
              Style s;
              s.width    = Value::px(100.0f + (float)(i++ & 0xF));
              s.height   = Value::px(50.0f);
              s.padding  = Spacing(Value::px(8.0f));
              s.margin   = Spacing(Value::px(4.0f));
              s.fontSize = Value::px(16.0f);
              ComputedStyle c = ComputedStyle::from(s, nullptr, nullptr);
              g_sink += c.width.terms[0].coeff; });

        R.add("micro.ComputedStyle.copy", "computed_style",
              /*iterations=*/200'000,
              /*setup=*/[] {},
              /*body=*/[]
              {
              static int i = 0;
              Style s;
              s.width    = Value::px(100.0f + (float)(i++ & 0xF));
              s.height   = Value::px(50.0f);
              ComputedStyle a = ComputedStyle::from(s, nullptr, nullptr);
              ComputedStyle b = a;
              g_sink += b.width.terms[0].coeff; });

        R.add("micro.ComputedStyle.from_with_parent", "computed_style",
              /*iterations=*/200'000,
              /*setup=*/[]
              {
                  // Un parent "ragionevole" con alcune proprietà ereditate settate.
              },
              /*body=*/[]
              {
          static int i = 0;
          static const ComputedStyle parent = [] {
              ComputedStyle p;
              p.font = "calibri";
              p.color = Color{200, 200, 200, 255};
              p.fontSize = Value::px(16.0f);
              p.letterSpacing = Value::px(1.0f);
              p.textAlign = Align::Start;
              return p;
          }();

          Style s;
          // Solo proprietà non ereditate, così che ognuna di esse
          // debba passare attraverso isInheritedProp quando resolveFor
          // consulta il parent.
          s.width    = Value::px(100.0f + (float)(i++ & 0xF));
          s.height   = Value::px(50.0f);
          s.padding  = Spacing(Value::px(8.0f));
          s.margin   = Spacing(Value::px(4.0f));
          s.background = Color{40, 40, 45, 255};

          ComputedStyle c = ComputedStyle::from(s, &parent, nullptr);
          g_sink += c.width.terms[0].coeff; });

        // =======================================================================
        //  Animation
        // =======================================================================

        R.add("micro.evaluateKeyframes", "animation",
              /*iterations=*/200'000,
              /*setup=*/[]
              {
              auto &theme = sharedBenchTheme();
              theme.clear();
              ZMarkup::loadStyleString(R"(
                  @keyframes k {
                      0%   { opacity: 0.0; scale: 1.0; }
                      50%  { opacity: 1.0; scale: 1.2; }
                      100% { opacity: 0.0; scale: 1.0; }
                  }
              )", theme); },
              /*body=*/[]
              {
              static int i = 0;
              const auto& anim = sharedBenchTheme().keyframes.at("k");
              float t = (float)(i++ & 0xFF) / 255.0f;
              Style s = evaluateKeyframes(anim, t, TransitionFunction::Linear);
              g_sink += s.opacity.is_set ? s.opacity.value : 0.0f; });

        R.add("micro.tickCss.100", "animation",
              /*iterations=*/1000,
              /*setup=*/[]
              {
              auto &theme = sharedBenchTheme();
              theme.clear();
              ZMarkup::loadStyleString(R"(
                  @keyframes k { 0% { opacity: 0.0; } 100% { opacity: 1.0; } }
              )", theme);
              // Reset the shared vector to 100 fresh animations.
              auto& anims = sharedTickCssAnims();
              anims.clear();
              for (int i = 0; i < 100; ++i)
              {
                  ActiveCssAnimation a;
                  a.name       = "k";
                  a.duration   = 1.0f;
                  a.iterations = -1;
                  a.elapsed    = 0.5f;
                  anims.push_back(a);
              } },
              /*body=*/[]
              {
              Anim::tickIncremental(sharedTickCssAnims(), 1.0f / 60.0f, sharedBenchTheme());
              g_sink += (float)sharedTickCssAnims().size(); });

        R.add("micro.tickCssAndOverlay.100", "animation",
              /*iterations=*/1000,
              /*setup=*/[]
              {
          auto &theme = sharedBenchTheme();
          theme.clear();
          ZMarkup::loadStyleString(R"(
              @keyframes k { 0% { opacity: 0.0; } 100% { opacity: 1.0; } }
          )", theme);
          auto& anims = sharedTickCssAnims();
          anims.clear();
          for (int i = 0; i < 100; ++i)
          {
              ActiveCssAnimation a;
              a.name       = "k";
              a.duration   = 1.0f;
              a.iterations = -1;
              a.elapsed    = 0.5f;
              anims.push_back(a);
          } },
              /*body=*/[]
              {
          auto& anims = sharedTickCssAnims();
          const auto &theme = sharedBenchTheme();
          // Tick: advances elapsed and populates the cache.
          Anim::tickIncremental(anims, 1.0f / 60.0f, theme);
          // Overlay: reads the cache instead of recomputing sampleActive.
          ComputedStyle out;
          Anim::overlayCssComputed(out, anims, theme);
          g_sink += (float)anims.size(); });

        {
            static UIContext imperativeCtx;

            R.add("micro.tickImperative.100", "animation",
                  /*iterations=*/500,
                  /*setup=*/[&]
                  {
                  imperativeCtx.detachAllRoots();
                  auto& nodes = sharedImperativeNodes();
                  nodes.clear();
                  nodes.reserve(100);
                  for (int i = 0; i < 100; ++i)
                  {
                      auto n = std::make_shared<Layout>(LayoutType::Stack);
                      imperativeCtx.mount(n);
                      auto anim = std::make_shared<UIAnimation>(1.0f);
                      anim->addTrack<float>(0.0f, 1.0f,
                          [](Layout* l, float v) { l->getInlineBase().opacity = v; });
                      n->addAnimation("fade", anim);
                      n->playAnimation("fade");
                      nodes.push_back(n);
                  } },
                  /*body=*/[]
                  {
                  for (auto& n : sharedImperativeNodes())
                      n->update(1.0f / 60.0f, false);
                  g_sink += (float)sharedImperativeNodes().size(); });
        }

        // =======================================================================
        //  Tree
        // =======================================================================

        R.add("micro.tree.addChild.1000", "tree",
              /*iterations=*/200,
              /*setup=*/[]
              {
              auto& nodes = sharedTreeAddNodes();
              nodes.clear();
              nodes.reserve(1000);
              for (int i = 0; i < 1000; ++i)
                  nodes.push_back(std::make_shared<Layout>(LayoutType::Stack)); },
              /*body=*/[]
              {
              auto root = std::make_shared<Layout>(LayoutType::Vertical);
              for (auto& c : sharedTreeAddNodes())
                  root->addChild(c);
              g_sink += (float)root->children.size(); });

        R.add("micro.tree.removeAllChildren.1000", "tree",
              /*iterations=*/200,
              /*setup=*/[]
              {
              auto& root = sharedTreeRemoveRoot();
              root = std::make_shared<Layout>(LayoutType::Vertical);
              for (int i = 0; i < 1000; ++i)
                  root->addChild(std::make_shared<Layout>(LayoutType::Stack)); },
              /*body=*/[]
              {
              sharedTreeRemoveRoot()->removeAllChildren();
              g_sink += (float)sharedTreeRemoveRoot()->children.size(); });

        {
            static Env drawEnv;

            R.add("micro.tree.drawChildren.500", "tree",
                  /*iterations=*/500,
                  /*setup=*/[&]
                  {
              drawEnv.reset();
              // Root con 500 figli, tutti con z-index esplicito così
              // finiscono nei bucket posZ/negZ (che sono i due vettori
              // "nuovi" che vogliamo verificare).
              auto& root = sharedTreeRemoveRoot();   // riusa lo slot già esistente
              root = std::make_shared<Layout>(LayoutType::Stack);
              root->size(Percent(100), Percent(100));

              for (int i = 0; i < 500; ++i)
              {
                  auto n = std::make_shared<Layout>(LayoutType::Stack);
                  n->size(Px(20), Px(20));
                  n->getInlineBase().position = Position::Relative;
                  n->getInlineBase().zIndex   = ZIndex(i % 10);
                  root->addChild(n);
              }

              drawEnv.ctx.mount(root);
              // Un measure+arrange preliminare, così draw non causa
              // resize del subtree.
              root->measure(800, 600);
              root->arrange({0, 0, 800, 600}); },
                  /*body=*/[]
                  {
              auto& root = sharedTreeRemoveRoot();
              // draw() chiama drawChildren() che riusa i vettori membri.
              root->draw();
              g_sink += (float)root->children.size(); });
        }

        // =======================================================================
        //  SmallVector
        // =======================================================================

        R.add("micro.SmallVector.push_back_inline", "small_vector",
              /*iterations=*/1'000'000,
              /*setup=*/[] {},
              /*body=*/[]
              {
              static int i = 0;
              SmallVector<int, 2> v;
              v.push_back(i++);
              v.push_back(i++);
              g_sink += (float)v[0]; });

        // =======================================================================
        //  Frame
        // =======================================================================

        {
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 500;

            R.add("system.frame.static100", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                  env.reset();
                  root = makeFlatTree(100);
                  for (int i = 0; i < 5; ++i) env.frame(root); },
                  /*body=*/[&]
                  {
                  env.frame(root);
                  g_sink += (float)root->children.size(); });
        }

        {
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 200;

            R.add("system.frame.200rules", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                  env.reset();
                  ZMarkup::loadStyleString(makeCss(200), env.ctx.theme);
                  root = makeFlatTree(100);
                  for (int i = 0; i < 5; ++i) env.frame(root); },
                  /*body=*/[&]
                  {
                  static int i = 0;
                  float op = (i++ % 2) ? 1.0f : 0.99f;
                  for (auto& c : root->children)
                      c->getInlineBase().opacity = op;
                  env.frame(root);
                  g_sink += (float)root->children.size(); });
        }

        {
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 500;

            R.add("system.frame.transitions100", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                  env.reset();
                  ZMarkup::loadStyleString(R"(
                      .row { background: #202020ff;
                             transition: background 0.2s linear; }
                      .row:hover { background: #4a9effff; }
                  )", env.ctx.theme);
                  root = std::make_shared<Layout>(LayoutType::Vertical);
                  root->size(Percent(100), Percent(100));
                  for (int i = 0; i < 100; ++i)
                  {
                      auto n = std::make_shared<Layout>(LayoutType::Stack);
                      n->cls("row");
                      n->size(Px(100), Px(20));
                      n->setInteractive(true);
                      n->setBlocksRaycast(true);
                      root->addChild(n);
                  }
                  for (int i = 0; i < 5; ++i) env.frame(root); },
                  /*body=*/[&]
                  {
                  static int i = 0;
                  float y = (i++ % 2 == 0) ? 10.0f : 30.0f;
                  env.platform.moveMouse({50.0f, y});
                  env.frame(root);
                  g_sink += (float)root->children.size(); });
        }

        {
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 500;

            R.add("system.frame.animated100", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                  env.reset();
                  ZMarkup::loadStyleString(R"(
                      @keyframes pulse { 0% { opacity: 0.5; }
                                         50% { opacity: 1.0; }
                                         100% { opacity: 0.5; } }
                      .pulse { animation: pulse 1s linear infinite; }
                  )", env.ctx.theme);
                  root = std::make_shared<Layout>(LayoutType::Vertical);
                  root->size(Percent(100), Percent(100));
                  for (int i = 0; i < 100; ++i)
                  {
                      auto n = std::make_shared<Layout>(LayoutType::Stack);
                      n->cls("pulse");
                      n->size(Px(100), Px(20));
                      root->addChild(n);
                  }
                  for (int i = 0; i < 5; ++i) env.frame(root); },
                  /*body=*/[&]
                  {
                  env.frame(root);
                  g_sink += (float)root->children.size(); });
        }

        {
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 500;

            R.add("system.frame.wide200", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                  env.reset();
                  root = std::make_shared<Layout>(LayoutType::Stack);
                  root->size(Percent(100), Percent(100));
                  for (int i = 0; i < 200; ++i)
                  {
                      auto n = std::make_shared<Layout>(LayoutType::Stack);
                      n->size(Px(20), Px(20));
                      n->getInlineBase().position = Position::Relative;
                      n->getInlineBase().zIndex   = ZIndex(i % 10);
                      root->addChild(n);
                  }
                  for (int i = 0; i < 5; ++i) env.frame(root); },
                  /*body=*/[&]
                  {
                  env.frame(root);
                  g_sink += (float)root->children.size(); });
        }

        // -----------------------------------------------------------------------
        //  Frame — extra scenarios (added after review)
        // -----------------------------------------------------------------------

        {
            // Layout stress: force a real re-measure + re-arrange every frame
            // by changing the container width. This exercises the full layout
            // algorithm (measure + arrange), not just the tree traversal
            // (which is what system.frame.static100 measures).
            static Env env;
            static std::shared_ptr<Layout> root;
            static std::shared_ptr<Layout> container;
            constexpr int FRAMES = 500;

            R.add("system.frame.layout_stress", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                  env.reset();

                  root = std::make_shared<Layout>(LayoutType::Stack);
                  root->size(Percent(100), Percent(100));

                  container = std::make_shared<Layout>(LayoutType::Vertical);
                  container->size(Px(800), Px(600));
                  root->addChild(container);

                  for (int i = 0; i < 100; ++i)
                  {
                      auto n = std::make_shared<Layout>(LayoutType::Stack);
                      n->size(Percent(50), Px(20));
                      container->addChild(n);
                  }
                  for (int i = 0; i < 5; ++i) env.frame(root); },
                  /*body=*/[&]
                  {
                  static int i = 0;
                  float w = 800.0f + (float)(i++ % 2);
                  container->getInlineBase().width = Px(w);
                  env.frame(root);
                  g_sink += (float)container->children.size(); });
        }

        {
            // Mixed 200: 1 animated, 1 hovered, 10 dirty, 188 static.
            // This is the shape of a real frame in an app that has one
            // continuously animated element, one hovered control, and a
            // handful of nodes whose content changed this frame.
            static Env env;
            static std::shared_ptr<Layout> root;
            static std::vector<std::shared_ptr<Layout>> dirtyNodes;
            constexpr int FRAMES = 500;

            R.add("system.frame.mixed200", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                  env.reset();
                  ZMarkup::loadStyleString(R"(
                      @keyframes pulse {
                          0%   { opacity: 0.7; }
                          50%  { opacity: 1.0; }
                          100% { opacity: 0.7; }
                      }
                      .pulse { animation: pulse 1s linear infinite; }
                      .row { background: #202020ff; }
                      .row:hover { background: #4a9effff; }
                  )", env.ctx.theme);

                  root = std::make_shared<Layout>(LayoutType::Vertical);
                  root->size(Percent(100), Percent(100));

                  dirtyNodes.clear();
                  dirtyNodes.reserve(10);

                  for (int i = 0; i < 200; ++i)
                  {
                      auto n = std::make_shared<Layout>(LayoutType::Stack);
                      n->cls("row");
                      n->size(Px(100), Px(20));

                      if (i == 0)
                          n->cls("pulse");                     // 1 animated
                      if (i >= 1 && i <= 10)
                          dirtyNodes.push_back(n);             // 10 dirty

                      root->addChild(n);
                  }

                  for (int i = 0; i < 5; ++i) env.frame(root);
                  env.platform.moveMouse({50.0f, 10.0f});      // hover on first
                  env.frame(root); },
                  /*body=*/[&]
                  {
                  static int i = 0;
                  float op = (i++ % 2) ? 1.0f : 0.95f;
                  for (auto& n : dirtyNodes)
                      n->getInlineBase().opacity = op;
                  env.frame(root);
                  g_sink += (float)root->children.size(); });
        }

        {
            // Large 1000: an app with a long list. Frustum culling skips
            // most of them. Measures the "iterate 1000 nodes, cache-hit
            // measure, cull draw" path — the realistic case for a
            // scrollable list of items.
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 200;

            R.add("system.frame.large1000", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                  env.reset();

                  root = std::make_shared<Layout>(LayoutType::Vertical);
                  root->size(Percent(100), Percent(100));

                  for (int i = 0; i < 1000; ++i)
                  {
                      auto n = std::make_shared<Layout>(LayoutType::Stack);
                      n->size(Px(100), Px(10));
                      root->addChild(n);
                  }
                  for (int i = 0; i < 5; ++i) env.frame(root); },
                  /*body=*/[&]
                  {
                  env.frame(root);
                  g_sink += (float)root->children.size(); });
        }

        {
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 200;

            R.add("system.frame.classChange.1000", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                  env.reset();
                  ZMarkup::loadStyleString(R"(
                      .btn { position: absolute; width: 10px; height: 10px; }
                      .panel .btn { left: 100px; }
                  )", env.ctx.theme);

                  root = std::make_shared<Layout>(LayoutType::Stack);
                  root->size(Percent(100), Percent(100));

                  for (int i = 0; i < 1000; ++i)
                  {
                      auto n = std::make_shared<Layout>(LayoutType::Stack);
                      n->size(Px(10), Px(10));
                      if (i < 5)
                          n->cls("btn");
                      root->addChild(n);
                  }
                  for (int i = 0; i < 5; ++i)
                      env.frame(root); },
                  /*body=*/[&]
                  {
                  static int i = 0;
                  if (i % 2 == 0) root->addClass("panel");
                  else            root->removeClass("panel");
                  ++i;
                  env.frame(root);
                  g_sink += (float)root->children.size(); });
        }

        {
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 200;

            R.add("system.frame.filtered50.static", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                      bindFilterBenchEnv(env);
                      root = makeFiltered50Tree(/*withDropShadow=*/true);
                      for (int i = 0; i < 5; ++i)
                          env.frame(root);
                  },
                  /*body=*/[&]
                  {
                      env.frame(root);
                      g_sink += (float)env.ctx.renderStats.pushTargetCalls;
                  });
        }

        {
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 200;

            R.add("system.frame.filtered50.scaleAnim", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                      bindFilterBenchEnv(env);
                      root = makeFiltered50Tree(/*withDropShadow=*/true);
                      for (int i = 0; i < 5; ++i)
                          env.frame(root);
                  },
                  /*body=*/[&]
                  {
                      static int i = 0;
                      float s = 1.0f + 0.15f * ((float)(i++ % 60) / 59.0f);
                      for (auto &c : root->children)
                          c->getInlineBase().scale = s;
                      env.frame(root);
                      g_sink += (float)env.ctx.renderStats.pushTargetCalls;
                  });
        }

        {
            static Env env;
            static std::shared_ptr<Layout> root;
            constexpr int FRAMES = 200;

            R.add("system.frame.filtered50.rotAnim", "frame", FRAMES,
                  /*setup=*/[&]
                  {
                      bindFilterBenchEnv(env);
                      root = makeFiltered50Tree(/*withDropShadow=*/false);
                      for (int i = 0; i < 5; ++i)
                          env.frame(root);
                  },
                  /*body=*/[&]
                  {
                      static int i = 0;
                      float r = (float)((i++ * 3) % 360);
                      for (auto &c : root->children)
                          c->getInlineBase().rotation = r;
                      env.frame(root);
                      g_sink += (float)env.ctx.renderStats.pushTargetCalls;
                  });
        }
    }

} // namespace Bench