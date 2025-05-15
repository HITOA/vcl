in float inputFloat;
in int inputInt;
in bool inputBool;

in vfloat inputVFloat;
in vint inputVInt;

out float outputFloat;
out int outputInt;
out bool outputBool;

out vfloat outputVFloat;
out vint outputVInt;

void Main() {
    outputFloat = inputFloat;
    outputInt = inputInt;
    outputBool = inputBool;
    
    outputVFloat = inputVFloat;
    outputVInt = inputVInt;
}