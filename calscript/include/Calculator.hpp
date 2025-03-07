#pragma once
#include <unordered_map>
#include <deque>
#include <chrono>
#include <functional>
#include <optional>
#include <vector>
#include "Token.hpp"
#include <cmath>

namespace calc {
    class Calculator {
        public:
            struct HistoryEntry {
                std::string input;
                std::optional<double> result;
                std::chrono::system_clock::time_point timestamp;
            };
            
            // Custom function class
            class Function {
                public:
                    Function() = default;
                    Function(std::string name, std::vector<std::string> params, std::vector<Token> body)
                        : name_(std::move(name)), parameters_(std::move(params)), body_(std::move(body)) {}
                    
                    const std::string& getName() const { return name_; }
                    const std::vector<std::string>& getParameters() const { return parameters_; }
                    const std::vector<Token>& getBody() const { return body_; }
                    
                private:
                    std::string name_;
                    std::vector<std::string> parameters_;
                    std::vector<Token> body_;
            };
        
            using CommandHandler = std::function<void(const std::vector<std::string>&)>;

            Calculator();

            void processInput(std::string_view input);
            void defineVariable(std::string_view name, double value);
            void deleteVariable(std::string_view name);
            void updateVariable(std::string_view name, double value);
            const std::unordered_map<std::string, double>& getVariables() const { return variables_; }
            const std::deque<HistoryEntry>& getHistory() const { return history_; }
            void clearHistory();
            void deleteAllVariables();
            
            // Function management
            void defineFunction(const std::string& name, const std::vector<std::string>& params, const std::vector<Token>& body);
            void deleteFunction(const std::string& name);
            double callFunction(const std::string& name, const std::vector<double>& args);
            bool functionExists(const std::string& name) const;

        private:
            std::unordered_map<std::string, double> variables_;
            std::deque<HistoryEntry> history_;
            std::unordered_map<std::string, CommandHandler> commands_;
            std::unordered_map<std::string, Function> functions_;

            double lastResult_{0.0};

            void setupCommands();
            void addToHistory(std::string_view input, std::optional<double> result = std::nullopt);
            double evaluateExpression(const std::vector<Token>& tokens);
            void handleCommand(std::string_view cmd, const std::vector<std::string>& args);
            double evaluateMathFunction(const std::string& func, double arg);
            bool isMathFunction(const std::string& op) const {
                return op == "sin" || op == "cos" || op == "tan" || op == "log" ||
                       op == "ln" || op == "sqrt";
            }

            void handleDefine(const std::string& varName, const std::string& valueExpr);
            void handleDelete(const std::vector<std::string>& args);
            void handleUpdate(const std::string& varName, const std::string& valueExpr);
            void handleList(const std::vector<std::string>& args);
            
            // Function handling
            void handleFunctionDefinition(const std::vector<Token>& tokens);
            void handleFunctionCall(const std::vector<Token>& tokens);
            double evaluateWithLocalScope(const std::vector<Token>& body, 
                                         const std::vector<std::string>& paramNames,
                                         const std::vector<double>& paramValues);
    };
}