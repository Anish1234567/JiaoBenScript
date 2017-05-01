#include "node_repr.h"
#include "node.h"


static std::string _make_indent(uint32_t indent) {
    return std::string(4 * indent, ' ');
}


std::string S_Block::repr(uint32_t indent) const {
    std::string ans;
    ans += _make_indent(indent) + "{\n";
    for (const Node::Ptr &child : this->stmts) {
        ans += child->repr(indent + 1) + "\n";
    }
    ans += _make_indent(indent) + "}";
    return ans;
}


// FIXME: function args
std::string S_DeclareList::repr(uint32_t indent) const {
    std::string ans;
    ans += _make_indent(indent) + "let ";

    int count = 0;
    std::string comma = "";
    for (const auto &pair : this->decls) {
        ans += comma;
        comma = ", ";
        ans += u8_encode(std::get<0>(pair));
        if (std::get<1>(pair)) {
            ans += " = " + std::get<1>(pair)->repr(indent + 1);
        }
        count++;
    }
    ans += ";";
    return ans;
}


std::string S_Condition::repr(uint32_t indent) const {
    std::string ans;
    ans += _make_indent(indent) + "if (" + this->condition->repr(indent + 1) + ")\n";
    ans += this->then_block->repr(indent);
    if (this->else_block) {
        ans += " else " + this->else_block->repr(indent);
    }
    return ans;
}


std::string S_While::repr(uint32_t indent) const {
    std::string ans;
    ans += _make_indent(indent) + "while (" + this->condition->repr(indent + 1) + ")\n";
    ans += this->block->repr(indent);
    return ans;
}


std::string S_Return::repr(uint32_t indent) const {
    if (!this->value) {
        return _make_indent(indent) + "return;";
    } else {
        return _make_indent(indent) + "return " + this->value->repr(indent + 1) + ";";
    }
}


std::string S_Break::repr(uint32_t indent) const {
    return _make_indent(indent) + "break;";
}

std::string S_Continue::repr(uint32_t indent) const {
    return _make_indent(indent) + "continue;";
}


std::string S_Exp::repr(uint32_t indent) const {
    return _make_indent(indent) + this->value->repr(indent + 1);
}


std::string S_Empty::repr(uint32_t indent) const {
    return _make_indent(indent) + ";";
}


static std::string opcode_to_string(OpCode opcode) {
    std::string ans;
    uint32_t u32 = static_cast<uint32_t>(opcode);
    for (size_t i = 0; i < 4; ++i) {
        char ch = static_cast<char>(u32 >> 24);
        if (ch != 0) {
            ans.push_back(ch);
        }
        u32 <<= 8;
    }
    return ans;
}


std::string E_Op::repr(uint32_t indent) const {
    uint32_t opint = static_cast<uint32_t>(this->op_code);
    std::string opstr = opcode_to_string(this->op_code);

    switch (opint) {
        case '+': case '-': case '*': case '/': case '%':
        case '<': case '<=': case '>': case '>=': case '==': case '!=':
        case '&&': case '||':
        case '=': case '+=': case '-=': case '*=': case '/=': case '%=':
            if (this->args.size() == 1) {
                assert(opint == '+' || opint == '-');
                return string_fmt(
                    "(%s%s)",
                    opstr.data(),
                    this->args[0]->repr(indent + 1).data()
                );
            } else {
                assert(this->args.size() == 2);
                return string_fmt(
                    "(%s %s %s)",
                    this->args[0]->repr(indent + 1).data(),
                    opstr.data(),
                    this->args[1]->repr(indent + 1).data()
                );
            }
        case '!':
            assert(this->args.size() == 1);
            return string_fmt("!(%s)", this->args[0]->repr(indent + 1).data());
        case '()':  // call
            if (this->args.size() == 1) {
                return this->args[0]->repr(indent + 1) + "()";
            } else {
                assert(this->args.size() == 2);
                return string_fmt(
                    "%s(%s)",
                    this->args[0]->repr(indent + 1).data(),
                    this->args[1]->repr(indent + 1).data()
                );
            }
        case '[]':
            assert(this->args.size() == 2);
            return string_fmt(
                "%s[%s]",
                this->args[0]->repr(indent + 1).data(),
                this->args[1]->repr(indent + 1).data()
            );
        case ',': {
            std::string ans;
            std::string comma = "";
            for (const Node::Ptr &child : this->args) {
                ans += comma;
                comma = ", ";
                ans += child->repr(indent + 1);
            }
            return ans;
        }
        default:
            assert(!"Unreachable");
    }
}


std::string E_Var::repr(uint32_t) const {
    return u8_encode(this->name);
}


std::string E_Func::repr(uint32_t indent) const {
    return string_fmt(
        "function(%s)\n%s",
        this->args ? this->args->repr().data() : "",
        this->block->repr(indent).data()
    );
}


std::string E_List::repr(uint32_t indent) const {
    std::string ans;
    ans += "[";
    std::string comma = "\n";
    for (const Node::Ptr &child : this->value) {
        ans += comma + _make_indent(indent + 1) + child->repr(indent + 1);
        comma = ",\n";
    }
    ans += "\n" + _make_indent(indent) + "]";
    return ans;
}


std::string E_Null::repr(uint32_t) const {
    return "null";
}
