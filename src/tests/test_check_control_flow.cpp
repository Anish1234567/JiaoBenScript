#include <vector>
#include "catch.hpp"

#include "../check_control_flow.h"
#include "helper_node.hpp"


static std::vector<Node::Ptr> g;


S_Block *mb(const std::vector<Node *> &stmts) {
    S_Block *block = make_block(stmts);
    g.emplace_back(block);
    return block;
}


TEST_CASE("Test bad stmt") {
    CHECK_THROWS_AS(check_control_flow(*mb({new S_Break()})), BadBreak);
    CHECK_THROWS_AS(check_control_flow(*mb({ new S_Continue() })), BadContinue);
    CHECK_THROWS_AS(check_control_flow(*mb({ make_return(T(1)) })), BadReturn);
    CHECK_THROWS_AS(check_control_flow(*mb({
        make_while(T(1), make_block({
            make_s_exp(make_func(nullptr, make_block({
                new S_Break})))}))})),
        BadBreak
    );
}


TEST_CASE("Test normal") {
    check_control_flow(*mb({
        make_while(T(1), make_block({
            new S_Break(),
            new S_Continue(),
            make_block({
                new S_Break(),
                new S_Continue()}),
            make_s_exp(make_func(
                make_decl_list({
                    {"a", make_func(
                        nullptr,
                        make_block({
                            make_return(T(2))}))}}),
                make_block({
                    make_block({
                        make_return((T(3)))}),
                    make_return(make_func(nullptr, make_block({
                        make_return(T(4))})))})))}))}));
}
