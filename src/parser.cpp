#include <algorithm>
#include <cassert>
#include <utility>

#include "parser.h"


void Parser::start_program() {
    this->enter_program();
}


void Parser::start_repl() {
    this->enter_program_or_exp();
}


bool Parser::can_end() const {
    assert(!this->states.empty());
    assert(
        this->states.front() == ParserState::PROGRAM_OR_EXP
        || this->states.front() == ParserState::PROGRAM
    );

    auto stop = --this->states.rend();
    for (auto it = this->states.rbegin(); it != stop; ++it) {
        switch (*it) {
        // can terminate a statement
        case ParserState::COND_ELSE:
        case ParserState::COND_END:
        case ParserState::WHILE_END:

        // can transit to expression mode and terminate
        case ParserState::STMT_EXP:

        // can terminate an expression
        case ParserState::EXP_LIST:
        case ParserState::EXP_LIST_ABS:
        case ParserState::EXP_ASSIGN:
        case ParserState::EXP_ASSIGN_END:

        case ParserState::EXP_OR:
        case ParserState::EXP_OR_END:
        case ParserState::EXP_AND:
        case ParserState::EXP_AND_END:
        case ParserState::EXP_EQ:
        case ParserState::EXP_EQ_END:
        case ParserState::EXP_CMP:
        case ParserState::EXP_CMP_END:
        case ParserState::EXP_A:
        case ParserState::EXP_A_END:
        case ParserState::EXP_X:
        case ParserState::EXP_X_END:

        case ParserState::EXP_X_HEAD_END:
        case ParserState::EXP_NOT_END:
        case ParserState::EXP_CALL_OR_SUBS:
        case ParserState::FUNC_END:
            continue;
        default:
            return false;
        }
    }
    return true;
}


void Parser::leave() {
    this->states.pop_back();
}


void Parser::pass_up(const Token &tok) {
    this->leave();
    this->feed(tok);
}


void Parser::shift(ParserState state) {
    this->states.back() = state;
}


Node::Ptr Parser::pop_result() {
    assert(this->can_end());
    this->feed(Token(TokenCode::END));
    assert(this->nodes.size() == 1);
    assert(this->states.size() == 1);

    Node::Ptr ret = std::move(this->nodes.back());
    this->nodes.pop_back();
    this->states.pop_back();
    return ret;
}


void Parser::unpected_token(const Token &tok, const std::string &addtional) {
    std::string msg = "Unexpected token " + tok.repr_short();
    if (!addtional.empty()) {
        msg += " " + addtional;
    }
    throw ParserError(msg, tok.pos_start, tok.pos_end);
}


bool Parser::match_id(const Token &tok, const ustring &id) {
    const TokenId *idtok = dynamic_cast<const TokenId *>(&tok);
    return idtok != nullptr && idtok->value == id;
}


