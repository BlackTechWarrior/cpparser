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
    // Free function in the namespace
    bool isValidVariableName(const std::string& name) {
        if (name.empty() || isdigit(name[0])) return false;

        return std::all_of(name.begin(), name.end(),
                        [](char c){return isalnum(c) || c == '_';});
    }

    // Debug helper to print tokens
    void printTokens(const std::vector<Token>& tokens) {
        std::cout << "Tokens: ";
        for (const auto& token : tokens) {
            std::cout << token.getValue() << "(" << static_cast<int>(token.getType()) << ") ";
        }
        std::cout << std::endl;
    }

    Calculator::Calculator() {
        setupCommands();
    }

    void Calculator::setupCommands() {
        commands_ = {
            {"del", [this](const auto& args) { handleDelete(args); }},
            {"upd", [this](const auto& args) {
                if (args.size() < 2) {
                    throw CalcError("Usage: upd <variable> <expression>");
                }

                std::string valueExpr = args[1];

                for (size_t i = 2; i < args.size(); ++i) {
                    valueExpr += " " + args[i];
                }

                handleUpdate(args[0], valueExpr);
            }},
            {"ls", [this](const auto& args) { handleList(args); }},
            {"create", [this](const auto& args) {
                if (args.empty()) {
                    throw CalcError("Usage: create func <n>(params...): body");
                }

                if (args[0] == "func") {
                    // This is handled directly in processInput
                    throw CalcError("Function creation syntax: create func name(params...): body");
                } else {
                    throw CalcError("Unknown create command. Use 'create func'");
                }
            }},
            {"use", [this](const auto& args) {
                if (args.empty()) {
                    throw CalcError("Usage: use func <n>(args...)");
                }

                if (args[0] == "func") {
                    // This is handled directly in processInput
                    throw CalcError("Function call syntax: use func name(args...)");
                } else {
                    throw CalcError("Unknown use command. Use 'use func'");
                }
            }}
        };
    }

    void Calculator::processInput(std::string_view input) {
        if (input.empty()) return;

        try {
            // Debug command to dump defined functions and their bodies
            if (input == "debug funcs") {
                std::cout << "Defined functions:" << std::endl;
                for (const auto& [name, func] : functions_) {
                    std::cout << name << "(";
                    const auto& params = func.getParameters();
                    for (size_t i = 0; i < params.size(); ++i) {
                        if (i > 0) std::cout << ", ";
                        std::cout << params[i];
                    }
                    std::cout << "): ";
                    printTokens(func.getBody());
                }
                addToHistory(input);
                return;
            }

            // Check for function commands
            if (input.length() >= 12 && std::string(input.substr(0, 12)) == "create func ") {
                std::string fullCmd = std::string(input);

                // Parse the function definition
                size_t nameStart = 12; // after "create func "
                size_t nameEnd = fullCmd.find('(', nameStart);

                if (nameEnd == std::string::npos) {
                    throw CalcError("Invalid function syntax. Expected '(' after function name.");
                }

                std::string funcName = fullCmd.substr(nameStart, nameEnd - nameStart);
                funcName.erase(0, funcName.find_first_not_of(" \t"));
                funcName.erase(funcName.find_last_not_of(" \t") + 1);

                // Find the closing parenthesis
                size_t paramsEnd = fullCmd.find(')', nameEnd);
                if (paramsEnd == std::string::npos) {
                    throw CalcError("Invalid function syntax. Missing ')' after parameters.");
                }

                // Extract parameters
                std::string paramsStr = fullCmd.substr(nameEnd + 1, paramsEnd - nameEnd - 1);
                std::vector<std::string> params;

                if (!paramsStr.empty()) {
                    size_t pos = 0;
                    std::string paramToken = paramsStr;
                    while ((pos = paramToken.find(',')) != std::string::npos) {
                        std::string param = paramToken.substr(0, pos);
                        param.erase(0, param.find_first_not_of(" \t"));
                        param.erase(param.find_last_not_of(" \t") + 1);
                        params.push_back(param);
                        paramToken.erase(0, pos + 1);
                    }

                    // Add the last parameter
                    paramToken.erase(0, paramToken.find_first_not_of(" \t"));
                    paramToken.erase(paramToken.find_last_not_of(" \t") + 1);
                    if (!paramToken.empty()) {
                        params.push_back(paramToken);
                    }
                }

                // Check for colon
                size_t colonPos = fullCmd.find(':', paramsEnd);
                if (colonPos == std::string::npos) {
                    throw CalcError("Invalid function syntax. Expected ':' after parameters.");
                }

                // Extract body
                std::string body = fullCmd.substr(colonPos + 1);
                body.erase(0, body.find_first_not_of(" \t"));

                if (body.empty()) {
                    throw CalcError("Function body cannot be empty.");
                }

                // Tokenize the body
                auto bodyTokens = TokenProcessor::tokenize(body);

                // Create the function
                defineFunction(funcName, params, bodyTokens);
                std::cout << "Defined function " << funcName << "(";
                for (size_t i = 0; i < params.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << params[i];
                }
                std::cout << ")" << std::endl;

                addToHistory(input);
                return;
            }

            // Check for function call with "use func" prefix
            if (input.length() >= 9 && std::string(input.substr(0, 9)) == "use func ") {
                std::string fullCmd = std::string(input);

                // Parse the function call
                size_t nameStart = 9; // after "use func "
                size_t nameEnd = fullCmd.find('(', nameStart);

                if (nameEnd == std::string::npos) {
                    throw CalcError("Invalid function call syntax. Expected '(' after function name.");
                }

                std::string funcName = fullCmd.substr(nameStart, nameEnd - nameStart);
                funcName.erase(0, funcName.find_first_not_of(" \t"));
                funcName.erase(funcName.find_last_not_of(" \t") + 1);

                // Find the matching closing parenthesis
                size_t argsEnd = nameEnd + 1;
                int parenCount = 1;

                while (parenCount > 0 && argsEnd < fullCmd.length()) {
                    if (fullCmd[argsEnd] == '(') {
                        parenCount++;
                    } else if (fullCmd[argsEnd] == ')') {
                        parenCount--;
                    }
                    argsEnd++;
                }

                if (parenCount > 0) {
                    throw CalcError("Invalid function call syntax. Missing closing parenthesis.");
                }

                // Extract and parse arguments
                std::string argsStr = fullCmd.substr(nameEnd + 1, argsEnd - nameEnd - 2);
                std::vector<double> argValues;

                if (!argsStr.empty()) {
                    // Split by commas, but respect nested parentheses
                    std::vector<std::string> argExpressions;
                    int nestedParens = 0;
                    size_t start = 0;

                    for (size_t i = 0; i < argsStr.length(); i++) {
                        if (argsStr[i] == '(') {
                            nestedParens++;
                        } else if (argsStr[i] == ')') {
                            nestedParens--;
                        } else if (argsStr[i] == ',' && nestedParens == 0) {
                            // Found a top-level comma
                            std::string arg = argsStr.substr(start, i - start);
                            arg.erase(0, arg.find_first_not_of(" \t"));
                            arg.erase(arg.find_last_not_of(" \t") + 1);
                            argExpressions.push_back(arg);
                            start = i + 1;
                        }
                    }

                    // Add the last argument
                    std::string lastArg = argsStr.substr(start);
                    lastArg.erase(0, lastArg.find_first_not_of(" \t"));
                    lastArg.erase(lastArg.find_last_not_of(" \t") + 1);
                    if (!lastArg.empty()) {
                        argExpressions.push_back(lastArg);
                    }

                    // Evaluate each argument
                    for (const auto& argExpr : argExpressions) {
                        if (argExpr.empty()) continue;

                        // Handle simple number case
                        if (argExpr.find_first_not_of("-0123456789.") == std::string::npos) {
                            argValues.push_back(std::stod(argExpr));
                        }
                        // Handle simple variable case
                        else if (argExpr.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") == std::string::npos) {
                            // Check for constants first
                            if (argExpr == "pi") {
                                argValues.push_back(Constants::PI);
                            } else if (argExpr == "e") {
                                argValues.push_back(Constants::E);
                            } else if (argExpr == "phi") {
                                argValues.push_back(Constants::PHI);
                            } else if (argExpr == "sqrt2") {
                                argValues.push_back(Constants::SQRT2);
                            } else if (argExpr == "ans") {
                                argValues.push_back(lastResult_);
                            } else {
                                auto it = variables_.find(argExpr);
                                if (it == variables_.end()) {
                                    throw CalcError("Undefined variable in function argument: " + argExpr);
                                }
                                argValues.push_back(it->second);
                            }
                        }
                        // Evaluate the expression
                        else {
                            auto tokens = TokenProcessor::tokenize(argExpr);
                            argValues.push_back(evaluateExpression(tokens));
                        }
                    }
                }

                // Call the function
                double result = callFunction(funcName, argValues);
                lastResult_ = result;
                std::cout << "= " << result << std::endl;

                addToHistory(input, result);
                return;
            }

            // Check for direct function call (without "use func" prefix)
            // Look for function name followed by opening parenthesis
            std::string fullCmd = std::string(input);
            size_t openParenPos = fullCmd.find('(');

            if (openParenPos != std::string::npos && openParenPos > 0) {
                // Extract potential function name
                std::string potentialFuncName = fullCmd.substr(0, openParenPos);
                potentialFuncName.erase(0, potentialFuncName.find_first_not_of(" \t"));
                potentialFuncName.erase(potentialFuncName.find_last_not_of(" \t") + 1);

                // Check if this is a defined function
                if (functionExists(potentialFuncName)) {
                    // Find the matching closing parenthesis
                    size_t argsEnd = openParenPos + 1;
                    int parenCount = 1;

                    while (parenCount > 0 && argsEnd < fullCmd.length()) {
                        if (fullCmd[argsEnd] == '(') {
                            parenCount++;
                        } else if (fullCmd[argsEnd] == ')') {
                            parenCount--;
                        }
                        argsEnd++;
                    }

                    if (parenCount > 0) {
                        throw CalcError("Invalid function call syntax. Missing closing parenthesis.");
                    }

                    // Check if there's anything after the function call
                    // If there is, we need to treat this as a complex expression
                    if (argsEnd < fullCmd.length()) {
                        // There's more after the function call, so it's part of a larger expression
                        // Tokenize the entire input and use evaluateExpression
                        auto tokens = TokenProcessor::tokenize(input);
                        double result = evaluateExpression(tokens);
                        lastResult_ = result;
                        std::cout << "= " << result << std::endl;
                        addToHistory(input, result);
                        return;
                    }

                    // Extract arguments
                    std::string argsStr = fullCmd.substr(openParenPos + 1, argsEnd - openParenPos - 2);
                    std::vector<double> argValues;

                    if (!argsStr.empty()) {
                        // Split by commas, but respect nested parentheses
                        std::vector<std::string> argExpressions;
                        int nestedParens = 0;
                        size_t start = 0;

                        for (size_t i = 0; i < argsStr.length(); i++) {
                            if (argsStr[i] == '(') {
                                nestedParens++;
                            } else if (argsStr[i] == ')') {
                                nestedParens--;
                            } else if (argsStr[i] == ',' && nestedParens == 0) {
                                // Found a top-level comma
                                std::string arg = argsStr.substr(start, i - start);
                                arg.erase(0, arg.find_first_not_of(" \t"));
                                arg.erase(arg.find_last_not_of(" \t") + 1);
                                argExpressions.push_back(arg);
                                start = i + 1;
                            }
                        }

                        // Add the last argument
                        std::string lastArg = argsStr.substr(start);
                        lastArg.erase(0, lastArg.find_first_not_of(" \t"));
                        lastArg.erase(lastArg.find_last_not_of(" \t") + 1);
                        if (!lastArg.empty()) {
                            argExpressions.push_back(lastArg);
                        }

                        // Evaluate each argument
                        for (const auto& argExpr : argExpressions) {
                            if (argExpr.empty()) continue;

                            // Handle simple number case
                            if (argExpr.find_first_not_of("-0123456789.") == std::string::npos) {
                                argValues.push_back(std::stod(argExpr));
                            }
                            // Handle simple variable case
                            else if (argExpr.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") == std::string::npos) {
                                // Check for constants first
                                if (argExpr == "pi") {
                                    argValues.push_back(Constants::PI);
                                } else if (argExpr == "e") {
                                    argValues.push_back(Constants::E);
                                } else if (argExpr == "phi") {
                                    argValues.push_back(Constants::PHI);
                                } else if (argExpr == "sqrt2") {
                                    argValues.push_back(Constants::SQRT2);
                                } else if (argExpr == "ans") {
                                    argValues.push_back(lastResult_);
                                } else {
                                    auto it = variables_.find(argExpr);
                                    if (it == variables_.end()) {
                                        throw CalcError("Undefined variable in function argument: " + argExpr);
                                    }
                                    argValues.push_back(it->second);
                                }
                            }
                            // Evaluate the expression
                            else {
                                auto tokens = TokenProcessor::tokenize(argExpr);
                                argValues.push_back(evaluateExpression(tokens));
                            }
                        }
                    }

                    // Call the function
                    double result = callFunction(potentialFuncName, argValues);
                    lastResult_ = result;
                    std::cout << "= " << result << std::endl;

                    addToHistory(input, result);
                    return;
                }
            }

            // Check for direct def command with raw string splitting to handle spaces correctly
            if (input.length() >= 4 && std::string(input.substr(0, 4)) == "def ") {
                std::string fullCmd = std::string(input);

                // Skip the "def " prefix
                size_t cmdEnd = 4;
                // Skip any whitespace after "def "
                while (cmdEnd < fullCmd.length() && std::isspace(fullCmd[cmdEnd])) {
                    cmdEnd++;
                }

                // Find the variable name (until the next whitespace)
                size_t varStart = cmdEnd;
                size_t varEnd = fullCmd.find_first_of(" \t", varStart);

                if (varEnd == std::string::npos) {
                    throw CalcError("Usage: def <variable> <value>");
                }

                std::string varName = fullCmd.substr(varStart, varEnd - varStart);

                // Skip whitespace after the variable name
                size_t exprStart = varEnd;
                while (exprStart < fullCmd.length() && std::isspace(fullCmd[exprStart])) {
                    exprStart++;
                }

                // The rest is the expression
                std::string valueExpr = fullCmd.substr(exprStart);

                if (valueExpr.empty()) {
                    throw CalcError("Variable definition requires a value.");
                }

                handleDefine(varName, valueExpr);
                addToHistory(input);
                return;
            }

            // Check for direct upd command with raw string splitting
            if (input.length() >= 4 && std::string(input.substr(0, 4)) == "upd ") {
                std::string fullCmd = std::string(input);

                // Skip the "upd " prefix
                size_t cmdEnd = 4;
                // Skip any whitespace after "upd "
                while (cmdEnd < fullCmd.length() && std::isspace(fullCmd[cmdEnd])) {
                    cmdEnd++;
                }

                // Find the variable name (until the next whitespace)
                size_t varStart = cmdEnd;
                size_t varEnd = fullCmd.find_first_of(" \t", varStart);

                if (varEnd == std::string::npos) {
                    throw CalcError("Usage: upd <variable> <value>");
                }

                std::string varName = fullCmd.substr(varStart, varEnd - varStart);

                // Skip whitespace after the variable name
                size_t exprStart = varEnd;
                while (exprStart < fullCmd.length() && std::isspace(fullCmd[exprStart])) {
                    exprStart++;
                }

                // The rest is the expression
                std::string valueExpr = fullCmd.substr(exprStart);

                if (valueExpr.empty()) {
                    throw CalcError("Variable update requires a value.");
                }

                handleUpdate(varName, valueExpr);
                addToHistory(input);
                return;
            }

            // Normal expression or command
            auto tokens = TokenProcessor::tokenize(input);
            if (tokens.empty()) return;

            if (tokens[0].getType() == Token::Type::Command) {
                const std::string& cmd = tokens[0].getValue();

                // Other commands (not def or upd which are handled above)
                std::vector<std::string> args;
                for (size_t i = 1; i < tokens.size(); i++) {
                    args.push_back(tokens[i].getValue());
                }
                handleCommand(cmd, args);
                addToHistory(input);
            }
            else {
                try {
                    double result = evaluateExpression(tokens);
                    lastResult_ = result;
                    std::cout << "= " << result << std::endl;
                    addToHistory(input, result);
                } catch (const CalcError& e) {
                    throw; // Just rethrow the error
                } catch (const std::exception& e) {
                    throw CalcError(e.what());
                }
            }
        }
        catch (const CalcError& e) {
            std::cout << "Error: " << e.what() << std::endl;
            addToHistory(input);
        }
    }

    void Calculator::handleDefine(const std::string& varName, const std::string& valueExpr) {
        if (!isValidVariableName(varName)) {
            throw CalcError("Invalid variable name. Must start with a letter and contain only letters, numbers, or underscores.");
        }

        // Check if the name is a constant
        if (varName == "pi" || varName == "e" || varName == "phi" || varName == "sqrt2" || varName == "ans") {
            throw CalcError("Cannot use constant '" + varName + "' as a variable name.");
        }

        // Check if the name is a math function
        if (varName == "sin" || varName == "cos" || varName == "tan" ||
            varName == "log" || varName == "ln" || varName == "sqrt") {
            throw CalcError("Cannot use math function '" + varName + "' as a variable name.");
        }

        if (commands_.find(varName) != commands_.end() ||
            functions_.count(varName) > 0) {
            throw CalcError("Name '" + varName + "' is already used as a command or function name.");
        }

        if (variables_.count(varName) > 0) {
            throw CalcError("Variable already exists. Use 'upd' to modify it.");
        }

        if (valueExpr.empty()) {
            throw CalcError("Variable definition requires a value.");
        }

        try {
            // Handle direct number case (this covers both positive and negative)
            if (valueExpr.find_first_not_of("+-0123456789.") == std::string::npos) {
                double value = std::stod(valueExpr);
                defineVariable(varName, value);
                std::cout << "Defined " << varName << " = " << value << std::endl;
                return;
            }

            // Handle direct variable reference (no spaces or operators)
            if (valueExpr.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") == std::string::npos) {
                // Check if it's a constant
                if (valueExpr == "pi") {
                    defineVariable(varName, Constants::PI);
                    std::cout << "Defined " << varName << " = " << Constants::PI << std::endl;
                    return;
                }
                else if (valueExpr == "e") {
                    defineVariable(varName, Constants::E);
                    std::cout << "Defined " << varName << " = " << Constants::E << std::endl;
                    return;
                }
                else if (valueExpr == "phi") {
                    defineVariable(varName, Constants::PHI);
                    std::cout << "Defined " << varName << " = " << Constants::PHI << std::endl;
                    return;
                }
                else if (valueExpr == "sqrt2") {
                    defineVariable(varName, Constants::SQRT2);
                    std::cout << "Defined " << varName << " = " << Constants::SQRT2 << std::endl;
                    return;
                }
                else if (valueExpr == "ans") {
                    defineVariable(varName, lastResult_);
                    std::cout << "Defined " << varName << " = " << lastResult_ << std::endl;
                    return;
                }

                // Check if it's a variable
                auto it = variables_.find(valueExpr);
                if (it != variables_.end()) {
                    defineVariable(varName, it->second);
                    std::cout << "Defined " << varName << " = " << it->second << std::endl;
                    return;
                } else {
                    throw CalcError("Undefined variable: " + valueExpr);
                }
            }

            // For complex expressions, use the tokenizer and evaluator
            auto tokens = TokenProcessor::tokenize(valueExpr);
            if (tokens.empty()) {
                throw CalcError("Empty expression");
            }

            double value = evaluateExpression(tokens);
            defineVariable(varName, value);
            std::cout << "Defined " << varName << " = " << value << std::endl;
        } catch (const std::exception& e) {
            throw CalcError("Invalid expression: " + std::string(e.what()));
        }
    }

    void Calculator::handleCommand(std::string_view cmd, const std::vector<std::string>& args) {
        if (cmd == "def" || cmd == "upd") {
            throw CalcError("Internal error: " + std::string(cmd) + " command should be handled separately");
        }

        auto it = commands_.find(std::string(cmd));
        if (it != commands_.end()) {
            it->second(args);
        } else {
            throw CalcError("Unknown command: " + std::string(cmd));
        }
    }

    void Calculator::handleUpdate(const std::string& varName, const std::string& valueExpr) {
        if (variables_.find(varName) == variables_.end()) {
            throw CalcError("Variable does not exist. Use 'def' to create it.");
        }

        if (valueExpr.empty()) {
            throw CalcError("Update requires a valid expression.");
        }

        try {
            // Handle direct number case (this covers both positive and negative)
            if (valueExpr.find_first_not_of("+-0123456789.") == std::string::npos) {
                double value = std::stod(valueExpr);
                updateVariable(varName, value);
                std::cout << "Updated " << varName << " = " << value << std::endl;
                return;
            }

            // Handle direct variable reference (no spaces or operators)
            if (valueExpr.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") == std::string::npos) {
                // Check if it's a constant
                if (valueExpr == "pi") {
                    updateVariable(varName, Constants::PI);
                    std::cout << "Updated " << varName << " = " << Constants::PI << std::endl;
                    return;
                }
                else if (valueExpr == "e") {
                    updateVariable(varName, Constants::E);
                    std::cout << "Updated " << varName << " = " << Constants::E << std::endl;
                    return;
                }
                else if (valueExpr == "phi") {
                    updateVariable(varName, Constants::PHI);
                    std::cout << "Updated " << varName << " = " << Constants::PHI << std::endl;
                    return;
                }
                else if (valueExpr == "sqrt2") {
                    updateVariable(varName, Constants::SQRT2);
                    std::cout << "Updated " << varName << " = " << Constants::SQRT2 << std::endl;
                    return;
                }
                else if (valueExpr == "ans") {
                    updateVariable(varName, lastResult_);
                    std::cout << "Updated " << varName << " = " << lastResult_ << std::endl;
                    return;
                }

                // Check if it's a variable
                auto it = variables_.find(valueExpr);
                if (it != variables_.end()) {
                    updateVariable(varName, it->second);
                    std::cout << "Updated " << varName << " = " << it->second << std::endl;
                    return;
                } else {
                    throw CalcError("Undefined variable: " + valueExpr);
                }
            }

            // For complex expressions, use the tokenizer and evaluator
            auto tokens = TokenProcessor::tokenize(valueExpr);
            if (tokens.empty()) {
                throw CalcError("Empty expression");
            }

            double value = evaluateExpression(tokens);
            updateVariable(varName, value);
            std::cout << "Updated " << varName << " = " << value << std::endl;
        } catch (const std::exception& e) {
            throw CalcError("Invalid expression: " + std::string(e.what()));
        }
    }

    void Calculator::handleDelete(const std::vector<std::string>& args) {
        if (args.empty()) {
            throw CalcError("Usage: del <variable|hist|func|vars>");
        }

        if (args[0] == "hist") {
            clearHistory();
            std::cout << "History cleared" << std::endl;
        }
        else if (args[0] == "vars") {
            deleteAllVariables();
            std::cout << "Variables cleared" << std::endl;
        }
        else if (args[0] == "func" && args.size() > 1) {
            deleteFunction(args[1]);
            std::cout << "Deleted function " << args[1] << std::endl;
        }
        else {
            deleteVariable(args[0]);
            std::cout << "Deleted variable " << args[0] << std::endl;
        }
    }


    void Calculator::handleList(const std::vector<std::string>& args) {
        if (args.empty()) {
            // If no args, show all categories
            std::cout << "Available categories: vars, hist, funcs" << std::endl;
            return;
        }

        if (args.size() > 2) {
            throw CalcError("Usage: ls <vars|hist|funcs>");
        }

        if (args[0] == "vars") {
            std::cout << "Variables:" << std::endl;
            if (variables_.empty()) {
                std::cout << "  No variables defined" << std::endl;
                return;
            }
            for (const auto& [name, value] : variables_) {
                std::cout << name << " = " << value << std::endl;
            }
        }
        else if (args[0] == "hist") {
            std::cout << "History:" << std::endl;
            if (history_.empty()) {
                std::cout << "  No history" << std::endl;
                return;
            }
            for (const auto& entry : history_) {
                std::cout << entry.input;
                if(entry.result) {
                    std::cout << " = " << *entry.result;
                }
                std::cout << std::endl;
            }
        }
        else if (args[0] == "funcs") {
            std::cout << "Functions:" << std::endl;
            if (functions_.empty()) {
                std::cout << "  No functions defined" << std::endl;
                return;
            }
            for (const auto& [name, func] : functions_) {
                std::cout << name << "(";
                const auto& params = func.getParameters();
                for (size_t i = 0; i < params.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << params[i];
                }
                std::cout << ")" << std::endl;
            }
        }
        else {
            throw CalcError("Invalid list command. Use 'vars', 'hist', or 'funcs'");
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

    // Function-related methods
    void Calculator::defineFunction(const std::string& name, const std::vector<std::string>& params, const std::vector<Token>& body) {
        if (!isValidVariableName(name)) {
            throw CalcError("Invalid function name. Must start with a letter and contain only letters, numbers, or underscores.");
        }

        // Check if name is a constant
        if (name == "pi" || name == "e" || name == "phi" || name == "sqrt2" || name == "ans") {
            throw CalcError("Cannot use constant '" + name + "' as a function name.");
        }

        // Check if name is a math function
        if (name == "sin" || name == "cos" || name == "tan" ||
            name == "log" || name == "ln" || name == "sqrt") {
            throw CalcError("Cannot use math function '" + name + "' as a function name.");
        }

        if (variables_.count(name) > 0 || functions_.count(name) > 0) {
            throw CalcError("Function/variable name '" + name + "' already exists.");
        }

        // Validate parameter names
        for (const auto& param : params) {
            if (!isValidVariableName(param)) {
                throw CalcError("Invalid parameter name: " + param);
            }

            // Check if parameter name is a reserved constant
            if (param == "pi" || param == "e" || param == "phi" || param == "sqrt2" || param == "ans") {
                throw CalcError("Cannot use constant '" + param + "' as a parameter name.");
            }
        }

        // Check for duplicate parameters
        std::unordered_map<std::string, bool> paramCheck;
        for (const auto& param : params) {
            if (paramCheck.count(param) > 0) {
                throw CalcError("Duplicate parameter name: " + param);
            }
            paramCheck[param] = true;
        }

        functions_.emplace(name, Function(name, params, body));
    }

    void Calculator::deleteFunction(const std::string& name) {
        if (functions_.erase(name) == 0) {
            throw CalcError("Function not found: " + name);
        }
    }

    bool Calculator::functionExists(const std::string& name) const {
        return functions_.count(name) > 0;
    }

    double Calculator::callFunction(const std::string& name, const std::vector<double>& args) {
        auto it = functions_.find(name);
        if (it == functions_.end()) {
            throw CalcError("Function not found: " + name);
        }

        const auto& func = it->second;
        const auto& params = func.getParameters();

        if (args.size() != params.size()) {
            throw CalcError("Function '" + name + "' expects " + std::to_string(params.size()) +
                            " parameters, but " + std::to_string(args.size()) + " were provided");
        }

        return evaluateWithLocalScope(func.getBody(), params, args);
    }

    double Calculator::evaluateWithLocalScope(const std::vector<Token>& body,
        const std::vector<std::string>& paramNames,
        const std::vector<double>& paramValues) {
    if (body.empty()) {
    throw CalcError("Empty function body");
    }

    // Save current variables
    auto savedVariables = variables_;  // Make a copy

    // Create local scope with parameter values
    for (size_t i = 0; i < paramNames.size(); ++i) {
    std::string paramName = paramNames[i];
    variables_[paramName] = paramValues[i];
    }

    // Evaluate the body with the local scope
    double result;
    try {
    result = evaluateExpression(body);
    } catch (const CalcError& e) {
    // Restore original variables before rethrowing
    variables_ = savedVariables;  // Use copy assignment instead of move
    throw;
    } catch (...) {
    // Restore original variables before rethrowing
    variables_ = savedVariables;  // Use copy assignment instead of move
    throw CalcError("Unknown error in function evaluation");
    }

    // Restore original variables
    variables_ = savedVariables;  // Use copy assignment instead of move

    return result;
    }

    void Calculator::handleFunctionDefinition(const std::vector<Token>& tokens) {
        // This is now handled directly in processInput
        throw CalcError("Function definition must be in the format: create func name(param1, param2, ...) : body");
    }

    void Calculator::handleFunctionCall(const std::vector<Token>& tokens) {
        // This is now handled directly in processInput
        throw CalcError("Function call must be in the format: use func name(arg1, arg2, ...)");
    }

    double Calculator::evaluateExpression(const std::vector<Token>& tokens) {
        if (tokens.empty()) {
            throw CalcError("Empty expression");
        }

        std::stack<double> values;
        std::stack<std::string> operators;

        auto precedence = [](const std::string& op) -> int {
            if (op == "neg") return 6;
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
            if (op == "neg") return -a;
            if(op == "!") {
                if (a < 0 || std::floor(a) != a) throw CalcError("Factorial requires non-negative integer");
                double result = 1;
                for (int i = 2; i <= a; ++i) result *= i;
                return result;
            }
            throw CalcError("Unknown unary operator: " + op);
        };

        // Process tokens
        for (size_t i = 0; i < tokens.size(); i++) {
            const auto& token = tokens[i];

            // First, handle function calls
            if (token.getType() == Token::Type::Variable &&
                i + 1 < tokens.size() &&
                tokens[i+1].getType() == Token::Type::Bracket &&
                tokens[i+1].getValue() == "(") {

                std::string funcName = token.getValue();

                if (functionExists(funcName)) {

                    // Find the matching closing parenthesis
                    int parenCount = 1;
                    size_t j = i + 2;  // Skip the variable token and opening parenthesis
                    size_t argStart = j;
                    std::vector<std::vector<Token>> argTokens;

                    while (j < tokens.size() && parenCount > 0) {
                        if (tokens[j].getType() == Token::Type::Bracket) {
                            if (tokens[j].getValue() == "(") {
                                parenCount++;
                            } else if (tokens[j].getValue() == ")") {
                                parenCount--;

                                if (parenCount == 0) {
                                    // End of function call, add the last argument if there's one
                                    if (j > argStart) {
                                        std::vector<Token> arg(tokens.begin() + argStart, tokens.begin() + j);
                                        if (!arg.empty()) {
                                            argTokens.push_back(arg);
                                        }
                                    }
                                }
                            }
                        } else if (tokens[j].getType() == Token::Type::Comma && parenCount == 1) {
                            std::vector<Token> arg(tokens.begin() + argStart, tokens.begin() + j);
                            if (!arg.empty()) {
                                argTokens.push_back(arg);
                            }
                            argStart = j + 1;  // Start of next argument
                        }
                        j++;
                    }

                    if (parenCount > 0) {
                        throw CalcError("Unclosed parenthesis in function call to " + funcName);
                    }

                    // Evaluate each argument
                    std::vector<double> argValues;
                    for (const auto& arg : argTokens) {
                        try {
                            argValues.push_back(evaluateExpression(arg));
                        } catch (const CalcError& e) {
                            // Improve error messages for nested function calls
                            std::string errorMsg = e.what();
                            throw CalcError("In argument to " + funcName + "(): " + errorMsg);
                        }
                    }

                    // Call the function and push the result
                    double result = callFunction(funcName, argValues);
                    values.push(result);

                    // Skip over the processed tokens
                    i = j - 1;  // -1 because the loop will increment i
                    continue;
                }
            }

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
                    std::string varName = token.getValue();

                    // Check if this might be a function call with parentheses
                    if (i + 1 < tokens.size() &&
                        tokens[i+1].getType() == Token::Type::Bracket &&
                        tokens[i+1].getValue() == "(") {

                        // Skip this - function calls are now handled at the beginning of the loop
                        // The function detection code at the top of the method will handle it
                        break;
                    }

                    // Not a function call - check if it's a constant first
                    if (varName == "pi") {
                        values.push(Constants::PI);
                    } else if (varName == "e") {
                        values.push(Constants::E);
                    } else if (varName == "phi") {
                        values.push(Constants::PHI);
                    } else if (varName == "sqrt2") {
                        values.push(Constants::SQRT2);
                    } else if (varName == "ans") {
                        values.push(lastResult_);
                    } else {
                        // Then check if it's a variable
                        auto it = variables_.find(varName);
                        if (it == variables_.end()) {
                            // Also check if it's a function (which would be invalid without parentheses)
                            if (functionExists(varName)) {
                                throw CalcError("Function '" + varName + "' used without parentheses. Did you mean '" + varName + "(...)'?");
                            }
                            throw CalcError("Undefined variable: " + varName);
                        }
                        values.push(it->second);
                    }
                    break;
                }

                case Token::Type::Boolean: {
                    if (token.getValue() == "true") {
                        values.push(1.0);
                    } else if (token.getValue() == "false") {
                        values.push(0.0);
                    } else {
                        throw CalcError("Invalid boolean value: " + token.getValue());
                    }
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
                          (precedence(operators.top()) > precedence(op) ||
                           (precedence(operators.top()) == precedence(op) && op != "^"))) {
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
                        } else if (curr_op == "neg") {
                            if (values.empty()) throw CalcError("Invalid expression");
                            double val = values.top();
                            values.pop();
                            values.push(applyUnary(curr_op, val));
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
                            } else if (op == "neg") {
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

                case Token::Type::Comma:
                case Token::Type::Colon:
                case Token::Type::Command:
                    throw CalcError("Unexpected token in expression: " + token.getValue());
                    break;
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
            } else if (op == "neg") {
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