#ifndef CALCXX_SOURCEPOS_H
#define CALCXX_SOURCEPOS_H


#include <string>

#include "utils.hpp"


struct SourcePos {
    int lineno;
    int rowno;

private:
    bool last_newline = true;

public:
    SourcePos(int lineno, int rowno)
        : lineno(lineno), rowno(rowno)
    {}

    // invalid
    SourcePos() : lineno(-1), rowno(-1) {}

    bool operator==(const SourcePos &other) const;
    bool operator!=(const SourcePos &other) const;
    bool is_valid() const;
    void add_char(unsigned int ch);
};


REPR(SourcePos) {
    return "<Pos " + std::to_string(value.lineno) + ":" + std::to_string(value.rowno) + ">";
}


#endif //CALCXX_SOURCEPOS_H
