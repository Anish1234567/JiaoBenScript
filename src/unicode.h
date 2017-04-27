#ifndef JSON_CXX_UNICODE_H
#define JSON_CXX_UNICODE_H

#include <cstdint>
#include <stdexcept>
#include <string>

#include "string_fmt.hpp"


typedef char32_t unichar;
typedef std::u32string ustring;


class UnicodeError : public std::runtime_error {
public:
    UnicodeError(const std::string &msg) : runtime_error(msg) {}
};


class DecodeError : public UnicodeError {
public:
    DecodeError(const std::string &msg, uint8_t byte)
        : UnicodeError(string_fmt("%s: \\x%x", msg.data(), byte))
    {}
};


int u8_read_char_len(const char *s);
unichar u8_read_char(const char *s);
int u8_char_len(unichar ch);
char *u8_write_char(char *buf, unichar ch);
size_t u8_unicode_len(const char *s);
size_t u8_unicode_len(const std::string &s);
ustring u8_decode(const char *s);
ustring u8_decode(const std::string &str);
size_t u8_byte_len(const ustring &us);
std::string u8_encode(const ustring &us);

bool is_space(unichar ch);
bool is_digit(unichar ch);
bool is_xdigit(unichar ch);
bool is_alpha(unichar ch);

unichar to_lower(unichar ch);

bool is_surrogate_high(unichar ch);
bool is_surrogate_low(unichar ch);
bool is_surrogate(unichar ch);
unichar u16_assemble_surrogate(unichar hi, unichar lo);


#define UCHAR u8_read_char  // convert a string literal to unichar
#define USTRING u8_decode   // convert a string literal to ustring


#endif //JSON_CXX_UNICODE_H
