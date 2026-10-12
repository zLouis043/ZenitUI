#include "bench_framework.hpp"

#include "Mocks.hpp"
#include "UI.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

namespace Bench {
    float g_sink = 0.0f;
}

namespace {

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

// Shared state for benchmarks whose body must operate on pre-built data.
// The setup() callback rebuilds this state before every run; the body()
// then measures only the operation of interest, not the construction.

std::vector<ActiveCssAnimation>& sharedTickCssAnims()
{
    static std::vector<ActiveCssAnimation> v;
    return v;
}

std::vector<std::shared_ptr<Layout>>& sharedImperativeNodes()
{
    static std::vector<std::shared_ptr<Layout>> v;
    return v;
}

std::vector<std::shared_ptr<Layout>>& sharedTreeAddNodes()
{
    static std::vector<std::shared_ptr<Layout>> v;
    return v;
}

std::shared_ptr<Layout>& sharedTreeRemoveRoot()
{
    static std::shared_ptr<Layout> v;
    return v;
}

} // namespace

namespace Bench {

void registerAll(Runner& R)
{
    // =======================================================================
    //  Parsing
    // =======================================================================

    R.add("parse.zstyle.200rules", "parsing",
          /*iterations=*/50,
          /*setup=*/[]{},
          /*body=*/ []{
              ZMarkup::loadStyleString(makeCss(200));
              Theme::get().clear();
          });

    R.add("parse.zmarkup.100elements", "parsing",
          /*iterations=*/200,
          /*setup=*/[]{
              // Pre-build the source string once; we measure parsing, not
              // string concatenation.
          },
          /*body=*/ []{
              static const std::string src = []{
                  std::string s;
                  for (int i = 0; i < 100; ++i)
                      s += "Button#b" + std::to_string(i) + " \"Item\"\n";
                  return s;
              }();
              auto ui = ZMarkup::build(src);
              g_sink += (float)(ui.root() ? 1 : 0);
          });

    // =======================================================================
    //  Value
    // =======================================================================

    R.add("micro.Value.construct_1term", "value",
          /*iterations=*/1'000'000,
          /*setup=*/[]{},
          /*body=*/ []{
              static int i = 0;
              Value v = Value::px(100.0f + (float)(i++ & 0xFF));
              g_sink += v.terms[0].coeff;
          });

    R.add("micro.Value.construct_2term", "value",
          /*iterations=*/1'000'000,
          /*setup=*/[]{},
          /*body=*/ []{
              static int i = 0;
              Value v = Value::percent(100.0f) - Value::px(20.0f + (float)(i++ & 0xF));
              g_sink += v.terms[0].coeff;
          });

    R.add("micro.Value.lerp", "value",
          /*iterations=*/1'000'000,
          /*setup=*/[]{},
          /*body=*/ []{
              static int i = 0;
              Value a = Value::px(0.0f);
              Value b = Value::percent(100.0f);
              Value c = lerpProp(a, b, (float)(i++ & 0xFF) / 255.0f);
              g_sink += c.terms[0].coeff;
          });

    // =======================================================================
    //  ComputedStyle
    // =======================================================================

    R.add("micro.ComputedStyle.from", "computed_style",
          /*iterations=*/200'000,
          /*setup=*/[]{},
          /*body=*/ []{
              static int i = 0;
              Style s;
              s.width    = Value::px(100.0f + (float)(i++ & 0xF));
              s.height   = Value::px(50.0f);
              s.padding  = Spacing(Value::px(8.0f));
              s.margin   = Spacing(Value::px(4.0f));
              s.fontSize = Value::px(16.0f);
              ComputedStyle c = ComputedStyle::from(s, nullptr, nullptr);
              g_sink += c.width.terms[0].coeff;
          });

    R.add("micro.ComputedStyle.copy", "computed_style",
          /*iterations=*/200'000,
          /*setup=*/[]{},
          /*body=*/ []{
              static int i = 0;
              Style s;
              s.width    = Value::px(100.0f + (float)(i++ & 0xF));
              s.height   = Value::px(50.0f);
              ComputedStyle a = ComputedStyle::from(s, nullptr, nullptr);
              ComputedStyle b = a;
              g_sink += b.width.terms[0].coeff;
          });

    // =======================================================================
    //  Animation
    // =======================================================================

    R.add("micro.evaluateKeyframes", "animation",
          /*iterations=*/200'000,
          /*setup=*/[]{
              Theme::get().clear();
              ZMarkup::loadStyleString(R"(
                  @keyframes k {
                      0%   { opacity: 0.0; scale: 1.0; }
                      50%  { opacity: 1.0; scale: 1.2; }
                      100% { opacity: 0.0; scale: 1.0; }
                  }
              )");
          },
          /*body=*/ []{
              static int i = 0;
              const auto& anim = Theme::get().keyframes.at("k");
              float t = (float)(i++ & 0xFF) / 255.0f;
              Style s = evaluateKeyframes(anim, t, TransitionFunction::Linear);
              g_sink += s.opacity.is_set ? s.opacity.value : 0.0f;
          });

    R.add("micro.tickCss.100", "animation",
          /*iterations=*/1000,
          /*setup=*/[]{
              Theme::get().clear();
              ZMarkup::loadStyleString(R"(
                  @keyframes k { 0% { opacity: 0.0; } 100% { opacity: 1.0; } }
              )");
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
              }
          },
          /*body=*/ []{
              Anim::tickIncremental(sharedTickCssAnims(), 1.0f / 60.0f);
              g_sink += (float)sharedTickCssAnims().size();
          });