void Parser::feed(const Token &tok) {
    // TODO: replace ParserState with member function pointer
    if (this->states.empty()) {
        return this->unpected_token(tok, "parser not started");
    }
    if (tok.tokencode == TokenCode::COMMENT) {
        return;
    }

    ParserState cur = this->states.back();
    if (cur == ParserState::END) {
        this->unpected_token(tok, "parser ended");
    } else if (cur == ParserState::PROGRAM || cur == ParserState::PROGRAM_OR_EXP) {
        Program *p = this->get_top2<Program>();
        p->stmts.emplace_back(this->pop_top());
        this->set_pos_start(*p->stmts.front());
        if (tok.tokencode == TokenCode::END) {
            this->set_pos_end(*p->stmts.back());
            this->shift(ParserState::END);
        } else {
            this->enter_stmt();
            this->feed(tok);
        }
    } else if (cur == ParserState::STMT) {
        if (tok.tokencode == TokenCode::SEMICOLON) {
            // empty stmt
            this->nodes.emplace_back(new S_Empty());
            this->set_pos_start_end(tok);
            this->leave();
        } else if (tok.tokencode == TokenCode::LBRACE) {
            this->leave();
            this->enter_block(tok);
        } else if (match_id(tok, USTRING("return"))) {
            this->leave();
            this->enter_return(tok);
        } else if (match_id(tok, USTRING("continue"))) {
            this->leave();
            this->enter_continue(tok);
        } else if (match_id(tok, USTRING("break"))) {
            this->leave();
            this->enter_break(tok);
        } else if (match_id(tok, USTRING("let"))) {
            this->leave();
            this->enter_var_decl(tok);
        } else if (match_id(tok, USTRING("if"))) {
            this->leave();
            this->enter_condition(tok);
        } else if (match_id(tok, USTRING("while"))) {
            this->leave();
            this->enter_while(tok);
        }
        // TODO: do-while, for
        else {
            this->shift(ParserState::STMT_EXP);
            this->enter_exp();
            this->feed(tok);
        }
    } else if (cur == ParserState::STMT_EXP) {
        if (tok.tokencode == TokenCode::SEMICOLON) {
            // replace expression with S_Exp
            S_Exp *stmt = new S_Exp();
            stmt->value.reset(this->pop_top());
            stmt->pos_start = stmt->value->pos_start;
            this->nodes.emplace_back(stmt);
            this->set_pos_end(tok);
            this->leave();
        } else if (this->states.front() == ParserState::PROGRAM_OR_EXP) {
            if (tok.tokencode == TokenCode::END) {
                assert(this->nodes.size() == 2);
                // replace Program with expression
                this->nodes.front().reset(this->nodes.back().release());
                this->nodes.pop_back();

                assert(this->states.size() == 2);
                this->leave();
                this->shift(ParserState::END);
            } else {
                this->unpected_token(tok, "expect semicolon or END");
            }
        } else {
            this->unpected_token(tok, "expect semicolon");
        }
    } else if (cur == ParserState::BLOCK) {
        if (tok.tokencode == TokenCode::RBRACE) {
            this->set_pos_end(tok);
            // empty block
            this->leave();
        } else {
            this->shift(ParserState::BLOCK_END);
            this->enter_stmt();
            this->feed(tok);
        }
    } else if (cur == ParserState::BLOCK_END) {
        S_Block *block = this->get_top2<S_Block>();
        block->stmts.emplace_back(this->pop_top());
        if (tok.tokencode == TokenCode::RBRACE) {
            this->set_pos_end(tok);
            this->leave();
        } else {
            this->enter_stmt();
            this->feed(tok);
        }
    } else if (cur == ParserState::BLOCK_PRE) {
        if (tok.tokencode == TokenCode::LBRACE) {
            this->leave();
            this->enter_block(tok);
        } else {
            this->unpected_token(tok, "expect {");
        }
    } else if (cur == ParserState::RETURN) {
        if (tok.tokencode == TokenCode::SEMICOLON) {
            this->set_pos_end(tok);
            this->leave();
        } else {
            this->shift(ParserState::RETURN_EXP);
            this->enter_exp();
            this->feed(tok);
        }
    } else if (cur == ParserState::RETURN_EXP) {
        if (tok.tokencode == TokenCode::SEMICOLON) {
            S_Return *ret = this->get_top2<S_Return>();
            ret->value.reset(this->pop_top());
            this->set_pos_end(tok);
            this->leave();
        } else {
            this->unpected_token(tok, "expect semicolon");
        }
    } else if (cur == ParserState::COND_LPAR) {
        if (tok.tokencode == TokenCode::LPAR) {
            this->shift(ParserState::COND_RPAR);
            this->enter_exp();
        } else {
            this->unpected_token(tok, "expect (");
        }
    } else if (cur == ParserState::COND_RPAR) {
        if (tok.tokencode == TokenCode::RPAR) {
            S_Condition *cond = this->get_top2<S_Condition>();
            cond->condition.reset(this->pop_top());
            this->shift(ParserState::COND_ELSE);
            this->enter_block_pre();
        } else {
            this->unpected_token(tok, "expect )");
        }
    } else if (cur == ParserState::COND_ELSE) {
        S_Condition *cond = this->get_top2<S_Condition>();
        cond->then_block.reset(this->pop_top());
        if (match_id(tok, USTRING("else"))) {
            this->shift(ParserState::COND_END);
            this->enter_else();
        } else {
            this->set_pos_end(*cond->then_block);
            this->pass_up(tok);
        }
    } else if (cur == ParserState::COND_END) {
        S_Condition *cond = this->get_top2<S_Condition>();
        cond->else_block.reset(this->pop_top());
        this->set_pos_end(*cond->else_block);
        this->pass_up(tok);
    } else if (cur == ParserState::ELSE) {
        if (match_id(tok, USTRING("if"))) {
            this->leave();
            this->enter_condition(tok);
        } else if (tok.tokencode == TokenCode::LBRACE) {
            this->leave();
            this->enter_block(tok);
        } else {
            this->unpected_token(tok, "expect 'if' or block");
        }
    } else if (
        cur == ParserState::CONTINUE || cur == ParserState::BREAK
        || cur == ParserState::VAR_DECL)
    {
        if (tok.tokencode == TokenCode::SEMICOLON) {
            this->set_pos_end(tok);
            this->leave();
        } else {
            this->unpected_token(tok, "expect semicolon");
        }
    } else if (cur == ParserState::VAR_DECL_LIST) {
        if (tok.tokencode == TokenCode::COMMA) {
            this->enter_var_decl_item();    // item collected in VAR_DECL_ITEM state
        } else {
            this->pass_up(tok);
        }
    } else if (cur == ParserState::VAR_DECL_ITEM) {
        S_DeclareList *decls = this->get_top1<S_DeclareList>();
        if (tok.tokencode == TokenCode::ID) {
            decls->decls.emplace_back(static_cast<const TokenId &>(tok).value, Node::Ptr());
            decls->pos_end = tok.pos_end;
            this->shift(ParserState::VAR_DECL_ITEM_END);
        } else {
            this->unpected_token(tok, "expect identifier");
        }
    } else if (cur == ParserState::VAR_DECL_ITEM_END) {
        if (tok.tokencode == TokenCode::ASSIGN) {
            this->shift(ParserState::VAR_DECL_ITEM_INIT);
            this->enter_exp_assign();
        } else {
            this->pass_up(tok);
        }
    } else if (cur == ParserState::VAR_DECL_ITEM_INIT) {
        S_DeclareList *decls = this->get_top2<S_DeclareList>();
        decls->decls.back().initial.reset(this->pop_top());
        this->set_pos_end(*decls->decls.back().initial);
        this->pass_up(tok);
    } else if (cur == ParserState::WHILE_LPAR) {
        if (tok.tokencode == TokenCode::LPAR) {
            this->shift(ParserState::WHILE_RPAR);
            this->enter_exp();
        } else {
            this->unpected_token(tok, "expect (");
        }
    } else if (cur == ParserState::WHILE_RPAR) {
        if (tok.tokencode == TokenCode::RPAR) {
            S_While *wh = this->get_top2<S_While>();
            wh->condition.reset(this->pop_top());
            this->shift(ParserState::WHILE_END);
            this->enter_block_pre();
        } else {
            this->unpected_token(tok, "expect )");
        }
    } else if (cur == ParserState::WHILE_END) {
        S_While *wh = this->get_top2<S_While>();
        wh->block.reset(this->pop_top());
        this->set_pos_end(*wh->block);
        this->pass_up(tok);
    } else if (cur == ParserState::EXP_LIST || cur == ParserState::EXP_LIST_ABS) {
        E_Op *list = this->get_top2<E_Op>();
        list->args.emplace_back(this->pop_top());
        this->set_pos_start(*list->args.front());
        this->set_pos_end(*list->args.back());

        if (tok.tokencode == TokenCode::COMMA) {
            this->enter_exp_assign();
        } else {
            if (cur == ParserState::EXP_LIST && list->args.size() == 1) {
                // unwrap comma operator
                Node *value = list->args.front().release();
                this->nodes.back().reset(value);
            }
            this->pass_up(tok);
        }
    } else if (cur == ParserState::EXP_ASSIGN) {
        uint32_t tc = static_cast<uint32_t>(tok.tokencode);
        if (tc == '=' || tc == '+=' || tc == '-=' || tc == '/=' || tc == '%=') {
            E_Op *item;
            if (dynamic_cast<E_Var *>(this->nodes.back().get())
                || ((item = dynamic_cast<E_Op *>(this->nodes.back().get()))
                    && item->op_code == OpCode::SUBSCRIPT))
            {
                this->grow_head(static_cast<OpCode>(tc));
                this->shift(ParserState::EXP_ASSIGN_END);
                this->enter_exp_assign();
            } else {
                this->unpected_token(tok, "can not assign to expression");
            }
        } else {
            this->pass_up(tok);
        }
    } else if (cur == ParserState::EXP_ASSIGN_END) {
        E_Op *assign = this->get_top2<E_Op>();
        assign->args.emplace_back(this->pop_top());
        this->set_pos_end(*assign->args.back());
        this->pass_up(tok);
    } else if (cur == ParserState::EXP_OR) {
        this->do_exp_op(tok, {'||'}, ParserState::EXP_OR_END, &Parser::enter_exp_and);
    } else if (cur == ParserState::EXP_OR_END) {
        this->do_exp_op_end(tok, {'||'}, &Parser::enter_exp_and);
    } else if (cur == ParserState::EXP_AND) {
        this->do_exp_op(tok, {'&&'}, ParserState::EXP_AND_END, &Parser::enter_exp_eq);
    } else if (cur == ParserState::EXP_AND_END) {
        this->do_exp_op_end(tok, {'&&'}, &Parser::enter_exp_eq);
    } else if (cur == ParserState::EXP_EQ) {
        this->do_exp_op(tok, {'==', '!='}, ParserState::EXP_EQ_END, &Parser::enter_exp_cmp);
    } else if (cur == ParserState::EXP_EQ_END) {
        this->do_exp_op_end(tok, {'==', '!='}, &Parser::enter_exp_cmp);
    } else if (cur == ParserState::EXP_CMP) {
        this->do_exp_op(tok, {'<', '<=', '>', '>='}, ParserState::EXP_CMP_END, &Parser::enter_exp_a);
    } else if (cur == ParserState::EXP_CMP_END) {
        this->do_exp_op_end(tok, {'<', '<=', '>', '>='}, &Parser::enter_exp_a);
    } else if (cur == ParserState::EXP_A) {
        this->do_exp_op(tok, {'+', '-'}, ParserState::EXP_A_END, &Parser::enter_exp_x);
    } else if (cur == ParserState::EXP_A_END) {
        this->do_exp_op_end(tok, {'+', '-'}, &Parser::enter_exp_x);
    } else if (cur == ParserState::EXP_X) {
        this->do_exp_op(tok, {'*', '/', '%'}, ParserState::EXP_X_END, &Parser::enter_exp_not);
    } else if (cur == ParserState::EXP_X_END) {
        this->do_exp_op_end(tok, {'*', '/', '%'}, &Parser::enter_exp_not);
    } else if (cur == ParserState::EXP_X_HEAD) {
        uint32_t tc = static_cast<uint32_t>(tok.tokencode);
        if (tc == '+' || tc == '-') {
            OpCode opcode = static_cast<OpCode>(tok.tokencode);
            E_Op *exp = new E_Op(opcode);
            this->nodes.emplace_back(exp);
            this->set_pos_start(tok);
            this->shift(ParserState::EXP_X_HEAD_END);
            this->enter_exp_not();
        } else {
            this->leave();
            this->enter_exp_not();
            this->feed(tok);
        }
    } else if (cur == ParserState::EXP_X_HEAD_END) {
        this->grow_body();
        this->pass_up(tok);
    } else if (cur == ParserState::EXP_NOT) {
        if (tok.tokencode == TokenCode::NOT) {
            this->nodes.emplace_back(new E_Op(OpCode::NOT));
            this->set_pos_start(tok);
            this->shift(ParserState::EXP_NOT_END);
            this->enter_exp_call_or_subs();
        } else {
            this->leave();
            this->enter_exp_call_or_subs();
            this->feed(tok);
        }
    } else if (cur == ParserState::EXP_NOT_END) {
        this->grow_body();
        this->pass_up(tok);
    } else if (cur == ParserState::EXP_CALL_OR_SUBS) {
        if (tok.tokencode == TokenCode::LSQUARE) {
            this->grow_head(OpCode::SUBSCRIPT);
            this->shift(ParserState::EXP_SUBS_RSQ);
            this->enter_exp();
        } else if (tok.tokencode == TokenCode::LPAR) {
            this->grow_head(OpCode::CALL);
            this->shift(ParserState::EXP_CALL_AFTER_LPAR);
        } else {
            this->pass_up(tok);
        }
    } else if (cur == ParserState::EXP_SUBS_RSQ) {
        if (tok.tokencode == TokenCode::RSQUARE) {
            this->grow_body();
            this->set_pos_end(tok);
            this->shift(ParserState::EXP_CALL_OR_SUBS);
        } else {
            this->unpected_token(tok, "expect ]");
        }
    } else if (cur == ParserState::EXP_CALL_AFTER_LPAR) {
        if (tok.tokencode == TokenCode::RPAR) {
            this->set_pos_end(tok);
            this->shift(ParserState::EXP_CALL_OR_SUBS);
        } else {
            this->shift(ParserState::EXP_CALL_RPAR);
            this->enter_exp_list_abs();
            this->feed(tok);
        }
    } else if (cur == ParserState::EXP_CALL_RPAR) {
        if (tok.tokencode == TokenCode::RPAR) {
            this->grow_body();
            this->set_pos_end(tok);
            this->shift(ParserState::EXP_CALL_OR_SUBS);
        } else {
            this->unpected_token(tok, "expect )");
        }
    } else if (cur == ParserState::EXP_T) {
        if (tok.tokencode == TokenCode::LPAR) {
            this->shift(ParserState::EXP_T_RPAR);
            this->enter_exp();
        } else if (tok.tokencode == TokenCode::INT) {
            this->do_const<TokenInt, E_Int>(tok);
        } else if (tok.tokencode == TokenCode::FLOAT) {
            this->do_const<TokenFloat, E_Float>(tok);
        } else if (tok.tokencode == TokenCode::STRING) {
            this->do_const<TokenString, E_String>(tok);
        } else if (tok.tokencode == TokenCode::LSQUARE) {
            this->leave();
            this->enter_list(tok);
        } else if (match_id(tok, USTRING("function"))) {
            this->leave();
            this->enter_function(tok);
        } else if (match_id(tok, USTRING("null"))) {
            this->nodes.emplace_back(new E_Null());
            this->set_pos_start_end(tok);
            this->leave();
        } else if (match_id(tok, USTRING("true"))) {
            this->nodes.emplace_back(new E_Bool(true));
            this->set_pos_start_end(tok);
            this->leave();
        } else if (match_id(tok, USTRING("false"))) {
            this->nodes.emplace_back(new E_Bool(false));
            this->set_pos_start_end(tok);
            this->leave();
        } else if (tok.tokencode == TokenCode::ID) {
            // FIXME: check reserved word
            this->do_const<TokenId, E_Var>(tok);
        } else {
            this->unpected_token(tok, "expect terminal");
        }
    } else if (cur == ParserState::EXP_T_RPAR) {
        if (tok.tokencode == TokenCode::RPAR) {
            this->leave();
        } else {
            this->unpected_token(tok, "expect )");
        }
    } else if (cur == ParserState::LIST) {
        if (tok.tokencode == TokenCode::RSQUARE) {
            this->set_pos_end(tok);
            this->leave();
        } else {
            this->shift(ParserState::LIST_END);
            this->enter_exp_list_abs();
            this->feed(tok);
        }
    } else if (cur == ParserState::LIST_END) {
        if (tok.tokencode == TokenCode::RSQUARE) {
            E_List *list = this->get_top2<E_List>();
            E_Op *args = this->get_top1<E_Op>();
            assert(args->op_code == OpCode::EXPLIST);
            for (Node::Ptr &item : args->args) {
                list->value.emplace_back(item.release());
            }
            this->nodes.pop_back();
            this->set_pos_end(tok);
            this->leave();
        } else {
            this->unpected_token(tok, "expect ]");
        }
    } else if (cur == ParserState::FUNC) {
        if (tok.tokencode == TokenCode::LPAR) {
            this->shift(ParserState::FUNC_AFTER_LPAR);
        } else {
            this->unpected_token(tok, "expect (");
        }
    } else if (cur == ParserState::FUNC_AFTER_LPAR) {
        if (tok.tokencode == TokenCode::RPAR) {
            this->shift(ParserState::FUNC_END);
            this->enter_block_pre();
        } else {
            this->shift(ParserState::FUNC_RPAR);
            this->enter_arg_decl_list(tok);
            this->feed(tok);
        }
    } else if (cur == ParserState::FUNC_RPAR) {
        if (tok.tokencode == TokenCode::RPAR) {
            // FIXME: check the position of default values
            E_Func *func = this->get_top2<E_Func>();
            func->args.reset(this->pop_top());
            this->shift(ParserState::FUNC_END);
            this->enter_block_pre();
        } else {
            this->unpected_token(tok, "expect )");
        }
    } else if (cur == ParserState::FUNC_END) {
        E_Func *func = this->get_top2<E_Func>();
        func->block.reset(this->pop_top());
        this->set_pos_end(*func->block);
        this->pass_up(tok);
    } else {
        assert(!"Unreachable");
    }
}


