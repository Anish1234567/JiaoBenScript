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


InteractiveRepl::InteractiveRepl()
    : tokenizer(), parser(), interp()
{
    this->interp.set_default_builtin_table();
    this->print_start_info();
}


void InteractiveRepl::print_start_info() {
    std::cout << "JiaoBenScript\n";
}


#define CATCH_AND_MSG(Type) \
    catch (Type &exc) { this->error(#Type, exc.what(), exc.pos_start, exc.pos_end); }


void InteractiveRepl::feed(const std::string &line) {
    try {
        this->feed_inner(line);
    } catch (std::exception &exc) {
        try {
            throw;
        } catch (DecodeError &exc) {
            this->error("DecodeError", exc.what());
        }
        CATCH_AND_MSG(TokenizerError)
        CATCH_AND_MSG(ParserError)
        CATCH_AND_MSG(CompileError)
        CATCH_AND_MSG(JBError)
        catch(...) {
            this->error("Unknown", "unknown exception caught");
        }

        this->reset();
    }
}


#undef CATCH_AND_MSG


void InteractiveRepl::error(
    const std::string &type, const std::string &msg,
    const SourcePos &pos_start, const SourcePos &pos_end)
{
    std::cerr << type << ": " << msg << std::endl;
    if (pos_start.is_valid()) {
        this->print_line_highlights(pos_start, pos_end);
    }
}


void InteractiveRepl::print_line_highlights(const SourcePos &start, const SourcePos &end) {
    assert(start.is_valid() && end.is_valid());
    assert(static_cast<size_t>(start.lineno) < this->lines.size());
    assert(static_cast<size_t>(end.lineno) < this->lines.size());
    if (start.lineno == end.lineno) {
        this->print_single_line_highlight(
            static_cast<size_t>(start.lineno),
            static_cast<size_t>(start.rowno), static_cast<size_t>(end.rowno)
        );
    } else {
        assert(start.lineno < end.lineno);
        assert(this->lines[start.lineno].size() > 1);
        this->print_single_line_highlight(
            static_cast<size_t>(start.lineno),
            static_cast<size_t>(start.rowno), this->lines[start.lineno].size() - 2
        );
        for (int index = start.lineno + 1; index < end.lineno; ++index) {
            size_t i = static_cast<size_t>(index);
            this->print_single_line_highlight(i, 0, this->lines[i].size() - 1);
        }
        this->print_single_line_highlight(
            static_cast<size_t>(end.lineno), 0, static_cast<size_t>(end.rowno)
        );
    }
}


void InteractiveRepl::print_single_line_highlight(size_t index, size_t start, size_t end) {
    assert(index < this->lines.size());
    const ustring &line = this->lines[index];
    assert(!line.empty() && line.back() == '\n');
    assert(start <= end && end < line.size());
    std::cerr << u8_encode(line);
    std::cerr << std::string(start, ' ') << std::string(end - start + 1, '~') << std::endl;
}


void InteractiveRepl::feed_inner(const std::string &line) {
    ustring uline = u8_decode(line);
    uline.push_back('\n');
    this->lines.push_back(uline);

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
            // TODO: skip comment line
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
        std::string ret(line);
        free((void *)line);

        if (ret.size() != 0) {
            // non-empty line
            add_history(ret.data());
        }
        return ret;
    } else {
        // EOF
        this->eof = true;
        return "";
    }
}


bool InteractiveRepl::is_ready() const {
    return this->tokenizer.is_ready() && this->parser.is_empty();
}


void InteractiveRepl::reset() {
    this->tokenizer.reset();
    this->parser.reset();
    this->lines.clear();
}