    R.add("micro.tickImperative.100", "animation",
          /*iterations=*/500,
          /*setup=*/[]{
              auto& nodes = sharedImperativeNodes();
              nodes.clear();
              nodes.reserve(100);
              for (int i = 0; i < 100; ++i)
              {
                  auto n = std::make_shared<Layout>(LayoutType::Stack);
                  auto anim = std::make_shared<UIAnimation>(1.0f);
                  anim->addTrack<float>(0.0f, 1.0f,
                      [](Layout* l, float v) { l->getInlineBase().opacity = v; });
                  n->addAnimation("fade", anim);
                  n->playAnimation("fade");
                  nodes.push_back(n);
              }
          },
          /*body=*/ []{
              for (auto& n : sharedImperativeNodes())
                  n->update(1.0f / 60.0f, false);
              g_sink += (float)sharedImperativeNodes().size();
          });

    // =======================================================================
    //  Tree
    // =======================================================================

    R.add("micro.tree.addChild.1000", "tree",
          /*iterations=*/200,
          /*setup=*/[]{
              auto& nodes = sharedTreeAddNodes();
              nodes.clear();
              nodes.reserve(1000);
              for (int i = 0; i < 1000; ++i)
                  nodes.push_back(std::make_shared<Layout>(LayoutType::Stack));
          },
          /*body=*/ []{
              auto root = std::make_shared<Layout>(LayoutType::Vertical);
              for (auto& c : sharedTreeAddNodes())
                  root->addChild(c);
              g_sink += (float)root->children.size();
          });

    R.add("micro.tree.removeAllChildren.1000", "tree",
          /*iterations=*/200,
          /*setup=*/[]{
              auto& root = sharedTreeRemoveRoot();
              root = std::make_shared<Layout>(LayoutType::Vertical);
              for (int i = 0; i < 1000; ++i)
                  root->addChild(std::make_shared<Layout>(LayoutType::Stack));
          },
          /*body=*/ []{
              sharedTreeRemoveRoot()->removeAllChildren();
              g_sink += (float)sharedTreeRemoveRoot()->children.size();
          });

    // =======================================================================
    //  SmallVector
    // =======================================================================

