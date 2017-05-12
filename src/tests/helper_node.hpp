#ifndef HELPER_NODE_HPP
#define HELPER_NODE_HPP

#include "../node.h"


E_Op *make_ops(OpCode opcode, const std::vector<Node *> &args) {
    E_Op *exp = new E_Op(opcode);
    for (Node *item : args) {
        exp->args.emplace_back(item);
    }
    return exp;
}


E_Op *make_ops(uint32_t opcode, const std::vector<Node *> &args) {
    return make_ops(static_cast<OpCode>(opcode), args);
}


E_Op *make_binop(uint32_t opcode, Node *lhs, Node *rhs) {
    return make_ops(opcode, {lhs, rhs});
}


E_Op *make_call(Node *func, const std::vector<Node *> &args) {
    return make_binop('()', func, make_ops(',', args));
}


E_Int *T(int value) {
    return new E_Int(value);
}


E_Var *V(const std::string &name) {
    return new E_Var(u8_decode(name));
}


S_Exp *make_s_exp(Node *exp) {
    S_Exp *stmt = new S_Exp();
    stmt->value.reset(exp);
    return stmt;
}


E_List *make_list(const std::vector<Node *> &args) {
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


S_Return *make_return(Node *value) {
    S_Return *ret = new S_Return();
    ret->value.reset(value);
    return ret;
}


#endif  // HELPER_NODE_HPP
