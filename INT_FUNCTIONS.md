# Integer Functions

Similar to double precision scientific mode, integer mode has functions: real and extended.

## Real Functions

Definition: `int foo(int64_t value, int64_t *result);`

Currently only one function: `not(value)`

## Extended Functions

All of the following functions perform bitwise operations:

- `and, nand, or, xor, xnor`: Expects 2 arguments, name is self explanatory.
- `rr(value, rot)`: Performs right rotation of `value` bits by `rot` bits.
- `rl(value, rot)`: Performs left rotation of `value` bits by `rot` bits.
- `sr(value, shift)`: Performs right shift of `value` bits by `shift` bits.
- `sra(value, shift)`: Performs arithmetic right shift.
- `sl(value, shift)`: Performs left shift of `value` bits by `shift` bits.
