# Mathematical Expression Parser

A command-line calculator and expression parser written in C++ that supports complex mathematical operations, variable management, and command history.

## Building the Project

To compile the project, use:
```bash
g++ -I include src/*.cpp
```

Run the resulting executable to start the calculator.

## Features

### Constants
The following mathematical constants are built-in:
- `pi` - Pi (3.14159...)
- `e` - Euler's number (2.71828...)
- `phi` - Golden ratio (1.61803...)
- `sqrt2` - Square root of 2 (1.41421...)

### Mathematical Functions
All functions require brackets for proper parsing. Singular values, variables should work fine both ways:
- `sin(x)` - Sine (in degrees)
- `cos(x)` - Cosine (in degrees)
- `tan(x)` - Tangent (in degrees)
- `log(x)` - Base-10 logarithm
- `ln(x)` - Natural logarithm
- `sqrt(x)` - Square root

### Operators
Supported mathematical operators in order of precedence:
1. `!` - Factorial
2. `^` - Exponentiation
3. `*`, `/`, `%` - Multiplication, Division, Modulo
4. `+`, `-` - Addition, Subtraction

### Implicit Multiplication
The parser supports implicit multiplication in various forms:
- `-(2)` evaluates to `-2`
- `(2)(3)` evaluates to `6`
- `4(2-3)` evaluates to `-4`

### Variable Management

#### Defining Variables
```
def [variable_name] [value]
```
Example:
```
def x 10
x + 5  # Evaluates to 15
```

#### Updating Variables
```
upd [variable_name] [value]
```
Example:
```
upd x ans  # Updates x to the last calculated result
```

### Special Commands

#### Listing Information
- `ls vars` - Display all defined variables
- `ls hist` - Show calculation history

#### Deletion Commands
- `del var` - Delete a specific variable
- `del vars` - Delete all variables
- `del hist` - Clear calculation history

#### Exit
- `exit` - Close the calculator

## Important Notes

1. Trigonometric calculations are performed in degrees. Using radian values directly will not provide expected results.

2. Function calls can work without brackets for single numbers or variables, but brackets are recommended for clarity and required for expressions:
   ```
   sin(60)      # Correct, recommended
   sin 60       # Works for single numbers
   8(sin 30)    # Works, evaluates to 4
   sin 60/2     # Evaluates as (sin 60)/2 = 0.433, not sin(60/2)
   sin(60/2)    # Use brackets for expressions
   ```

3. The special variable `ans` always contains the result of the last calculation.

## Examples

```
# Basic arithmetic
2 + 3 * 4     # Returns 14

# Using functions
sin(30)       # Returns 0.5
log(100)      # Returns 2

# Variable operations
def radius 5
pi * radius ^ 2  # Calculate area
def area ans # saves computed value to area
ls vars       # Shows radius & area variables

# Updating variables
upd radius 10   # Updates radius
pi * radius ^ 2 #find new area
upd area ans #area takes new value, quite tedious (trying to add a feature to simplify it)

# Implicit multiplication
2(3+4)        # Returns 14
```

## Error Handling

The parser includes error handling for:
- Invalid syntax
- Undefined variables
- Division by zero
- Invalid function arguments
- Malformed expressions

If an error occurs, an appropriate error message will be displayed, and the calculator will continue running. Working on more features and a better GUI. If you have recommendations, pm me, add issue or fork this and add your idea.
