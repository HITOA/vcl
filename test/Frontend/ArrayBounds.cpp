#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/Target.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>

#include <llvm/IR/Constants.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>

#include "../Common/MakeModule.hpp"

#include <cstdint>


// Array-bound assumptions (CodeGenOptions::SetEmitArrayBoundAssumptions): a run-time index into an
// array of static length N is assumed to be below N.

static const char* boundsSource =
    "Array<float32, 16> arr;\n"
    "Lanes<float32> lanes;\n"
    "float32 ReadArray(uint32 i) { return arr[i]; }\n"
    "float32 ReadLanes(uint32 i) { return lanes[i]; }\n"
    "float32 ReadParam(Array<float32, 16> a, int32 i) { return a[i]; }\n"
    "float32 ReadConstant() { return arr[3] + lanes[1]; }\n"
    "float32 ReadSpan(Span<float32> s, uint32 i) { return s[i]; }\n"
    "float32 ReadNarrow(Array<float32, 1000> a, uint8 i) { return a[i]; }\n";

// The bounds of the `index <u N` assumptions in the function whose name ends with `.name`.
static std::vector<uint64_t> AssumedBounds(llvm::Module& module, llvm::StringRef name) {
    std::vector<uint64_t> bounds{};
    for (llvm::Function& function : module) {
        if (!function.getName().ends_with(("." + name).str()))
            continue;
        for (llvm::Instruction& inst : llvm::instructions(function)) {
            auto* assume = llvm::dyn_cast<llvm::AssumeInst>(&inst);
            if (!assume)
                continue;
            auto* cmp = llvm::dyn_cast<llvm::ICmpInst>(assume->getArgOperand(0));
            REQUIRE(cmp != nullptr);
            REQUIRE(cmp->getPredicate() == llvm::CmpInst::ICMP_ULT);
            auto* bound = llvm::dyn_cast<llvm::ConstantInt>(cmp->getOperand(1));
            REQUIRE(bound != nullptr);
            bounds.push_back(bound->getZExtValue());
        }
    }
    return bounds;
}

TEST_CASE("Array Bound Assumptions", "[Frontend][CodeGen]") {
    SECTION("Emitted for a run-time index into an Array or Lanes, and nothing else") {
        llvm::orc::ThreadSafeModule module = MakeModuleFromSource(boundsSource, false);
        uint64_t width = VCL::Target{}.GetVectorWidthInElement();
        module.withModuleDo([&](llvm::Module& m) {
            REQUIRE(AssumedBounds(m, "ReadArray") == std::vector<uint64_t>{ 16 });
            REQUIRE(AssumedBounds(m, "ReadLanes") == std::vector<uint64_t>{ width });
            REQUIRE(AssumedBounds(m, "ReadParam") == std::vector<uint64_t>{ 16 });
            REQUIRE(AssumedBounds(m, "ReadConstant").empty());
            REQUIRE(AssumedBounds(m, "ReadSpan").empty()); // no static length
            REQUIRE(AssumedBounds(m, "ReadNarrow").empty()); // every uint8 is below 1000
        });
    }
    SECTION("Not emitted when the option is off") {
        llvm::orc::ThreadSafeModule module = MakeModuleFromSource(boundsSource, false, [](VCL::CompilerInvocation& invocation) {
            invocation.GetCodeGenOptions().SetEmitArrayBoundAssumptions(false);
        });
        module.withModuleDo([&](llvm::Module& m) {
            REQUIRE(AssumedBounds(m, "ReadArray").empty());
            REQUIRE(AssumedBounds(m, "ReadLanes").empty());
            REQUIRE(AssumedBounds(m, "ReadParam").empty());
        });
    }
    SECTION("A delay line written at a run-time index keeps its own index") {
        // The assumption is what lets LLVM tell `buffer[index]` from `index` (research round 2):
        // the store into the buffer must not be taken for a store into the index.
        VCL::ExecutionSession session{};
        REQUIRE(session.SubmitModule(MakeModuleFromSource(
            "struct Delay { Array<float32, 8> buffer; uint32 index; }\n"
            "Delay line;\n"
            "out float32 o_read;\n"
            "out uint32 o_index;\n"
            "void Write(inout Delay d, float32 v) {\n"
            "    d.buffer[d.index] = v;\n"
            "    d.index = d.index + 1;\n"
            "    if (d.index >= 8)\n"
            "        d.index = 0;\n"
            "}\n"
            "float32 Read(Delay d, uint32 n) {\n"
            "    uint32 i = d.index + 8 - n;\n"
            "    if (i >= 8)\n"
            "        i -= 8;\n"
            "    return d.buffer[i];\n"
            "}\n"
            "[EntryPoint] void Main() {\n"
            "    for (uint32 k = 0; k < 11; ++k)\n"
            "        Write(line, (float32)k);\n"
            "    o_index = line.index;\n"
            "    o_read = Read(line, 1);\n"
            "}\n")));

        auto* o_read = (float*)session.Lookup("o_read");
        auto* o_index = (uint32_t*)session.Lookup("o_index");
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        REQUIRE(*o_index == 3);
        REQUIRE(*o_read == 10.0f);
    }
}
