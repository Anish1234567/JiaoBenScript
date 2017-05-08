#include <vector>
#include <tuple>
#include "catch.hpp"

#include "../node.h"
#include "../parser.h"


Node *make_ops(OpCode opcode, const std::vector<Node *> &args) {
    E_Op *exp = new E_Op(opcode);
    for (Node *item : args) {
        exp->args.emplace_back(item);
    }
    return exp;
}


Node *make_ops(uint32_t opcode, const std::vector<Node *> &args) {
    return make_ops(static_cast<OpCode>(opcode), args);
}


Node *make_binop(uint32_t opcode, Node *lhs, Node *rhs) {
    return make_ops(opcode, {lhs, rhs});
}


Node *T(int value) {
    return new E_Int(value);
}


Node *V(const std::string &name) {
    return new E_Var(u8_decode(name));
}


Node *make_list(const std::vector<Node *> &args) {
    E_List *list = new E_List();
    for (Node *item : args) {
        list->value.emplace_back(item);
    }
    return list;
}


S_DeclareList *make_decl_list(const std::vector<std::pair<std::string, Node *>> &pairs) {
    S_DeclareList *decls = new S_DeclareList();
    for (const auto &p : pairs) {
        decls->decls.emplace_back(u8_decode(p.first), Node::Ptr(p.second));
    }
    return decls;
}

S_Block *make_block(const std::vector<Node *> stmts) {
    S_Block *block = new S_Block();
    for (Node *stmt : stmts) {
        block->stmts.emplace_back(stmt);
    }
    return block;
}


E_Func *make_func(Node *args, Node *block) {
    E_Func *func = new E_Func();
    func->args.reset(args);
    func->block.reset(block);
    return func;
}


S_Condition *make_cond(Node *test, Node *then_block, Node *else_block) {
    S_Condition *cond = new S_Condition();
    cond->condition.reset(test);
    cond->then_block.reset(then_block);
    cond->else_block.reset(else_block);
    return cond;
}


S_While *make_while(Node *test, Node *block) {
    S_While *wh = new S_While();
    wh->condition.reset(test);
    wh->block.reset(block);
    return wh;
}


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
        make_binop('()',
            make_binop('()',
                V("a"),
                make_ops(',', {T(1)})),
            make_ops(',', {T(2)})));
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
