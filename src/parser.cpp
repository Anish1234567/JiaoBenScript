#include <algorithm>
#include <cassert>
#include <cstring>
#include <utility>

#include "parser.h"


void Parser::start_program() {
    this->enter_program();
}


void Parser::start_repl() {
    this->enter_program_or_exp();
}


bool Parser::is_empty() const {
    return this->states.empty();
}


bool Parser::can_end() const {
    assert(!this->states.empty());
    assert(
        this->states.front() == &Parser::state_PROGRAM_OR_EXP
        || this->states.front() == &Parser::state_PROGRAM
    );

    auto stop = --this->states.rend();
    for (auto it = this->states.rbegin(); it != stop; ++it) {
        if (Parser::terminating_states.contains(*it)) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}


Parser::SortedState::SortedState(std::initializer_list<Parser::StateHandler> states)
    : states(states)
{
    std::sort(this->states.begin(), this->states.end(), &SortedState::cmp);
}


bool Parser::SortedState::contains(const Parser::StateHandler &state) const {
    return std::binary_search(
        this->states.begin(), this->states.end(), state, &SortedState::cmp
    );
}


bool Parser::SortedState::cmp(const Parser::StateHandler &lhs, const Parser::StateHandler &rhs) {
    return memcmp(&lhs, &rhs, sizeof(StateHandler)) < 0;
}


Parser::SortedState Parser::terminating_states {
    // can terminate a statement
    &Parser::state_COND_ELSE,
    &Parser::state_COND_END,
    &Parser::state_WHILE_END,

    // can transit to expression mode and terminate
    &Parser::state_STMT_EXP,

    // can terminate an expression
    &Parser::state_EXP_LIST,
    &Parser::state_EXP_LIST_ABS,
    &Parser::state_EXP_ASSIGN,
    &Parser::state_EXP_ASSIGN_END,

    &Parser::state_EXP_OR,
    &Parser::state_EXP_OR_END,
    &Parser::state_EXP_AND,
    &Parser::state_EXP_AND_END,
    &Parser::state_EXP_EQ,
    &Parser::state_EXP_EQ_END,
    &Parser::state_EXP_CMP,
    &Parser::state_EXP_CMP_END,
    &Parser::state_EXP_A,
    &Parser::state_EXP_A_END,
    &Parser::state_EXP_X,
    &Parser::state_EXP_X_END,

    &Parser::state_EXP_X_HEAD_END,
    &Parser::state_EXP_NOT_END,
    &Parser::state_EXP_CALL_OR_SUBS,
    &Parser::state_FUNC_END,
};


void Parser::leave() {
    this->states.pop_back();
}


void Parser::pass_up(const Token &tok) {
    this->leave();
    this->feed(tok);
}


void Parser::shift(Parser::StateHandler state) {
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
    return ret;     // Program or expression
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
    if (this->states.empty()) {
        return this->unpected_token(tok, "parser not started");
    }
    if (tok.tokencode == TokenCode::COMMENT) {
        return;
    }

    (this->*this->states.back())(tok);
}


void Parser::state_END(const Token & tok) {
    this->unpected_token(tok, "parser ended");
}


void Parser::state_PROGRAM_OR_EXP(const Token & tok) {
    Program *p = this->get_top2<Program>();
    p->stmts.emplace_back(this->pop_top());
    this->set_pos_start(*p->stmts.front());
    if (tok.tokencode == TokenCode::END) {
        this->set_pos_end(*p->stmts.back());
        this->shift(&Parser::state_END);
    } else {
        this->enter_stmt();
        this->feed(tok);
    }
}


void Parser::state_PROGRAM(const Token & tok) {
    return this->state_PROGRAM_OR_EXP(tok);
}


void Parser::state_STMT(const Token & tok) {
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
        this->shift(&Parser::state_STMT_EXP);
        this->enter_exp();
        this->feed(tok);
    }
}


