#include "TokenProcessor.hpp"
#include "Constants.hpp"
#include <cctype>
#include <algorithm>

namespace calc {
    std::vector<Token> TokenProcessor::tokenize(std::string_view expression) {
        std::vector<Token> tokens;
        tokens.reserve(expression.length() / 2);
        auto& pool = getTokenPool();
        pool.reset();
    
        std::string_view remaining = expression;
        bool expectingValue = true;  // Determines if we expect a value (true) or operator (false)
    
        while (!remaining.empty()) {
            char c = remaining.front();
    
            if (std::isspace(c)) { 
                remaining.remove_prefix(1); 
                continue; 
            }
            
            // Handle comma for function parameters
            if (c == ',') {
                tokens.push_back(*pool.allocate(Token::Type::Comma, ","));
                expectingValue = true;
                remaining.remove_prefix(1);
                continue;
            }
            
            // Handle colon for function definition
            if (c == ':') {
                tokens.push_back(*pool.allocate(Token::Type::Colon, ":"));
                expectingValue = true;
                remaining.remove_prefix(1);
                continue;
            }
    
            // Handle words (variables, constants, functions, commands)
            if (std::isalpha(c)) {  
                size_t peek_idx = 0;
                while (peek_idx < remaining.length() && (std::isalnum(remaining[peek_idx]) || remaining[peek_idx] == '_')) { 
                    peek_idx++; 
                }
    
                std::string word(remaining.substr(0, peek_idx));
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    
                // If not a command, check for implicit multiplication
                if (!tokens.empty() && !expectingValue && 
                    (tokens.back().getType() == Token::Type::Number || 
                     tokens.back().getType() == Token::Type::Variable || 
                     tokens.back().getType() == Token::Type::Constant || 
                     tokens.back().getType() == Token::Type::PrevResult || 
                     tokens.back().getValue() == ")" || 
                     tokens.back().getValue() == "!")) {
                    tokens.push_back(*pool.allocate(Token::Type::Operator, "*"));
                }
    
                handleWord(remaining, tokens);
                expectingValue = false;
                continue;
            }
    
            // Handle unary and binary minus
            if (c == '-') {
                remaining.remove_prefix(1);
                
                if (expectingValue) {
                    // Check if it's a negative number
                    if (!remaining.empty() && std::isdigit(remaining.front())) {
                        size_t idx = 0;
                        bool hasDecimal = false;
                        
                        while (idx < remaining.length() && 
                               (std::isdigit(remaining[idx]) || (!hasDecimal && remaining[idx] == '.'))) {
                            if (remaining[idx] == '.') hasDecimal = true;
                            idx++;
                        }
                        
                        if (idx > 0) {
                            std::string number = "-" + std::string(remaining.substr(0, idx));
                            remaining.remove_prefix(idx);
                            tokens.push_back(*pool.allocate(Token::Type::Number, number));
                            expectingValue = false;
                            continue;
                        }
                    }
                    
                    tokens.push_back(*pool.allocate(Token::Type::Operator, "neg")); // Unary negation
                } else {
                    tokens.push_back(*pool.allocate(Token::Type::Operator, "-")); // Subtraction
                    expectingValue = true;
                }
                continue;
            }
    
            // Handle numbers with implicit multiplication
            if (auto numToken = parseNumber(remaining)) {
                if (!tokens.empty() && !expectingValue &&
                    (tokens.back().getType() == Token::Type::Variable || 
                     tokens.back().getType() == Token::Type::Constant || 
                     tokens.back().getType() == Token::Type::PrevResult || 
                     tokens.back().getValue() == ")" || 
                     tokens.back().getValue() == "!")) {
                    tokens.push_back(*pool.allocate(Token::Type::Operator, "*"));
                }
    
                tokens.push_back(*numToken);
                expectingValue = false;
    
                // If a number is followed by a variable, function, or `(`
                if (!remaining.empty() && (std::isalpha(remaining.front()) || remaining.front() == '(')) {
                    tokens.push_back(*pool.allocate(Token::Type::Operator, "*"));
                    expectingValue = true;
                }
    
                continue;
            }
    
            // Handle operators (excluding '-')
            if (isOperator(c) && c != '-') {
                tokens.push_back(*pool.allocate(Token::Type::Operator, std::string(1, c)));
                expectingValue = true;
                remaining.remove_prefix(1);
    
                // If factorial `!` is followed by a number, variable, function, or open bracket
                if (c == '!' && !remaining.empty() && 
                    (std::isalpha(remaining.front()) || std::isdigit(remaining.front()) || 
                     remaining.front() == '(' || remaining.front() == '.')) {
                    tokens.push_back(*pool.allocate(Token::Type::Operator, "*"));
                    expectingValue = true;
                }
                continue;
            }
    
            // Handle brackets with implicit multiplication
            if (c == '(' || c == ')') {
                if (c == '(' && !tokens.empty() && !expectingValue &&
                    (tokens.back().getType() == Token::Type::Number || 
                     tokens.back().getType() == Token::Type::Constant || 
                     tokens.back().getType() == Token::Type::Variable || 
                     tokens.back().getType() == Token::Type::PrevResult || 
                     tokens.back().getValue() == "!" || 
                     tokens.back().getValue() == ")")) {
                    tokens.push_back(*pool.allocate(Token::Type::Operator, "*"));
                }
    
                tokens.push_back(*pool.allocate(Token::Type::Bracket, std::string(1, c)));
                expectingValue = (c == '(');
                remaining.remove_prefix(1);
                continue;
            }
    
            // Invalid character
            if (!std::isspace(c)) {
                throw CalcError("Invalid character in expression");
            }
            remaining.remove_prefix(1);
        }
    
        return tokens;
    }
    
