#include <algorithm>
#include <iterator>
#include <vector>
#include <string>
#include "catch.hpp"

#include "../name_resolve.h"
#include "../node.h"
#include "../unicode.h"
#include "helper_node.hpp"


std::vector<S_Block::AttrType::VarInfo>
make_local_info(const std::vector<std::string> &names) {
    std::vector<S_Block::AttrType::VarInfo> ans;
    std::transform(
        names.begin(), names.end(), std::back_inserter(ans),
        [](const std::string &str) { return u8_decode(str); }
    );
    return ans;
}


std::vector<S_Block::AttrType::NonLocalInfo>
make_nonlocal_indexes(std::vector<S_Block::AttrType::NonLocalInfo> input) {
    return input;
}


TEST_CASE("Test name resolve basic") {
    S_Block *block = make_block({
        make_decl_list({
            {"a", nullptr},
            {"b", nullptr},
        }),
    });
    Node::Ptr g(block);

    resolve_names(*block);
    CHECK(block->attr.local_info == make_local_info({"a", "b"}));
}


TEST_CASE("Test name resolve S_DelcareList attribute") {
    S_DeclareList *d1 = make_decl_list({
        {"a", nullptr},
    });
    S_DeclareList *d2 = make_decl_list({
        {"b", nullptr},
    });
    S_Block *block = make_block({
        d1, d2,
    });
    Node::Ptr g(block);

    resolve_names(*block);
    CHECK(block->attr.local_info == make_local_info({"a", "b"}));
    CHECK(d1->attr.start_index == 0);
    CHECK(d2->attr.start_index == 1);
}


TEST_CASE("Test name resolve nested") {
    E_Var *va = V("a");
    E_Var *vb = V("b");
    E_Var *vc = V("c");
    S_Block *inner = make_block({
        make_decl_list({
            {"a", nullptr},
            {"b", nullptr},
        }),
        make_s_exp(va),
        make_s_exp(vb),
        make_s_exp(vc),
    });
    S_Block *outter = make_block({
        make_decl_list({
            {"c", nullptr},
            {"b", nullptr},
        }),
        inner,
    });
    Node::Ptr g(outter);

    resolve_names(*outter);
    CHECK(outter->attr.local_info == make_local_info({"c", "b"}));
    CHECK(outter->attr.nonlocal_indexes == make_nonlocal_indexes({}));
    CHECK(inner->attr.local_info == make_local_info({"a", "b"}));
    CHECK(inner->attr.nonlocal_indexes == make_nonlocal_indexes({
        {outter, 0},    // c
    }));
    CHECK(va->attr.is_local);
    CHECK(va->attr.index == 0);
    CHECK(vb->attr.is_local);
    CHECK(vb->attr.index == 1);
    CHECK_FALSE(vc->attr.is_local);
    CHECK(vc->attr.index == 0);
}


TEST_CASE("Test declaration list default value") {
    E_Var *vb = V("b");
    E_Var *vc = V("c");
    S_Block *block = make_block({
        make_decl_list({
            {"a", nullptr},
            {"b", vb},
            {"c", vc},
        }),
    });
    Node::Ptr g(block);

    resolve_names(*block);
    CHECK(vb->attr.is_local);   CHECK(vb->attr.index == 1);
    CHECK(vc->attr.is_local);   CHECK(vc->attr.index == 2);
}


TEST_CASE("Test name resovle function") {
    E_Var *ob = V("b");

    E_Var *fa = V("a");
    E_Var *fb = V("b");
    E_Var *fc = V("c");
    E_Func *func = make_func(
        make_decl_list({
            {"a", nullptr},
            {"b", ob},
        }),
        make_block({
            make_return(make_binop('+', fa, make_binop('-', fb, fc))),
        })
    );

    S_Block *outter = make_block({
        make_decl_list({
            {"a", nullptr},
            {"b", V("a")},
            {"c", V("b")},
        }),
        make_s_exp(func),
    });
    Node::Ptr g(outter);

    resolve_names(*outter);
    S_Block &func_block = static_cast<S_Block &>(*func->block);
    CHECK(func_block.attr.local_info == make_local_info({"a", "b"}));
    CHECK(func_block.attr.nonlocal_indexes == make_nonlocal_indexes({
        {outter, 1},    // b
        {outter, 2},    // c
    }));
    CHECK(fa->attr.is_local);   CHECK(fa->attr.index == 0);
    CHECK(fb->attr.is_local);   CHECK(fb->attr.index == 1);
    CHECK(!ob->attr.is_local);  CHECK(ob->attr.index == 0);
    CHECK(!fc->attr.is_local);  CHECK(fc->attr.index == 1);
}


