/**
*   Builtin vcl span
*/

// This is essentially a ptr and a length. It must be extern/global with either in or out keyword.
// It can be considered as a view on some memory. It is used like an array and its sized can be fetched with the len builtin function.
in span<vfloat> inputs; 

out vfloat output;

void Main() {
    // Accumulate inputs in output
    output = 0;
    for (int i = 0; i < len(inputs); ++i)
        output = output + inputs[i];
}