# Introduction
libtmsolve is a math library for expression evaluation, basic manipulation of matrices and some other useful features.

The most important feature is the ability to parse an expression into a structure called `math_expr`, which allows reuse, supports variable operands and function calls.

# Basic use
To simply evaluate an expression and get the answer:
```
#include <libtmsolve/libtmsolve.h>
double answer;
// enable_complex toggles support for complex numbers, set to false if not needed.
answer = calculate_expr(expr, enable_complex);
```
# Tips
Documentation about the parser, evaluator, and some useful features like factorization can be found in `scientific.h`.

For matrix related features, refer to `matrix.h`.

For functions used internally to manage the calculator (like error handling), refer to `internals.h`.

All error messages are defined in `m_errors.h`.

For functions that assist in manipulating strings (char *), refer to `string_tools.h`.

For functions used to manipulate variables and the calculator functions, refer to `function.h`.

# Note
The library uses double precision floating point values with complex extension for its operations.