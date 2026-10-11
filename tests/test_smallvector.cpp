#include "TestFramework.hpp"
#include "SmallVector.hpp"

#include <string>
#include <utility>

using namespace ZenitUI;

// ============================================================
//  SmallVector
// ============================================================

// ---- costruzione base --------------------------------------------------

TEST(SmallVector, default_is_empty)
{
    SmallVector<int, 2> v;
    CHECK(v.empty());
    CHECK(v.size() == 0);
}

TEST(SmallVector, from_initializer_list)
{
    SmallVector<int, 2> v{1, 2};
    CHECK((int)v.size() == 2);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
}

TEST(SmallVector, from_initializer_list_overflow)
{
    SmallVector<int, 2> v{1, 2, 3, 4};
    CHECK((int)v.size() == 4);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 3);
    CHECK(v[3] == 4);
}

// ---- push_back ---------------------------------------------------------

TEST(SmallVector, push_back_within_inline)
{
    SmallVector<int, 3> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);

    CHECK((int)v.size() == 3);
    CHECK(v[0] == 10);
    CHECK(v[1] == 20);
    CHECK(v[2] == 30);
}

TEST(SmallVector, push_back_overflow_to_heap)
{
    SmallVector<int, 2> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);   // overflow
    v.push_back(4);

    CHECK((int)v.size() == 4);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
    CHECK(v[2] == 3);
    CHECK(v[3] == 4);
}

TEST(SmallVector, push_back_rvalue_moves)
{
    // Con std::string posso verificare che il move sia effettivo:
    // dopo il move, la stringa sorgente è vuota.
    SmallVector<std::string, 2> v;

    std::string a = "hello";
    v.push_back(std::move(a));

    CHECK((int)v.size() == 1);
    CHECK(v[0] == "hello");
    CHECK(a.empty());   // il move ha svuotato la sorgente
}

TEST(SmallVector, push_back_rvalue_overflow_moves)
{
    SmallVector<std::string, 2> v;

    std::string a = "one";
    std::string b = "two";
    std::string c = "three";

    v.push_back(std::move(a));
    v.push_back(std::move(b));
    v.push_back(std::move(c));   // overflow, ma i primi due vengono mossi

    CHECK((int)v.size() == 3);
    CHECK(v[0] == "one");
    CHECK(v[1] == "two");
    CHECK(v[2] == "three");
    CHECK(a.empty());
    CHECK(b.empty());
    CHECK(c.empty());
}

// ---- copia / move ------------------------------------------------------

TEST(SmallVector, copy_constructor)
{
    SmallVector<int, 2> a{1, 2, 3};
    SmallVector<int, 2> b = a;

    CHECK(b.size() == a.size());
    CHECK(b[0] == 1);
    CHECK(b[1] == 2);
    CHECK(b[2] == 3);
}

TEST(SmallVector, copy_assignment)
{
    SmallVector<int, 2> a{1, 2, 3};
    SmallVector<int, 2> b;
    b = a;

    CHECK(b.size() == a.size());
    CHECK(b[0] == 1);
    CHECK(b[2] == 3);
}

TEST(SmallVector, move_constructor)
{
    SmallVector<int, 2> a{1, 2, 3};
    SmallVector<int, 2> b = std::move(a);

    CHECK((int)b.size() == 3);
    CHECK(b[0] == 1);
    CHECK(b[1] == 2);
    CHECK(b[2] == 3);
}

TEST(SmallVector, move_assignment)
{
    SmallVector<int, 2> a{1, 2, 3};
    SmallVector<int, 2> b;
    b = std::move(a);

    CHECK((int)b.size() == 3);
    CHECK(b[1] == 2);
}

// ---- clear / reserve ---------------------------------------------------

TEST(SmallVector, clear_resets_to_empty)
{
    SmallVector<int, 2> v{1, 2, 3};
    CHECK((int)v.size() == 3);

    v.clear();
    CHECK(v.empty());
    CHECK(v.size() == 0);
}