void Parser::grow_head(OpCode opcode) {
    E_Op *head = new E_Op(opcode);
    head->args.emplace_back(this->pop_top());
    this->nodes.emplace_back(head);
    this->set_pos_start(*head->args.front());
}


void Parser::grow_body() {
    E_Op *exp = this->get_top2<E_Op>();
    exp->args.emplace_back(this->pop_top());
    this->set_pos_end(*exp->args.back());
}


void Parser::do_exp_op(
    const Token &tok, const std::vector<uint32_t> &accepts,
    ParserState next, Parser::MemFunc next_enter)
{
    uint32_t code = static_cast<uint32_t>(tok.tokencode);
    if (std::find(accepts.begin(), accepts.end(), code) != accepts.end()) {
        OpCode opcode = static_cast<OpCode>(tok.tokencode);
        this->grow_head(opcode);
        this->shift(next);
        (this->*next_enter)();
    } else {
        this->pass_up(tok);
    }
}


void Parser::do_exp_op_end(
    const Token &tok, const std::vector<uint32_t> &accepts, Parser::MemFunc next_enter)
{
    E_Op *exp_op = this->get_top2<E_Op>();
    exp_op->args.emplace_back(this->pop_top());

    uint32_t code = static_cast<uint32_t>(tok.tokencode);
    if (std::find(accepts.begin(), accepts.end(), code) != accepts.end()) {
        OpCode opcode = static_cast<OpCode>(tok.tokencode);
        this->grow_head(opcode);
        (this->*next_enter)();
    } else {
        this->set_pos_end(*exp_op->args.back());
        this->pass_up(tok);
    }
}


