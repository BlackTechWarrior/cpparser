#pragma once 
#include <string>
#include <variant>
#include <stdexcept>

namespace calc {
    class Token {
        public: 
            enum class Type {
                Number, 
                Operator, 
                Bracket, 
                Constant, 
                Variable, 
                Command, 
                PrevResult,
                MathFunction, 
                Boolean,
                Comma,         // For function parameters
                Colon          // For function definition
            };

            Token (Type t, std::string v) : type_(t), value_(std::move(v)) {}
            Type getType() const {return type_;}
            const std::string& getValue() const {return value_;}

        private:
            Type type_;
            std::string value_;
    };

    class CalcError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}