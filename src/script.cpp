#include <cassert>
#include <iterator>
#include <iosfwd>
#include <vector>
#include <string>

#include "script.h"
#include "tokenizer.h"
#include "parser.h"
#include "eval_ast.h"
#include "line_highlight.h"
#include "sourcepos.h"
#include "unicode.h"


static void print_error(
    const std::string &type, const std::string &msg, const std::vector<ustring> &lines = {},
    const SourcePos &pos_start = SourcePos(), const SourcePos &pos_end = SourcePos())
{
    std::cerr << type << ": " << msg << std::endl;
    if (pos_start.is_valid()) {
        line_lighlight(lines, pos_start, pos_end);
    }
}


static std::vector<ustring> split_lines(std::istream &input) {
    std::string line;
    std::vector<ustring> lines;
    while (std::getline(input, line)) {
        ustring uline = u8_decode(line);
        uline.push_back('\n');
        lines.push_back(uline);
    }
    return lines;
}


static Node::Ptr parse(const std::vector<ustring> &lines) {
    Parser parser;
    parser.start_program();

    Tokenizer tokenizer;
    for (const ustring &uline : lines) {
        for (unichar ch : uline) {
            tokenizer.feed(ch);
        }
        while (Token::Ptr tok = tokenizer.pop()) {
            parser.feed(*tok);
        }
    }

    return parser.pop_result();
}


static void _run_script_inner(const std::vector<ustring> &lines, bool main) {
    Node::Ptr node = parse(lines);
    assert(dynamic_cast<Program *>(node.get()));

    AstInterpreter interp;
    interp.set_default_builtin_table();

    Program &prog = static_cast<Program &>(*node);
    interp.eval_incomplete_raw_block(prog);

    if (main) {
        E_Op *call = new E_Op(OpCode::CALL);
        Node::Ptr _(call);
        call->args.emplace_back(new E_Var(USTRING("main")));
        call->args.emplace_back(new E_Op(OpCode::EXPLIST));
        interp.eval_raw_exp(*call);
    }
}

#define CATCH_AND_RETURN(Type, ret) \
    catch (Type &exc) { \
        print_error(#Type, exc.what(), lines, exc.pos_start, exc.pos_end); \
        return ret; \
    }


static int _run_script(std::istream &input, bool main) {
    std::vector<ustring> lines;
    try {
        lines = split_lines(input);
    } catch (DecodeError &exc) {
        print_error("DecodeError", exc.what());
        return 1;
    }

    try {
        _run_script_inner(lines, main);
        return 0;
    }
    CATCH_AND_RETURN(TokenizerError, 2)
    CATCH_AND_RETURN(ParserError, 3)
    CATCH_AND_RETURN(CompileError, 4)
    CATCH_AND_RETURN(JBError, 5)
    catch (...) {
        print_error("Unknown", "Unknown exception caught");
        return 6;
    }
}


#undef CATCH_AND_RETURN


int run_script(std::istream &input) {
    return _run_script(input, false);
}


int run_script_main(std::istream &input) {
    return _run_script(input, true);
}
