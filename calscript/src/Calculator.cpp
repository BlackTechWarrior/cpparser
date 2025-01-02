#include "Calculator.hpp"
#include "TokenProcessor.hpp"
#include "Constants.hpp"
#include <sstream>
#include <algorithm>
#include <stack>
#include <cmath>
#include <iostream>
#include <string>
#include <cctype>

namespace calc {
    Calculator::Calculator() {
        setupCommands();
    }

    void Calculator::setupCommands() {
        commands_ = {
            {"def", [this](const auto& args){handleDefine(args);}},
            {"del", [this](const auto& args){handleDelete(args);}},
            {"upd", [this](const auto& args){handleUpdate(args);}},
            {"ls", [this](const auto& args){handleList(args);}},
        };
    }

    void Calculator::processInput(std::string_view input) {
        if (input.empty()) return;

        try {
            auto tokens = TokenProcessor::tokenize(input);
            if(tokens.empty()) return;

            if(tokens[0].getType() == Token::Type::Command) {
                std::vector<std::string> args;
                
                for(size_t i = 1; i < tokens.size(); i++) {
                    args.push_back(tokens[i].getValue());
                }

                handleCommand(tokens[0].getValue(), args);
                addToHistory(input);
            } else {
                double result = evaluateExpression(tokens);
                lastResult_ = result;
                std::cout << "= " << result << std::endl;
                addToHistory(input, result);
            }
        } catch (const CalcError& e) {
            std::cout << "Error: " << e.what() << std::endl;
            addToHistory(input);
        }
    }

    void Calculator::handleCommand(std::string_view cmd, const std::vector<std::string>& args) {
        auto it = commands_.find(std::string(cmd));
        if (it != commands_.end()) {
            it->second(args);
        } else {
            throw CalcError("Unknown command");
        }
    }

    bool isValidVariableName(const std::string& name){
        if(name.empty() || isdigit(name[0])) return false;

        return std::all_of(name.begin(), name.end(),  
                           [](char c){return isalnum(c) || c == '_';});
    }

    void Calculator::handleDefine(const std::vector<std::string>& args) {
        if (args.size() != 2) {
            throw CalcError("Usage: def <variable> <value>");
        }

        if(commands_.find(args[0]) != commands_.end()){
            throw CalcError("Cannot use command name as variable name");
        }

        if(!isValidVariableName(args[0])){
            throw CalcError("Invalid variable Name. Must start with letter/underscore and only contain letters, numbers, underscores");
        }

        double value;
        if (args[1] == "ans") {
            value = lastResult_;
        } else {
            try {
                value = std::stod(args[1]);
            } catch (...) {
                throw CalcError("Invalid value for variable defination");
            }
        }

        defineVariable(args[0], value);
        std::cout << "Defined " << args[0] << " = " << value << std::endl;
    }

    void Calculator::handleDelete(const std::vector<std::string>& args) {
        if (args.empty() || args.size() > 2) {
            throw CalcError("Usage: del <variable|hist>");
        }

        if (args[0] == "hist") {
            clearHistory(); 
            std::cout << "History cleared" << std::endl;
        } 
        else if(args[0] == "vars"){
            deleteAllVariables();
            std::cout << "Variables cleared" << std::endl;
        }
        else {
            deleteVariable(args[0]);
            std::cout << "Deleted variable " << args[0] << std::endl;
        }
    }

    void Calculator::handleUpdate(const std::vector<std::string>& args) {
        if (args.size() != 2) {
            throw CalcError("Usage: upd <variable> <value>"); 
        }

        double value; 
        if (args[1] == "ans") {
            value = lastResult_;
        } else {
            try {
                value = std::stod(args[1]);
            } catch (...) {
                throw CalcError("Invalid value for variable update");
            }
        }

        updateVariable(args[0], value);
        std::cout << "Updated " << args[0] << " = " << value << std::endl;
    }

