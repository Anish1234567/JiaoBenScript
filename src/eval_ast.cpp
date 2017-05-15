#include <cassert>
#include <functional>
#include <utility>

#include "builtins.h"
#include "eval_ast.h"
#include "name_resolve.h"
#include "check_control_flow.h"
#include "string_fmt.hpp"


struct Signal {};


class BreakSignal : public Signal {};


class ContinueSignal : public Signal {};


class ReturnSignal : public Signal {
public:
    ReturnSignal(JBValue &value) : value(value) {}
    JBValue &value;
};


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


void AstInterpreter::eval_incomplete_raw_block(S_Block &block) {
    assert(this->cur_frame == nullptr);
    this->analyze_node(block);
    this->cur_frame = &this->create_frame(nullptr, block);
    this->handle_block(block);
}


void AstInterpreter::eval_raw_decl_list(S_DeclareList &decls) {
    this->analyze_node(decls);
    assert(this->cur_frame);
    Frame &frame = *this->cur_frame;
    // extend frame.vars
    frame.vars.reserve(frame.vars.size() + decls.decls.size());
    for (size_t i = 0; i < decls.decls.size(); ++i) {
        frame.vars.push_back(nullptr);
    }
    decls.accept(*this);
}


JBValue& AstInterpreter::eval_raw_exp(Node &exp) {
    this->analyze_node(exp);
    JBValue &ret = this->eval_exp(exp);
    return ret;
}


void AstInterpreter::eval_raw_stmt(Node &node) {
    if (S_DeclareList *decls = dynamic_cast<S_DeclareList *>(&node)) {
        this->eval_raw_decl_list(*decls);
    } else {
        this->analyze_node(node);
        node.accept(*this);
    }
}


void AstInterpreter::visit_block(S_Block &block) {
    auto _ = this->enter(block);
    this->handle_block(block);
}


void AstInterpreter::visit_program(Program &prog) {
    assert(this->cur_frame == nullptr);
    this->visit_block(prog);
}


void AstInterpreter::visit_declare_list(S_DeclareList &decls) {
    assert(this->cur_frame);
    Frame &frame = *this->cur_frame;
    for (size_t i = 0; i < decls.decls.size(); ++i) {
        const auto &pair = decls.decls[i];
        if (pair.initial) {
            assert(decls.attr.start_index + i < frame.vars.size());
            frame.vars[decls.attr.start_index + i] = &this->eval_exp(*pair.initial);
        }
    }
}


void AstInterpreter::visit_condition(S_Condition &cond) {
    JBValue &test = this->eval_exp(*cond.condition);
    if (this->builtins.is_truthy(test)) {
        S_Block &then_block = static_cast<S_Block &>(*cond.then_block);
        auto _ = this->enter(then_block);
        then_block.accept(*this);
    } else if (cond.else_block) {
        cond.else_block->accept(*this);
    }
}


void AstInterpreter::visit_while(S_While &wh) {
    S_Block &block = static_cast<S_Block &>(*wh.block);
    while (this->builtins.is_truthy(this->eval_exp(*wh.condition))) {
        auto _ = this->enter(block);
        try {
            block.accept(*this);
        } catch (BreakSignal &) {
            break;
        } catch (ContinueSignal &) {
            continue;
        }
    }
}


void AstInterpreter::visit_return(S_Return &ret) {
    if (ret.value) {
        throw ReturnSignal(this->eval_exp(*ret.value));
    } else{
        throw ReturnSignal(this->create<JBNull>());
    }
}


void AstInterpreter::visit_break(S_Break &) {
    throw BreakSignal();
}


void AstInterpreter::visit_continue(S_Continue &) {
    throw ContinueSignal();
}


void AstInterpreter::visit_stmt_exp(S_Exp &stmt) {
    this->eval_exp(*stmt.value);
}


void AstInterpreter::visit_stmt_empty(S_Empty &) {}


#define U(op_name) std::bind( \
    &Builtins::builtin_##op_name, &this->builtins, std::placeholders::_1 \
)

#define B(op_name) std::bind( \
    &Builtins::builtin_##op_name, &this->builtins, \
    std::placeholders::_1, std::placeholders::_2 \
)


