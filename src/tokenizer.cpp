#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

#include "exceptions.h"
#include "string_fmt.hpp"
#include "tokenizer.h"


std::string tokencode_to_string(TokenCode tc) {
    std::string ans;
    uint32_t u32 = static_cast<uint32_t>(tc);
    for (size_t i = 0; i < 4; ++i) {
        char ch = static_cast<char>(u32 >> 24);
        if (ch != 0) {
            ans.push_back(ch);
        }
        u32 <<= 8;
    }
    return ans;
}


bool Token::operator==(const Token &rhs) const {
    return this->tokencode == rhs.tokencode;
}


bool Token::operator!=(const Token &rhs) const {
    return !(*this == rhs);
}


std::string Token::name() const {
    return "Tok:" + tokencode_to_string(this->tokencode);
}


std::string Token::repr_full() const {
    return string_fmt(
        "<%s start=%s end=%s>",
        this->repr_short().data(), repr(this->pos_start).data(), repr(this->pos_end).data()
    );
}


std::string Token::repr_short() const {
    return string_fmt("%s %s", this->name().data(), this->repr_value().data());
}


std::string Token::repr_value() const {
    return "";
}


void Tokenizer::feed(unichar ch) {
    // TODO: limit stack depth
    this->prev_pos = this->cur_pos;
    this->cur_pos.add_char(ch);
    this->refeed(ch);
}


void Tokenizer::refeed(unichar ch) {
    switch (this->state) {
    case TokenizerState::INIT:
        return this->st_init(ch);
    case TokenizerState::OP:
        return this->st_op(ch);
    case TokenizerState::STRING:
        return this->st_string(ch);
    case TokenizerState::NUMBER:
        return this->st_number(ch);
    case TokenizerState::ID:
        return this->st_id(ch);
    case TokenizerState::LINE_COMMENT:
        return this->st_line_comment(ch);
    case TokenizerState::BLOCK_COMMENT:
        return this->st_block_comment(ch);
    }
    assert(!"Unreachable");
}


Token::Ptr Tokenizer::pop() {
    if (!this->buffer.empty()) {
        Token::Ptr ret = std::move(this->buffer.front());
        this->buffer.pop();
        return ret;
    } else {
        return Token::Ptr();
    }
}


bool Tokenizer::is_ready() const {
    return this->state == TokenizerState::INIT;
}


void Tokenizer::st_init(unichar ch) {
    this->start_pos = this->cur_pos;
    if (is_space(ch)) {
        // pass
    } else if (USTRING("[]{}(),;").find(ch) != ustring::npos) {
        Token *tok = new Token(static_cast<TokenCode>(ch));
        tok->pos_start = this->cur_pos;
        tok->pos_end = this->cur_pos;
        this->buffer.emplace(tok);
    } else if (USTRING("+-*/%<>=!&|").find(ch) != ustring::npos) {
        this->op_state.op1 = ch;
        this->state = TokenizerState::OP;
    } else if (ch == '"') {
        this->state = TokenizerState::STRING;
        this->refeed(ch);
    } else if (ch == '.' || is_digit(ch)) {
        this->state = TokenizerState::NUMBER;
        this->refeed(ch);
    } else if (ch == '_' || is_alpha(ch)) { // TODO: unicode identifier
        this->state = TokenizerState::ID;
        this->refeed(ch);
    } else {
        this->unknown_char(ch);     // TODO: msg?
    }
}