    void Calculator::handleList(const std::vector<std::string>& args) {
        if (args.empty() || args.size() > 2) {
            throw CalcError("Usage: ls <vars|hist>");
        }

        if (args[0] == "vars") {
            std::cout << "Variables:" << std::endl;
            for (const auto& [name, value] : variables_) {
                std::cout << name << " = " << value << std::endl;
            }
        } else if (args[0] == "hist") {
            std::cout << "History:" << std::endl;
            for (const auto& entry : history_) {
                std::cout << entry.input;
                if(entry.result) {
                    std::cout << " = " << *entry.result;
                }
                std::cout << std::endl;
            }
        } else {
            throw CalcError("Invalid list command. Use 'vars' or 'hist'");
        }
    }

    void Calculator::defineVariable(std::string_view name, double value) {
        variables_.emplace(std::string(name), value);
    }

    void Calculator::deleteVariable(std::string_view name) {
        if(variables_.erase(std::string(name)) == 0) {
            throw CalcError("Variable not found");
        }
    }

    void Calculator::deleteAllVariables() {
        variables_.clear();
    }

    void Calculator::updateVariable(std::string_view name, double value) {
        auto it = variables_.find(std::string(name));
        if (it == variables_.end()) {
            throw CalcError("Variable not found");
        }
        it->second = value;
    }

    void Calculator::addToHistory(std::string_view input, std::optional<double> result) {
        history_.push_back({
            std::string(input), 
            result,
            std::chrono::system_clock::now()
        });

        if(history_.size() > Constants::MAX_HISTORY) history_.pop_front();
    }

    void Calculator::clearHistory() { 
        history_.clear(); 
    }

    double Calculator::evaluateMathFunction(const std::string& func, double arg) {
        constexpr double DEG_TO_RAD = Constants::PI / 180.0; 
        constexpr double EPSILON = 1e-10;
        if (func == "sin") {
            double result = std::sin(arg * DEG_TO_RAD);
            if(std::abs(result) < EPSILON) return 0;
            if(std::abs(result - 1) < EPSILON) return 1;
            if(std::abs(result + 1) < EPSILON) return -1;
            return result; 
        }
        if (func == "cos") {
            double result = std::cos(arg * DEG_TO_RAD);
            if(std::abs(result) < EPSILON) return 0;
            if(std::abs(result - 1) < EPSILON) return 1;
            if(std::abs(result + 1) < EPSILON) return -1;
            return result; 
        }
        if (func == "tan") {
            if(std::fmod(std::abs(arg), 180) == 90) {
                throw CalcError("Tangent undefined at 90 (and its odd multiples)");
            }
            return std::tan(arg * DEG_TO_RAD);
        }
        if (func == "log") return std::log10(arg);
        if (func == "ln") return std::log(arg);
        if (func == "sqrt") {
            if (arg < 0) throw CalcError("Square root of negative number");
            return std::sqrt(arg);
        }
        throw CalcError("Unknown function: " + func);
    }

