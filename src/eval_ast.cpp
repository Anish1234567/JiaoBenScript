#include <cassert>
#include <functional>
#include <utility>

#include "builtins.h"
#include "eval_ast.h"
#include "name_resolve.h"
#include "string_fmt.hpp"


void Frame::each_ref(std::function<void(JBObject &child)> callback) {
    for (JBObject *v : this->vars) {
        if (v != nullptr) {
            callback(*v);
        }
    }
    if (this->parent != nullptr) {
        callback(*this->parent);
    }
}


Frame &AstInterpreter::create_frame(Frame *parent, S_Block &block) {
    Frame &frame = this->create<Frame>();
    frame.parent = parent;
    frame.block = &block;
    frame.vars = std::move(std::vector<JBValue *> (block.attr.local_info.size(), nullptr));
    return frame;
}


void AstInterpreter::eval_block(Frame &frame, S_Block &block) {
    for (Node::Ptr &stmt : block.stmts) {
        this->eval_stmt(frame, *stmt);
    }
}


void AstInterpreter::eval_stmt(Frame &frame, Node &stmt) {
    if (S_Exp *exp = dynamic_cast<S_Exp *>(&stmt)) {
        this->eval_exp(frame, *exp->value);
    } else if (S_Block *block = dynamic_cast<S_Block *>(&stmt)) {
        Frame &block_frame = this->create_frame(&frame, *block);
        this->eval_block(block_frame, *block);
    } else if (S_Condition *cond = dynamic_cast<S_Condition *>(&stmt)) {
        this->eval_condition(frame, *cond);
    } else if (S_While *wh = dynamic_cast<S_While *>(&stmt)) {
        this->eval_while(frame, *wh);
    } else if (S_DeclareList *decls = dynamic_cast<S_DeclareList *>(&stmt)) {
        this->eval_decl_list(frame, *decls);
    } else if (dynamic_cast<S_Empty *>(&stmt)) {
        // pass
    } else if (S_Return *ret = dynamic_cast<S_Return *>(&stmt)) {
        if (ret->value) {
            throw ReturnSignal(this->eval_exp(frame, *ret->value));
        } else{
            throw ReturnSignal(this->create<JBNull>());
        }
    } else if (dynamic_cast<S_Break *>(&stmt)) {
        throw BreakSignal();
    } else if (dynamic_cast<S_Continue *>(&stmt)) {
        throw ContinueSignal();
    } else {
        assert(!"Unreachable");
    }
}


void AstInterpreter::eval_condition(Frame &frame, S_Condition &cond) {
    JBValue &test = this->eval_exp(frame, *cond.condition);
    if (this->builtins.is_truthy(test)) {
        S_Block &then_block = static_cast<S_Block &>(*cond.then_block);
        Frame &then_frame = this->create_frame(&frame, then_block);
        this->eval_block(then_frame, then_block);
    } else if (cond.else_block) {
        this->eval_stmt(frame, *cond.else_block);
    }
}


void AstInterpreter::eval_while(Frame &frame, S_While &wh) {
    while (this->builtins.is_truthy(this->eval_exp(frame, *wh.condition))) {
        S_Block &block = static_cast<S_Block &>(*wh.block);
        Frame &block_frame = this->create_frame(&frame, block);
        try {
            this->eval_block(block_frame, block);
        } catch (BreakSignal &) {
            break;
        } catch (ContinueSignal &) {
            continue;
        }
    }
}


void AstInterpreter::eval_decl_list(Frame &frame, S_DeclareList &decls) {
    for (size_t i = 0; i < decls.decls.size(); ++i) {
        const auto &pair = decls.decls[i];
        if (pair.initial) {
            frame.vars[decls.attr.start_index + i] = &this->eval_exp(frame, *pair.initial);
        }
    }
}


void AstInterpreter::eval_raw_decl_list(Frame &frame, S_DeclareList &decls) {
    assert(frame.block);
    resolve_names_in_block(*frame.block, decls);
    // extend frame.vars
    frame.vars.reserve(frame.vars.size() + decls.decls.size());
    for (size_t i = 0; i < decls.decls.size(); ++i) {
        frame.vars.push_back(nullptr);
    }
    this->eval_decl_list(frame, decls);
}


JBValue &AstInterpreter::eval_exp(Frame &frame, Node &exp) {
    if (E_Bool *b = dynamic_cast<E_Bool *>(&exp)) {
        return this->create<JBBool>(b->value);
    } else if (E_Int *int_obj = dynamic_cast<E_Int *>(&exp)) {
        return this->create<JBInt>(int_obj->value);
    } else if (E_Float *float_obj = dynamic_cast<E_Float *>(&exp)) {
        return this->create<JBFloat>(float_obj->value);
    } else if (E_String *str_obj = dynamic_cast<E_String *>(&exp)) {
        return this->create<JBString>(str_obj->value);
    } else if (dynamic_cast<E_Null *>(&exp)) {
        return this->create<JBNull>();
    } else if (E_List *list = dynamic_cast<E_List *>(&exp)) {
        return this->eval_list(frame, *list);
    } else if (E_Op *op = dynamic_cast<E_Op *>(&exp)) {
        return this->eval_op(frame, *op);
    } else if (E_Func *func = dynamic_cast<E_Func *>(&exp)) {
        return this->eval_func(frame, *func);
    } else if (E_Var *var = dynamic_cast<E_Var *>(&exp)) {
        return this->eval_var(frame, *var);
    } else {
        assert(!"Unreachable");
    }
}


