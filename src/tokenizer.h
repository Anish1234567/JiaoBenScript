#ifndef JIAOBENSCRIPT_TOKENIZER_H
#define JIAOBENSCRIPT_TOKENIZER_H


#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <queue>

#include "unicode.h"
#include "sourcepos.h"
#include "utils.hpp"


enum class TokenCode : uint32_t {
    PLUS            = '+',
    MINUS           = '-',
    STAR            = '*',
    SLASH           = '/',
    PERCENT         = '%',
    LESS            = '<',
    LESSEQ          = '<=',
    GREAT           = '>',
    GREATEQ         = '>=',
    EQ              = '==',
    NEQ             = '!=',
    NOT             = '!',
    AND             = '&&',
    OR              = '||',
    ASSIGN          = '=',
    PLUS_ASSIGN     = '+=',
    MINUS_ASSIGN    = '-=',
    STAR_ASSIGN     = '*=',
    SLASH_ASSIGN    = '/=',
    PERCENT_ASSIGN  = '%=',
    LSQUARE         = '[',
    RSQUARE         = ']',
    LPAR            = '(',
    RPAR            = ')',
    LBRACE          = '{',
    RBRACE          = '}',
    COMMA           = ',',
    SEMICOLON       = ';',
    ID              = 'ID',
    INT             = 'INT',
    FLOAT           = 'FLT',
    STRING          = 'STR',
    COMMENT         = 'CMT',
    END             = 'END',
};


std::string tokencode_to_string(TokenCode tc);


REPR(TokenCode) {
    return tokencode_to_string(value);
}


struct Token {
    typedef std::unique_ptr<Token> Ptr;

    explicit Token(TokenCode tc) : tokencode(tc) {}
    virtual ~Token() {}

    virtual bool operator==(const Token &rhs) const;
    virtual bool operator!=(const Token &rhs) const;

    virtual std::string name() const;
    virtual std::string repr_full() const;
    virtual std::string repr_short() const;
    virtual std::string repr_value() const;

    TokenCode tokencode;
    SourcePos pos_start;
    SourcePos pos_end;
};


REPR(Token) {
    return value.repr_full();
}


template<class ValueType>
inline std::string my_to_string(const ValueType &value) {
    return std::to_string(value);
}


template<>
inline std::string my_to_string<ustring>(const ustring &value) {
    return u8_encode(value);
}


template<class ValueType, TokenCode tokcode>
class ExtendedToken : public Token {
public:
    typedef ExtendedToken<ValueType, tokcode> _SelfType;
    typedef std::unique_ptr<_SelfType> Ptr;

    explicit ExtendedToken(const ValueType &value) : Token(tokcode), value(value) {}

    virtual std::string repr_value() const {
        return my_to_string(this->value);
    }

    virtual bool operator==(const Token &other) const {
        auto tok = dynamic_cast<const _SelfType *>(&other);
        if (tok != nullptr) {
            return this->value == tok->value;
        } else {
            return false;
        }
    }

    ValueType value;
};


typedef ExtendedToken<int64_t, TokenCode::INT> TokenInt;
typedef ExtendedToken<double, TokenCode::FLOAT> TokenFloat;
typedef ExtendedToken<ustring, TokenCode::STRING> TokenString;
typedef ExtendedToken<ustring, TokenCode::ID> TokenId;
typedef ExtendedToken<ustring, TokenCode::COMMENT> TokenComment;


enum class TokenizerState {
    INIT,
    OP,
    STRING,
    NUMBER,
    ID,
    LINE_COMMENT,
    BLOCK_COMMENT,
};


enum class StringSubState {
    INIT,
    NORMAL,
    ESCAPE,
    HEX,
    SURROGATED,
    SURROGATED_ESCAPE,
};


struct StringState {
    StringSubState state = StringSubState::INIT;
    ustring value;
    std::string hex;
    bool last_surrogate = false;
};


struct IdState {
    ustring value;
};


struct OpState {
    unichar op1;
};


enum class NumberSubState {
    INIT,
    SIGNED,
    INT_DIGIT,
    DOTTED,
    LEADING_DOT,
    EXP,
    EXP_SIGNED,
    EXP_DIGIT,
};


struct NumberState {
    NumberSubState state = NumberSubState::INIT;

    std::string int_digits;
    std::string dot_digits;
    std::string exp_digits;
    int num_sign = 1;
    int exp_sign = 1;
    bool has_dot = false;

    Token *to_token() const;
};


struct LineCommentState {
    ustring value;
};


enum class BlockCommentSubState {
    NORMAL,
    STARED,
};


struct BlockCommentState {
    BlockCommentSubState state = BlockCommentSubState::NORMAL;
    ustring value;
    SourcePos terminate_pos;
};


class Tokenizer {
public:
    Tokenizer() : linetracer(cur_pos) {}

    void feed(unichar ch);
    Token::Ptr pop();
    bool is_ready() const;

private:
    void refeed(unichar ch);
    void st_init(unichar ch);
    void st_op(unichar ch);
    void st_string(unichar ch);
    void st_number(unichar ch);
    void st_id(unichar ch);
    void st_line_comment(unichar ch);
    void st_block_comment(unichar ch);
    void unknown_char(unichar ch, const std::string &additional = "");
    void finish_num(unichar ch);

    TokenizerState state = TokenizerState::INIT;
    std::queue<Token::Ptr> buffer;

    SourcePos start_pos;
    SourcePos prev_pos;
    SourcePos cur_pos;
    LineTracer linetracer;

    OpState op_state {};
    StringState string_state {};
    NumberState num_state {};
    IdState id_state {};
    LineCommentState line_cmt_state {};
    BlockCommentState block_cmt_state {};

    static const std::map<unichar, unichar> escape_map;
    static const std::map<unichar, TokenCode> single_char_op_map;
    static const std::map<uint32_t, TokenCode> double_char_op_map;
};


#endif //JIAOBENSCRIPT_TOKENIZER_H