void Parser::state_STMT_EXP(const Token & tok) {
    if (tok.tokencode == TokenCode::SEMICOLON) {
        // replace expression with S_Exp
        S_Exp *stmt = new S_Exp();
        stmt->value.reset(this->pop_top());
        stmt->pos_start = stmt->value->pos_start;
        this->nodes.emplace_back(stmt);
        this->set_pos_end(tok);
        this->leave();
    } else if (this->states.front() == &Parser::state_PROGRAM_OR_EXP) {
        if (tok.tokencode == TokenCode::END) {
            assert(this->nodes.size() == 2);
            // replace Program with expression
            this->nodes.front().reset(this->nodes.back().release());
            this->nodes.pop_back();

            assert(this->states.size() == 2);
            this->leave();
            this->shift(&Parser::state_END);
        } else {
            this->unpected_token(tok, "expect semicolon or END");
        }
    } else {
        this->unpected_token(tok, "expect semicolon");
    }
}


void Parser::state_BLOCK(const Token & tok) {
    if (tok.tokencode == TokenCode::RBRACE) {
        this->set_pos_end(tok);
        // empty block
        this->leave();
    } else {
        this->shift(&Parser::state_BLOCK_END);
        this->enter_stmt();
        this->feed(tok);
    }
}


void Parser::state_BLOCK_END(const Token & tok) {
    S_Block *block = this->get_top2<S_Block>();
    block->stmts.emplace_back(this->pop_top());
    if (tok.tokencode == TokenCode::RBRACE) {
        this->set_pos_end(tok);
        this->leave();
    } else {
        this->enter_stmt();
        this->feed(tok);
    }
}


void Parser::state_BLOCK_PRE(const Token & tok) {
    if (tok.tokencode == TokenCode::LBRACE) {
        this->leave();
        this->enter_block(tok);
    } else {
        this->unpected_token(tok, "expect {");
    }
}


void Parser::state_RETURN(const Token & tok) {
    if (tok.tokencode == TokenCode::SEMICOLON) {
        this->set_pos_end(tok);
        this->leave();
    } else {
        this->shift(&Parser::state_RETURN_EXP);
        this->enter_exp();
        this->feed(tok);
    }
}


void Parser::state_RETURN_EXP(const Token & tok) {
    if (tok.tokencode == TokenCode::SEMICOLON) {
        S_Return *ret = this->get_top2<S_Return>();
        ret->value.reset(this->pop_top());
        this->set_pos_end(tok);
        this->leave();
    } else {
        this->unpected_token(tok, "expect semicolon");
    }
}


void Parser::state_COND_LPAR(const Token & tok) {
    if (tok.tokencode == TokenCode::LPAR) {
        this->shift(&Parser::state_COND_RPAR);
        this->enter_exp();
    } else {
        this->unpected_token(tok, "expect (");
    }
}


void Parser::state_COND_RPAR(const Token & tok) {
    if (tok.tokencode == TokenCode::RPAR) {
        S_Condition *cond = this->get_top2<S_Condition>();
        cond->condition.reset(this->pop_top());
        this->shift(&Parser::state_COND_ELSE);
        this->enter_block_pre();
    } else {
        this->unpected_token(tok, "expect )");
    }
}


void Parser::state_COND_ELSE(const Token & tok) {
    S_Condition *cond = this->get_top2<S_Condition>();
    cond->then_block.reset(this->pop_top());
    if (match_id(tok, USTRING("else"))) {
        this->shift(&Parser::state_COND_END);
        this->enter_else();
    } else {
        this->set_pos_end(*cond->then_block);
        this->pass_up(tok);
    }
}


void Parser::state_COND_END(const Token & tok) {
    S_Condition *cond = this->get_top2<S_Condition>();
    cond->else_block.reset(this->pop_top());
    this->set_pos_end(*cond->else_block);
    this->pass_up(tok);
}


void Parser::state_ELSE(const Token & tok) {
    if (match_id(tok, USTRING("if"))) {
        this->leave();
        this->enter_condition(tok);
    } else if (tok.tokencode == TokenCode::LBRACE) {
        this->leave();
        this->enter_block(tok);
    } else {
        this->unpected_token(tok, "expect 'if' or block");
    }
}


void Parser::state_CONTINUE(const Token & tok) {
    this->do_stmt_comma(tok);
}


void Parser::state_BREAK(const Token & tok) {
    this->do_stmt_comma(tok);
}


void Parser::state_VAR_DECL(const Token & tok) {
    this->do_stmt_comma(tok);
}


