#ifndef JIAOBENSCRIPT_REPLACE_RESTORE_HPP
#define JIAOBENSCRIPT_REPLACE_RESTORE_HPP


template<class T>
struct ReplaceRestore {
    ReplaceRestore(T *location, T new_value)
        : origin(*location), location(location)
    {
        *location = new_value;
    }

    ~ReplaceRestore() {
        if (this->location != nullptr) {
            *this->location = this->origin;
        }
    }

    ReplaceRestore(const ReplaceRestore<T> &) = delete;
    ReplaceRestore(ReplaceRestore<T> &&other)
        : origin(other.origin), location(other.location)
    {
        other.location = nullptr;
    }

    ReplaceRestore &operator=(const ReplaceRestore<T> &) = delete;
    ReplaceRestore &operator=(ReplaceRestore<T> &&other) {
        this->location = other.location;
        this->origin = other.origin;
        other.location = nullptr;
        return *this;
    }

    T origin;
    T *location;
};


#endif //JIAOBENSCRIPT_REPLACE_RESTORE_HPP