    double Calculator::evaluateExpression(const std::vector<Token>& tokens) {
        std::stack<double> values;
        std::stack<std::string> operators;

        auto precedence = [](const std::string& op) -> int {
            if(op == "sin" || op == "cos" || op == "tan" || 
               op == "log" || op == "ln" || op == "sqrt") return 5;
            if(op == "!") return 4;
            if(op == "^") return 3;
            if(op == "*" || op == "/" || op == "%") return 2;
            if(op == "+" || op == "-") return 1;
            return 0;
        };

        auto applyOperator = [](const std::string& op, double a, double b) -> double {
            if(op == "+") return a + b;
            if(op == "-") return a - b;
            if(op == "*") return a * b;
            if(op == "/") {
                if (b == 0) throw CalcError("Division by zero");
                return a / b;
            }
            if(op == "^") return std::pow(a, b);
            if(op == "%") {
                if (b == 0) throw CalcError("Modulo by zero");
                if (std::floor(a) != a || std::floor(b) != b) throw CalcError("Modulo requires integer operands");
                return std::fmod(a, b); 
            }
            throw CalcError("Unknown Operator: " + op);
        };

        auto applyUnary = [](const std::string& op, double a) -> double {
            if(op == "!") {
                if (a < 0 || std::floor(a) != a) throw CalcError("Factorial requires non-negative integer");
                double result = 1;
                for (int i = 2; i <= a; ++i) result *= i;
                return result;
            }
            throw CalcError("Unknown unary operator: " + op);
        };

        for (size_t i = 0; i < tokens.size(); i++) {
            const auto& token = tokens[i];
            switch(token.getType()) {
                case Token::Type::Number: {
                    values.push(std::stod(token.getValue()));
                    break;
                }

                case Token::Type::PrevResult: {
                    values.push(lastResult_);
                    break;
                }

                case Token::Type::MathFunction: {
                    operators.push(token.getValue());
                    break;
                }

                case Token::Type::Variable: {
                    auto it = variables_.find(token.getValue());
                    if (it == variables_.end()) throw CalcError("Undefined variable: " + token.getValue());
                    values.push(it->second);
                    break;
                }

                case Token::Type::Constant: {
                    if(token.getValue() == "pi") values.push(Constants::PI);
                    else if(token.getValue() == "e") values.push(Constants::E);
                    else if(token.getValue() == "phi") values.push(Constants::PHI);
                    else if(token.getValue() == "sqrt2") values.push(Constants::SQRT2);
                    else throw CalcError("Unknown constant: " + token.getValue());
                    break;
                }

                case Token::Type::Operator: {
                    const auto& op = token.getValue();

                    while(!operators.empty() && operators.top() != "(" &&
                          precedence(operators.top()) >= precedence(op)) {
                        const auto& curr_op = operators.top();
                        operators.pop();

                        if (curr_op == "!") {
                            if (values.empty()) throw CalcError("Invalid expression");
                            double val = values.top();
                            values.pop();
                            values.push(applyUnary(curr_op, val));
                        } else if (isMathFunction(curr_op)) {
                            if (values.empty()) throw CalcError("Function requires an argument");
                            double arg = values.top();
                            values.pop();
                            values.push(evaluateMathFunction(curr_op, arg));
                        } else {
                            if(values.size() < 2) throw CalcError("Invalid expression");
                            double b = values.top(); values.pop();
                            double a = values.top(); values.pop();
                            values.push(applyOperator(curr_op, a, b));
                        }
                    } 
                    operators.push(op);
                    break;
                }

                case Token::Type::Bracket: {
                    if(token.getValue() == "(") {
                        operators.push("(");
                    } else {
                        bool hadMathFunction = false;
                        
                        while(!operators.empty() && operators.top() != "(") {
                            const auto& op = operators.top();
                            operators.pop();
                            
                            if(isMathFunction(op)) {
                                if (values.empty()) throw CalcError("Function requires an argument");
                                double arg = values.top();
                                values.pop();
                                values.push(evaluateMathFunction(op, arg));
                                hadMathFunction = true;
                            }
                            else if(op == "!") {
                                if (values.empty()) throw CalcError("Invalid Expression");
                                double val = values.top();
                                values.pop();
                                values.push(applyUnary(op, val));
                            } else {
                                if (values.size() < 2) throw CalcError("Invalid Expression");
                                double b = values.top(); values.pop();
                                double a = values.top(); values.pop();
                                values.push(applyOperator(op, a, b));
                            }
                        } 
                        
                        if(operators.empty()) throw CalcError("Mismatched parenthesis");
                        operators.pop(); // remove opening bracket
                    }
                    break;
                }

                default: 
                    throw CalcError("Invalid token in expression");
            }
        }

        while(!operators.empty()) {
            const auto& op = operators.top();
            operators.pop();

            if(op == "(") throw CalcError("Mismatched parenthesis");

            if(isMathFunction(op)) {
                if (values.empty()) throw CalcError("Function requires an argument");
                double arg = values.top();
                values.pop();
                values.push(evaluateMathFunction(op, arg));
            }
            else if(op == "!") {
                if (values.empty()) throw CalcError("Invalid Expression");
                double val = values.top();
                values.pop();
                values.push(applyUnary(op, val));
            } else {
                if(values.size() < 2) throw CalcError("Invalid expression");
                double b = values.top(); values.pop();
                double a = values.top(); values.pop();
                values.push(applyOperator(op, a, b));
            }
        }

        if(values.empty()) throw CalcError("Empty expression");
        if(values.size() > 1) throw CalcError("Invalid expression");
        return values.top();
    }
}
