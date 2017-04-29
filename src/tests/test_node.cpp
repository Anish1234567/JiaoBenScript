#include "../node.h"

#include "catch.hpp"


TEST_CASE("Test node comparison") {
    CHECK(E_Null() == E_Null());
    CHECK(E_Null() != S_Break());
}
