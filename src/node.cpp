#include "node.h"


bool Node::operator==(const Node &rhs) const {
    return typeid(*this) == typeid(rhs);
}


bool Node::operator!=(const Node &rhs) const {
    return !(*this == rhs);
}


static bool node_list_eq(const std::vector<Node::Ptr> &a, const std::vector<Node::Ptr> &b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (*a[i] != *b[i]) {
            return false;
        }
    }
    return true;
}


#define _TO_OTHER(Type) \
    const Type *other = dynamic_cast<const Type *>(&rhs); \
    if (other == nullptr) { return false; }


bool S_Block::operator==(const Node &rhs) const {
    _TO_OTHER(S_Block);
    return node_list_eq(this->stmts, other->stmts);
}


bool S_DeclareList::operator==(const Node &rhs) const {
    _TO_OTHER(S_DeclareList);
    if (this->decls.size() != other->decls.size()) {
        return false;
    }
    for (size_t i = 0; i < this->decls.size(); ++i) {
        if (std::get<0>(this->decls[i]) != std::get<0>(other->decls[i])) {
            return false;
        }
        const Node::Ptr &lhs_item = std::get<1>(this->decls[i]);
        const Node::Ptr &rhs_item = std::get<1>(other->decls[i]);
        if (!((!lhs_item && !rhs_item) || *lhs_item == *rhs_item)) {
            return false;
        }
    }
    return true;
}


#define _ATTR_EQ(name) (*this->name == *other->name)
#define _ATTR_EQ_OPT(name) (this->name ? _ATTR_EQ(name) : !other->name)


bool S_Condition::operator==(const Node &rhs) const {
    _TO_OTHER(S_Condition);
    return _ATTR_EQ(condition) && _ATTR_EQ(then_block) && _ATTR_EQ_OPT(else_block);
}


bool S_While::operator==(const Node &rhs) const {
    _TO_OTHER(S_While);
    return _ATTR_EQ(condition) && _ATTR_EQ(block);
}


bool S_Return::operator==(const Node &rhs) const {
    _TO_OTHER(S_Return);
    return _ATTR_EQ_OPT(value);
}


bool S_Exp::operator==(const Node &rhs) const {
    _TO_OTHER(S_Exp);
    return _ATTR_EQ(value);
}


bool E_Op::operator==(const Node &rhs) const {
    _TO_OTHER(E_Op);
    return this->op_code == other->op_code && node_list_eq(this->args, other->args);
}


bool E_Var::operator==(const Node &rhs) const {
    _TO_OTHER(E_Var);
    return this->name == other->name;
}


bool E_Func::operator==(const Node &rhs) const {
    _TO_OTHER(E_Func);
    return _ATTR_EQ_OPT(args) && _ATTR_EQ(block);
}


bool E_List::operator==(const Node &rhs) const {
    _TO_OTHER(E_List);
    return node_list_eq(this->value, other->value);
}


#undef _TO_OTHER
#undef _ATTR_EQ
#undef _ATTR_EQ_OPT
