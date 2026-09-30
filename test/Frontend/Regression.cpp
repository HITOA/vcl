#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Core/Target.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>

#include <llvm/IR/Instructions.h>
#include <VCL/CodeGen/CodeGenModule.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "../Common/ExpectedDiagnostic.hpp"
#include "../Common/MakeModule.hpp"

#include <cstdint>


// Regression tests for bugs found in the 2026-09 review.

TEST_CASE("Review Regressions", "[Frontend][Regression]") {
    VCL::ExecutionSession session{};
    REQUIRE(session.SubmitModule(MakeModule("VCL/regression.vcl")));

    int32_t ri32 = -7;
    uint32_t ru32 = 0x80000000u;
    float rf32 = -1.5f;
    REQUIRE(session.DefineSymbolPtr("ri32", &ri32));
    REQUIRE(session.DefineSymbolPtr("ru32", &ru32));
    REQUIRE(session.DefineSymbolPtr("rf32", &rf32));

    auto* o_shr_unsigned = (uint32_t*)session.Lookup("o_shr_unsigned");
    auto* o_shr_signed = (int32_t*)session.Lookup("o_shr_signed");
    auto* o_abs_f32 = (float*)session.Lookup("o_abs_f32");
    auto* o_abs_i32 = (int32_t*)session.Lookup("o_abs_i32");
    auto* o_min_i32 = (int32_t*)session.Lookup("o_min_i32");
    auto* o_max_u32 = (uint32_t*)session.Lookup("o_max_u32");
    auto* o_min_f32 = (float*)session.Lookup("o_min_f32");
    auto* o_calls = (int32_t*)session.Lookup("o_calls");
    auto* o_and_result = (bool*)session.Lookup("o_and_result");
    auto* o_or_result = (bool*)session.Lookup("o_or_result");
    auto* o_literal_mul = (int32_t*)session.Lookup("o_literal_mul");
    auto* o_literal_big = (int64_t*)session.Lookup("o_literal_big");
    auto* o_dependent_cast = (float*)session.Lookup("o_dependent_cast");
    auto* o_misaligned_lanes_sum = (float*)session.Lookup("o_misaligned_lanes_sum");
    auto* o_after_return = (int32_t*)session.Lookup("o_after_return");
    auto* o_loop_locals = (int32_t*)session.Lookup("o_loop_locals");
    auto* o_comparison_not_template = (int32_t*)session.Lookup("o_comparison_not_template");
    auto* o_special_via_alias = (float*)session.Lookup("o_special_via_alias");
    auto* o_id_alias = (float*)session.Lookup("o_id_alias");
    auto* o_id_direct = (float*)session.Lookup("o_id_direct");

    void* main = session.Lookup("Main");
    REQUIRE(main != nullptr);
    ((void(*)())main)();

    SECTION("C1: right shift of unsigned is logical") {
        REQUIRE(*o_shr_unsigned == 0x40000000u);
        REQUIRE(*o_shr_signed == (-7 >> 1));
    }

    SECTION("C2: abs") {
        REQUIRE(*o_abs_f32 == 1.5f);
        REQUIRE(*o_abs_i32 == 7);
    }

    SECTION("C3: min / max on integers and floats") {
        REQUIRE(*o_min_i32 == -7);
        REQUIRE(*o_max_u32 == 0x80000000u);
        REQUIRE(*o_min_f32 == -1.5f);
    }

    SECTION("C4: && and || short-circuit") {
        REQUIRE(*o_and_result == false);
        REQUIRE(*o_or_result == true);
        REQUIRE(*o_calls == 0);
    }

    SECTION("C5: integer literals are at least int32") {
        REQUIRE(*o_literal_mul == 300);
        REQUIRE(*o_literal_big == 5000000000ll);
    }

    SECTION("C6: explicit cast to a template parameter") {
        REQUIRE(*o_dependent_cast == 0.5f);
    }

    SECTION("C7: pack/unpack of a misaligned Lanes field") {
        uint32_t width = VCL::Target{}.GetVectorWidthInElement();
        float expected = (float)(width * (width - 1)); // 2 * (0 + 1 + ... + width - 1)
        REQUIRE_THAT(*o_misaligned_lanes_sum, Catch::Matchers::WithinRel(expected, 1e-6f));
    }

    SECTION("C8: statements after return") {
        REQUIRE(*o_after_return == 7);
    }

    SECTION("Parser: comparison chain isn't a template argument list") {
        REQUIRE(*o_comparison_not_template == 1);
    }

    SECTION("P1: locals declared in a loop") {
        REQUIRE(*o_loop_locals == 12);
    }

    SECTION("T2: explicit specialization written with an alias") {
        REQUIRE(*o_special_via_alias == 3.0f);
        REQUIRE(*o_id_alias == 4.0f);
        REQUIRE(*o_id_direct == 5.0f);
    }
}

