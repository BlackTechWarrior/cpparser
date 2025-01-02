#pragma once 
#include <unordered_map>
#include <deque>
#include <chrono>
#include <functional>
#include <optional>
#include "Token.hpp"  
#include <cmath>

namespace calc { 
    class Calculator {
        public :
            struct HistoryEntry {
                std::string input;
                std::optional<double> result;
                std::chrono::system_clock::time_point timestamp;
            };
        
            using CommandHandler = std::function<void(const std::vector<std::string>&)>;

            Calculator();

            void processInput (std::string_view input);
            void defineVariable (std::string_view name, double value);
            void deleteVariable (std::string_view name);
            void updateVariable (std::string_view name, double value);
            const std::unordered_map <std::string, double>& getVariables() const;
            const std::deque<HistoryEntry>& getHistory() const;
            void clearHistory();
            void deleteAllVariables();

        private :
            std::unordered_map<std::string, double> variables_;
            std::deque<HistoryEntry> history_;
            std::unordered_map<std::string, CommandHandler> commands_;
            double lastResult_{0.0};

            void setupCommands();
            void addToHistory(std::string_view input, std::optional<double> result = std::nullopt);
            double evaluateExpression(const std::vector<Token>& tokens);
            void handleCommand(std::string_view cmd, const std::vector<std::string>& args);
            double evaluateMathFunction(const std::string& func, double arg);
            bool isMathFunction(const std::string& op) const{ 
                return op == "sin" || op == "cos" || op == "tan" || op == "log" ||
                       op == "ln" || op == "sqrt";
            }

            void handleDefine(const std::vector<std::string>& args);
            void handleDelete(const std::vector<std::string>& args);
            void handleUpdate(const std::vector<std::string>& args);
            void handleList(const std::vector<std::string>& args);
    };
}