    std::optional<Token> TokenProcessor::parseNumber(std::string_view& input) {
        size_t idx = 0;
        bool hasDecimal = false;

        while (idx < input.length() &&
               (std::isdigit(input[idx]) || (!hasDecimal && input[idx] == '.'))) {
            if (input[idx] == '.') hasDecimal = true;
            idx++;
        }

        if (idx == 0) {
            return std::nullopt;
        }

        std::string number(input.substr(0, idx));
        input.remove_prefix(idx);
        return *getTokenPool().allocate(Token::Type::Number, std::move(number));
    }

    void TokenProcessor::handleWord(std::string_view& input, std::vector<Token>& tokens) {
        size_t idx = 0;
        auto& pool = getTokenPool();
        
        while (idx < input.length() && (std::isalnum(input[idx]) || input[idx] == '_')) {
            idx++;
        }

        std::string word(input.substr(0, idx));
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);

        Token::Type type;
        
        // Check for commands first
        if (word == "def" || word == "del" || word == "upd" || word == "ls") {
            type = Token::Type::Command;
        }
        // Check if it's a function-related command
        else if (word == "create" || word == "use") {
            type = Token::Type::Command;
        }
        else if (word == "pi" || word == "e" || word == "phi" || word == "sqrt2") {
            type = Token::Type::Constant;
        }
        else if (word == "ans") {
            type = Token::Type::PrevResult;
        }
        else if (word == "sin" || word == "cos" || word == "tan" || 
                 word == "log" || word == "ln" || word == "sqrt") {
            type = Token::Type::MathFunction;
        }
        else if (word == "true" || word == "false") {
            type = Token::Type::Boolean;
        }
        else {
            type = Token::Type::Variable;
        }

        tokens.push_back(*pool.allocate(type, std::move(word)));
        input.remove_prefix(idx);
    }

    bool TokenProcessor::isOperator(char c) {
        return c == '+' || c == '-' || c == '*' ||
               c == '/' || c == '!' || c == '%' || c == '^';
    }
}