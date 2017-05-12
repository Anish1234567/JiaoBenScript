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


struct Signal {};


class BreakSignal : public Signal {};


class ContinueSignal : public Signal {};


class ReturnSignal : public Signal {
public:
    ReturnSignal(JBValue &value) : value(value) {}
    JBValue &value;
};


class Frame : public JBObject {
public:
    Frame *parent = nullptr;
    S_Block *block = nullptr;
    std::vector<JBValue *> vars;

    virtual void each_ref(std::function<void (JBObject &)> callback) override;
};


class AstInterpreter {
public:
    AstInterpreter() : allocator(), builtins(allocator) {}

    Frame &create_frame(Frame *parent, S_Block &block);
    void eval_block(Frame &frame, S_Block &block);
    void eval_stmt(Frame &frame, Node &stmt);
    void eval_condition(Frame &frame, S_Condition &cond);
    void eval_while(Frame &frame, S_While &wh);
    void eval_decl_list(Frame &frame, S_DeclareList &decls);
    void eval_raw_decl_list(Frame &frame, S_DeclareList &decls);
    JBValue &eval_exp(Frame &frame, Node &exp);
    JBList  &eval_list(Frame &frame, E_List &list);
    JBValue &eval_op(Frame &frame, E_Op &exp);
    JBFunc  &eval_func(Frame &frame, E_Func &func);
    JBValue &eval_var(Frame &frame, E_Var &var);

private:
    using UnaryFunc = std::function<JBValue &(JBValue &)>;
    using BinaryFunc = std::function<JBValue &(JBValue &, JBValue &)>;

    JBValue **resolve_var(Frame &frame, const E_Var &var);
    JBValue &handle_unary_or_binary_op(
        Frame &frame, E_Op &exp, UnaryFunc unary_func, BinaryFunc binary_func
    );
    JBValue &handle_binary_op(Frame &frame, E_Op &exp, BinaryFunc binary_func);
    JBValue &handle_unary_op(Frame &frame, E_Op &exp, UnaryFunc unary_func);
    JBValue &handle_logic_and(Frame &frame, E_Op &exp);
    JBValue &handle_logic_or(Frame &frame, E_Op &exp);
    JBValue &handle_assign(Frame &frame, E_Op &exp);
    JBValue &do_assign(Frame &frame, Node &lhs, JBValue &value);
    JBValue &handle_binop_assign(Frame &frame, E_Op &exp, BinaryFunc binary_func);
    JBValue &handle_call(Frame &frame, E_Op &call);
    JBValue &handle_func_body(Frame &frame, S_Block &block);
    JBValue &handle_getitem(Frame &frame, E_Op &exp);
    JBValue &handle_explist(Frame &frame, E_Op &exp);

    template<class T, class ...Args>
    T &create(Args &&...args) {
        return *this->allocator.construct<T>(std::forward<Args>(args)...);
    }

    Allocator allocator;
    Builtins builtins;
};


#endif //JIAOBENSCRIPT_EVAL_AST_H