void Parser::do_stmt_comma(const Token &tok) {
    if (tok.tokencode == TokenCode::SEMICOLON) {
        this->set_pos_end(tok);
        this->leave();
    } else {
        this->unpected_token(tok, "expect semicolon");
    }
}


void Parser::state_VAR_DECL_LIST(const Token & tok) {
    if (tok.tokencode == TokenCode::COMMA) {
        this->enter_var_decl_item();    // item collected in VAR_DECL_ITEM state
    } else {
        this->pass_up(tok);
    }
}


void Parser::state_VAR_DECL_ITEM(const Token & tok) {
    S_DeclareList *decls = this->get_top1<S_DeclareList>();
    if (tok.tokencode == TokenCode::ID) {
        decls->decls.emplace_back(static_cast<const TokenId &>(tok).value, Node::Ptr());
        decls->pos_end = tok.pos_end;
        this->shift(&Parser::state_VAR_DECL_ITEM_END);
    } else {
        this->unpected_token(tok, "expect identifier");
    }
}


void Parser::state_VAR_DECL_ITEM_END(const Token & tok) {
    if (tok.tokencode == TokenCode::ASSIGN) {
        this->shift(&Parser::state_VAR_DECL_ITEM_INIT);
        this->enter_exp_assign();
    } else {
        this->pass_up(tok);
    }
}


void Parser::state_VAR_DECL_ITEM_INIT(const Token & tok) {
    S_DeclareList *decls = this->get_top2<S_DeclareList>();
    decls->decls.back().initial.reset(this->pop_top());
    this->set_pos_end(*decls->decls.back().initial);
    this->pass_up(tok);
}


void Parser::state_WHILE_LPAR(const Token & tok) {
    if (tok.tokencode == TokenCode::LPAR) {
        this->shift(&Parser::state_WHILE_RPAR);
        this->enter_exp();
    } else {
        this->unpected_token(tok, "expect (");
    }
}


void Parser::state_WHILE_RPAR(const Token & tok) {
    if (tok.tokencode == TokenCode::RPAR) {
        S_While *wh = this->get_top2<S_While>();
        wh->condition.reset(this->pop_top());
        this->shift(&Parser::state_WHILE_END);
        this->enter_block_pre();
    } else {
        this->unpected_token(tok, "expect )");
    }
}


void Parser::state_WHILE_END(const Token & tok) {
    S_While *wh = this->get_top2<S_While>();
    wh->block.reset(this->pop_top());
    this->set_pos_end(*wh->block);
    this->pass_up(tok);
}


void Parser::state_EXP_LIST(const Token & tok) {
    E_Op *list = this->get_top2<E_Op>();
    list->args.emplace_back(this->pop_top());
    this->set_pos_start(*list->args.front());
    this->set_pos_end(*list->args.back());

    if (tok.tokencode == TokenCode::COMMA) {
        this->enter_exp_assign();
    } else {
        if (this->states.back() == &Parser::state_EXP_LIST && list->args.size() == 1) {
            // unwrap comma operator
            Node *value = list->args.front().release();
            this->nodes.back().reset(value);
        }
        this->pass_up(tok);
    }
}


void Parser::state_EXP_LIST_ABS(const Token & tok) {
    return this->state_EXP_LIST(tok);
}


void Parser::state_EXP_ASSIGN(const Token & tok) {
    uint32_t tc = static_cast<uint32_t>(tok.tokencode);
    if (tc == '=' || tc == '+=' || tc == '-=' || tc == '/=' || tc == '%=') {
        E_Op *item;
        if (dynamic_cast<E_Var *>(this->nodes.back().get())
            || ((item = dynamic_cast<E_Op *>(this->nodes.back().get()))
            && item->op_code == OpCode::SUBSCRIPT))
        {
            this->grow_head(static_cast<OpCode>(tc));
            this->shift(&Parser::state_EXP_ASSIGN_END);
            this->enter_exp_assign();
        } else {
            this->unpected_token(tok, "can not assign to expression");
        }
    } else {
        this->pass_up(tok);
    }
}


void Parser::state_EXP_ASSIGN_END(const Token & tok) {
    E_Op *assign = this->get_top2<E_Op>();
    assign->args.emplace_back(this->pop_top());
    this->set_pos_end(*assign->args.back());
    this->pass_up(tok);
}


