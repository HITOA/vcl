#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>
#include <VCL/Frontend/ModuleCache.hpp>
#include <VCL/Frontend/Directives.hpp>
#include <VCL/AST/DeclTemplate.hpp>

#include "../Common/ExpectedDiagnostic.hpp"


// Compiles `path` with an `@import` handler rooted at the test VCL directory.
static llvm::orc::ThreadSafeModule MakeModuleWithImports(llvm::StringRef path) {
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
    cc.CreateModuleCache();
    cc.CreateLLVMContext();
    cc.GetDirectiveRegistry().CreateDirectiveHandler<VCL::ImportDirective>(cc.GetIdentifierTable().Get("import"), cc, "VCL");

    VCL::Source* source = cc.GetSourceManager().LoadFromDisk(path);
    REQUIRE(source != nullptr);

    VCL::EmitLLVMAction act{};
    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    REQUIRE(instance->ExecuteAction(act));
    instance->EndSource();
    return act.MoveModule();
}

TEST_CASE("Template names bind in their own module", "[Frontend][Modules]") {
    VCL::ExecutionSession session{};
    session.DefineDefaultMemIntrinsic();
    REQUIRE(session.SubmitModule(MakeModuleWithImports("VCL/modules_noshadow.vcl")));

    float* o_use_helpers = (float*)session.Lookup("o_use_helpers");
    float* o_use_qualified = (float*)session.Lookup("o_use_qualified");
    ((void(*)())session.Lookup("Main"))();

    // Helper(10) = 11, Twice(2) = 4, Scale = 2
    REQUIRE(*o_use_helpers == 17.0f);
    REQUIRE(*o_use_qualified == 11.0f);
}

TEST_CASE("Template names are not captured by the instantiating module", "[Frontend][Modules]") {
    VCL::ExecutionSession session{};
    session.DefineDefaultMemIntrinsic();
    REQUIRE(session.SubmitModule(MakeModuleWithImports("VCL/modules_shadow.vcl")));

    float* o_use_helpers = (float*)session.Lookup("o_use_helpers");
    ((void(*)())session.Lookup("Main"))();

    REQUIRE(*o_use_helpers == 17.0f);
}

TEST_CASE("Errors inside an imported template are reported", "[Frontend][Modules]") {
    ExpectedDiagnostic<VCL::Diagnostic::IdentifierUndefined> consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateAttributeTable();
    cc.CreateDirectiveRegistry();
    cc.CreateSourceManager();
    cc.CreateTarget();
    cc.CreateTypeCache();
    cc.CreateModuleCache();
    cc.CreateLLVMContext();
    cc.GetDirectiveRegistry().CreateDirectiveHandler<VCL::ImportDirective>(cc.GetIdentifierTable().Get("import"), cc, "VCL");

    VCL::Source* source = cc.GetSourceManager().LoadFromDisk("VCL/modules_broken.vcl");
    REQUIRE(source != nullptr);

    VCL::EmitLLVMAction act{};
    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    REQUIRE(!instance->ExecuteAction(act));
    instance->EndSource();
    consumer.Require();
}

TEST_CASE("Library templates don't keep instantiations for an importer's types", "[Frontend][Modules]") {
    // A client may compile a module again on each edit while the libraries it imports stay cached.
    // A library template instantiated with the importer's own struct must not stay attached to the
    // library: the importer's AST, and the struct with it, is gone after the compile.
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
    cc.CreateModuleCache();
    cc.CreateLLVMContext();
    cc.GetDirectiveRegistry().CreateDirectiveHandler<VCL::ImportDirective>(cc.GetIdentifierTable().Get("import"), cc, "VCL");

    VCL::Source* source = cc.GetSourceManager().LoadFromDisk("VCL/modules_importerstruct.vcl");
    REQUIRE(source != nullptr);

    for (int compile = 0; compile < 2; ++compile) {
        VCL::ExecutionSession session{};
        session.DefineDefaultMemIntrinsic();
        {
            VCL::EmitLLVMAction act{};
            std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
            instance->BeginSource(source);
            REQUIRE(instance->ExecuteAction(act));
            instance->EndSource();
            REQUIRE(session.SubmitModule(act.MoveModule()));
        } // the importer's CompilerInstance and AST die here, as after a client's compile

        float* o_sum = (float*)session.Lookup("o_sum");
        ((void(*)())session.Lookup("Main"))();
        REQUIRE(*o_sum == 3.5f);
    }

    VCL::Source* library = cc.GetSourceManager().LoadFromDisk("VCL/libmath.vcl");
    VCL::Module* module = cc.GetModuleCache().Get(library);
    REQUIRE(module != nullptr);
    VCL::Decl* exported = module->GetCompilerInstance()->GetExportSymbolTable().Get(cc.GetIdentifierTable().Get("SumFields"));
    REQUIRE(exported != nullptr);
    REQUIRE(exported->GetDeclClass() == VCL::Decl::TemplateDeclClass);
    VCL::TemplateDecl* templateDecl = (VCL::TemplateDecl*)exported;
    size_t specializations = 0;
    for (auto it = templateDecl->Begin(); it != templateDecl->End(); ++it)
        if (it->GetDeclClass() == VCL::Decl::TemplateSpecializationDeclClass)
            ++specializations;
    REQUIRE(specializations == 0);
}
