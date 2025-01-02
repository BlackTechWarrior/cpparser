#pragma once 
#include "Token.hpp"
#include <vector>
#include <string_view>
#include <optional>
#include <algorithm>

namespace calc {
    class TokenProcessor {
        public: 
            static std::vector<Token> tokenize(std::string_view expression);
        
        private:
            static std::optional<Token> parseNumber(std::string_view& input);
            static bool isOperator(char c);
            static bool isCommand(std::string_view str);
            static void handleWord(std::string_view& input, std::vector<Token>& tokens);
    };
}