void Parser::state_EXP_OR(const Token & tok) {
    this->do_exp_op(tok, {'||'}, &Parser::state_EXP_OR_END, &Parser::enter_exp_and);
}


void Parser::state_EXP_OR_END(const Token & tok) {
    this->do_exp_op_end(tok, {'||'}, &Parser::enter_exp_and);
}


void Parser::state_EXP_AND(const Token & tok) {
    this->do_exp_op(tok, {'&&'}, &Parser::state_EXP_AND_END, &Parser::enter_exp_eq);
}


void Parser::state_EXP_AND_END(const Token & tok) {
    this->do_exp_op_end(tok, {'&&'}, &Parser::enter_exp_eq);
}


void Parser::state_EXP_EQ(const Token & tok) {
    this->do_exp_op(tok, {'==', '!='}, &Parser::state_EXP_EQ_END, &Parser::enter_exp_cmp);
}


void Parser::state_EXP_EQ_END(const Token & tok) {
    this->do_exp_op_end(tok, {'==', '!='}, &Parser::enter_exp_cmp);
}


void Parser::state_EXP_CMP(const Token & tok) {
    this->do_exp_op(tok, {'<', '<=', '>', '>='}, &Parser::state_EXP_CMP_END, &Parser::enter_exp_a);
}


void Parser::state_EXP_CMP_END(const Token & tok) {
    this->do_exp_op_end(tok, {'<', '<=', '>', '>='}, &Parser::enter_exp_a);
}


void Parser::state_EXP_A(const Token & tok) {
    this->do_exp_op(tok, {'+', '-'}, &Parser::state_EXP_A_END, &Parser::enter_exp_x);
}


void Parser::state_EXP_A_END(const Token & tok) {
    this->do_exp_op_end(tok, {'+', '-'}, &Parser::enter_exp_x);
}


void Parser::state_EXP_X(const Token & tok) {
    this->do_exp_op(tok, {'*', '/', '%'}, &Parser::state_EXP_X_END, &Parser::enter_exp_not);
}


void Parser::state_EXP_X_END(const Token & tok) {
    this->do_exp_op_end(tok, {'*', '/', '%'}, &Parser::enter_exp_not);
}


void Parser::state_EXP_X_HEAD(const Token & tok) {
    uint32_t tc = static_cast<uint32_t>(tok.tokencode);
    if (tc == '+' || tc == '-') {
        OpCode opcode = static_cast<OpCode>(tok.tokencode);
        E_Op *exp = new E_Op(opcode);
        this->nodes.emplace_back(exp);
        this->set_pos_start(tok);
        this->shift(&Parser::state_EXP_X_HEAD_END);
        this->enter_exp_not();
    } else {
        this->leave();
        this->enter_exp_not();
        this->feed(tok);
    }
}


void Parser::state_EXP_X_HEAD_END(const Token & tok) {
    this->grow_body();
    this->pass_up(tok);
}


void Parser::state_EXP_NOT(const Token & tok) {
    if (tok.tokencode == TokenCode::NOT) {
        this->nodes.emplace_back(new E_Op(OpCode::NOT));
        this->set_pos_start(tok);
        this->shift(&Parser::state_EXP_NOT_END);
        this->enter_exp_call_or_subs();
    } else {
        this->leave();
        this->enter_exp_call_or_subs();
        this->feed(tok);
    }
}


void Parser::state_EXP_NOT_END(const Token & tok) {
    this->grow_body();
    this->pass_up(tok);
}


void Parser::state_EXP_CALL_OR_SUBS(const Token & tok) {
    if (tok.tokencode == TokenCode::LSQUARE) {
        this->grow_head(OpCode::SUBSCRIPT);
        this->shift(&Parser::state_EXP_SUBS_RSQ);
        this->enter_exp();
    } else if (tok.tokencode == TokenCode::LPAR) {
        this->grow_head(OpCode::CALL);
        this->shift(&Parser::state_EXP_CALL_AFTER_LPAR);
    } else {
        this->pass_up(tok);
    }
}


