#include <string>
#include "catch.hpp"

#include "../unicode.h"


using std::string;
using namespace Catch::Matchers;


void check_read_char(const char *str, int len, unichar ch) {
    REQUIRE(u8_read_char_len(str) == len);
    CHECK(u8_read_char(str) == ch);
}


TEST_CASE("Test unicode read char") {
    check_read_char("\x7f", 1, 0x7f);
    check_read_char("±", 2, 0x00b1);
    check_read_char("啊", 3, 0x554a);
    check_read_char("𠀀", 4, 0x20000);
    check_read_char("\xf4\x8f\xbf\xbf", 4, 0x10ffff);
    check_read_char("\0", 1, 0);

    CHECK_THROWS_AS(u8_read_char_len("\x80"), DecodeError);
    CHECK_THROWS_WITH(u8_read_char_len("\x80"), Contains("Bad leading"));

    CHECK_THROWS_AS(u8_read_char("\xe5\x95\xe5"), DecodeError);
    CHECK_THROWS_WITH(u8_read_char("\xe5\x95\xe5"), Contains("Bad following"));
}


void check_write_char(const string &str) {
    const unichar ch = u8_read_char(str.data());
    REQUIRE(u8_char_len(ch) == str.size());

    char buf[8] = {};
    char *ret = u8_write_char(buf, ch);
    REQUIRE(ret - buf == str.size());
    CHECK(string(buf, buf + str.size()) == str);
}


TEST_CASE("Test unicode write char") {
    check_write_char(string(1, '\0'));
    check_write_char("±");
    check_write_char("啊");
    check_write_char("\xf4\x8f\xbf\xbf");
}


void check_decode_encode(const string &input) {
    ustring us1 = u8_decode(input.data());
    ustring us2 = u8_decode(input);
    CHECK(us1 == us2);
    string bytes = u8_encode(us1);
    CHECK(input == bytes);
}


TEST_CASE("Test unicode encode/decode") {
    check_decode_encode("");
    check_decode_encode("asdf");
    check_decode_encode("asdf啊啊23545");
    check_decode_encode("as±df");
}


TEST_CASE("Test unicode length") {
    CHECK(u8_unicode_len("啊123") == 4);
    CHECK(u8_unicode_len(string("啊123")) == 4);
    CHECK(u8_unicode_len("") == 0);
}


TEST_CASE("Test surrogate") {
    CHECK(UCHAR("𤭢") == u16_assemble_surrogate(0xd852, 0xdf62));
}