template<class TokenType, class NodeType>
void Parser::do_const(const Token &tok) {
    const TokenType &tokcast = static_cast<const TokenType &>(tok);
    this->nodes.emplace_back(new NodeType(tokcast.value));
    this->set_pos_start_end(tok);
    this->leave();
}


void Parser::enter_program_or_exp() {
    this->nodes.emplace_back(new Program());
    this->states.push_back(ParserState::PROGRAM_OR_EXP);
    this->enter_stmt();
}


void Parser::enter_program() {
    this->nodes.emplace_back(new Program());
    this->states.push_back(ParserState::PROGRAM);
    this->enter_stmt();
}


void Parser::enter_stmt() {
    this->states.push_back(ParserState::STMT);
}


void Parser::enter_block(const Token &tok) {
    this->nodes.emplace_back(new S_Block());
    this->set_pos_start(tok);
    this->states.push_back(ParserState::BLOCK);
}


void Parser::enter_block_pre() {
    this->states.push_back(ParserState::BLOCK_PRE);
}


void Parser::enter_return(const Token &tok) {
    this->nodes.emplace_back(new S_Return());
    this->set_pos_start(tok);
    this->states.push_back(ParserState::RETURN);
}


void Parser::enter_condition(const Token &tok) {
    this->nodes.emplace_back(new S_Condition());
    this->set_pos_start(tok);
    this->states.push_back(ParserState::COND_LPAR);
}


