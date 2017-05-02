#include "catch.hpp"

#include "../sourcepos.h"


TEST_CASE("Test SourcePos") {
    SourcePos pos;
    LineTracer lt(pos);
    CHECK_FALSE(pos.is_valid());

    lt.add_char('a');
    CHECK(pos.lineno == 0);
    CHECK(pos.rowno == 0);
    CHECK(pos.is_valid());

    lt.add_char('b');
    CHECK(pos.lineno == 0);
    CHECK(pos.rowno == 1);

    lt.add_char('\n');
    CHECK(pos.lineno == 0);
    CHECK(pos.rowno == 2);

    lt.add_char('c');
    CHECK(pos.lineno == 1);
    CHECK(pos.rowno == 0);

    pos = SourcePos();
    LineTracer lt2(pos);
    lt2.add_char('\n');
    CHECK(pos == SourcePos(0, 0));

    lt2.add_char('a');
    CHECK(pos == SourcePos(1, 0));
}
