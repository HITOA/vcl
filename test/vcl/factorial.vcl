/**
* VCL Test factorial
*/


in int input;

out int output;

int Factorial(int n) {
    if (n == 0 || n == 1)
        return 1;
    return n * Factorial(n - 1);
}

void Main() {
    output = Factorial(input);
}