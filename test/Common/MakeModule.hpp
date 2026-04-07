#pragma once

#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>

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