#pragma once


namespace VCL {

    class CodeGenOptions {
    public:
        CodeGenOptions() = default;
        CodeGenOptions(const CodeGenOptions& other) = default;
        CodeGenOptions(CodeGenOptions&& other) = default;
        ~CodeGenOptions() = default;

        CodeGenOptions& operator=(const CodeGenOptions& other) = default;
        CodeGenOptions& operator=(CodeGenOptions&& other) = default;

        /**
         * Emit `llvm.assume(index <u N)` before each run-time index into an array (or Lanes) of
         * static length N. VCL doesn't check bounds, so an out-of-bounds index is already undefined;
         * the assumption lets LLVM rely on it (e.g. to tell a delay line's buffer from its index).
         */
        inline bool GetEmitArrayBoundAssumptions() const { return emitArrayBoundAssumptions; }
        inline void SetEmitArrayBoundAssumptions(bool emit) { emitArrayBoundAssumptions = emit; }

    private:
        bool emitArrayBoundAssumptions = true;
    };

}