void AstInterpreter::visit_op(E_Op &exp) {
    uint32_t code = static_cast<uint32_t>(exp.op_code);
    switch (code) {
    case '+':
        return this->handle_unary_or_binary_op(exp, U(pos), B(add));
    case '-':
        return this->handle_unary_or_binary_op(exp, U(neg), B(sub));
    case '*':
        return this->handle_binary_op(exp, B(mul));
    case '/':
        return this->handle_binary_op(exp, B(div));
    case '%':
        return this->handle_binary_op(exp, B(mod));
    case '<':
        return this->handle_binary_op(exp, B(lt));
    case '<=':
        return this->handle_binary_op(exp, B(le));
    case '>':
        return this->handle_binary_op(exp, B(gt));
    case '>=':
        return this->handle_binary_op(exp, B(ge));
    case '==':
        return this->handle_binary_op(exp, B(eq));
    case '!=':
        return this->handle_binary_op(exp, B(ne));
    case '!':
        return this->handle_unary_op(exp, U(not));
    case '&&':
        return this->handle_logic_and(exp);
    case '||':
        return this->handle_logic_or(exp);
    case '=':
        return this->handle_assign(exp);
    case '+=':
        return this->handle_binop_assign(exp, B(add));
    case '-=':
        return this->handle_binop_assign(exp, B(sub));
    case '*=':
        return this->handle_binop_assign(exp, B(mul));
    case '/=':
        return this->handle_binop_assign(exp, B(div));
    case '%=':
        return this->handle_binop_assign(exp, B(mod));
    case '()':
        return this->handle_call(exp);
    case '[]':
        return this->handle_getitem(exp);
    case ',':
        return this->handle_explist(exp);
    default:
        assert(!"Unreachable");
    }
}


#undef U
#undef B


void AstInterpreter::visit_var(E_Var &var) {
    if (JBValue *value = *this->resolve_var(var)) {
        this->return_value(*value);
    } else {
        throw JBError("Unbound variable: " + u8_encode(var.name), var.pos_start, var.pos_end);
    }
}


void AstInterpreter::visit_func(E_Func &func) {
    // TODO: set frame to nullptr if func.block and its children do not have non-local variable
    // S_Block &block = static_cast<S_Block &>(*func.block);
    assert(this->cur_frame);
    this->return_value(this->create<JBFunc>(this->cur_frame, func));
}


void AstInterpreter::visit_bool(E_Bool &bool_node) {
    this->return_value(this->create<JBBool>(bool_node.value));
}


void AstInterpreter::visit_int(E_Int &int_node) {
    this->return_value(this->create<JBInt>(int_node.value));
}


void AstInterpreter::visit_float(E_Float &float_node) {
    this->return_value(this->create<JBFloat>(float_node.value));
}


void AstInterpreter::visit_string(E_String &str) {
    this->return_value(this->create<JBString>(str.value));
}


void AstInterpreter::visit_list(E_List &list) {
    JBList &jblist = this->create<JBList>();
    for (Node::Ptr &item : list.value) {
        jblist.value.push_back(&this->eval_exp(*item));
    }
    this->return_value(jblist);
}


void AstInterpreter::visit_null(E_Null &) {
    return this->return_value(this->create<JBNull>());
}


Frame &AstInterpreter::create_frame(Frame *parent, S_Block &block) {
    Frame &frame = this->create<Frame>();
    frame.parent = parent;
    frame.block = &block;
    frame.vars = std::move(std::vector<JBValue *> (block.attr.local_info.size(), nullptr));
    return frame;
}


void AstInterpreter::return_value(JBValue &value) {
    this->returned = &value;
}


JBValue &AstInterpreter::eval_exp(Node &node) {
    node.accept(*this);
    assert(this->returned);
    JBValue *ret = nullptr;
    std::swap(this->returned, ret);
    return *ret;
}


ReplaceRestore<Frame *> AstInterpreter::enter(S_Block &block, Frame *parent_frame) {
    if (parent_frame == nullptr) {
        parent_frame = this->cur_frame;
    }
    return ReplaceRestore<Frame *>(&this->cur_frame, &this->create_frame(parent_frame, block));
}


