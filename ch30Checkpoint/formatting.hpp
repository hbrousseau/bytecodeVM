#ifndef FORMATTING_HPP
#define FORMATTING_HPP

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// attempting to get the same formatting as the book instead of using %g
std::string trimTrailingZeros (const std::string& str) {
    size_t dotPos = str.find('.');
    if (dotPos != std::string::npos) {
        size_t lastNonZero = str.find_last_not_of('0');
        if (lastNonZero != std::string::npos && lastNonZero > dotPos) {
            return str.substr(0, lastNonZero + 1);
        }
        else {
            return str.substr(0, dotPos);
        }
    }
    return str;
}

#endif