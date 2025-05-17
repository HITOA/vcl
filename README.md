# VCL - Vector Computation Language

**VCL** (Vector Computation Language) is a high-performance domain-specific language designed for digital signal processing (DSP) and vectorized computations. VCL is JIT-compiled using LLVM, automatically selecting the most efficient SIMD instruction set (SSE/AVX/AVX-512) available on the target platform. With VCL, you write code that naturally leverages SIMD operations to deliver real-time performance in audio processing, filtering, and other compute-intensive tasks.

## Key Features

- **SIMD-First Design:**  
  VCL introduces explicit vector types (e.g., `vfloat`) so that operations are automatically vectorized for parallel processing.

- **Easy I/O Integration:**  
  Use the `in` and `out` keywords to declare global input and output variables that interface directly with external systems (like audio hosts).

- **Custom Attributes:**  
  Annotate code with metadata using custom attributes (e.g., `[Name="Input Audio"]`) fetchable from the c++ library.

- **Seamless C++ Integration:**  
  The VCL C++ library allows you to compile and execute VCL code at runtime, binding global variables and invoking the entry point with minimal overhead.

## License

VCL is released under the MIT License. See [LICENSE](LICENSE) for details.