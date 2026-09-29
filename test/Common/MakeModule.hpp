#pragma once

#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>
#include <VCL/Frontend/CompilerInvocation.hpp>

#include <functional>

#include "ExpectedDiagnostic.hpp"


inline llvm::orc::ThreadSafeModule MakeModule(llvm::StringRef path) {
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

    VCL::Source* source = cc.GetSourceManager().LoadFromDisk(path);
    REQUIRE(source != nullptr);

    VCL::EmitLLVMAction act{};

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    REQUIRE(instance->ExecuteAction(act));
    instance->EndSource();

    return act.MoveModule();
}

/** Compiles `src` (no diagnostic expected), optimized unless `optimize` is false. */
inline llvm::orc::ThreadSafeModule MakeModuleFromSource(const char* src, bool optimize = true,
        std::function<void(VCL::CompilerInvocation&)> configure = {}) {
    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    if (configure)
        configure(*cc.GetInvocation());
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateAttributeTable();
    cc.CreateDirectiveRegistry();
    cc.CreateSourceManager();
    cc.CreateTarget();
    cc.CreateTypeCache();
    cc.CreateLLVMContext();

    VCL::Source* source = cc.GetSourceManager().LoadFromMemory(src, "buff");
    REQUIRE(source != nullptr);

    VCL::EmitLLVMAction act{};
    act.SetRunOptimization(optimize);

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    REQUIRE(instance->ExecuteAction(act));
    instance->EndSource();

    return act.MoveModule();
}

/** Compiles `src` and requires that it reports `Message`. */
template<VCL::Diagnostic::DiagnosticMsg Message>
inline void RequireDiagnosticFromSource(const char* src) {
    ExpectedDiagnostic<Message> consumer{};
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

    VCL::Source* source = cc.GetSourceManager().LoadFromMemory(src, "buff");
    REQUIRE(source != nullptr);

    VCL::EmitLLVMAction act{};
    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    instance->ExecuteAction(act);
    instance->EndSource();

    consumer.Require();
}