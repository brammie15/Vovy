#ifndef CHALK_H
#define CHALK_H
#include <iostream>
#include <string>

class Chalk {
public:
    static constexpr const char* Reset   = "\033[0m";
    static constexpr const char* Red     = "\033[31m";
    static constexpr const char* Green   = "\033[32m";
    static constexpr const char* Yellow  = "\033[33m";
    static constexpr const char* Blue    = "\033[34m";
    static constexpr const char* Magenta = "\033[35m";
    static constexpr const char* Cyan    = "\033[36m";
    static constexpr const char* White   = "\033[37m";
    static constexpr const char* Bold    = "\033[1m";
    static constexpr const char* Underline = "\033[4m";

    static std::string color(const std::string& text, const char* colorCode) {
        return std::string(colorCode) + text + Reset;
    }
};

#endif //CHALK_H
