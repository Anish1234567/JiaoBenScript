#include "catch.hpp"

#include "../jbobject.h"


TEST_CASE("Test ==") {
    CHECK(JBNull() == JBNull());
    CHECK(JBInt(1) != JBFloat(1.0));
    CHECK(JBBool(true) != JBBool(false));
    CHECK(JBBool(true) == JBBool(true));
    CHECK(JBBool(false) != JBNull());
}


TEST_CASE("Test eq") {
    CHECK(JBNull().eq(JBNull()));
    CHECK(!JBNull().eq(JBBool(false)));
    CHECK(JBInt(1).eq(JBFloat(1.0)));
    CHECK(JBFloat(1.0).eq(JBFloat(1.0)));
    CHECK(JBFloat(1.0).eq(JBInt(1)));
    CHECK(JBString(USTRING("asdf")).eq(JBString(USTRING("asdf"))));
    CHECK(!JBString(USTRING("asdf")).eq(JBString(USTRING("bsdf"))));
    CHECK(JBList().eq(JBList()));

    JBList la;
    JBList lb;
    JBInt a(1);
    JBInt b(2);
    la.value.push_back(&a);
    CHECK(!la.eq(lb));
    lb.value.push_back(&b);
    CHECK(!la.eq(lb));
    lb.value.pop_back();
    lb.value.push_back(&a);
    CHECK(la.eq(lb));
}


TEST_CASE("Test truth") {
    CHECK_FALSE(JBNull().is_truthy());
    CHECK_FALSE(JBInt(0).is_truthy());
    CHECK_FALSE(JBBool(false).is_truthy());
    CHECK(JBInt(123).is_truthy());
    CHECK(JBBool(true).is_truthy());
    CHECK(JBString(USTRING("asdf")).is_truthy());
    CHECK_FALSE(JBString(USTRING("")).is_truthy());

    JBList list;
    CHECK_FALSE(list.is_truthy());
    JBInt a(1);
    list.value.push_back(&a);
    CHECK(list.is_truthy());
}