void Parser::state_EXP_SUBS_RSQ(const Token & tok) {
    if (tok.tokencode == TokenCode::RSQUARE) {
        this->grow_body();
        this->set_pos_end(tok);
        this->shift(&Parser::state_EXP_CALL_OR_SUBS);
    } else {
        this->unpected_token(tok, "expect ]");
    }
}


void Parser::state_EXP_CALL_AFTER_LPAR(const Token & tok) {
    if (tok.tokencode == TokenCode::RPAR) {
        this->set_pos_end(tok);
        this->shift(&Parser::state_EXP_CALL_OR_SUBS);
    } else {
        this->shift(&Parser::state_EXP_CALL_RPAR);
        this->enter_exp_list_abs();
        this->feed(tok);
    }
}


void Parser::state_EXP_CALL_RPAR(const Token & tok) {
    if (tok.tokencode == TokenCode::RPAR) {
        this->grow_body();
        this->set_pos_end(tok);
        this->shift(&Parser::state_EXP_CALL_OR_SUBS);
    } else {
        this->unpected_token(tok, "expect )");
    }
}


void Parser::state_EXP_T(const Token & tok) {
    if (tok.tokencode == TokenCode::LPAR) {
        this->shift(&Parser::state_EXP_T_RPAR);
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
}


void Parser::state_EXP_T_RPAR(const Token & tok) {
    if (tok.tokencode == TokenCode::RPAR) {
        this->leave();
    } else {
        this->unpected_token(tok, "expect )");
    }
}


void Parser::state_LIST(const Token & tok) {
    if (tok.tokencode == TokenCode::RSQUARE) {
        this->set_pos_end(tok);
        this->leave();
    } else {
        this->shift(&Parser::state_LIST_END);
        this->enter_exp_list_abs();
        this->feed(tok);
    }
}


void Parser::state_LIST_END(const Token & tok) {
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
}


void Parser::state_FUNC(const Token & tok) {
    if (tok.tokencode == TokenCode::LPAR) {
        this->shift(&Parser::state_FUNC_AFTER_LPAR);
    } else {
        this->unpected_token(tok, "expect (");
    }
}


void Parser::state_FUNC_AFTER_LPAR(const Token & tok) {
    if (tok.tokencode == TokenCode::RPAR) {
        this->shift(&Parser::state_FUNC_END);
        this->enter_block_pre();
    } else {
        this->shift(&Parser::state_FUNC_RPAR);
        this->enter_arg_decl_list(tok);
        this->feed(tok);
    }
}


void Parser::state_FUNC_RPAR(const Token & tok) {
    if (tok.tokencode == TokenCode::RPAR) {
        // FIXME: check the position of default values
        E_Func *func = this->get_top2<E_Func>();
        func->args.reset(this->pop_top());
        this->shift(&Parser::state_FUNC_END);
        this->enter_block_pre();
    } else {
        this->unpected_token(tok, "expect )");
    }
}


void Parser::state_FUNC_END(const Token & tok) {
    E_Func *func = this->get_top2<E_Func>();
    func->block.reset(this->pop_top());
    this->set_pos_end(*func->block);
    this->pass_up(tok);
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
    Parser::StateHandler next, Parser::MemFunc next_enter)
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
    this->states.push_back(&Parser::state_PROGRAM_OR_EXP);
    this->enter_stmt();
}


void Parser::enter_program() {
    this->nodes.emplace_back(new Program());
    this->states.push_back(&Parser::state_PROGRAM);
    this->enter_stmt();
}


void Parser::enter_stmt() {
    this->states.push_back(&Parser::state_STMT);
}


void Parser::enter_block(const Token &tok) {
    this->nodes.emplace_back(new S_Block());
    this->set_pos_start(tok);
    this->states.push_back(&Parser::state_BLOCK);
}


void Parser::enter_block_pre() {
    this->states.push_back(&Parser::state_BLOCK_PRE);
}


void Parser::enter_return(const Token &tok) {
    this->nodes.emplace_back(new S_Return());
    this->set_pos_start(tok);
    this->states.push_back(&Parser::state_RETURN);
}


void Parser::enter_condition(const Token &tok) {
    this->nodes.emplace_back(new S_Condition());
    this->set_pos_start(tok);
    this->states.push_back(&Parser::state_COND_LPAR);
}


