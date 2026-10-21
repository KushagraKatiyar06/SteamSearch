#ifndef STEAMSEARCH_URLUTILS_H
#define STEAMSEARCH_URLUTILS_H

#include <cstdlib>
#include <string>

inline std::string urlDecode(std::string str) {
    std::string res;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            res += (char)std::strtol(str.substr(i + 1, 2).c_str(), nullptr, 16);
            i += 2;
        } else if (str[i] == '+') {
            res += ' ';
        } else {
            res += str[i];
        }
    }
    return res;
}

#endif //STEAMSEARCH_URLUTILS_H
