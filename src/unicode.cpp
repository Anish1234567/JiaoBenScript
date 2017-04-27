#include <cassert>
#include "unicode.h"


static const uint8_t PREFIX_MASKS[8] = {
    0b11111111,
    0b01111111,
    0b00111111,
    0b00011111,
    0b00001111,
    0b00000111,
    0b00000011,
    0b00000001,
};


static const uint8_t PREFIXES[8] = {
    0b00000000,
    0b10000000,
    0b11000000, // 2
    0b11100000, // 3
    0b11110000, // 4
    0b11111000, // 5
    0b11111100, // 6
    0b11111110,
};


int u8_read_char_len(const char *s) {
    uint8_t c0 = static_cast<uint8_t>(s[0]);
    if ((c0 >> 7) == 0) {
        return 1;
    } else if ((c0 >> 5) == 0b110) {
        return 2;
    } else if ((c0 >> 4) == 0b1110) {
        return 3;
    } else if ((c0 >> 3) == 0b11110) {
        return 4;
    } else if ((c0 >> 2) == 0b111110) {
        return 5;
    } else if ((c0 >> 1) == 0b1111110) {
        return 6;
    } else {
        throw DecodeError("Bad leading char", c0);
    }
}


static unichar _u8_read_char_with_len(const char *s, int len) {
    assert(1 <= len && len <= 6);

    const uint8_t *c = reinterpret_cast<const uint8_t *>(s);
    if (len == 1) {
        return c[0];
    }

    unichar ans = c[0] & PREFIX_MASKS[len + 1];
    for (int i = 1; i < len; i++) {
        if ((c[i] >> 6) != 0b10) {
            throw DecodeError("Bad following char", (uint8_t)(c[i]));
        }
        ans = (ans << 6) | (c[i] & PREFIX_MASKS[2]);
    }
    return ans;
}


unichar u8_read_char(const char *s) {
    int len = u8_read_char_len(s);
    return _u8_read_char_with_len(s, len);
}


int u8_char_len(unichar ch) {
    assert(ch < 0x80000000);
    if (ch < 0x80) {
        return 1;
    } else {
        int ans = 2;
        for (unichar upper = 0x800; upper <= ch; upper <<= 5) {
            ans++;
        }
        return ans;
    }
}


static char *_u8_write_char_with_len(char *buf, unichar ch, int len) {
    assert(1 <= len && len <= 6);
    if (len == 1) {
        *buf++ = static_cast<char>(ch);
    } else {
        char leading = static_cast<char>(ch >> (6 * (len - 1)));
        *buf++ = leading | PREFIXES[len];

        unichar remain = ch << (38 - 6 * len);
        for (int i = 1; i < len; i++) {
            *buf++ = static_cast<char>((remain >> 26) | PREFIXES[1]);
            remain <<= 6;
        }
    }
    return buf;
}


char *u8_write_char(char *buf, unichar ch) {
    int len = u8_char_len(ch);
    return _u8_write_char_with_len(buf, ch, len);
}


size_t u8_unicode_len(const char *s) {
    size_t ans = 0;
    for (const char *start = s; *start != '\0'; start += u8_read_char_len(start)) {
        ans++;
    }
    return ans;
}


// FIXME: duplicated code
size_t u8_unicode_len(const std::string &s) {
    size_t ans = 0;
    const char *const end = s.data() + s.size();
    for (const char *start = s.data(); start < end; start += u8_read_char_len(start)) {
        ans++;
    }
    return ans;
}


// TODO: reject overlong encoding
ustring u8_decode(const char *s) {
    size_t len = u8_unicode_len(s);
    ustring ans;
    ans.reserve(len);

    while (*s != '\0') {
        int clen = u8_read_char_len(s);
        ans.push_back(u8_read_char(s));
        s += clen;
    }
    return ans;
}


// FIXME: duplicated code
ustring u8_decode(const std::string &str) {
    size_t len = u8_unicode_len(str);
    ustring ans;
    ans.reserve(len);

    const char *const end = str.data() + str.size();
    const char *start = str.data();
    while (start < end) {
        int clen = u8_read_char_len(start);
        ans.push_back(u8_read_char(start));
        start += clen;
    }
    return ans;
}


size_t u8_byte_len(const ustring &us) {
    size_t ans = 0;
    for (unichar ch : us) {
        ans += u8_char_len(ch);
    }
    return ans;
}


std::string u8_encode(const ustring &us) {
    size_t len = u8_byte_len(us);
    std::string ans(len, '\0');
    char *data = const_cast<char *>(ans.data());
    for (unichar ch : us) {
        data = u8_write_char(data, ch);
    }
    return ans;
}


bool is_space(unichar ch) {
    // \t\n\v\f\r and space
    return ('\t' <= ch && ch <= '\r') || ch == ' ';
}


bool is_digit(unichar ch) {
    return '0' <= ch && ch <= '9';
}


bool is_xdigit(unichar ch) {
    return is_digit(ch) || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F');
}


bool is_alpha(unichar ch) {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}


unichar to_lower(unichar ch) {
    if ('A' <= ch && ch <= 'Z') {
        ch -= 'A' - 'a';
    }
    return ch;
}


bool is_surrogate_high(unichar ch) {
    return 0xd800 <= ch && ch <= 0xdbff;
}


bool is_surrogate_low(unichar ch) {
    return 0xdc00 <= ch && ch <= 0xdfff;
}


bool is_surrogate(unichar ch) {
    return is_surrogate_high(ch) || is_surrogate_low(ch);
}


unichar u16_assemble_surrogate(unichar hi, unichar lo) {
    assert(is_surrogate_high(hi));
    assert(is_surrogate_low(lo));
    return (((hi - 0xd800) << 10) | (lo - 0xdc00)) + 0x010000;
}
