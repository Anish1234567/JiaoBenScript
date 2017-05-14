#ifndef JIAOBENSCRIPT_PARSER_H
#define JIAOBENSCRIPT_PARSER_H

#include <vector>

#include "exceptions.h"
#include "tokenizer.h"
#include "node.h"


enum class ParserState {
    END,
    PROGRAM_OR_EXP,
    PROGRAM,
    STMT,
    STMT_EXP,
    BLOCK_PRE,
    BLOCK,
    BLOCK_END,
    RETURN,
    RETURN_EXP,
    COND_LPAR,
    COND_RPAR,
    COND_ELSE,
    COND_END,
    ELSE,
    CONTINUE,
    BREAK,
    VAR_DECL,
    VAR_DECL_LIST,
    VAR_DECL_ITEM,
    VAR_DECL_ITEM_END,
    VAR_DECL_ITEM_INIT,
    WHILE_LPAR,
    WHILE_RPAR,
    WHILE_END,
    EXP_LIST,
    EXP_LIST_ABS,
    EXP_ASSIGN,
    EXP_ASSIGN_END,
    EXP_OR,
    EXP_OR_END,
    EXP_AND,
    EXP_AND_END,
    EXP_EQ,
    EXP_EQ_END,
    EXP_CMP,
    EXP_CMP_END,
    EXP_A,
    EXP_A_END,
    EXP_X,
    EXP_X_END,
    EXP_X_HEAD,
    EXP_X_HEAD_END,
    EXP_NOT,
    EXP_NOT_END,
    EXP_CALL_OR_SUBS,
    EXP_SUBS_RSQ,
    EXP_CALL_AFTER_LPAR,
    EXP_CALL_RPAR,
    EXP_T,
    EXP_T_RPAR,
    LIST,
    LIST_END,
    FUNC,
    FUNC_AFTER_LPAR,
    FUNC_RPAR,
    FUNC_END,
};


class Parser {
public:
    void start_program();
    void start_repl();
    bool can_end() const;
    void feed(const Token &tok);
    Node::Ptr pop_result();

private:
    std::vector<ParserState> states;
    std::vector<Node::Ptr> nodes;

private:
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
    void shift(ParserState state);
    void grow_head(OpCode opcode);
    void grow_body();

    typedef void (Parser::* MemFunc)();
    void do_exp_op(
        const Token &tok, const std::vector<uint32_t> &accepts,
        ParserState next, MemFunc next_enter);
    void do_exp_op_end(
        const Token &tok, const std::vector<uint32_t> &accepts,
        MemFunc next_enter);

    template<class TokenType, class NodeType>
    void do_const(const Token &tok);

    void unpected_token(const Token &tok, const std::string &addtional = "");

    template<class NodeType>
    NodeType *get_top1() {
        assert(!this->nodes.empty());
        return static_cast<NodeType *>(this->nodes.back().get());
    }

    template<class NodeType>
    NodeType *get_top2() {
        assert(this->nodes.size() >= 2);
        return static_cast<NodeType *>((++this->nodes.rbegin())->get());
    }

    template<class NodeType = Node>
    NodeType *pop_top() {
        NodeType *top = this->nodes.back().release();
        this->nodes.pop_back();
        return top;
    }

    void set_pos_start(const Token &tok);
    void set_pos_start(const Node &node);
    void set_pos_end(const Token &tok);
    void set_pos_end(const Node &node);
    void set_pos_start_end(const Token &tok);

    static bool match_id(const Token &tok, const ustring &id);
};


#endif //JIAOBENSCRIPT_PARSER_H
