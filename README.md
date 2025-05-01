# VCL - Vector Computation Language

**VCL** (Vector Computation Language) is a high-performance domain-specific language designed for digital signal processing (DSP) and vectorized computations. VCL is JIT-compiled using LLVM, automatically selecting the most efficient SIMD instruction set (SSE/AVX/AVX-512) available on the target platform. With VCL, you write code that naturally leverages SIMD operations to deliver real-time performance in audio processing, filtering, and other compute-intensive tasks.

## Key Features

- **SIMD-First Design:**  
  VCL introduces explicit vector types (e.g., `vfloat`) so that operations are automatically vectorized for parallel processing.

- **Static Memory & Real-Time Safety:**  
  Built-in container types such as `ring<T, N>` (for ring buffers) and `array<T, N>` ensure fixed-size, statically allocated storage—ideal for real-time DSP without dynamic memory allocation.

- **Easy I/O Integration:**  
  Use the `in` and `out` keywords to declare global input and output variables that interface directly with external systems (like audio hosts).

- **Custom Attributes:**  
  Annotate code with metadata using custom attributes (e.g., `[Name="Input Audio"]`) fetchable from the c++ library.

- **Seamless C++ Integration:**  
  The VCL C++ library allows you to compile and execute VCL code at runtime, binding global variables and invoking the entry point with minimal overhead.

## Example VCL Code

A simple VCL program that adds two vectors:

```c
in vfloat v1;
in vfloat v2;
out vfloat r;

void Main() {
    r = v1 + v2;
}
```

## Using the VCL C++ Library

Integrate VCL with your C++ application as follows:

```cpp
#include <memory>
#include <VCL/JITContext.hpp>
#include <VCL/Module.hpp>

int main(int argc, char** argv) {
    // Create the JIT context
    std::shared_ptr<VCL::JITContext> context = VCL::JITContext::Create();

    // Create a VCL module from source code
    std::unique_ptr<VCL::Module> module = VCL::Module::Create("Module Name", context);
    module->BindSource(source);

    // Optimize the module using LLVM passes
    module->Optimize();

    // Add the optimized module to the JIT context
    context->AddModule(std::move(module));
    
    // Bind global I/O variables (e.g., v1, v2, r)
    context->BindGlobalVariable("v1", v1);
    context->BindGlobalVariable("v2", v2);
    context->BindGlobalVariable("r", r);

    // Look up the entry point and run it
    void(*Main)() = (void(*)())context->Lookup("Main");
    Main();

    return 0;
}
```

## License

VCL is released under the MIT License. See [LICENSE](LICENSE) for details.