#include "TestFramework.hpp"
#include "Unit.hpp"

using namespace ZenitUI;

// ------------------------------------------------------------
//  Costruzione e query di base
// ------------------------------------------------------------

TEST(Unit, default_is_auto) {
    Value v;
    CHECK(v.isAuto());
    CHECK(v.terms.empty());
    CHECK_NEAR(v.resolveH(100, 100), 0.0f, 1e-6);
}

TEST(Unit, px_is_simple) {
    Value v = Px(10.0f);
    CHECK(!v.isAuto());
    CHECK(v.isSimple());
    CHECK(v.terms.size() == 1);
    CHECK(v.terms[0].unit == Unit::Pixel);
    CHECK_NEAR(v.terms[0].coeff, 10.0f, 1e-6);
}

// ------------------------------------------------------------
//  resolveH / resolveV / resolveSelfH / resolveSelfV
// ------------------------------------------------------------

TEST(Unit, resolve_px) {
    Value v = Px(42.0f);
    CHECK_NEAR(v.resolveH(200, 100), 42.0f, 1e-6);
    CHECK_NEAR(v.resolveV(200, 100), 42.0f, 1e-6);
}

TEST(Unit, resolve_percent_is_axis_dependent) {
    Value v = Percent(50.0f);
    CHECK_NEAR(v.resolveH(200, 100), 100.0f, 1e-6);  // 50% di larghezza
    CHECK_NEAR(v.resolveV(200, 100), 50.0f,  1e-6);  // 50% di altezza
}

TEST(Unit, resolve_pw_always_width) {
    Value v = PW(25.0f);
    CHECK_NEAR(v.resolveH(200, 100), 50.0f, 1e-6);
    CHECK_NEAR(v.resolveV(200, 100), 50.0f, 1e-6);
}

TEST(Unit, resolve_ph_always_height) {
    Value v = PH(25.0f);
    CHECK_NEAR(v.resolveH(200, 100), 25.0f, 1e-6);
    CHECK_NEAR(v.resolveV(200, 100), 25.0f, 1e-6);
}

TEST(Unit, resolve_vw_against_viewport) {
    Value v = VW(50.0f);
    CHECK_NEAR(v.resolveH(100, 100), 960.0f, 1e-4);
    CHECK_NEAR(v.resolveV(100, 100), 960.0f, 1e-4);
}

TEST(Unit, resolve_vh_against_viewport) {
    Value v = VH(10.0f);
    CHECK_NEAR(v.resolveH(100, 100), 108.0f, 1e-4);
    CHECK_NEAR(v.resolveV(100, 100), 108.0f, 1e-4);
}

TEST(Unit, resolveSelf_percent_uses_self) {
    Value v = Percent(50.0f);
    CHECK_NEAR(v.resolveSelfH(200.0f, 100.0f), 100.0f, 1e-6);
    CHECK_NEAR(v.resolveSelfV(200.0f, 100.0f), 50.0f,  1e-6);
}

// ------------------------------------------------------------
//  Somma, sottrazione, moltiplicazione, normalizzazione
// ------------------------------------------------------------

TEST(Unit, add_same_unit_collapses) {
    Value c = Px(10.0f) + Px(5.0f);
    CHECK(c.terms.size() == 1);
    CHECK_NEAR(c.terms[0].coeff, 15.0f, 1e-6);
}

TEST(Unit, add_mixed_units_keeps_both_terms) {
    Value c = Px(10.0f) + Percent(50.0f);
    CHECK(c.terms.size() == 2);
    // Su parent 200: 10 + 100 = 110
    CHECK_NEAR(c.resolveH(200, 100), 110.0f, 1e-6);
}

TEST(Unit, sub_two_units) {
    Value c = PW(100.0f) - PH(80.0f);
    // Su parent 100x50: 100 - 40 = 60
    CHECK_NEAR(c.resolveH(100, 50), 60.0f, 1e-4);
}

TEST(Unit, negate) {
    Value v = Px(10.0f);
    Value n = -v;
    CHECK(n.terms.size() == 1);
    CHECK_NEAR(n.terms[0].coeff, -10.0f, 1e-6);
}

TEST(Unit, mul_by_scalar) {
    Value v = Percent(50.0f) * 2.0f;
    CHECK_NEAR(v.resolveH(100, 100), 100.0f, 1e-6);
}

TEST(Unit, div_by_scalar) {
    Value v = Px(10.0f) / 2.0f;
    CHECK_NEAR(v.resolveH(0, 0), 5.0f, 1e-6);
}

TEST(Unit, normalize_combines_same_units) {
    Value sum = Px(10.0f) + Px(5.0f) + Px(3.0f);
    CHECK(sum.terms.size() == 1);
    CHECK_NEAR(sum.terms[0].coeff, 18.0f, 1e-6);
}

// ------------------------------------------------------------
//  Uguaglianza
// ------------------------------------------------------------

TEST(Unit, eq_simple) {
    CHECK(Px(10.0f) == Px(10.0f));
    CHECK(Px(10.0f) != Px(11.0f));
}

TEST(Unit, eq_after_normalize) {
    Value a = Px(5.0f) + Px(5.0f);
    Value b = Px(10.0f);
    CHECK(a == b);
}

TEST(Unit, eq_mixed) {
    Value a = PW(100.0f) - PH(80.0f);
    Value b = PW(100.0f) - PH(80.0f);
    CHECK(a == b);
    CHECK(a != (PW(100.0f) - PH(79.0f)));
}

// ------------------------------------------------------------
//  Mul/div con unità (calcola l'operazione di calc)
// ------------------------------------------------------------

TEST(Unit, mulWith_number_on_right) {
    Value a = Percent(50.0f);
    Value b = Value::number(2.0f);
    Value c = a.mulWith(b);
    CHECK_NEAR(c.resolveH(100, 100), 100.0f, 1e-6);
}

TEST(Unit, mulWith_number_on_left) {
    Value a = Value::number(2.0f);
    Value b = Percent(50.0f);
    Value c = a.mulWith(b);
    CHECK_NEAR(c.resolveH(100, 100), 100.0f, 1e-6);
}

TEST(Unit, divWith_number) {
    Value a = Percent(100.0f);
    Value b = Value::number(4.0f);
    Value c = a.divWith(b);
    CHECK_NEAR(c.resolveH(100, 100), 25.0f, 1e-6);
}