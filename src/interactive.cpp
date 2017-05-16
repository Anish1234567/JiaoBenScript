#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iosfwd>
#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>

#include "interactive.h"
#include "exceptions.h"
#include "unicode.h"
#include "string_fmt.hpp"


InteractiveRepl::InteractiveRepl() {
    this->interp.set_default_builtin_table();
    this->print_start_info();
}


void InteractiveRepl::print_start_info() {
    std::cout << "JiaoBenScript\n";
}


void InteractiveRepl::feed(const std::string &line) {
    ustring uline = u8_decode(line);
    uline.push_back('\n');

    if (this->parser.is_empty()) {
        this->parser.start_repl();
    }
    for (unichar ch : uline) {
        this->tokenizer.feed(ch);
    }
    while (Token::Ptr tok = this->tokenizer.pop()) {
        this->parser.feed(*tok);
    }

    if (!this->tokenizer.is_ready() || !this->parser.can_end()) {
        return;
    }

    this->nodes.emplace_back(this->parser.pop_result());
    Node &node = *this->nodes.back();
    assert(parser.is_empty());

    if (Program *prog = dynamic_cast<Program *>(&node)) {
        for (Node::Ptr &stmt : prog->stmts) {
            this->interp.eval_raw_stmt(*stmt);
        }
    } else {
        JBValue &ret = this->interp.eval_raw_exp(node);
        this->print_result(ret);
    }
}


std::string InteractiveRepl::get_input_prompt() {
    return string_fmt("In [%d]: ", this->count++);
}


void InteractiveRepl::print_result(JBValue &value) {
    std::cout << string_fmt("Out[%d]: ", this->count - 1) << value.repr() << std::endl;
}


void InteractiveRepl::start() {
    std::string line;
    while (!this->eof) {
        std::string prompt = this->is_ready() ? this->get_input_prompt() : "";
        line = this->read_line(prompt);

        if (!(this->is_ready() && line.empty())) {
            this->feed(line);
        } else {
            // do not increase counter for empty line
            this->count--;
        }
    }

    // last new line
    std::cout << std::endl;
}


std::string InteractiveRepl::read_line(const std::string &prompt) {
    const char *line = ::readline(prompt.data());
    if (line != nullptr) {
        if (strlen(line) != 0) {
            // non-empty line
            add_history(line);
            return line;
        } else {
            // empty line
            free((void *)line);
            return "";
        }
    } else {
        // EOF
        this->eof = true;
        return "";
    }
}


bool InteractiveRepl::is_ready() const {
    return this->tokenizer.is_ready() && this->parser.is_empty();
}
