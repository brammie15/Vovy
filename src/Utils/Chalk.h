#ifndef CHALK_H
#define CHALK_H

#include <string>

class Chalk {
public:
    static constexpr auto Reset   = "\033[0m";
    static constexpr auto Red     = "\033[31m";
    static constexpr auto Green   = "\033[32m";
    static constexpr auto Yellow  = "\033[33m";
    static constexpr auto Blue    = "\033[34m";
    static constexpr auto Magenta = "\033[35m";
    static constexpr auto Cyan    = "\033[36m";
    static constexpr auto White   = "\033[37m";
    static constexpr auto Bold    = "\033[1m";
    static constexpr auto Underline = "\033[4m";

    static std::string color(const std::string& text, const char* colorCode) {
        return std::string(colorCode) + text + Reset;
    }
};

#endif //CHALK_H