void Tokenizer::st_op(unichar ch) {
    uint32_t combined = (this->op_state.op1 << 8) | ch;
    if (combined == '//') {
        this->state = TokenizerState::LINE_COMMENT;
    } else if (combined == '/*') {
        this->state = TokenizerState::BLOCK_COMMENT;
    } else {
        auto dit = Tokenizer::double_char_op_map.find(combined);
        if (dit != Tokenizer::double_char_op_map.end()) {
            Token *tok = new Token(dit->second);
            tok->pos_start = this->start_pos;
            tok->pos_end = this->cur_pos;
            this->buffer.emplace(tok);
            // reset
            this->state = TokenizerState::INIT;
            this->op_state = OpState();
        } else {
            auto sit = Tokenizer::single_char_op_map.find(this->op_state.op1);
            if (sit != Tokenizer::single_char_op_map.end()) {
                Token *tok = new Token(sit->second);
                tok->pos_start = this->start_pos;
                tok->pos_end = this->prev_pos;
                this->buffer.emplace(tok);
                // reset
                this->state = TokenizerState::INIT;
                this->op_state = OpState();

                this->refeed(ch);
            } else {
                assert(this->op_state.op1 == '&' || this->op_state.op1 == '|');
                this->unknown_char(ch, "expect && or ||");
            }
        }
    }
}


// TODO: augment this
void Tokenizer::st_string(unichar ch) {
    // TODO: limit length
    StringState &ss = this->string_state;
    if (ss.state == StringSubState::INIT) {
        if (ch != '"') {
            this->unknown_char(ch, "expect double quote");
        } else {
            ss.state = StringSubState::NORMAL;
        }
    } else if (ss.state == StringSubState::NORMAL) {
        if (ch == '"') {
            Token *tok = new TokenString(ss.value);
            tok->pos_start = this->start_pos;
            tok->pos_end = this->cur_pos;
            this->buffer.emplace(tok);
            // reset
            this->string_state = StringState();
            this->state = TokenizerState::INIT;
        } else if (ch == '\\') {
            ss.state = StringSubState::ESCAPE;
        } else if (ch < 0x20) {
            this->unknown_char(ch, "unescaped control char");
        } else {
            ss.value.push_back(ch);
        }
    } else if (ss.state == StringSubState::ESCAPE) {
        auto it = Tokenizer::escape_map.find(ch);
        if (it != Tokenizer::escape_map.end()) {
            ss.value.push_back(it->second);
            ss.state = StringSubState::NORMAL;
        } else if (ch == 'u') {
            ss.state = StringSubState::HEX;
        } else {
            this->unknown_char(ch, "unknown escapes");
        }
    } else if (ss.state == StringSubState::HEX) {
        if (ss.hex.size() == 4) {
            StringSubState next_state = StringSubState::NORMAL;
            unichar uch = static_cast<unichar>(strtol(ss.hex.data(), nullptr, 16));
            if (ss.last_surrogate) {
                if (is_surrogate_low(uch)) {
                    unichar hi = ss.value.back();
                    ss.value.pop_back();
                    uch = u16_assemble_surrogate(hi, uch);
                } else {
                    this->unknown_char(uch, "expect lower surrogate");
                }
                ss.last_surrogate = false;
            } else {
                if (is_surrogate_high(uch)) {
                    ss.last_surrogate = true;
                    next_state = StringSubState::SURROGATED;
                } else if (is_surrogate_low(uch)) {
                    this->unknown_char(uch, "unexpected lower surrogate");
                }
            }

            ss.value.push_back(uch);
            ss.hex.clear();
            ss.state = next_state;
            this->refeed(ch);
        } else if (is_xdigit(ch)) {
            ss.hex.push_back(static_cast<char>(to_lower(ch)));
        } else {
            this->unknown_char(ch, "expect hex digit");
        }
    } else if (ss.state == StringSubState::SURROGATED) {
        if (ch == '\\') {
            ss.state = StringSubState::SURROGATED_ESCAPE;
        } else {
            this->unknown_char(ch, "expect lower surrogate escape");
        }
    } else if (ss.state == StringSubState::SURROGATED_ESCAPE) {
        if (ch == 'u') {
            ss.state = StringSubState::HEX;
        } else {
            this->unknown_char(ch, "expect lower surrogate escape");
        }
    } else {
        assert(!"Unreachable");
    }
}


