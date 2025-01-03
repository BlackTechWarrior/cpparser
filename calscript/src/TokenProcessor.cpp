#include "TokenProcessor.hpp"
#include "Constants.hpp"
#include <cctype>
#include <algorithm>

namespace calc {
    std::vector<Token> TokenProcessor::tokenize(std::string_view expression) {
    std::vector<Token> tokens;
    tokens.reserve(expression.length() / 2);

    std::string_view remaining = expression;
    while (!remaining.empty()) {
        char c = remaining.front();

        // Skip whitespace
        if (std::isspace(c)) {
            remaining.remove_prefix(1);
            continue;
        }

        // Handle neg numbers
        if(c == '-' && remaining.length() > 1 && 
           (std::isdigit(remaining[1]) || remaining[1] == '.') &&
           (tokens.empty() || tokens.back().getType() == Token::Type::Operator ||
           (tokens.back().getType() == Token::Type::Bracket && tokens.back().getValue() == "("))) {
            remaining.remove_prefix(1);
            if (auto numToken = parseNumber(remaining)) {
                tokens.emplace_back(Token::Type::Number, "-" + numToken->getValue());
                continue;
            }
            remaining = std::string_view(remaining.data() - 1, remaining.length() + 1);
        }

        // Handle pos numbers
        if (auto numToken = parseNumber(remaining)) {
            if (!tokens.empty() && tokens.back().getType() == Token::Type::Bracket && 
                tokens.back().getValue() == ")") {
                tokens.emplace_back(Token::Type::Operator, "*");  
            }
            tokens.emplace_back(std::move(*numToken));
            continue;
        }

        // Handle words (functions, constants, variables)
        if(std::isalpha(c)) {
            if (!tokens.empty() && tokens.back().getType() == Token::Type::Bracket && 
                tokens.back().getValue() == ")") {
                tokens.emplace_back(Token::Type::Operator, "*");
            }
            handleWord(remaining, tokens);
            continue;
        }

        // Handle operators
        if(isOperator(c)) {
            // Always allow operators after closing parentheses, constants, numbers
            if(c == '-' && remaining.length() > 1 && remaining[1] == '(' &&
               (tokens.empty() || tokens.back().getType() == Token::Type::Operator ||
               (tokens.back().getType() == Token::Type::Bracket && tokens.back().getValue() == "("))) {
                tokens.emplace_back(Token::Type::Number, "-1");
                tokens.emplace_back(Token::Type::Operator, "*");
                remaining.remove_prefix(1);
                continue;
            }
            tokens.emplace_back(Token::Type::Operator, std::string(1,c));
            remaining.remove_prefix(1);
            continue;
        } 

        // Handle brackets
        if(c == '(' || c == ')') {  
            if (!tokens.empty() && c == '(' && 
               (tokens.back().getType() == Token::Type::Number || 
                tokens.back().getType() == Token::Type::Constant || 
                tokens.back().getType() == Token::Type::Variable ||
                tokens.back().getType() == Token::Type::PrevResult ||
                (tokens.back().getType() == Token::Type::Bracket && 
                 tokens.back().getValue() == ")")
               )) {
                tokens.emplace_back(Token::Type::Operator, "*");
            }
            tokens.emplace_back(Token::Type::Bracket, std::string(1,c));
            remaining.remove_prefix(1);
            continue;
        }

        // Skip whitespace at the end
        if (!std::isspace(c)) {
            throw CalcError("Invalid character in Expression");
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
        return Token(Token::Type::Number, std::move(number));
    }

    void TokenProcessor::handleWord(std::string_view& input, std::vector<Token>& tokens) {
        size_t idx = 0;
        
        while (idx < input.length() && std::isalnum(input[idx])) {
            idx++;
        }

        std::string word(input.substr(0, idx));
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);

        Token::Type type;
        if (word == "pi" || word == "e" || word == "phi" || word == "sqrt2") 
            type = Token::Type::Constant;
        else if(word == "ans") 
            type = Token::Type::PrevResult;
        else if (word == "def" || word == "del" || word == "upd" || word == "ls") 
            type = Token::Type::Command;
        else if (word == "sin" || word == "cos" || word == "tan" || 
                 word == "log" || word == "ln" || word == "sqrt") 
            type = Token::Type::MathFunction;
        else 
            type = Token::Type::Variable;

        tokens.emplace_back(type, std::move(word));
        input.remove_prefix(idx);
    }

    bool TokenProcessor::isOperator(char c) {
        return c == '+' || c == '-' || c == '*' ||
               c == '/' || c == '!' || c == '%' || c == '^';
    }
}
