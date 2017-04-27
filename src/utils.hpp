#ifndef UTILS_HPP
#define UTILS_HPP


#include <string>
#include <iostream>


#define REPR(T) \
    std::string repr(const T &value); \
    inline std::ostream &operator <<(std::ostream &os, const T &value) { \
        os << repr(value); \
        return os; \
    } \
    inline std::string repr(const T &value)


inline std::string repr(int value) {
    return std::to_string(value);
}
inline std::string repr(long value) {
    return std::to_string(value);
}
inline std::string repr(long long value) {
    return std::to_string(value);
}
inline std::string repr(unsigned value) {
    return std::to_string(value);
}
inline std::string repr(unsigned long value) {
    return std::to_string(value);
}
inline std::string repr(unsigned long long value) {
    return std::to_string(value);
}
inline std::string repr(float value) {
    return std::to_string(value);
}
inline std::string repr(double value) {
    return std::to_string(value);
}
inline std::string repr(long double value) {
    return std::to_string(value);
}
inline std::string repr(bool value) {
    return value ? "true" : "false";
}


#endif // UTILS_HPP
