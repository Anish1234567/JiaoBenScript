#include <functional>
#include <vector>
#include "catch.hpp"

#include "../exceptions.h"
#include "../eval_ast.h"
#include "../unicode.h"
#include "helper_node.hpp"


#define CHECK_EXP(exp, expect) \
    do { \
        Node *node = exp; \
        g.emplace_back(node); \
        REQUIRE(interp.eval_raw_exp(*node) == expect); \
    } while (0)


TEST_CASE("Test AstInterpreter") {
    std::vector<Node::Ptr> g;
    AstInterpreter interp;

    S_Block *root_block = make_block({
        make_decl_list({
            {"zero", T(0)},
            {"one", T(1)},
            {"two", T(2)},
            {"three", T(3)},
            {"negone", make_ops('-', {V("one")})},
            {"a", V("one")},
            {"b", V("two")},
            {"c", V("three")},
            {"count", V("zero")},
            {"x", nullptr},
            {"L", make_list({V("one"), V("two"), V("three")})},
            {"f1", nullptr},
            {"f2", nullptr},
            {"f3", nullptr},
            {"f4", nullptr},
        }),
    });
    g.emplace_back(root_block);

    interp.eval_incomplete_raw_block(*root_block);

    auto eval_exp = [&](Node *exp) {
        g.emplace_back(exp);
        interp.eval_raw_exp(*exp);
    };

    auto eval_stmt = [&](Node *stmt) {
        g.emplace_back(stmt);
        interp.eval_raw_stmt(*stmt);
    };

    JBInt zero(0);
    JBInt one(1);
    JBInt two(2);
    JBInt three(3);
    JBInt four(4);

    SECTION("arithmeic expression") {
        CHECK_EXP(V("zero"), zero);
        CHECK_EXP(make_binop('-', V("two"), V("one")), one);
        CHECK_EXP(make_binop('+', V("two"), T(-1)), one);
    }

    SECTION("unbound variable") {
        E_Var x(USTRING("x"));
        CHECK_THROWS_AS(interp.eval_raw_exp(x), JBError);
    }

    SECTION("assign") {
        CHECK_EXP(make_binop('=', V("x"), T(1)), one);
        CHECK_EXP(V("x"), one);
    }

    SECTION("declare") {
        S_DeclareList *decls = make_decl_list({
            {"y", V("one")},
            {"z", nullptr},
        });
        g.emplace_back(decls);
        interp.eval_raw_decl_list(static_cast<S_DeclareList &>(*decls));
        CHECK_EXP(V("y"), one);
    }

    SECTION("function call") {
        E_Func *func = make_func(nullptr, make_block({
            make_return(V("one")),
        }));
        CHECK_EXP(make_call(func, {}), one);
    }

    eval_exp(make_binop('=', V("f1"), make_func(
        make_decl_list({
            {"a", nullptr},
            {"b", V("b")}}),
        make_block({
            make_return(
                make_binop('+',
                    V("a"),
                    make_binop('+', V("b"), V("one"))))}))));

    SECTION("function call args") {
        // use default args
        CHECK_EXP(make_call(V("f1"), {T(0)}), three);
        CHECK_EXP(make_call(V("f1"), {T(1)}), four);
        // fill default args
        CHECK_EXP(make_call(V("f1"), {T(1), T(1)}), three);

        // too few args
        CHECK_THROWS_AS(eval_exp(make_call(V("f1"), {})), JBError);
        // too much args
        CHECK_THROWS_AS(eval_exp(make_call(V("f1"), {T(1), T(2), T(3)})), JBError);
    }

    eval_exp(make_binop('=', V("f2"), make_func(
        nullptr, make_block({
            make_s_exp(make_binop('+=', V("count"), V("one")))}))));

    JBNull nil;

    SECTION("function closure write") {
        CHECK_EXP(make_call(V("f2"), {}), nil);
        CHECK_EXP(V("count"), one);
        CHECK_EXP(make_call(V("f2"), {}), nil);
        CHECK_EXP(V("count"), two);
    }

    eval_exp(make_binop('=', V("f3"), make_func(
        nullptr, make_block({
            make_cond(
                V("x"),
                make_block({
                    make_return(V("one"))}),
                make_block({
                    make_return(V("two"))}))}))));

    SECTION("if-else") {
        eval_exp(make_binop('=', V("x"), T(1)));
        CHECK_EXP(make_call(V("f3"), {}), one);
        eval_exp(make_binop('=', V("x"), T(0)));
        CHECK_EXP(make_call(V("f3"), {}), two);
    }

    eval_exp(make_binop('=', V("f4"), make_func(
        make_decl_list({
            {"left", nullptr},
            {"right", nullptr},
        }),
        make_block({
            make_decl_list({
                {"i", T(-1)},
                {"sum", T(0)},
            }),
            make_while(
                // while (i < right)
                make_binop('<', V("i"), V("right")),
                make_block({
                    // if (left > right) break;
                    make_cond(
                        make_binop('>', V("left"), V("right")),
                        make_block({
                            new S_Break(),
                        }),
                        nullptr
                    ),
                    // i += 1;
                    make_s_exp(make_binop('+=', V("i"), T(1))),
                    // if (i < left) continue;
                    make_cond(
                        make_binop('<', V("i"), V("left")),
                        make_block({
                            new S_Continue(),
                        }),
                        nullptr
                    ),
                    // sum += i;
                    make_s_exp(make_binop('+=', V("sum"), V("i"))),
                })
            ),
            // return sum;
            make_return(V("sum")),
        })
    )));

    SECTION("while") {
        JBInt seven(7);
        CHECK_EXP(make_call(V("f4"), {T(3), T(4)}), seven);
        CHECK_EXP(make_call(V("f4"), {T(3), T(2)}), zero);
    }

    SECTION("list") {
        // getitem
        CHECK_EXP(make_binop('[]', V("L"), V("one")), two);
        CHECK_EXP(make_binop('[]', V("L"), V("zero")), one);

        // setitem
        CHECK_EXP(
            make_binop('=',
                make_binop('[]', V("L"), V("one")),
                V("zero")),
            zero
        );
        CHECK_EXP(make_binop('[]', V("L"), V("one")), zero);

        // range error
        CHECK_THROWS_AS(eval_exp(make_binop('[]', V("L"), V("three"))), JBError);
        CHECK_THROWS_AS(eval_exp(
            make_binop('=',
                make_binop('[]', V("L"), V("three")),
                V("zero"))),
            JBError
        );
        CHECK_THROWS_AS(eval_exp(make_binop('[]', V("L"), V("negone"))), JBError);
        CHECK_THROWS_AS(eval_exp(make_binop('[]', V("x"), V("zero"))), JBError);
    }

    SECTION("Bad stmt") {
        CHECK_THROWS_AS(eval_exp(make_func(
            nullptr, make_block({ new S_Break() }))),
            BadBreak
        );
        CHECK_THROWS_AS(eval_stmt(make_return(T(1))), BadReturn);
        CHECK_THROWS_AS(eval_stmt(make_block({ make_return(T(1)) })), BadReturn);
    }

    SECTION("factorial") {
        eval_stmt(make_decl_list({
            {"f", make_func(
                make_decl_list({{"n", nullptr}}),
                make_block({
                    make_cond(
                        make_binop('==', V("n"), T(0)),
                        make_block({
                            make_return(T(1))}),
                        make_block({
                            make_return(
                                make_binop('*',
                                    V("n"),
                                    make_call(
                                        V("f"),
                                        {make_binop('-', V("n"), T(1))})))}))}))}}));

        JBInt n120(120);
        CHECK_EXP(make_call(V("f"), {T(5)}), n120);
    }

    // TODO: test and, or, explist
}


TEST_CASE("Test set_builtin_table") {
    AstInterpreter interp;

    JBInt one(1);
    interp.set_builtin_table({
        {USTRING("one"), &one},
    });

    E_Var *v = V("one");
    Node::Ptr _(v);
    CHECK(interp.eval_raw_exp(*v) == one);
}


TEST_CASE("Test default builtin table") {
    AstInterpreter interp;
    interp.set_default_builtin_table();

    E_Op *call = make_call(V("print"), {T(1), T(2)});
    Node::Ptr _(call);
    CHECK(interp.eval_raw_exp(*call) == JBNull());
}
