#ifndef JSON_CXX_STRING_FMT_HPP
#define JSON_CXX_STRING_FMT_HPP


#include <cstdio>
#include <iostream>
#include <memory>
#include <string>


// http://stackoverflow.com/a/26221725/3886899
template<typename ...Args>
std::string string_fmt(const std::string &format, Args... args) {
    int size = snprintf(nullptr, 0, format.data(), args...) + 1;    // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), static_cast<size_t>(size), format.data(), args...);
    return std::string(buf.get(), buf.get() + size - 1);    // We don't want the '\0' inside
}


#endif //JSON_CXX_STRING_FMT_HPP