    R.add("micro.SmallVector.push_back_inline", "small_vector",
          /*iterations=*/1'000'000,
          /*setup=*/[]{},
          /*body=*/ []{
              static int i = 0;
              SmallVector<int, 2> v;
              v.push_back(i++);
              v.push_back(i++);
              g_sink += (float)v[0];
          });

    // =======================================================================
    //  Frame
    // =======================================================================

    {
        static Env env;
        static std::shared_ptr<Layout> root;
        constexpr int FRAMES = 500;

        R.add("system.frame.static100", "frame",
              FRAMES,
              /*setup=*/[&]{
                  env.reset();
                  Theme::get().clear();
                  root = makeFlatTree(100);
                  for (int i = 0; i < 5; ++i) env.frame(root);
              },
              /*body=*/ [&]{
                  env.frame(root);
                  g_sink += (float)root->children.size();
              });
    }

    {
        static Env env;
        static std::shared_ptr<Layout> root;
        constexpr int FRAMES = 200;

        R.add("system.frame.200rules", "frame",
              FRAMES,
              /*setup=*/[&]{
                  env.reset();
                  Theme::get().clear();
                  ZMarkup::loadStyleString(makeCss(200));
                  root = makeFlatTree(100);
                  for (int i = 0; i < 5; ++i) env.frame(root);
              },
              /*body=*/ [&]{
                  static int i = 0;
                  float op = (i++ % 2) ? 1.0f : 0.99f;
                  for (auto& c : root->children)
                      c->getInlineBase().opacity = op;
                  env.frame(root);
                  g_sink += (float)root->children.size();
              });
    }

    {
        static Env env;
        static std::shared_ptr<Layout> root;
        constexpr int FRAMES = 500;

        R.add("system.frame.transitions100", "frame",
              FRAMES,
              /*setup=*/[&]{
                  env.reset();
                  Theme::get().clear();
                  ZMarkup::loadStyleString(R"(
                      .row { background: #202020ff;
                             transition: background 0.2s linear; }
                      .row:hover { background: #4a9effff; }
                  )");
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
                  for (int i = 0; i < 5; ++i) env.frame(root);
              },
              /*body=*/ [&]{
                  static int i = 0;
                  float y = (i++ % 2 == 0) ? 10.0f : 30.0f;
                  env.platform.moveMouse({50.0f, y});
                  env.frame(root);
                  g_sink += (float)root->children.size();
              });
    }

    {
        static Env env;
        static std::shared_ptr<Layout> root;
        constexpr int FRAMES = 500;

        R.add("system.frame.animated100", "frame",
              FRAMES,
              /*setup=*/[&]{
                  env.reset();
                  Theme::get().clear();
                  ZMarkup::loadStyleString(R"(
                      @keyframes pulse { 0% { opacity: 0.5; }
                                         50% { opacity: 1.0; }
                                         100% { opacity: 0.5; } }
                      .pulse { animation: pulse 1s linear infinite; }
                  )");
                  root = std::make_shared<Layout>(LayoutType::Vertical);
                  root->size(Percent(100), Percent(100));
                  for (int i = 0; i < 100; ++i)
                  {
                      auto n = std::make_shared<Layout>(LayoutType::Stack);
                      n->cls("pulse");
                      n->size(Px(100), Px(20));
                      root->addChild(n);
                  }
                  for (int i = 0; i < 5; ++i) env.frame(root);
              },
              /*body=*/ [&]{
                  env.frame(root);
                  g_sink += (float)root->children.size();
              });
    }

    {
        static Env env;
        static std::shared_ptr<Layout> root;
        constexpr int FRAMES = 500;

        R.add("system.frame.wide200", "frame",
              FRAMES,
              /*setup=*/[&]{
                  env.reset();
                  Theme::get().clear();
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
                  for (int i = 0; i < 5; ++i) env.frame(root);
              },
              /*body=*/ [&]{
                  env.frame(root);
                  g_sink += (float)root->children.size();
              });
    }
}

} // namespace Bench