JBList &AstInterpreter::eval_list(Frame &frame, E_List &list) {
    JBList &jblist = this->create<JBList>();
    for (Node::Ptr &item : list.value) {
        jblist.value.push_back(&this->eval_exp(frame, *item));
    }
    return jblist;
}


JBValue **AstInterpreter::resolve_var(Frame &frame, const E_Var &var) {
    if (var.attr.is_local) {
        return &frame.vars[var.attr.index];
    } else {
        auto nli = frame.block->attr.nonlocal_indexes[var.attr.index];
        Frame *parent_frame = frame.parent;
        assert(parent_frame);
        while (nli.parent != parent_frame->block) {
            parent_frame = parent_frame->parent;
            assert(parent_frame);
        }
        return &parent_frame->vars[nli.index];
    }
}


JBFunc &AstInterpreter::eval_func(Frame &frame, E_Func &func) {
    // TODO: set frame to nullptr if func.block and its children do not have non-local variable
    // S_Block &block = static_cast<S_Block &>(*func.block);
    return this->create<JBFunc>(&frame, func);
}


JBValue &AstInterpreter::eval_var(Frame &frame, E_Var &var) {
    if (JBValue *value = *this->resolve_var(frame, var)) {
        return *value;
    } else {
        throw JBError("Unbound variable: " + u8_encode(var.name));
    }
}


#define U(op_name) std::bind( \
    &Builtins::builtin_##op_name, &this->builtins, std::placeholders::_1 \
)

#define B(op_name) std::bind( \
    &Builtins::builtin_##op_name, &this->builtins, \
    std::placeholders::_1, std::placeholders::_2 \
)


JBValue &AstInterpreter::eval_op(Frame &frame, E_Op &exp) {
    uint32_t code = static_cast<uint32_t>(exp.op_code);
    switch (code) {
    case '+':
        return this->handle_unary_or_binary_op(frame, exp, U(pos), B(add));
    case '-':
        return this->handle_unary_or_binary_op(frame, exp, U(neg), B(sub));
    case '*':
        return this->handle_binary_op(frame, exp, B(mul));
    case '/':
        return this->handle_binary_op(frame, exp, B(div));
    case '%':
        return this->handle_binary_op(frame, exp, B(mod));
    case '<':
        return this->handle_binary_op(frame, exp, B(lt));
    case '<=':
        return this->handle_binary_op(frame, exp, B(le));
    case '>':
        return this->handle_binary_op(frame, exp, B(gt));
    case '>=':
        return this->handle_binary_op(frame, exp, B(ge));
    case '==':
        return this->handle_binary_op(frame, exp, B(eq));
    case '!=':
        return this->handle_binary_op(frame, exp, B(ne));
    case '!':
        return this->handle_unary_op(frame, exp, U(not));
    case '&&':
        return this->handle_logic_and(frame, exp);
    case '||':
        return this->handle_logic_or(frame, exp);
    case '=':
        return this->handle_assign(frame, exp);
    case '+=':
        return this->handle_binop_assign(frame, exp, B(add));
    case '-=':
        return this->handle_binop_assign(frame, exp, B(sub));
    case '*=':
        return this->handle_binop_assign(frame, exp, B(mul));
    case '/=':
        return this->handle_binop_assign(frame, exp, B(div));
    case '%=':
        return this->handle_binop_assign(frame, exp, B(mod));
    case '()':
        return this->handle_call(frame, exp);
    case '[]':
        return this->handle_getitem(frame, exp);
    case ',':
        return this->handle_explist(frame, exp);
    default:
        assert(!"Unreachable");
    }
}


#undef U
#undef B


JBValue &AstInterpreter::handle_unary_or_binary_op(
    Frame &frame, E_Op &exp,
    AstInterpreter::UnaryFunc unary_func, AstInterpreter::BinaryFunc binary_func)
{
    switch (exp.args.size()) {
    case 1:
        return this->handle_unary_op(frame, exp, unary_func);
    case 2:
        return this->handle_binary_op(frame, exp, binary_func);
    default:
        assert(!"Unreachable");
    }
}


JBValue &AstInterpreter::handle_binary_op(
    Frame &frame, E_Op &exp, AstInterpreter::BinaryFunc binary_func)
{
    assert(exp.args.size() == 2);
    return binary_func(
        this->eval_exp(frame, *exp.args[0]),
        this->eval_exp(frame, *exp.args[1])
    );
}


JBValue &AstInterpreter::handle_unary_op(
    Frame &frame, E_Op &exp, AstInterpreter::UnaryFunc unary_func)
{
    assert(exp.args.size() == 1);
    return unary_func(this->eval_exp(frame, *exp.args[0]));
}


