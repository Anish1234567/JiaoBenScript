#include <cmath>
#include <functional>
#include <string>

#include "builtins.h"
#include "exceptions.h"


#define CAST(Type) Type *obj = dynamic_cast<Type *>(&lhs)


JBValue &Builtins::builtin_pos(JBValue &lhs) {
    if (CAST(JBInt)) {
        return lhs;
    } else if (CAST(JBFloat)) {
        return lhs;
    } else {
        throw JBError("Type error: expect number");
    }
}


JBValue &Builtins::builtin_neg(JBValue &lhs) {
    if (CAST(JBInt)) {
        return this->create<JBInt>(-obj->value);
    } else if (CAST(JBFloat)) {
        return this->create<JBFloat>(-obj->value);
    } else {
        throw JBError("Type error: expect number");
    }
}


static double jb_value_to_double(JBValue &lhs) {
    if (CAST(JBInt)) {
        return obj->value;
    } else if (CAST(JBFloat)) {
        return obj->value;
    } else {
        throw JBError("Type error: expect number");
    }
}


template<class IntOp, class FloatOp>
JBValue &do_simple_binop(Allocator &allocator, JBValue &lhs, JBValue &rhs) {
    if (dynamic_cast<JBInt *>(&lhs) && dynamic_cast<JBInt *>(&rhs)) {
        return *allocator.construct<JBInt>(IntOp()(
            static_cast<JBInt &>(lhs).value,
            static_cast<JBInt &>(rhs).value
        ));
    } else {
        return *allocator.construct<JBFloat>(FloatOp()(
            jb_value_to_double(lhs),
            jb_value_to_double(rhs)
        ));
    }
}

#define SIMPLE_BINOP(op_name) \
do_simple_binop<std::op_name<int64_t>, std::op_name<double>>(this->allocator, lhs, rhs)


JBValue &Builtins::builtin_add(JBValue &lhs, JBValue &rhs) {
    // TODO: string cat
    // TODO: list cat
    return SIMPLE_BINOP(plus);
}


JBValue &Builtins::builtin_sub(JBValue &lhs, JBValue &rhs) {
    return SIMPLE_BINOP(minus);
}


JBValue &Builtins::builtin_mul(JBValue &lhs, JBValue &rhs) {
    return SIMPLE_BINOP(multiplies);
}


JBValue &Builtins::builtin_div(JBValue &lhs, JBValue &rhs) {
    if (dynamic_cast<JBInt *>(&lhs) && dynamic_cast<JBInt *>(&rhs)) {
        if (static_cast<JBInt &>(rhs).value == 0) {
            throw JBError("Arithmetic error: zero division");
        } else {
            return *allocator.construct<JBInt>(
                static_cast<JBInt &>(lhs).value / static_cast<JBInt &>(rhs).value
            );
        }
    } else {
        return *allocator.construct<JBFloat>(
            jb_value_to_double(lhs) / jb_value_to_double(rhs)
        );
    }
}


JBValue &Builtins::builtin_mod(JBValue &lhs, JBValue &rhs) {
    if (dynamic_cast<JBInt *>(&lhs) && dynamic_cast<JBInt *>(&rhs)) {
        if (static_cast<JBInt &>(rhs).value == 0) {
            throw JBError("Arithmetic error: zero remainder");
        } else {
            return *allocator.construct<JBInt>(
                static_cast<JBInt &>(lhs).value % static_cast<JBInt &>(rhs).value
            );
        }
    } else {
        return *allocator.construct<JBFloat>(
            std::remainder(jb_value_to_double(lhs), jb_value_to_double(rhs))
        );
    }
}


#define CMP_IMPL(name, OP) \
JBValue &Builtins::builtin_##name(JBValue &lhs, JBValue &rhs) { \
    return *this->allocator.construct<JBBool>( \
        jb_value_to_double(lhs) OP jb_value_to_double(rhs)); \
}


// TODO: string comparison
CMP_IMPL(lt, <)
CMP_IMPL(le, <=)
CMP_IMPL(gt, >)
CMP_IMPL(ge, >=)


JBValue &Builtins::builtin_eq(JBValue &lhs, JBValue &rhs) {
    return *this->allocator.construct<JBBool>(lhs.eq(rhs));
}


JBValue &Builtins::builtin_ne(JBValue &lhs, JBValue &rhs) {
    return *this->allocator.construct<JBBool>(!lhs.eq(rhs));
}


JBBool &Builtins::builtin_truth(JBValue &value) {
    return *this->allocator.construct<JBBool>(this->is_truthy(value));
}


JBBool &Builtins::builtin_not(JBValue &value) {
    return *this->allocator.construct<JBBool>(!this->is_truthy(value));;
}


bool Builtins::is_truthy(JBValue &value) {
    return value.is_truthy();
}


static JBValue **getitem(JBValue &base, JBValue &offset) {
    if (JBList *list = dynamic_cast<JBList *>(&base)) {
        if (JBInt *int_obj = dynamic_cast<JBInt *>(&offset)) {
            if (0 <= int_obj->value && static_cast<size_t>(int_obj->value) < list->value.size()) {
                return &list->value[int_obj->value];
            } else {
                throw JBError(string_fmt(
                    "Index error: length=%zu, index=%d", list->value.size(), int_obj->value
                ));
            }
        } else {
            throw JBError("Type error: indices must be int");
        }
    } else {
        throw JBError("Type error: expect list");
    }
}


// TODO: string
JBValue &Builtins::builtin_setitem(JBValue &base, JBValue &offset, JBValue &value) {
    *getitem(base, offset) = &value;
    return value;
}


JBValue &Builtins::builtin_getitem(JBValue &base, JBValue &offset) {
    return **getitem(base, offset);
}


#undef CAST
#undef CMP_IMPL