TEST(SmallVector, clear_allows_return_to_inline)
{
    SmallVector<int, 2> v{1, 2, 3, 4};
    CHECK((int)v.size() == 4);

    v.clear();
    CHECK(v.empty());

    // Dopo clear, i push tornano a usare lo storage inline
    // (non è verificabile direttamente, ma il comportamento funzionale
    // lo è).
    v.push_back(10);
    v.push_back(20);
    CHECK((int)v.size() == 2);
    CHECK(v[0] == 10);
    CHECK(v[1] == 20);
}

TEST(SmallVector, reserve_on_inline_is_noop)
{
    SmallVector<int, 4> v;
    v.reserve(2);   // < N: nessun effetto
    CHECK(v.empty());
}

TEST(SmallVector, reserve_on_heap_does_not_lose_data)
{
    SmallVector<int, 2> v{1, 2};
    v.reserve(10);   // > N: riserva sull'heap

    CHECK((int)v.size() == 2);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);

    v.push_back(3);
    v.push_back(4);
    CHECK((int)v.size() == 4);
    CHECK(v[3] == 4);
}

// ---- iterazione --------------------------------------------------------

TEST(SmallVector, range_based_for_inline)
{
    SmallVector<int, 4> v{1, 2, 3};

    int sum = 0;
    for (int x : v) sum += x;

    CHECK(sum == 6);
}

TEST(SmallVector, range_based_for_heap)
{
    SmallVector<int, 2> v{1, 2, 3, 4, 5};

    int sum = 0;
    for (int x : v) sum += x;

    CHECK(sum == 15);
}

TEST(SmallVector, range_based_for_empty)
{
    SmallVector<int, 4> v;

    int count = 0;
    for (int x : v) { (void)x; count++; }

    CHECK(count == 0);
}

// ---- confronto ---------------------------------------------------------

TEST(SmallVector, equality_same_content)
{
    SmallVector<int, 2> a{1, 2, 3};
    SmallVector<int, 2> b{1, 2, 3};

    CHECK(a == b);
    CHECK(!(a != b));
}

TEST(SmallVector, equality_different_size)
{
    SmallVector<int, 2> a{1, 2};
    SmallVector<int, 2> b{1, 2, 3};

    CHECK(a != b);
}

TEST(SmallVector, equality_different_content)
{
    SmallVector<int, 2> a{1, 2};
    SmallVector<int, 2> b{1, 3};

    CHECK(a != b);
}

TEST(SmallVector, equality_empty)
{
    SmallVector<int, 2> a;
    SmallVector<int, 2> b;

    CHECK(a == b);
}

// ---- casi limite di N --------------------------------------------------

TEST(SmallVector, N_equal_one)
{
    SmallVector<int, 1> v;
    v.push_back(1);
    v.push_back(2);

    CHECK((int)v.size() == 2);
    CHECK(v[0] == 1);
    CHECK(v[1] == 2);
}

TEST(SmallVector, N_equal_two_typical_case)
{
    // Il caso reale per Value: 2 termini inline.
    SmallVector<int, 2> v;
    v.push_back(100);
    v.push_back(-20);

    CHECK((int)v.size() == 2);
    CHECK(v[0] == 100);
    CHECK(v[1] == -20);
}

TEST(SmallVector, N_equal_three)
{
    SmallVector<int, 3> v{1, 2, 3, 4};
    CHECK((int)v.size() == 4);
    CHECK(v[3] == 4);
}

// ---- operator= da initializer_list --------------------------------------

TEST(SmallVector, assign_from_initializer_list_replaces)
{
    SmallVector<int, 2> v{1, 2, 3};
    v = {4, 5};

    CHECK((int)v.size() == 2);
    CHECK(v[0] == 4);
    CHECK(v[1] == 5);
}

TEST(SmallVector, assign_from_initializer_list_overflow)
{
    SmallVector<int, 2> v{1};
    v = {1, 2, 3, 4, 5};

    CHECK((int)v.size() == 5);
    for (int i = 0; i < 5; ++i)
        CHECK(v[i] == i + 1);
}