#ifndef JIAOBENSCRIPT_EVAL_AST_H
#define JIAOBENSCRIPT_EVAL_AST_H

#include <functional>
#include <utility>
#include <vector>

#include "allocator.h"
#include "exceptions.h"
#include "builtins.h"
#include "jbobject.h"
#include "node.h"
#include "visitor.h"
#include "replace_restore.hpp"


class Frame : public JBObject {
public:
    Frame *parent = nullptr;
    S_Block *block = nullptr;
    std::vector<JBValue *> vars;

    virtual void each_ref(std::function<void (JBObject &)> callback) override;
};


class AstInterpreter : private NodeVisitor {
public:
    AstInterpreter() : allocator(), builtins(allocator) {}

    void set_builtin_table(const std::vector<std::pair<ustring, JBValue *>> &table);
    void set_default_builtin_table();
    void eval_incomplete_raw_block(S_Block &block);
    void eval_raw_decl_list(S_DeclareList &decls);
    JBValue &eval_raw_exp(Node &exp);
    void eval_raw_stmt(Node &node);

private:
    virtual void visit_block(S_Block &block);
    virtual void visit_program(Program &prog);
    virtual void visit_declare_list(S_DeclareList &decls);
    virtual void visit_condition(S_Condition &cond);
    virtual void visit_while(S_While &wh);
    virtual void visit_return(S_Return &ret);
    virtual void visit_break(S_Break &brk);
    virtual void visit_continue(S_Continue &cont);
    virtual void visit_stmt_exp(S_Exp &stmt);
    virtual void visit_stmt_empty(S_Empty &stmt);
    virtual void visit_op(E_Op &op);
    virtual void visit_var(E_Var &var);
    virtual void visit_func(E_Func &func);
    virtual void visit_bool(E_Bool &bool_node);
    virtual void visit_int(E_Int &int_node);
    virtual void visit_float(E_Float &float_node);
    virtual void visit_string(E_String &str);
    virtual void visit_list(E_List &list);
    virtual void visit_null(E_Null &nil);

    template<class T, class ...Args>
    T &create(Args &&...args) {
        return *this->allocator.construct<T>(std::forward<Args>(args)...);
    }
    Frame &create_frame(Frame *parent, S_Block &block);

    void return_value(JBValue &value);
    ReplaceRestore<Frame *> enter(S_Block &block, Frame *parent_frame = nullptr);
    JBValue &eval_exp(Node &node);
    JBValue **resolve_var(const E_Var &var);
    void analyze_node(Node &node);

    using UnaryFunc = std::function<JBValue &(JBValue &)>;
    using BinaryFunc = std::function<JBValue &(JBValue &, JBValue &)>;

    void handle_unary_or_binary_op(E_Op &exp, UnaryFunc unary_func, BinaryFunc binary_func);
    void handle_binary_op(E_Op &exp, BinaryFunc binary_func);
    void handle_unary_op(E_Op &exp, UnaryFunc unary_func);
    void handle_logic_and(E_Op &exp);
    void handle_logic_or(E_Op &exp);
    void handle_assign(E_Op &exp);
    void do_assign(Node &lhs, JBValue &value);
    void handle_binop_assign(E_Op &exp, BinaryFunc binary_func);
    void handle_call(E_Op &call);
    void handle_func_body(S_Block &block);
    void handle_getitem(E_Op &exp);
    void handle_explist(E_Op &exp);
    void handle_block(S_Block &block);

    Frame *cur_frame = nullptr;
    JBValue *returned = nullptr;

    Allocator allocator;
    Builtins builtins;
    Node::Ptr builtin_block;
};


#endif //JIAOBENSCRIPT_EVAL_AST_H