void Parser::enter_else() {
    this->states.push_back(ParserState::ELSE);
}


void Parser::enter_continue(const Token &tok) {
    this->nodes.emplace_back(new S_Continue());
    this->set_pos_start(tok);
    this->states.push_back(ParserState::CONTINUE);
}


void Parser::enter_break(const Token &tok) {
    this->nodes.emplace_back(new S_Break());
    this->set_pos_start(tok);
    this->states.push_back(ParserState::BREAK);
}


void Parser::enter_var_decl(const Token &tok) {
    this->states.push_back(ParserState::VAR_DECL);
    this->enter_var_decl_list(tok);
}


void Parser::enter_var_decl_list(const Token &tok) {
    this->nodes.emplace_back(new S_DeclareList());
    this->set_pos_start(tok);
    this->states.push_back(ParserState::VAR_DECL_LIST);
    this->enter_var_decl_item();
}


void Parser::enter_var_decl_item() {
    this->states.push_back(ParserState::VAR_DECL_ITEM);
}


void Parser::enter_while(const Token &tok) {
    this->nodes.emplace_back(new S_While());
    this->set_pos_start(tok);
    this->states.push_back(ParserState::WHILE_LPAR);
}


void Parser::enter_exp() {
    this->enter_exp_list();
}


void Parser::enter_exp_list() {
    this->nodes.emplace_back(new E_Op(OpCode::EXPLIST));
    this->states.push_back(ParserState::EXP_LIST);
    this->enter_exp_assign();
}


