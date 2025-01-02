#pragma once
#include <string_view>

namespace calc {
    struct Constants {
        static constexpr double PI = 3.14159265358979323846;
        static constexpr double E = 2.71828182845904523536;
        static constexpr double PHI = 1.61803398874989484820;   
        static constexpr double SQRT2 = 1.41421356237309504880; 
        static constexpr std::string_view PROMPT = "> ";
        static constexpr size_t MAX_HISTORY = 100;
    };
}
