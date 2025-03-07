#pragma once 
#include "Token.hpp"
#include "MemoryPool.hpp"
#include <vector>
#include <string_view>
#include <optional>
#include <algorithm>

namespace calc {
    class TokenProcessor {
        public: 
            static std::vector<Token> tokenize(std::string_view expression);
        
        private:
            static MemoryPool<Token>& getTokenPool() {
                static MemoryPool<Token> pool;
                return pool;
            }
            
            static std::optional<Token> parseNumber(std::string_view& input);
            static bool isOperator(char c);
            static void handleWord(std::string_view& input, std::vector<Token>& tokens);
    };
}