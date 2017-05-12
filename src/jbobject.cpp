#include <utility>

#include "jbobject.h"
#include "eval_ast.h"


bool JBValue::eq(const JBValue &rhs) const {
    return *this == rhs;
}


bool JBValue::is_truthy() const {
    return true;
}


static std::pair<bool, double> to_double(const JBValue &value) {
    if (const JBInt *obj_int = dynamic_cast<const JBInt *>(&value)) {
        return {true, obj_int->value};
    } else if (const JBFloat *obj_float = dynamic_cast<const JBFloat *>(&value)) {
        return {true, obj_float->value};
    } else {
        return {false, 0};
    }
}


template<>
bool JBInt::eq(const JBValue &rhs) const {
    auto result = to_double(rhs);
    return result.first && this->value == result.second;
}


template<>
bool JBFloat::eq(const JBValue &rhs) const {
    auto result = to_double(rhs);
    return result.first && this->value == result.second;
}


template<class T>
bool value_eq(const T &lhs, const JBValue &rhs) {
    if (const T *other = dynamic_cast<const T *>(&rhs)) {
        return lhs.value == other->value;
    } else {
        return false;
    }
}


template<>
bool JBBool::eq(const JBValue &rhs) const {
    return value_eq(*this, rhs);
}


bool JBString::operator==(const JBValue &rhs) const {
    return value_eq(*this, rhs);
}


bool JBString::is_truthy() const {
    return !this->value.empty();
}


void JBList::each_ref(std::function<void(JBObject &)> callback) {
    for (JBValue *item : this->value) {
        callback(*item);
    }
}


bool JBList::is_truthy() const {
    return !this->value.empty();
}


bool JBList::operator==(const JBValue &rhs) const {
    const JBList *other = dynamic_cast<const JBList *>(&rhs);
    if (other == nullptr) {
        return false;
    }
    if (this->value.size() != other->value.size()) {
        return false;
    }
    for (size_t i = 0; i < this->value.size(); ++i) {
        if (*this->value[i] != *other->value[i]) {
            return false;
        }
    }
    return true;
}


void JBFunc::each_ref(std::function<void(JBObject &)> callback) {
    for (Frame *frame = this->parent_frame; frame != nullptr; frame = frame->parent) {
        callback(*frame);
    }
}


bool JBFunc::operator==(const JBValue &rhs) const {
    return this == &rhs;
}