JBValue &AstInterpreter::handle_logic_and(Frame &frame, E_Op &exp) {
    assert(exp.op_code == OpCode::AND);
    assert(exp.args.size() == 2);
    JBValue &lhs = this->eval_exp(frame, *exp.args[0]);
    if (!this->builtins.is_truthy(lhs)) {
        return lhs;
    } else {
        // eval rhs
        return this->eval_exp(frame, *exp.args[1]);
    }
}


JBValue &AstInterpreter::handle_logic_or(Frame &frame, E_Op &exp) {
    assert(exp.op_code == OpCode::OR);
    assert(exp.args.size() == 2);
    JBValue &lhs = this->eval_exp(frame, *exp.args[0]);
    if (this->builtins.is_truthy(lhs)) {
        return lhs;
    } else {
        // eval rhs
        return this->eval_exp(frame, *exp.args[1]);
    }
}


JBValue &AstInterpreter::handle_assign(Frame &frame, E_Op &exp) {
    assert(exp.args.size() == 2);
    Node &lhs = *exp.args[0];
    JBValue &value = this->eval_exp(frame, *exp.args[1]);
    return this->do_assign(frame, lhs, value);
}


JBValue &AstInterpreter::do_assign(Frame &frame, Node &lhs, JBValue &value) {
    if (E_Var *var = dynamic_cast<E_Var *>(&lhs)) {
        *this->resolve_var(frame, *var) = &value;
        return value;
    } else if (E_Op *subscript = dynamic_cast<E_Op *>(&lhs)) {
        assert(subscript->op_code == OpCode::SUBSCRIPT);
        assert(subscript->args.size() == 2);
        return this->builtins.builtin_setitem(
            this->eval_exp(frame, *subscript->args[0]),
            this->eval_exp(frame, *subscript->args[1]),
            value
        );
    } else {
        assert(!"Unreachable");
    }
}

JBValue &AstInterpreter::handle_binop_assign(
    Frame &frame, E_Op &exp, AstInterpreter::BinaryFunc binary_func)
{
    assert(exp.args.size() == 2);
    Node &lhs = *exp.args[0];
    JBValue &result = binary_func(
        this->eval_exp(frame, lhs),
        this->eval_exp(frame, *exp.args[1])
    );
    return this->do_assign(frame, lhs, result);
}


JBValue &AstInterpreter::handle_call(Frame &frame, E_Op &call) {
    assert(call.op_code == OpCode::CALL);
    assert(call.args.size() == 2);
    JBValue &lhs = this->eval_exp(frame, *call.args[0]);
    if (JBFunc *func = dynamic_cast<JBFunc *>(&lhs)) {
        E_Op &args = static_cast<E_Op &>(*call.args[1]);
        S_DeclareList *decl_list = nullptr;
        if (func->code.args) {
            decl_list = static_cast<S_DeclareList *>(func->code.args.get());
        }

        // check number of argument
        size_t func_max_args = decl_list ? decl_list->decls.size() : 0;
        if (args.args.size() > func_max_args) {
            throw JBError(string_fmt(
                "Bad call: too many args, expect %zu, got %zu",
                func_max_args, args.args.size()
            ));
        }
        if (args.args.size() < func_max_args) {
            if (!decl_list->decls[args.args.size()].initial) {
                throw JBError("Bad Call: missing args");
            }
        }

        // create new frame
        S_Block &func_block = static_cast<S_Block &>(*func->code.block);
        Frame &func_frame = this->create_frame(func->parent_frame, func_block);

        // eval arguments
        std::vector<JBValue *> arg_values;
        for (size_t i = 0; i < func_max_args; ++i) {
            if (i < args.args.size()) {
                // supplied arguments
                func_frame.vars[i] = &this->eval_exp(frame, *args.args[i]);
            } else {
                // default arguments
                assert(decl_list->decls[i].initial);
                func_frame.vars[i] = &this->eval_exp(func_frame, *decl_list->decls[i].initial);
            }
        }

        // execute function
        return this->handle_func_body(func_frame, func_block);
    } else {
        throw JBError("Bad call: not a function");
    }
}


JBValue &AstInterpreter::handle_func_body(Frame &frame, S_Block &block) {
    try {
        this->eval_block(frame, block);
    } catch (ReturnSignal &ret) {
        return ret.value;
    }
    // default return value of function of null
    return this->create<JBNull>();
}


JBValue &AstInterpreter::handle_getitem(Frame &frame, E_Op &exp) {
    assert(exp.op_code == OpCode::SUBSCRIPT);
    assert(exp.args.size() == 2);
    return this->builtins.builtin_getitem(
        this->eval_exp(frame, *exp.args[0]),
        this->eval_exp(frame, *exp.args[1])
    );
}


JBValue &AstInterpreter::handle_explist(Frame &frame, E_Op &exp) {
    assert(exp.op_code == OpCode::EXPLIST);
    assert(exp.args.size() > 1);
    JBValue *ret = nullptr;
    for (Node::Ptr &item : exp.args) {
        ret = &this->eval_exp(frame, *item);
    }
    return *ret;    // last expression
}
