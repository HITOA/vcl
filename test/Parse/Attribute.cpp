#include <catch2/catch_test_macros.hpp>

#include <VCL/AST/ASTConsumer.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>

#include "../Common/ExpectedDiagnostic.hpp"


namespace {

    class LastDeclConsumer : public VCL::ASTConsumer {
    public:
        void HandleTopLevelDecl(VCL::Decl* decl) override { last = decl; }

        VCL::Decl* last = nullptr;
    };

}

TEST_CASE("Attribute Negative Numeric Arguments", "[Parse]") {
    ExpectedNoDiagnostic diagnostics{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&diagnostics);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateAttributeTable();
    cc.CreateDirectiveRegistry();
    cc.CreateTypeCache();
    cc.CreateSourceManager();

    VCL::AttributeDefinition* range = cc.GetAttributeTable().AddDefinition(cc.GetIdentifierTable().Get("Range"), 3, 3);
    REQUIRE(range != nullptr);

    VCL::Source* source = cc.GetSourceManager().LoadFromMemory("[Range(-3, -2.5, 4)] float32 v;", "buff");
    REQUIRE(source != nullptr);

    LastDeclConsumer consumer{};
    VCL::ParseSyntaxOnlyAction act{};
    act.SetASTConsumer(&consumer);

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    instance->ExecuteAction(act);

    REQUIRE(consumer.last != nullptr);
    VCL::AttributeInstance* attribute = consumer.last->HasAttribute(range);
    REQUIRE(attribute != nullptr);
    REQUIRE(attribute->GetArgsCount() == 3);

    auto* first = (VCL::ConstantScalar*)attribute->GetArgs()[0];
    auto* second = (VCL::ConstantScalar*)attribute->GetArgs()[1];
    auto* third = (VCL::ConstantScalar*)attribute->GetArgs()[2];
    REQUIRE(first->GetKind() == VCL::BuiltinType::Int64);
    REQUIRE(first->Get<int64_t>() == -3);
    REQUIRE(second->GetKind() == VCL::BuiltinType::Float64);
    REQUIRE(second->Get<double>() == -2.5);
    REQUIRE(third->Get<int64_t>() == 4);

    instance->EndSource();
}