void Parser::enter_exp_list_abs() {
    this->nodes.emplace_back(new E_Op(OpCode::EXPLIST));
    this->states.push_back(ParserState::EXP_LIST_ABS);
    this->enter_exp_assign();
}


void Parser::enter_exp_assign() {
    this->states.push_back(ParserState::EXP_ASSIGN);
    this->enter_exp_or();
}


void Parser::enter_exp_or() {
    this->states.push_back(ParserState::EXP_OR);
    this->enter_exp_and();
}


void Parser::enter_exp_and() {
    this->states.push_back(ParserState::EXP_AND);
    this->enter_exp_eq();
}


void Parser::enter_exp_eq() {
    this->states.push_back(ParserState::EXP_EQ);
    this->enter_exp_cmp();
}


void Parser::enter_exp_cmp() {
    this->states.push_back(ParserState::EXP_CMP);
    this->enter_exp_a();
}


void Parser::enter_exp_a() {
    this->states.push_back(ParserState::EXP_A);
    this->enter_exp_x();
}


void Parser::enter_exp_x() {
    this->states.push_back(ParserState::EXP_X);
    this->enter_exp_x_head();
}


void Parser::enter_exp_x_head() {
    this->states.push_back(ParserState::EXP_X_HEAD);
}


void Parser::enter_exp_not() {
    this->states.push_back(ParserState::EXP_NOT);
}


