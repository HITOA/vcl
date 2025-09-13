/**
*   List of the language's features
*/

// Variable Decl with no initializer will by default be initialized to null (0)
// Variable Decl in the global scope are global variable and need to be initialized by compile-time known value
// Builtin type

float32 f32;    // 32 bit (4 byte) floating point numeric value
float64 f64;    // 64 bit (8 byte) floating point numeric value
int8 i8;        // 8 bit (1 byte) signed numeric value
int16 i16;      // 16 bit (2 byte) signed numeric value
int32 i32;      // 32 bit (4 byte) signed numeric value
int64 i64;      // 64 bit (8 byte) signed numeric value
uint8 u8;       // 8 bit (1 byte) unsigned numeric value
uint16 u16;     // 16 bit (2 byte) unsigned numeric value
uint32 u32;     // 32 bit (4 byte) unsigned numeric value
uint64 u64;     // 64 bit (8 byte) unsigned numeric value
bool b;         // boolean value

// Vector type (all the same length element wise)

Vec<float32> vf32;  // simd-vector of float32 representation 
Vec<float64> vf64;  // simd-vector of float64 representation
Vec<int32> vi32;    // simd-vector of int32 representation

// Array type

Array<float32, 1024> af32;  // stack array of 1024 float32 elements

// Span

Span<float32> sf32;     // view of an array of float32 allocated by the host (usually on the heap)

// Custom type

struct MyStruct {
    Array<Vec<float32>, 2> f0; // initializer aren't allowed here.
    int64 f1; // or here
}

// Initialization with numeric constant

float32 if32 = 1.0;             // initialized to 1.0
Vec<float32> ivf32 = 1.0;       // each element of the vector are initialized to 1.0    

// Aggregate value

Array<float32, 4> iaf32 = { 0.0, 1.0, 2.0, 3.0 };   // each element of the array are initialized with the corresponding element of the aggregate
Array<int32, 4> iai32 = {};                         // each element without value in the aggregate are gonna be null initialized
MyStruct myStruct = { { 0.0, 0.0 }, 0 };            // the same apply for struct, and aggregate value can be nested and can contain multiple type

// Function Decl
void Dummy() {}

// Function Decl with arguments and returning value
float32 Mul(float32 lhs, float32 rhs) {
    return lhs * rhs;
}

// Templated Function Decl
T Lerp<typename T>(T a, T b, T t) {
    return a + (b - a) * t;
}

// Namespace
namespace Audio {

    // Templated structure
    struct Delay<typename T, int64 Size> {
        Array<T, Size> buffer;
        int64 currentIndex;

        // You can create function inside struct. This isn't OOP but essentally
        // just sugar over a function that accept the struct as its first argument (as 'self').
        T Read(int64 delay) {
            int64 readingIndex = (currentIndex + Size - delay) % Size;
            return buffer[readingIndex];
        }

        void Write(T value) {
            buffer[currentIndex] = value;
            currentIndex = (currentIndex + 1) % Size;
        }
    }

}

// Access struct inside namespace
Audio.Delay<float32, 1024> delay1;

// Bump namespace into current one
using Audio;
// Now you can
Delay<float32, 1024> delay2;

// Global Variable Decl can also be qualified as in, out, or inout

// Initialization of in-qualified global variable can be a bit tricky.
// This does not emit any IR to initialize the variable as it does not have storage.
// Instead it is used as a hint for the host when it will create storage for this variable.
// It also mean this value can be ignored.
// the 'in' qualifier also implies const
in float32 inputFloat32 = 0.0;

// Initialization for out-qualified variable behave as you would expect
// the 'out' qualifier also implies write-only
out float32 outputFloat32 = 0.0;

// this behave like out but you can also read to it.
inout int64 inoutInt64 = 0.0;

void Main() {
    // Local Variable Decl const qualified
    const int64 delay = inoutInt64;

    delay1.Write(inputFloat32);
    outputFloat32 = delay1.Read(delay);

    float32 a = 0.0;
    float32 b = 1.0;
    float32 t = 0.5;

    // Call of templated function with explicit template argument
    float r = Lerp<float32>(a, b, t);
    // Template argument deduction (partial also work)
    r = Lerp(a, b, t);
}