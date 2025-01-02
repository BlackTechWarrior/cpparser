#include "Calculator.hpp"
#include "Constants.hpp"
#include <iostream>
#include <string>

int main() {
    calc:: Calculator calculator;

    std::cout << "Calscript v2.0" << std::endl;
    std::cout << "Enter expression to solve or use commands below" << std::endl;
    std::cout << "Supported constants: - pi, e, phi, sqrt2" << std::endl;
    std::cout << "cos, sin, tan supported. Calculation done in degrees" << std::endl;
    std::cout << "Current commands:" << std::endl;
    std::cout << "  def <var> <value> - Define variable" << std::endl;
    std::cout << "  upd <var> <value> - Update variable" << std::endl;
    std::cout << "  del <var|vars|hist>    - Delete variable or history" << std::endl;
    std::cout << "  ls <vars|hist>    - List variables or history" << std::endl;
    std::cout << "  exit              - Exit calculator" << std::endl;
    std::cout << std::endl;
    

    std::string input;
    double prevResult = 0;
    while(true) {
        std::cout << calc::Constants::PROMPT;
        std::getline(std::cin, input);
        if (input == "exit") break;
        calculator.processInput(input);
    }
    return 0;
}
