#ifndef JIAOBENSCRIPT_VISITOR_H
#define JIAOBENSCRIPT_VISITOR_H

#include "node.h"


class NodeVisitor {
public:
    virtual ~NodeVisitor() {}

    virtual void visit_block(S_Block &) {}
    virtual void visit_program(Program &) {}
    virtual void visit_declare_list(S_DeclareList &) {}
    virtual void visit_condition(S_Condition &) {}
    virtual void visit_while(S_While &) {}
    virtual void visit_return(S_Return &) {}
    virtual void visit_break(S_Break &) {}
    virtual void visit_continue(S_Continue &) {}
    virtual void visit_stmt_exp(S_Exp &) {}
    virtual void visit_stmt_empty(S_Empty &) {}
    virtual void visit_op(E_Op &) {}
    virtual void visit_var(E_Var &) {}
    virtual void visit_func(E_Func &) {}
    virtual void visit_bool(E_Bool &) {}
    virtual void visit_int(E_Int &) {}
    virtual void visit_float(E_Float &) {}
    virtual void visit_string(E_String &) {}
    virtual void visit_list(E_List &) {}
    virtual void visit_null(E_Null &) {}
};


class TraversalNodeVisitor : public NodeVisitor {
public:
    virtual void visit_block(S_Block &block);
    virtual void visit_program(Program &prog);
    virtual void visit_declare_list(S_DeclareList &decls);
    virtual void visit_condition(S_Condition &cond);
    virtual void visit_while(S_While &wh);
    virtual void visit_return(S_Return &ret);
    virtual void visit_stmt_exp(S_Exp &stmt);
    virtual void visit_op(E_Op &exp);
    virtual void visit_func(E_Func &func);
    virtual void visit_list(E_List &list);
};


#endif //JIAOBENSCRIPT_VISITOR_H