void Parser::enter_exp_call_or_subs() {
    this->states.push_back(ParserState::EXP_CALL_OR_SUBS);
    this->enter_exp_t();
}


void Parser::enter_exp_t() {
    this->states.push_back(ParserState::EXP_T);
}


void Parser::enter_list(const Token &tok) {
    this->nodes.emplace_back(new E_List());
    this->set_pos_start(tok);
    this->states.push_back(ParserState::LIST);
}


void Parser::enter_function(const Token &tok) {
    this->nodes.emplace_back(new E_Func());
    this->set_pos_start(tok);
    this->states.push_back(ParserState::FUNC);
}


void Parser::enter_arg_decl_list(const Token &tok) {
    this->enter_var_decl_list(tok);
}


void Parser::set_pos_start(const Token &tok) {
    assert(!this->nodes.empty());
    this->nodes.back()->pos_start = tok.pos_start;
}


void Parser::set_pos_start(const Node &node) {
    assert(!this->nodes.empty());
    this->nodes.back()->pos_start = node.pos_start;
}


void Parser::set_pos_end(const Token &tok) {
    assert(!this->nodes.empty());
    this->nodes.back()->pos_end = tok.pos_end;
}


void Parser::set_pos_end(const Node &node) {
    assert(!this->nodes.empty());
    this->nodes.back()->pos_end = node.pos_end;
}


void Parser::set_pos_start_end(const Token &tok) {
    this->set_pos_start(tok);
    this->set_pos_end(tok);
}