void Tokenizer::st_number(unichar ch) {
    NumberState &ns = this->num_state;
    if (ns.state == NumberSubState::INIT) {
        ns.state = NumberSubState::SIGNED;
        if (ch == '-') {
            ns.num_sign = -1;
        } else if (ch == '+') {
            // pass
        } else {
            this->st_number(ch);
        }
    } else if (ns.state == NumberSubState::SIGNED) {
        if (is_digit(ch)) {
            ns.int_digits.push_back(static_cast<char>(ch));
            ns.state = NumberSubState::INT_DIGIT;
        } else if (ch == '.') {
            ns.has_dot = true;
            ns.state = NumberSubState::LEADING_DOT;
        } else {
            this->unknown_char(ch, "expect dot or digit");
        }
    } else if (ns.state == NumberSubState::INT_DIGIT) {
        if (is_digit(ch)) {
            ns.int_digits.push_back(static_cast<char>(ch));
        } else if (ch == '.') {
            ns.has_dot = true;
            ns.state = NumberSubState::DOTTED;
        } else if (ch == 'e' || ch == 'E') {
            ns.state = NumberSubState::EXP;
        } else {
            this->finish_num(ch);
        }
    } else if (ns.state == NumberSubState::LEADING_DOT) {
        if (is_digit(ch)) {
            ns.dot_digits.push_back(static_cast<char>(ch));
            ns.state = NumberSubState::DOTTED;
        } else {
            this->unknown_char(ch, "expect digit");
        }
    } else if (ns.state == NumberSubState::DOTTED) {
        if (is_digit(ch)) {
            ns.dot_digits.push_back(static_cast<char>(ch));
        } else if (ch == 'e' || ch == 'E') {
            ns.state = NumberSubState::EXP;
        } else {
            this->finish_num(ch);
        }
    } else if (ns.state == NumberSubState::EXP) {
        if (ch == '+' || ch == '-') {
            ns.state = NumberSubState::EXP_SIGNED;
            if (ch == '-') {
                ns.exp_sign = -1;
            }
        } else if (is_digit(ch)) {
            ns.exp_digits.push_back(static_cast<char>(ch));
            ns.state = NumberSubState::EXP_DIGIT;
        } else {
            this->unknown_char(ch, "expect digit or sign");
        }
    } else if (ns.state == NumberSubState::EXP_SIGNED) {
        if (is_digit(ch)) {
            ns.exp_digits.push_back(static_cast<char>(ch));
            ns.state = NumberSubState::EXP_DIGIT;
        } else {
            this->unknown_char(ch, "expect digit");
        }
    } else if (ns.state == NumberSubState::EXP_DIGIT) {
        if (is_digit(ch)) {
            ns.exp_digits.push_back(static_cast<char>(ch));
        } else {
            this->finish_num(ch);
        }
    } else {
        assert(!"Unreachable");
    }
}


void Tokenizer::finish_num(unichar ch) {
    Token *tok = this->num_state.to_token();
    tok->pos_start = this->start_pos;
    tok->pos_end = this->prev_pos;
    this->buffer.emplace(tok);
    // reset states
    this->num_state = NumberState();
    this->state = TokenizerState::INIT;
    this->refeed(ch);
}


template<class T>
static T string_to_number(const std::string &digits) {
    T val = 0;
    for (auto ch : digits) {
        val *= 10;
        val += ch - '0';
    }
    return val;
}


Token *NumberState::to_token() const {
    int64_t iv = string_to_number<int64_t>(this->int_digits);
    double fv = string_to_number<double>(this->int_digits);

    fv += string_to_number<double>(this->dot_digits) \
        * pow(10, -static_cast<double>(this->dot_digits.size()));

    if (!this->exp_digits.empty()) {
        double exp = string_to_number<double>(this->exp_digits) * this->exp_sign;   // inf
        fv *= std::pow(10, exp);    // inf
        iv *= std::pow(10, exp);
    }

    iv *= this->num_sign;
    fv *= this->num_sign;

    if (!this->has_dot && this->exp_sign > 0
        && std::numeric_limits<int64_t>::min() < fv && fv < std::numeric_limits<int64_t>::max())
    {
        return new TokenInt(iv);
    } else {
        return new TokenFloat(fv);
    }
}