TEST_CASE("Test name resolve condition") {
    E_Var *ta = V("a");
    E_Var *tb = V("b");
    E_Var *va = V("a");
    E_Var *vb = V("b");
    E_Var *vc = V("c");
    S_Block *then1 = make_block({
        make_s_exp(va)
    });
    S_Block *then2 = make_block({
        make_s_exp(vb),
    });
    S_Block *else_block = make_block({
        make_s_exp(vc),
    });
    S_Condition *cond = make_cond(
        make_binop('+', ta, V("b")),
        then1,
        make_cond(
            make_binop('+', tb, V("a")),
            then2,
            else_block
        )
    );
    S_Block *outter = make_block({
        make_decl_list({
            {"a", nullptr},
            {"b", nullptr},
            {"c", nullptr},
        }),
        cond,
    });
    Node::Ptr g(outter);

    resolve_names(*outter);
    CHECK(outter->attr.local_info == make_local_info({"a", "b", "c"}));
    CHECK(ta->attr.is_local);   CHECK(ta->attr.index == 0);
    CHECK(tb->attr.is_local);   CHECK(tb->attr.index == 1);

    CHECK(then1->attr.nonlocal_indexes == make_nonlocal_indexes({
        {outter, 0},
    }));
    CHECK(then2->attr.nonlocal_indexes == make_nonlocal_indexes({
        {outter, 1},
    }));
    CHECK(else_block->attr.nonlocal_indexes == make_nonlocal_indexes({
        {outter, 2},
    }));
    for (E_Var *vx : {va, vb, vc}) {
        CHECK_FALSE(vx->attr.is_local);
        CHECK(vx->attr.index == 0);
    }
}


TEST_CASE("Test name resolve misc") {
    E_Var *vt = V("t");
    E_Var *va = V("a");
    E_Var *vb = V("b");
    E_Var *vc = V("c");
    S_While *wh = make_while(vt, make_block({
        make_s_exp(va),
        make_list({V("a"), make_binop('+', vb, vc)}),
    }));
    S_Block *outter = make_block({
        make_decl_list({
            {"t", nullptr},
            {"a", nullptr},
            {"b", nullptr},
            {"c", nullptr},
        }),
        wh,
    });
    Node::Ptr g(outter);

    resolve_names(*outter);
    CHECK(vt->attr.is_local);   CHECK(vt->attr.index == 0);

    int index = 0;
    for (E_Var *vx : {va, vb, vc}) {
        CHECK_FALSE(vx->attr.is_local);
        CHECK(vx->attr.index == index++);
    }
}


TEST_CASE("Test use before declare") {
    std::vector<Node::Ptr> g;

    S_Block *block = make_block({
        make_decl_list({
            {"a", V("b")},
            {"b", T(1)},
        }),
    });
    g.emplace_back(block);

    CHECK_THROWS_AS(resolve_names(*block), NoSuchName);

    block = make_block({
        make_s_exp(V("a")),
        make_decl_list({
            {"a", T(1)},
        }),
    });
    g.emplace_back(block);

    CHECK_THROWS_AS(resolve_names(*block), NoSuchName);
}


TEST_CASE("Test no such name") {
    S_Block *block = make_block({
        make_s_exp(V("a")),
    });
    Node::Ptr _(block);

    CHECK_THROWS_AS(resolve_names(*block), NoSuchName);

    // resolve_names() should not corrupt block->attr if failed
    CHECK(block->attr.local_info.size() == 0);
    CHECK(block->attr.nonlocal_indexes.size() == 0);
    CHECK(block->attr.name_to_local_index.size() == 0);
    CHECK(block->attr.name_to_nonlocal_index.size() == 0);
    CHECK_THROWS_AS(resolve_names(*block), NoSuchName);
}
