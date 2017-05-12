#include <vector>
#include "catch.hpp"

#include "../allocator.h"
#include "../builtins.h"
#include "../exceptions.h"
#include "../jbobject.h"


static JBList make_list(const std::vector<JBValue *> &values) {
    JBList list;
    for (JBValue *item : values) {
        list.value.push_back(item);
    }
    return list;
}


TEST_CASE("Test builtins") {
    Allocator allocator;
    Builtins b(allocator);

    JBInt zero(0);
    JBInt one(1);
    JBInt two(2);
    JBInt three(3);
    JBFloat fone(1.0);
    JBFloat ftwo(2.0);
    JBInt negone(-1);

    SECTION("arithmetic") {
        CHECK(b.builtin_add(one, two) == JBInt(3));
        CHECK(b.builtin_add(one, ftwo) == JBFloat(3));
        CHECK(b.builtin_add(one, ftwo) != JBInt(3));
        CHECK(b.builtin_add(fone, two) == JBFloat(3));
        CHECK(b.builtin_add(fone, ftwo) == JBFloat(3));
        CHECK(b.builtin_sub(one, two) == JBInt(-1));
        CHECK(b.builtin_neg(one) == JBInt(-1));
        CHECK(b.builtin_pos(negone) == negone);

        CHECK(b.builtin_div(two, fone) == ftwo);
        CHECK_THROWS_AS(b.builtin_div(one, zero), JBError);
    }

    JBBool jtrue(true);
    JBBool jfalse(false);

    SECTION("comparison") {
        CHECK(b.builtin_lt(one, two) == jtrue);
        CHECK(b.builtin_lt(two, one) == jfalse);
        CHECK(b.builtin_lt(two, two) == jfalse);
        CHECK(b.builtin_le(two, two) == jtrue);
        CHECK(b.builtin_eq(one, two) == jfalse);
        CHECK(b.builtin_ne(one, two) == jtrue);
        CHECK(b.builtin_eq(one, one) == jtrue);
    }

    SECTION("boolean") {
        CHECK(b.is_truthy(one));
        CHECK(b.builtin_truth(one) == jtrue);
        CHECK(b.builtin_truth(zero) == jfalse);
        CHECK(b.builtin_not(zero) == jtrue);
        JBList empty;
        CHECK(b.builtin_truth(empty) == jfalse);
        JBString empty_string(USTRING(""));
        CHECK(b.builtin_not(empty_string) == jtrue);
    }

    JBList list;
    list.value.push_back(&one);
    list.value.push_back(&two);
    list.value.push_back(&zero);

    SECTION("getitem") {
        CHECK(b.builtin_getitem(list, one) == two);
        CHECK(b.builtin_getitem(list, two) == zero);
        CHECK_THROWS_AS(b.builtin_getitem(list, three), JBError);
        CHECK_THROWS_AS(b.builtin_getitem(list, fone), JBError);
    }

    SECTION("setitem") {
        CHECK(b.builtin_setitem(list, one, three) == three);
        CHECK(list == make_list({&one, &three, &zero}));
        CHECK_THROWS_AS(b.builtin_setitem(list, three, three), JBError);
        CHECK_THROWS_AS(b.builtin_setitem(list, negone, three), JBError);
    }
}