TEST_CASE("Allocas are in the entry block", "[Frontend][Regression]") {
    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateAttributeTable();
    cc.CreateDirectiveRegistry();
    cc.CreateSourceManager();
    cc.CreateTarget();
    cc.CreateTypeCache();
    cc.CreateLLVMContext();

    VCL::Source* source = cc.GetSourceManager().LoadFromDisk("VCL/regression.vcl");
    REQUIRE(source != nullptr);

    VCL::EmitLLVMAction act{};
    act.SetRunOptimization(false);

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    REQUIRE(instance->ExecuteAction(act));
    instance->EndSource();

    llvm::orc::ThreadSafeModule module = act.MoveModule();
    module.withModuleDo([](llvm::Module& m) {
        // T2: Id<Real> and Id<float32> are the same specialization.
        int idSpecializations = 0;
        for (llvm::Function& function : m)
            if ((function.getName().ends_with(".Id") || function.getName().contains(".Id.")) && !function.empty())
                ++idSpecializations;
        REQUIRE(idSpecializations == 1);

        for (llvm::Function& function : m) {
            if (function.empty())
                continue;
            for (llvm::BasicBlock& bb : function) {
                if (&bb == &function.getEntryBlock())
                    continue;
                for (llvm::Instruction& inst : bb) {
                    INFO("alloca outside the entry block in " << function.getName().str());
                    REQUIRE(!llvm::isa<llvm::AllocaInst>(inst));
                }
            }
        }
    });
}

// Parses `path` into `instance` (with `prefix` as its mangling prefix) and emits it into `module`.
static bool EmitInto(VCL::CompilerContext& cc, llvm::Module& module, const char* path, const char* prefix,
        std::vector<std::shared_ptr<VCL::CompilerInstance>>& keepAlive) {
    VCL::Source* source = cc.GetSourceManager().LoadFromDisk(path);
    REQUIRE(source != nullptr);
    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    if (prefix)
        instance->SetManglingPrefix(prefix);
    VCL::ParseSyntaxOnlyAction parse{};
    instance->BeginSource(source);
    bool parsed = instance->ExecuteAction(parse);
    instance->EndSource();
    REQUIRE(parsed);
    VCL::CodeGenModule cgm{ module, instance->GetASTContext(), cc.GetDiagnosticReporter(), cc.GetTarget(),
        instance->GetImportModuleTable(), cc.GetAttributeTable(), cc.GetIdentifierTable() };
    bool emitted = cgm.Emit(false);
    keepAlive.push_back(instance); // the module refers to its declarations
    return emitted;
}

template<typename Consumer>
static void MakeContext(VCL::CompilerContext& cc, Consumer& consumer) {
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateAttributeTable();
    cc.CreateDirectiveRegistry();
    cc.CreateSourceManager();
    cc.CreateTarget();
    cc.CreateTypeCache();
    cc.CreateLLVMContext();
}

static std::vector<std::string> SymbolNames(const char* path) {
    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    MakeContext(cc, consumer);
    std::vector<std::string> names{};
    std::vector<std::shared_ptr<VCL::CompilerInstance>> keepAlive{};
    cc.GetLLVMContext().withContextDo([&](llvm::LLVMContext* context) {
        llvm::Module module{ "names", *context };
        REQUIRE(EmitInto(cc, module, path, nullptr, keepAlive));
        for (llvm::Function& function : module)
            names.push_back(function.getName().str());
        for (llvm::GlobalVariable& global : module.globals())
            names.push_back(global.getName().str());
    });
    std::sort(names.begin(), names.end());
    return names;
}

TEST_CASE("Symbol names are deterministic", "[Frontend][Regression]") {
    std::vector<std::string> first = SymbolNames("VCL/regression.vcl");
    std::vector<std::string> second = SymbolNames("VCL/regression.vcl");
    REQUIRE(first == second);

    // Readable: <module>.<name>, and in/out globals keep their plain name.
    auto hasSuffix = [&](const char* suffix) {
        return std::any_of(first.begin(), first.end(), [&](const std::string& name) { return llvm::StringRef{ name }.ends_with(suffix); });
    };
    REQUIRE(hasSuffix("regression.vcl.EarlyReturn"));
    REQUIRE(std::find(first.begin(), first.end(), "o_calls") != first.end());
}

TEST_CASE("Two ASTs sharing a mangling prefix are a symbol collision", "[Frontend][Regression]") {
    SECTION("Same prefix") {
        ExpectedDiagnostic<VCL::Diagnostic::SymbolNameCollision> consumer{};
        VCL::CompilerContext cc{};
        MakeContext(cc, consumer);
        std::vector<std::shared_ptr<VCL::CompilerInstance>> keepAlive{};
        cc.GetLLVMContext().withContextDo([&](llvm::LLVMContext* context) {
            llvm::Module module{ "collision", *context };
            REQUIRE(EmitInto(cc, module, "VCL/instances.vcl", "same", keepAlive));
            REQUIRE(!EmitInto(cc, module, "VCL/instances.vcl", "same", keepAlive));
        });
        consumer.Require();
    }
    SECTION("Different prefixes") {
        ExpectedNoDiagnostic consumer{};
        VCL::CompilerContext cc{};
        MakeContext(cc, consumer);
        std::vector<std::shared_ptr<VCL::CompilerInstance>> keepAlive{};
        cc.GetLLVMContext().withContextDo([&](llvm::LLVMContext* context) {
            llvm::Module module{ "instances", *context };
            REQUIRE(EmitInto(cc, module, "VCL/instances.vcl", "instance1", keepAlive));
            REQUIRE(EmitInto(cc, module, "VCL/instances.vcl", "instance2", keepAlive));
            // Two separate copies of the state.
            REQUIRE(module.getGlobalVariable("instance1.state", true) != nullptr);
            REQUIRE(module.getGlobalVariable("instance2.state", true) != nullptr);
        });
    }
}