void Parser::enter_else() {
    this->states.push_back(&Parser::state_ELSE);
}


void Parser::enter_continue(const Token &tok) {
    this->nodes.emplace_back(new S_Continue());
    this->set_pos_start(tok);
    this->states.push_back(&Parser::state_CONTINUE);
}


void Parser::enter_break(const Token &tok) {
    this->nodes.emplace_back(new S_Break());
    this->set_pos_start(tok);
    this->states.push_back(&Parser::state_BREAK);
}


void Parser::enter_var_decl(const Token &tok) {
    this->states.push_back(&Parser::state_VAR_DECL);
    this->enter_var_decl_list(tok);
}


void Parser::enter_var_decl_list(const Token &tok) {
    this->nodes.emplace_back(new S_DeclareList());
    this->set_pos_start(tok);
    this->states.push_back(&Parser::state_VAR_DECL_LIST);
    this->enter_var_decl_item();
}


void Parser::enter_var_decl_item() {
    this->states.push_back(&Parser::state_VAR_DECL_ITEM);
}


void Parser::enter_while(const Token &tok) {
    this->nodes.emplace_back(new S_While());
    this->set_pos_start(tok);
    this->states.push_back(&Parser::state_WHILE_LPAR);
}


void Parser::enter_exp() {
    this->enter_exp_list();
}


void Parser::enter_exp_list() {
    this->nodes.emplace_back(new E_Op(OpCode::EXPLIST));
    this->states.push_back(&Parser::state_EXP_LIST);
    this->enter_exp_assign();
}


void Parser::enter_exp_list_abs() {
    this->nodes.emplace_back(new E_Op(OpCode::EXPLIST));
    this->states.push_back(&Parser::state_EXP_LIST_ABS);
    this->enter_exp_assign();
}


void Parser::enter_exp_assign() {
    this->states.push_back(&Parser::state_EXP_ASSIGN);
    this->enter_exp_or();
}


void Parser::enter_exp_or() {
    this->states.push_back(&Parser::state_EXP_OR);
    this->enter_exp_and();
}


void Parser::enter_exp_and() {
    this->states.push_back(&Parser::state_EXP_AND);
    this->enter_exp_eq();
}


void Parser::enter_exp_eq() {
    this->states.push_back(&Parser::state_EXP_EQ);
    this->enter_exp_cmp();
}


void Parser::enter_exp_cmp() {
    this->states.push_back(&Parser::state_EXP_CMP);
    this->enter_exp_a();
}


void Parser::enter_exp_a() {
    this->states.push_back(&Parser::state_EXP_A);
    this->enter_exp_x();
}


void Parser::enter_exp_x() {
    this->states.push_back(&Parser::state_EXP_X);
    this->enter_exp_x_head();
}


void Parser::enter_exp_x_head() {
    this->states.push_back(&Parser::state_EXP_X_HEAD);
}


void Parser::enter_exp_not() {
    this->states.push_back(&Parser::state_EXP_NOT);
}


void Parser::enter_exp_call_or_subs() {
    this->states.push_back(&Parser::state_EXP_CALL_OR_SUBS);
    this->enter_exp_t();
}


void Parser::enter_exp_t() {
    this->states.push_back(&Parser::state_EXP_T);
}


void Parser::enter_list(const Token &tok) {
    this->nodes.emplace_back(new E_List());
    this->set_pos_start(tok);
    this->states.push_back(&Parser::state_LIST);
}


void Parser::enter_function(const Token &tok) {
    this->nodes.emplace_back(new E_Func());
    this->set_pos_start(tok);
    this->states.push_back(&Parser::state_FUNC);
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


template<class NodeType>
NodeType *Parser::get_top1() {
    assert(!this->nodes.empty());
    return static_cast<NodeType *>(this->nodes.back().get());
}

template<class NodeType>
NodeType *Parser::get_top2() {
    assert(this->nodes.size() >= 2);
    return static_cast<NodeType *>((++this->nodes.rbegin())->get());
}

template<class NodeType>
NodeType *Parser::pop_top() {
    NodeType *top = this->nodes.back().release();
    this->nodes.pop_back();
    return top;
}
