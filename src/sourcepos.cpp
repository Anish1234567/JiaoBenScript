#include <tuple>

#include "sourcepos.h"


bool SourcePos::operator==(const SourcePos &other) const {
    return std::tie(this->lineno, this->rowno) == std::tie(other.lineno, other.rowno);
}


bool SourcePos::operator!=(const SourcePos &other) const {
    return !(*this == other);
}


bool SourcePos::is_valid() const {
    return this->lineno >= 0 && this->rowno >= 0;
}


void LineTracer::add_char(unsigned int ch) {
    if (this->last_newline) {
        this->pos.lineno++;
        this->pos.rowno = 0;
    } else {
        this->pos.rowno++;
    }

    this->last_newline = (ch == '\n');
}
