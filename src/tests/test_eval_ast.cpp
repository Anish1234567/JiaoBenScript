#include <functional>
#include <vector>
#include "catch.hpp"

#include "../eval_ast.h"
#include "../node.h"
#include "../name_resolve.h"
#include "../unicode.h"
#include "helper_node.hpp"


std::function<void (Node *, JBValue *)>
make_exp_checker(S_Block &block, Frame &frame, AstInterpreter &interp) {
    return [&](Node *exp, JBValue *expect) {
        Node::Ptr g(exp);
        resolve_names_in_block(block, *exp);
        if (expect != nullptr) {
            CHECK(interp.eval_exp(frame, *exp) == *expect);
        } else {
            // unchecked
            interp.eval_exp(frame, *exp);
        }
    };
}


TEST_CASE("Test basic") {
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

    resolve_names(*root_block);
    Frame &root_frame = interp.create_frame(nullptr, *root_block);
    interp.eval_block(root_frame, *root_block);

    auto eval_exp = [&](Node *exp) {
        g.emplace_back(exp);
        resolve_names_in_block(*root_block, *exp);
        interp.eval_exp(root_frame, *exp);
    };
    auto checker = make_exp_checker(*root_block, root_frame, interp);

    JBInt zero(0);
    JBInt one(1);
    JBInt two(2);
    JBInt three(3);
    JBInt four(4);

    SECTION("arithmeic expression") {
        checker(V("zero"), &zero);
        checker(make_binop('-', V("two"), V("one")), &one);
        checker(make_binop('+', V("two"), T(-1)), &one);
    }

    SECTION("unbound variable") {
        E_Var x(USTRING("x"));
        resolve_names_in_block(*root_block, x);
        CHECK_THROWS_AS(interp.eval_exp(root_frame, x), JBError);
    }

    SECTION("assign") {
        checker(make_binop('=', V("x"), T(1)), &one);
        checker(V("x"), &one);
    }

    SECTION("declare") {
        S_DeclareList *decls = make_decl_list({
            {"y", V("one")},
            {"z", nullptr},
        });
        g.emplace_back(decls);
        interp.eval_raw_decl_list(root_frame, static_cast<S_DeclareList &>(*decls));
        checker(V("y"), &one);
    }

    SECTION("function call") {
        E_Func *func = make_func(nullptr, make_block({
            make_return(V("one")),
        }));
        checker(make_call(func, {}), &one);
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
        checker(make_call(V("f1"), {T(0)}), &three);
        checker(make_call(V("f1"), {T(1)}), &four);
        // fill default args
        checker(make_call(V("f1"), {T(1), T(1)}), &three);

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
        checker(make_call(V("f2"), {}), &nil);
        checker(V("count"), &one);
        checker(make_call(V("f2"), {}), &nil);
        checker(V("count"), &two);
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
        checker(make_call(V("f3"), {}), &one);
        eval_exp(make_binop('=', V("x"), T(0)));
        checker(make_call(V("f3"), {}), &two);
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
        checker(make_call(V("f4"), {T(3), T(4)}), &seven);
        checker(make_call(V("f4"), {T(3), T(2)}), &zero);
    }

    SECTION("list") {
        // getitem
        checker(make_binop('[]', V("L"), V("one")), &two);
        checker(make_binop('[]', V("L"), V("zero")), &one);

        // setitem
        checker(
            make_binop('=',
                make_binop('[]', V("L"), V("one")),
                V("zero")),
            &zero
        );
        checker(make_binop('[]', V("L"), V("one")), &zero);

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
}