void Tokenizer::st_id(unichar ch) {
    // TODO: limit length
    if (ch == '_' || is_alpha(ch)) {    // TODO: unicode
        this->id_state.value.push_back(ch);
    } else {
        TokenId *tok = new TokenId(this->id_state.value);
        tok->pos_start = this->start_pos;
        tok->pos_end = this->prev_pos;
        this->buffer.emplace(tok);
        // reset
        this->id_state = IdState();
        this->state = TokenizerState::INIT;
        this->refeed(ch);
    }
}


void Tokenizer::st_line_comment(unichar ch) {
    if (ch == '\n') {
        TokenComment *tok = new TokenComment(this->line_cmt_state.value);
        tok->pos_start = this->start_pos;
        tok->pos_end = this->prev_pos;
        this->buffer.emplace(tok);
        // reset
        this->line_cmt_state = LineCommentState();
        this->state = TokenizerState::INIT;
    } else {
        this->line_cmt_state.value.push_back(ch);
    }
}


void Tokenizer::st_block_comment(unichar ch) {
    BlockCommentState &cs = this->block_cmt_state;
    if (cs.state == BlockCommentSubState::NORMAL) {
        if (ch == '*') {
            cs.state = BlockCommentSubState::STARED;
            cs.terminate_pos = this->prev_pos;
        } else {
            cs.value.push_back(ch);
        }
    } else if (cs.state == BlockCommentSubState::STARED) {
        if (ch == '/') {
            TokenComment *tok = new TokenComment(cs.value);
            tok->pos_start = this->start_pos;
            tok->pos_end = cs.terminate_pos;
            this->buffer.emplace(tok);
            // reset
            this->block_cmt_state = BlockCommentState();
            this->state = TokenizerState::INIT;
        } else {
            cs.state = BlockCommentSubState::NORMAL;
            cs.value.push_back('*');
            cs.value.push_back(ch);
        }
    } else {
        assert(!"Unreachable");
    }
}


void Tokenizer::unknown_char(unichar ch, const std::string &additional) {
    std::string msg = string_fmt("Unknown char: \\u%04x", ch);
    if (!additional.empty()) {
        msg += ", " + additional;
    }
    throw TokenizerError(msg, this->start_pos, this->cur_pos);
}


const std::map<unichar, unichar> Tokenizer::escape_map {
    {'b', '\b'},
    {'f', '\f'},
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'"', '"'},
    {'\\', '\\'},
    {'/', '/'},
};


const std::map<unichar, TokenCode> Tokenizer::single_char_op_map {
    {'+', TokenCode::PLUS},
    {'-', TokenCode::MINUS},
    {'*', TokenCode::STAR},
    {'/', TokenCode::SLASH},
    {'%', TokenCode::PERCENT},
    {'<', TokenCode::LESS},
    {'>', TokenCode::GREAT},
    {'!', TokenCode::NOT},
    {'=', TokenCode::ASSIGN},
};


const std::map<uint32_t, TokenCode> Tokenizer::double_char_op_map {
    {'<=', TokenCode::LESSEQ},
    {'>=', TokenCode::GREATEQ},
    {'==', TokenCode::EQ},
    {'!=', TokenCode::NEQ},
    {'&&', TokenCode::AND},
    {'||', TokenCode::OR},
    {'+=', TokenCode::PLUS_ASSIGN},
    {'-=', TokenCode::MINUS_ASSIGN},
    {'*=', TokenCode::STAR_ASSIGN},
    {'/=', TokenCode::SLASH_ASSIGN},
    {'%=', TokenCode::PERCENT_ASSIGN},
};
