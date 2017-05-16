#ifndef JIAOBENSCRIPT_PARSER_H
#define JIAOBENSCRIPT_PARSER_H

#include <initializer_list>
#include <vector>

#include "exceptions.h"
#include "tokenizer.h"
#include "node.h"


class Parser {
public:
    void start_program();
    void start_repl();
    bool is_empty() const;
    bool can_end() const;
    void feed(const Token &tok);
    Node::Ptr pop_result();

private:
    typedef void (Parser::* StateHandler)(const Token &);
    void state_END(const Token &tok);
    void state_PROGRAM_OR_EXP(const Token &tok);
    void state_PROGRAM(const Token &tok);
    void state_STMT(const Token &tok);
    void state_STMT_EXP(const Token &tok);
    void state_BLOCK_PRE(const Token &tok);
    void state_BLOCK(const Token &tok);
    void state_BLOCK_END(const Token &tok);
    void state_RETURN(const Token &tok);
    void state_RETURN_EXP(const Token &tok);
    void state_COND_LPAR(const Token &tok);
    void state_COND_RPAR(const Token &tok);
    void state_COND_ELSE(const Token &tok);
    void state_COND_END(const Token &tok);
    void state_ELSE(const Token &tok);
    void state_CONTINUE(const Token &tok);
    void state_BREAK(const Token &tok);
    void state_VAR_DECL(const Token &tok);
    void state_VAR_DECL_LIST(const Token &tok);
    void state_VAR_DECL_ITEM(const Token &tok);
    void state_VAR_DECL_ITEM_END(const Token &tok);
    void state_VAR_DECL_ITEM_INIT(const Token &tok);
    void state_WHILE_LPAR(const Token &tok);
    void state_WHILE_RPAR(const Token &tok);
    void state_WHILE_END(const Token &tok);
    void state_EXP_LIST(const Token &tok);
    void state_EXP_LIST_ABS(const Token &tok);
    void state_EXP_ASSIGN(const Token &tok);
    void state_EXP_ASSIGN_END(const Token &tok);
    void state_EXP_OR(const Token &tok);
    void state_EXP_OR_END(const Token &tok);
    void state_EXP_AND(const Token &tok);
    void state_EXP_AND_END(const Token &tok);
    void state_EXP_EQ(const Token &tok);
    void state_EXP_EQ_END(const Token &tok);
    void state_EXP_CMP(const Token &tok);
    void state_EXP_CMP_END(const Token &tok);
    void state_EXP_A(const Token &tok);
    void state_EXP_A_END(const Token &tok);
    void state_EXP_X(const Token &tok);
    void state_EXP_X_END(const Token &tok);
    void state_EXP_X_HEAD(const Token &tok);
    void state_EXP_X_HEAD_END(const Token &tok);
    void state_EXP_NOT(const Token &tok);
    void state_EXP_NOT_END(const Token &tok);
    void state_EXP_CALL_OR_SUBS(const Token &tok);
    void state_EXP_SUBS_RSQ(const Token &tok);
    void state_EXP_CALL_AFTER_LPAR(const Token &tok);
    void state_EXP_CALL_RPAR(const Token &tok);
    void state_EXP_T(const Token &tok);
    void state_EXP_T_RPAR(const Token &tok);
    void state_LIST(const Token &tok);
    void state_LIST_END(const Token &tok);
    void state_FUNC(const Token &tok);
    void state_FUNC_AFTER_LPAR(const Token &tok);
    void state_FUNC_RPAR(const Token &tok);
    void state_FUNC_END(const Token &tok);

    void enter_program_or_exp();
    void enter_program();
    void enter_stmt();
    void enter_block(const Token &tok);
    void enter_block_pre();
    void enter_return(const Token &tok);
    void enter_condition(const Token &tok);
    void enter_else();
    void enter_continue(const Token &tok);
    void enter_break(const Token &tok);
    void enter_var_decl(const Token &tok);
    void enter_var_decl_list(const Token &tok);
    void enter_var_decl_item();
    void enter_while(const Token &tok);
    void enter_exp();
    void enter_exp_list();
    void enter_exp_list_abs();
    void enter_exp_assign();
    void enter_exp_or();
    void enter_exp_and();
    void enter_exp_eq();
    void enter_exp_cmp();
    void enter_exp_a();
    void enter_exp_x();
    void enter_exp_x_head();
    void enter_exp_not();
    void enter_exp_call_or_subs();
    void enter_exp_t();
    void enter_list(const Token &tok);
    void enter_function(const Token &tok);
    void enter_arg_decl_list(const Token &tok);

    void leave();
    void pass_up(const Token &tok);
    void shift(StateHandler state);
    void grow_head(OpCode opcode);
    void grow_body();

    typedef void (Parser::* MemFunc)();
    void do_exp_op(
        const Token &tok, const std::vector<uint32_t> &accepts,
        Parser::StateHandler next, MemFunc next_enter);
    void do_exp_op_end(
        const Token &tok, const std::vector<uint32_t> &accepts,
        MemFunc next_enter);
    void do_stmt_comma(const Token &tok);

    template<class TokenType, class NodeType>
    void do_const(const Token &tok);

    template<class NodeType>
    NodeType *get_top1();

    template<class NodeType>
    NodeType *get_top2();

    template<class NodeType = Node>
    NodeType *pop_top();

    void unpected_token(const Token &tok, const std::string &addtional = "");

    void set_pos_start(const Token &tok);
    void set_pos_start(const Node &node);
    void set_pos_end(const Token &tok);
    void set_pos_end(const Node &node);
    void set_pos_start_end(const Token &tok);

    static bool match_id(const Token &tok, const ustring &id);

    struct SortedState {
        SortedState(std::initializer_list<StateHandler> states);
        bool contains(const StateHandler &state) const;
        static bool cmp(const StateHandler &lhs, const StateHandler &rhs);

        std::vector<StateHandler> states;
    };

    static SortedState terminating_states;
    std::vector<StateHandler> states;
    std::vector<Node::Ptr> nodes;
};


#endif //JIAOBENSCRIPT_PARSER_H
