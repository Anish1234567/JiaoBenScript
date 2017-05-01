#ifndef JIAOBENSCRIPT_TEST_HELPER_H
#define JIAOBENSCRIPT_TEST_HELPER_H

#include <string>
#include <vector>

#include "../tokenizer.h"
#include "../parser.h"


std::vector<Token::Ptr> get_tokens(const std::string &input);
Node::Ptr parse_string(const std::string &input, bool repl = true);


#endif //JIAOBENSCRIPT_TEST_HELPER_H
