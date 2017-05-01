#include "catch.hpp"

#include "helper.h"


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


Node::Ptr parse_string(const std::string &input) {
    auto tokens = get_tokens(input);
    Parser parser;
    parser.start_program();
    for (const Token::Ptr &tok : tokens) {
        parser.feed(*tok);
    }
    parser.feed(Token(TokenCode::END));
    return parser.pop_result();
}
