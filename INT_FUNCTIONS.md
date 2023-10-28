# Integer Functions

Similar to double precision scientific mode, integer mode has functions: real and extended.

## Real Functions

Definition: `int64_t foo(int64_t value);`

Currently only one function: `not(value)`

## Extended Functions

All of the following functions perform bitwise operations:

- `and, nand, or, xor, xnor`: Expects 2 arguments, name is self explanatory.
- `rrc(value, shift)`: Performs right circular rotation of `value` bits by `shift` bits.
- `rlc(value, shift)`: Performs left circular rotation of `value` bits by `shift` bits.
