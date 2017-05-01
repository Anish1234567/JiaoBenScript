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


Node::Ptr parse_string(const std::string &input, bool repl) {
    auto tokens = get_tokens(input);
    Parser parser;
    if (repl) {
        parser.start_repl();
    } else {
        parser.start_program();
    }

    for (const Token::Ptr &tok : tokens) {
        parser.feed(*tok);
    }
    REQUIRE(parser.can_end());
    return parser.pop_result();
}
