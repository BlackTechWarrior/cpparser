# CalScript Calculator

A C++ command-line calculator capable of evaluating complex mathematical expressions, managing variables, and defining custom functions. This tool leverages an efficient token-based parsing system based upon the shunting-yard algorithm for accurate numerical calculations. 

## Building the Project

This project requires C++17 or above for features like `std::optional` and `std::string_view`.

```bash
g++ -std=c++17 -I include src/*.cpp -o calscript
```

Run the executable to start the calculator.

## Core Features

### Constants
Built-in mathematical constants:
* `pi` - Pi (3.14159...)
* `e` - Euler's number (2.71828...)
* `phi` - Golden ratio (1.61803...)
* `sqrt2` - Square root of 2 (1.41421...)
* `ans` - Result of the last calculation

### Mathematical Functions
* `sin(x)` - Sine (in degrees)
* `cos(x)` - Cosine (in degrees)
* `tan(x)` - Tangent (in degrees)
* `log(x)` - Base-10 logarithm
* `ln(x)` - Natural logarithm
* `sqrt(x)` - Square root

### Operators
In order of precedence:
1. `!` - Factorial
2. `^` - Exponentiation
3. `*`, `/`, `%` - Multiplication, Division, Modulo
4. `+`, `-` - Addition, Subtraction

### Implicit Multiplication
The parser intelligently recognizes:
* `2(3)` evaluates to `6`
* `4pi` evaluates to `4 * pi`
* `2sin(30)` evaluates to `2 * sin(30)`
* `(2+3)(4+5)` evaluates to `(2+3) * (4+5)`

## Variable Management

### Defining Variables
```
def [variable_name] [expression]
```

Example:
```
def radius 5
def area pi*radius^2
```

### Updating Variables
```
upd [variable_name] [expression]
```

Example:
```
upd radius 10
upd area pi*radius^2
```

## Custom Functions

### Defining Functions
```
create func [name]([param1], [param2], ...): [expression]
```

Example:
```
create func area(r): pi*r^2
create func hypotenuse(a, b): sqrt(a^2 + b^2)
```

### Using Functions
Functions can be called directly:
```
area(5)
hypotenuse(3, 4)
```

## Utility Commands

### Listing Information
* `ls vars` - Display all defined variables
* `ls hist` - Show calculation history
* `ls funcs` - List all defined functions

### Deletion Commands
* `del [variable_name]` - Delete a specific variable
* `del func [function_name]` - Delete a function
* `del vars` - Delete all variables
* `del hist` - Clear calculation history

### System Commands
* `exit` - Close the calculator
* `clear` - Clear the screen

## Examples

### Basic Arithmetic
```
2 + 3 * 4     # Returns 14
(2 + 3) * 4   # Returns 20
```

### Using Built-in Functions
```
sin(30)       # Returns 0.5
log(100)      # Returns 2
```

### Variable Operations
```
def radius 5
pi * radius^2  # Calculate circle area = 78.54
def area ans   # Save result to variable
```

### Custom Functions
```
create func circle_area(r): pi*r^2
circle_area(5)  # Returns 78.54

create func quad(a, b, c, x): a*x^2 + b*x + c
quad(1, -3, 2, 2)  # Returns 0
```

### Complex Calculations
```
create func normal_pdf(x, mean, std): (1/(std*sqrt(2*pi)))*e^(-0.5*((x-mean)/std)^2)
normal_pdf(0, 0, 1)  # Returns 0.3989 (standard normal at peak)
```

## Current Limitations
* No control structures (if/else, loops)
* No array/list support
* Limited symbolic manipulation capabilities
* Simple text-based interface

## Best Practices
1. Use parentheses to clarify operator precedence in complex expressions
2. Always use parentheses with function calls for clarity
3. Use descriptive variable and function names
4. Break complex calculations into smaller parts using intermediate variables

## Error Handling
The calculator provides error messages for:
* Syntax errors
* Undefined variables/functions
* Division by zero
* Invalid function arguments
* Mismatched parentheses