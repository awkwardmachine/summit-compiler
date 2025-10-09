# Summit Compiler
This is a small compiler for the Summit language, currently supporting variable declarations, constants, and the @println builtin.

## Building the Compiler
To build the compiler, simply run:
```bash
make
```
This will produce the compiler executable.

## Writing a Summit Program
Create a file with the .sm extension. Example:
```
var x: Int8 = 5;
const y: Int8 = 7;
@println(x);
@println(y);
```
- var declares a mutable variable.
- const declares an immutable constant.
- @println prints the value of a variable or constant.

## Compiling and Running
Compile your .sm file using the compiler:
```bash
./summit your_program.sm
```

If successful, this produces an executable with the same base name as your .sm file. Run it:
```bash
./your_program
```
You should see the output of your program in the terminal.
