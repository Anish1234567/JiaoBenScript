#include "visitor.h"


void TraversalNodeVisitor::visit_block(S_Block &block) {
    for (Node::Ptr &stmt : block.stmts) {
        stmt->accept(*this);
    }
}


void TraversalNodeVisitor::visit_program(Program &prog) {
    this->visit_block(prog);
}


void TraversalNodeVisitor::visit_declare_list(S_DeclareList &decls) {
    for (const auto &pair : decls.decls) {
        if (pair.initial) {
            pair.initial->accept(*this);
        }
    }
}


void TraversalNodeVisitor::visit_condition(S_Condition &cond) {
    cond.condition->accept(*this);
    cond.then_block->accept(*this);
    if (cond.else_block) {
        cond.else_block->accept(*this);
    }
}


void TraversalNodeVisitor::visit_while(S_While &wh) {
    wh.condition->accept(*this);
    wh.block->accept(*this);
}


void TraversalNodeVisitor::visit_return(S_Return &ret) {
    if (ret.value) {
        ret.value->accept(*this);
    }
}


void TraversalNodeVisitor::visit_stmt_exp(S_Exp &stmt) {
    stmt.value->accept(*this);
}


void TraversalNodeVisitor::visit_op(E_Op &exp) {
    for (Node::Ptr &arg : exp.args) {
        arg->accept(*this);
    }
}


void TraversalNodeVisitor::visit_func(E_Func &func) {
    if (func.args) {
        func.args->accept(*this);
    }
    func.block->accept(*this);
}


void TraversalNodeVisitor::visit_list(E_List &list) {
    for (Node::Ptr &item : list.value) {
        item->accept(*this);
    }
}
