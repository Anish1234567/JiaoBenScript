#include <vector>
#include "catch.hpp"

#include "../tokenizer.h"


TEST_CASE("Test tokencode_to_string") {
    CHECK(tokencode_to_string(TokenCode::STRING) == "STR");
    CHECK(tokencode_to_string(TokenCode::GREATEQ) == ">=");
}


std::vector<Token::Ptr> get_tokens(const std::string &input) {
    Tokenizer tokenizer;
    for (auto ch : u8_decode(input)) {
        tokenizer.feed(ch);
    }
    tokenizer.feed('\n');
    REQUIRE(tokenizer.is_ready());

    std::vector<Token::Ptr> tokens;
    Token::Ptr tok;
    while ((tok = tokenizer.pop())) {
        tokens.emplace_back(std::move(tok));
    }
    return tokens;
}


void check_single_chars(const std::string &input) {
    auto tokens = get_tokens(input);

    std::string got;
    for (const auto &tok : tokens) {
        CHECK(static_cast<uint32_t>(tok->tokencode) < 0xff);
        got.push_back(static_cast<char>(tok->tokencode));
    }
    CHECK(got == input);
}


TEST_CASE("Test single char operator") {
    check_single_chars("[]{}(),");
    check_single_chars("=+-*/%<>!");
}


std::vector<std::string> split(const std::string &input) {
    std::vector<std::string> ans;
    std::string cur;
    for (size_t i = 0; i <= input.size(); ++i) {
        if (input[i] == ' ' || input[i] == '\0') {
            if (!cur.empty()) {
                ans.emplace_back(std::move(cur));
                cur.clear();
            }
        } else {
            cur.push_back(input[i]);
        }
    }
    return ans;
}


void check_multi_chars(const std::string &input) {
    std::vector<TokenCode> got;
    for (const Token::Ptr &tok : get_tokens(input)) {
        got.emplace_back(tok->tokencode);
    }

    std::vector<TokenCode> expected;
    for (auto piece : split(input)) {
        uint32_t u32 = ((uint32_t)(piece[0]) << 8) | piece[1];
        expected.emplace_back(static_cast<TokenCode>(u32));
    }
    CHECK(got == expected);
}


TEST_CASE("Test multi-char operator") {
    check_multi_chars("<= >= == != && || += -= *= /= %=");
}


void check_tokens(const std::string &input, const std::vector<Token *> &expected) {
    std::vector<Token::Ptr> expected_wrapped;
    for (Token *tok : expected) {
        expected_wrapped.emplace_back(tok);
    }

    auto tokens = get_tokens(input);
    REQUIRE(tokens.size() == expected.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        CHECK(*tokens[i] == *expected[i]);
    }
}


TEST_CASE("Test basic") {
    check_tokens("123", {new TokenInt(123)});
    check_tokens("asdf", {new TokenId(USTRING("asdf"))});
    check_tokens(
        "asdf // 123\n"
        "qwer /* asdf\n"
        "*123 */ 1.0e-3",
        {
            new TokenId(USTRING("asdf")),
            new TokenComment(USTRING(" 123")),
            new TokenId(USTRING("qwer")),
            new TokenComment(USTRING(" asdf\n*123 ")),
            new TokenFloat(1e-3),
        }
    );
}
