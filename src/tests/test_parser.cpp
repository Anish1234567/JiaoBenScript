#include <vector>
#include <tuple>
#include "catch.hpp"

#include "../node.h"
#include "../parser.h"
#include "helper_node.hpp"


// XXX: copied from test_tokenizer.cpp
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


Node::Ptr parse_string(const std::string &input, bool repl = true) {
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


Node::Ptr parse_single_stmt(const std::string &input) {
    Node::Ptr node = parse_string(input);
    Program *prog = dynamic_cast<Program *>(node.get());
    REQUIRE(prog != nullptr);
    REQUIRE(prog->stmts.size() == 1);
    return std::move(prog->stmts[0]);
}


void check_exp(const std::string &input, Node *exp) {
    auto _g = Node::Ptr(exp);
    CHECK(*parse_string(input) == *exp);
}


void check_stmt(const std::string &input, Node *expected) {
    auto _g = Node::Ptr(expected);
    CHECK(*parse_single_stmt(input) == *expected);
}


TEST_CASE("Test terminal") {
    check_exp("1", T(1));
    check_exp("2.e3", new E_Float(2e3));
    check_exp("true", new E_Bool(true));
    check_exp("false", new E_Bool(false));
    check_exp("null", new E_Null());
    check_exp("\"asdf\"", new E_String(USTRING("asdf")));
    check_exp("asdf", V("asdf"));
}


TEST_CASE("Test list") {
    check_exp("[]", new E_List());
    check_exp("[1]", make_list({new E_Int(1)}));
    check_exp("[1, 1.2]", make_list({new E_Int(1), new E_Float(1.2)}));
    check_exp("[1, 1.2, 2]", make_list({new E_Int(1), new E_Float(1.2), new E_Int(2)}));
}


TEST_CASE("Test binary op") {
    check_exp("+1", make_ops('+', {new E_Int(1)}));
    check_exp("-1", make_ops('-', {new E_Int(1)}));
    check_exp("2+1", make_ops('+', {new E_Int(2), new E_Int(1)}));
    check_exp("2*1", make_ops('*', {new E_Int(2), new E_Int(1)}));
    check_exp("2/1", make_ops('/', {new E_Int(2), new E_Int(1)}));
    check_exp("2%1", make_ops('%', {new E_Int(2), new E_Int(1)}));
    check_exp("!2", make_ops('!', {T(2)}));
    check_exp("1==2", make_binop('==', T(1), T(2)));
    check_exp("1!=2", make_binop('!=', T(1), T(2)));
    check_exp("1<2", make_binop('<', T(1), T(2)));
    check_exp("1>2", make_binop('>', T(1), T(2)));
    check_exp("1<=2", make_binop('<=', T(1), T(2)));
    check_exp("1>=2", make_binop('>=', T(1), T(2)));
    check_exp("1&&2", make_binop('&&', T(1), T(2)));
    check_exp("1||2", make_binop('||', T(1), T(2)));
    check_exp("(1)", T(1));
    check_exp("a=1", make_binop('=', V("a"), T(1)));
    check_exp("a[1]", make_binop('[]', V("a"), T(1)));
    check_exp("a[1]=2",
        make_binop('=',
            make_binop('[]', V("a"), T(1)),
            T(2)));
    check_exp("a()",
        make_ops('()', {V("a")}));
    check_exp("a(1)",
        make_binop('()',
            V("a"),
            make_ops(',', {T(1)})));
}


TEST_CASE("Test binary op combinations") {
    check_exp("1-2-3",
        make_binop('-',
            make_binop('-',
                T(1),
                T(2)),
            T(3)));
    check_exp("-1-2-3",
        make_binop('-',
            make_binop('-',
                make_ops('-', {T(1)}),
                T(2)),
            T(3)));
    check_exp("1+2*3",
        make_binop('+',
            T(1),
            make_binop('*', T(2), T(3))));
    check_exp("(1+2)*3",
        make_binop('*',
            make_binop('+', T(1), T(2)),
            T(3)));
    check_exp("a && b && c",
        make_binop('&&',
            make_binop('&&',
                V("a"),
                V("b")),
            V("c")));
    check_exp("a || b && c",
        make_binop('||',
            V("a"),
            make_binop('&&',
                V("b"),
                V("c"))));
    check_exp("a && b || c",
        make_binop('||',
            make_binop('&&',
                V("a"),
                V("b")),
            V("c")));
    check_exp("a=b=c",
        make_binop('=',
            V("a"),
            make_binop('=',
                V("b"),
                V("c"))));
    check_exp("1,2", make_ops(',', {T(1), T(2)}));
    check_exp("1,2,3", make_ops(',', {T(1), T(2), T(3)}));
    check_exp("a[1][2]",
        make_binop('[]',
            make_binop('[]',
                V("a"),
                T(1)),
            T(2)));
    check_exp("a(1)(2)",
        make_call(
            make_call(V("a"), {T(1)}),
            {T(2)}));
    check_exp("a[1](2)[3]",
        make_binop('[]',
            make_binop('()',
                make_binop('[]',
                    V("a"),
                    T(1)),
                make_ops(',', {T(2)})),
            T(3)));
    check_exp("a(1)[2](3)",
        make_binop('()',
            make_binop('[]',
                make_binop('()',
                    V("a"),
                    make_ops(',', {T(1)})),
                T(2)),
            make_ops(',', {T(3)})));
}


TEST_CASE("Test simple stmt") {
    check_stmt(";", new S_Empty());
    check_stmt("break;", new S_Break());
    check_stmt("continue;", new S_Continue());
    check_stmt("return;", new S_Return());

    S_Return *ret = new S_Return();
    ret->value.reset(T(1));
    check_stmt("return 1;", ret);

    check_stmt("{}", make_block({}));
    check_stmt("{;}", make_block({new S_Empty()}));
    check_stmt("{;;}", make_block({new S_Empty(), new S_Empty()}));
}


TEST_CASE("Test var declaration") {
    check_stmt("let a;", make_decl_list({{"a", nullptr}}));
    check_stmt("let a = 1;", make_decl_list({{"a", T(1)}}));
    check_stmt("let a, b;", make_decl_list({
        {"a", nullptr},
        {"b", nullptr},
    }));
    check_stmt("let a = 1, b = 2;", make_decl_list({
        {"a", T(1)},
        {"b", T(2)},
    }));
    check_stmt("let a, b = 2;", make_decl_list({
        {"a", nullptr},
        {"b", T(2)},
    }));
    check_stmt("let a = 1, b, c = 3;", make_decl_list({
        {"a", T(1)},
        {"b", nullptr},
        {"c", T(3)},
    }));
}


TEST_CASE("Test function") {
    check_exp("function () {}", make_func(nullptr, make_block({})));
    check_exp("function (a) {}",
        make_func(
            make_decl_list({{"a", nullptr}}),
            make_block({})));
    check_exp("function (a, b = 1) {}",
        make_func(
            make_decl_list({
                {"a", nullptr},
                {"b", T(1)},
            }),
            make_block({})));
}


TEST_CASE("Test if-else") {
    check_stmt("if (a) {}", make_cond(V("a"), make_block({}), nullptr));
    check_stmt("if (a) {} else {}", make_cond(V("a"), make_block({}), make_block({})));
    check_stmt(
        "if (a) {} else if (b) {}",
        make_cond(V("a"),
            make_block({}),
            make_cond(V("b"),
                make_block({}),
                nullptr))
    );
    check_stmt(
        "if (a) {} else if (b) {} else {}",
        make_cond(V("a"),
            make_block({}),
            make_cond(V("b"),
                make_block({}),
                make_block({})))
    );
}


TEST_CASE("Test loop") {
    check_stmt("while (a) {}", make_while(V("a"), make_block({})));
}


std::tuple<int, int> get_node_start_end(const std::string &input) {
    Node::Ptr node = parse_string(input);
    Node *target = nullptr;
    if (Program *prog = dynamic_cast<Program *>(node.get())) {
        REQUIRE(prog->stmts.size() == 1);
        target = prog->stmts[0].get();
    } else {
        target = node.get();
    }
    return std::make_tuple(target->pos_start.rowno, target->pos_end.rowno);
}


TEST_CASE("Test source pos") {
    CHECK(get_node_start_end("1") == std::make_tuple(0, 0));
    CHECK(get_node_start_end("(1)") == std::make_tuple(1, 1));  // FIXME: include parenthesis
    CHECK(get_node_start_end("+1") == std::make_tuple(0, 1));
    CHECK(get_node_start_end("1 + 1") == std::make_tuple(0, 4));
    CHECK(get_node_start_end("- 1 + 1 * 2") == std::make_tuple(0, 10));

    CHECK(get_node_start_end("ab") == std::make_tuple(0, 1));
    CHECK(get_node_start_end("null") == std::make_tuple(0, 3));
    CHECK(get_node_start_end("true") == std::make_tuple(0, 3));
    CHECK(get_node_start_end("false") == std::make_tuple(0, 4));
    CHECK(get_node_start_end("\"als\"") == std::make_tuple(0, 4));

    CHECK(get_node_start_end(";") == std::make_tuple(0, 0));
    CHECK(get_node_start_end(" ; ") == std::make_tuple(1, 1));
    CHECK(get_node_start_end("a;") == std::make_tuple(0, 1));
    CHECK(get_node_start_end("break;") == std::make_tuple(0, 5));
    CHECK(get_node_start_end("continue;") == std::make_tuple(0, 8));
    CHECK(get_node_start_end("break ;") == std::make_tuple(0, 6));
    CHECK(get_node_start_end("{}") == std::make_tuple(0, 1));
    CHECK(get_node_start_end("function () {}") == std::make_tuple(0, 13));
    CHECK(get_node_start_end("if (a) {}") == std::make_tuple(0, 8));
    CHECK(get_node_start_end("if (a) {} else {}") == std::make_tuple(0, 16));
    CHECK(get_node_start_end("while (a) {}") == std::make_tuple(0, 11));

    CHECK(get_node_start_end("[]") == std::make_tuple(0, 1));
    CHECK(get_node_start_end("[1]") == std::make_tuple(0, 2));
    CHECK(get_node_start_end("a()") == std::make_tuple(0, 2));
    CHECK(get_node_start_end("a(1)") == std::make_tuple(0, 3));
    CHECK(get_node_start_end("a[1]") == std::make_tuple(0, 3));

    CHECK(get_node_start_end("let a;") == std::make_tuple(0, 5));
    CHECK(get_node_start_end("let a, b;") == std::make_tuple(0, 8));
    CHECK(get_node_start_end("let a = 1, b;") == std::make_tuple(0, 12));

    Node::Ptr func = parse_string("function (a) {}");
    Node::Ptr &args = dynamic_cast<E_Func &>(*func).args;
    CHECK(args->pos_start == SourcePos(0, 10));
    CHECK(args->pos_end == SourcePos(0, 10));

    Node::Ptr call = parse_string("a(b)");
    E_Op &call_args = dynamic_cast<E_Op &>(*dynamic_cast<E_Op &>(*call).args[1]);
    CHECK(call_args.pos_start.rowno == 2);
    CHECK(call_args.pos_end.rowno == 2);
}
