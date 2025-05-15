/**
* VCL Test infinite recursion
*/


in vint input;

out vint output;

vint Factorial(vint n) {
    // Select alwayse evaluate both "branches" so this recurse indefinitely
    return select(n == 0 || n == 1, 1, Factorial(n - 1) * n);
}

void Main() {
    output = Factorial(input);
}