JBValue **AstInterpreter::resolve_var(const E_Var &var) {
    assert(this->cur_frame);
    Frame &frame = *this->cur_frame;
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


void AstInterpreter::analyze_node(Node &node) {
    if (this->cur_frame) {
        assert(this->cur_frame->block);
    }

    S_Block *block = this->cur_frame ? this->cur_frame->block : nullptr;
    ::resolve_names_in_block(block, node);
    ::check_control_flow(node);
}


void AstInterpreter::handle_unary_or_binary_op(
    E_Op &exp, AstInterpreter::UnaryFunc unary_func, AstInterpreter::BinaryFunc binary_func)
{
    switch (exp.args.size()) {
    case 1:
        return this->handle_unary_op(exp, unary_func);
    case 2:
        return this->handle_binary_op(exp, binary_func);
    default:
        assert(!"Unreachable");
    }
}


void AstInterpreter::handle_binary_op(E_Op &exp, AstInterpreter::BinaryFunc binary_func) {
    assert(exp.args.size() == 2);
    this->return_value(binary_func(
        this->eval_exp(*exp.args[0]),
        this->eval_exp(*exp.args[1])
    ));
}


void AstInterpreter::handle_unary_op(E_Op &exp, AstInterpreter::UnaryFunc unary_func) {
    assert(exp.args.size() == 1);
    this->return_value(unary_func(this->eval_exp(*exp.args[0])));
}


void AstInterpreter::handle_logic_and(E_Op &exp) {
    assert(exp.op_code == OpCode::AND);
    assert(exp.args.size() == 2);
    JBValue &lhs = this->eval_exp(*exp.args[0]);
    if (!this->builtins.is_truthy(lhs)) {
        this->return_value(lhs);
    } else {
        // eval rhs
        this->return_value(this->eval_exp(*exp.args[1]));
    }
}


void AstInterpreter::handle_logic_or(E_Op &exp) {
    assert(exp.op_code == OpCode::AND);
    assert(exp.args.size() == 2);
    JBValue &lhs = this->eval_exp(*exp.args[0]);
    if (this->builtins.is_truthy(lhs)) {
        this->return_value(lhs);
    } else {
        // eval rhs
        this->return_value(this->eval_exp(*exp.args[1]));
    }
}


void AstInterpreter::handle_assign(E_Op &exp) {
    assert(exp.args.size() == 2);
    Node &lhs = *exp.args[0];
    JBValue &value = this->eval_exp(*exp.args[1]);
    this->do_assign(lhs, value);
}


void AstInterpreter::do_assign(Node &lhs, JBValue &value) {
    if (E_Var *var = dynamic_cast<E_Var *>(&lhs)) {
        *this->resolve_var(*var) = &value;
        this->return_value(value);
    } else if (E_Op *subscript = dynamic_cast<E_Op *>(&lhs)) {
        assert(subscript->op_code == OpCode::SUBSCRIPT);
        assert(subscript->args.size() == 2);
        this->return_value(this->builtins.builtin_setitem(
            this->eval_exp(*subscript->args[0]),
            this->eval_exp(*subscript->args[1]),
            value
        ));
    } else {
        assert(!"Unreachable");
    }
}


void AstInterpreter::handle_binop_assign(E_Op &exp, AstInterpreter::BinaryFunc binary_func) {
    assert(exp.args.size() == 2);
    Node &lhs = *exp.args[0];
    JBValue &result = binary_func(
        this->eval_exp(lhs),
        this->eval_exp(*exp.args[1])
    );
    this->do_assign(lhs, result);
}


void AstInterpreter::handle_call(E_Op &call) {
    assert(call.op_code == OpCode::CALL);
    assert(call.args.size() == 2);
    Node &lhs = *call.args[0];
    if (JBFunc *func = dynamic_cast<JBFunc *>(&this->eval_exp(lhs))) {
        E_Op &supplied = static_cast<E_Op &>(*call.args[1]);
        S_DeclareList *decl_list = nullptr;
        if (func->code.args) {
            decl_list = static_cast<S_DeclareList *>(func->code.args.get());
        }

        // check number of argument
        size_t func_max_args = decl_list ? decl_list->decls.size() : 0;
        if (supplied.args.size() > func_max_args) {
            // TODO: mark missing args
            throw JBError(string_fmt(
                "Bad call: too many args, expect %zu, got %zu",
                func_max_args, supplied.args.size()
            ), supplied.pos_start, supplied.pos_end);
        }
        if (supplied.args.size() < func_max_args) {
            if (!decl_list->decls[supplied.args.size()].initial) {
                // TODO: mark extra args
                throw JBError("Bad Call: missing args", supplied.pos_start, supplied.pos_end);
            }
        }

        // create new frame
        S_Block &func_block = static_cast<S_Block &>(*func->code.block);
        auto _ = this->enter(func_block, func->parent_frame);
        Frame &func_frame = *this->cur_frame;

        // eval arguments
        std::vector<JBValue *> arg_values;
        for (size_t i = 0; i < func_max_args; ++i) {
            if (i < supplied.args.size()) {
                // supplied arguments
                func_frame.vars[i] = &this->eval_exp(*supplied.args[i]);
            } else {
                // default arguments
                assert(decl_list->decls[i].initial);
                func_frame.vars[i] = &this->eval_exp(*decl_list->decls[i].initial);
            }
        }

        // execute function
        this->handle_func_body(func_block);
    } else {
        throw JBError("Bad call: not a function", lhs.pos_start, lhs.pos_end);
    }
}


void AstInterpreter::handle_func_body(S_Block &block) {
    try {
        this->handle_block(block);
    } catch (ReturnSignal &ret) {
        return this->return_value(ret.value);
    }
    // default return value of function of null
    this->return_value(this->create<JBNull>());
}


void AstInterpreter::handle_getitem(E_Op &exp) {
    assert(exp.op_code == OpCode::SUBSCRIPT);
    assert(exp.args.size() == 2);
    this->return_value(this->builtins.builtin_getitem(
        this->eval_exp(*exp.args[0]),
        this->eval_exp(*exp.args[1])
    ));
}


void AstInterpreter::handle_explist(E_Op &exp) {
    assert(exp.op_code == OpCode::EXPLIST);
    assert(exp.args.size() > 1);
    JBValue *ret = nullptr;
    for (Node::Ptr &item : exp.args) {
        ret = &this->eval_exp(*item);
    }
    this->return_value(*ret);   // last expression
}


void AstInterpreter::handle_block(S_Block &block) {
    for (Node::Ptr &stmt : block.stmts) {
        stmt->accept(*this);
    }
}
