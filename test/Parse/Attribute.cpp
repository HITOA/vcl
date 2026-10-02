#include <catch2/catch_test_macros.hpp>

#include <VCL/AST/ASTConsumer.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>

#include "../Common/ExpectedDiagnostic.hpp"

#include <functional>


namespace {

    class LastDeclConsumer : public VCL::ASTConsumer {
    public:
        void HandleTopLevelDecl(VCL::Decl* decl) override { last = decl; }

        VCL::Decl* last = nullptr;
    };

    /**
     * Parses `src` with `Range` (1 to 3 positional arguments) and `Knob` (2 to 4 arguments, named
     * `min`, `max`, `unit`), then hands the `Knob` or `Range` of the last declaration to `check`.
     */
    void Parse(VCL::DiagnosticConsumer& diagnostics, const char* src,
            const std::function<void(VCL::CompilerContext&, VCL::AttributeInstance*)>& check = {}) {
        VCL::CompilerContext cc{};
        cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&diagnostics);
        cc.CreateDiagnosticEngine();
        cc.CreateIdentifierTable();
        cc.CreateAttributeTable();
        cc.CreateDirectiveRegistry();
        cc.CreateTypeCache();
        cc.CreateSourceManager();

        VCL::IdentifierTable& identifiers = cc.GetIdentifierTable();
        VCL::AttributeDefinition* range = cc.GetAttributeTable().AddDefinition(identifiers.Get("Range"), 1, 3);
        VCL::IdentifierInfo* names[] = { identifiers.Get("min"), identifiers.Get("max"), identifiers.Get("unit") };
        VCL::AttributeDefinition* knob = cc.GetAttributeTable().AddDefinition(identifiers.Get("Knob"), 2, 4, names);
        REQUIRE(range != nullptr);
        REQUIRE(knob != nullptr);
        REQUIRE(knob->GetArgNames().size() == 3);

        VCL::Source* source = cc.GetSourceManager().LoadFromMemory(src, "buff");
        REQUIRE(source != nullptr);

        LastDeclConsumer consumer{};
        VCL::ParseSyntaxOnlyAction act{};
        act.SetASTConsumer(&consumer);

        std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
        instance->BeginSource(source);
        instance->ExecuteAction(act);

        if (check) {
            REQUIRE(consumer.last != nullptr);
            VCL::AttributeInstance* attribute = consumer.last->HasAttribute(knob);
            if (attribute == nullptr)
                attribute = consumer.last->HasAttribute(range);
            REQUIRE(attribute != nullptr);
            check(cc, attribute);
        }

        instance->EndSource();
    }

    template<VCL::Diagnostic::DiagnosticMsg Message>
    void CheckForError(const char* src) {
        ExpectedDiagnostic<Message> diagnostics{};
        Parse(diagnostics, src);
        diagnostics.Require();
    }

    int64_t Int(VCL::ConstantValue* value) {
        REQUIRE(value->GetConstantValueClass() == VCL::ConstantValue::ConstantScalarClass);
        REQUIRE(((VCL::ConstantScalar*)value)->GetKind() == VCL::BuiltinType::Int64);
        return ((VCL::ConstantScalar*)value)->Get<int64_t>();
    }

}

TEST_CASE("Attribute Negative Numeric Arguments", "[Parse]") {
    ExpectedNoDiagnostic diagnostics{};
    Parse(diagnostics, "[Range(-3, -2.5, 4)] float32 v;", [](VCL::CompilerContext&, VCL::AttributeInstance* attribute) {
        REQUIRE(attribute->GetArgsCount() == 3);

        auto* second = (VCL::ConstantScalar*)attribute->GetArgs()[1];
        REQUIRE(Int(attribute->GetArgs()[0]) == -3);
        REQUIRE(second->GetKind() == VCL::BuiltinType::Float64);
        REQUIRE(second->Get<double>() == -2.5);
        REQUIRE(Int(attribute->GetArgs()[2]) == 4);
    });
}

TEST_CASE("Attribute Named Arguments", "[Parse]") {
    ExpectedNoDiagnostic diagnostics{};
    Parse(diagnostics, "[Knob(1, unit = \"Hz\", max = 2)] float32 v;", [](VCL::CompilerContext& cc, VCL::AttributeInstance* attribute) {
        VCL::IdentifierTable& identifiers = cc.GetIdentifierTable();
        REQUIRE(attribute->GetArgsCount() == 3);
        REQUIRE(attribute->GetPositionalArgs().size() == 1);
        REQUIRE(Int(attribute->GetPositionalArgs()[0]) == 1);
        REQUIRE(attribute->GetArgNames()[0] == nullptr);
        REQUIRE(attribute->GetArgNames()[1] == identifiers.Get("unit"));
        REQUIRE(Int(attribute->GetNamedArg(identifiers.Get("max"))) == 2);
        REQUIRE(attribute->GetNamedArg(identifiers.Get("unit"))->GetConstantValueClass() == VCL::ConstantValue::ConstantStringClass);
        REQUIRE(attribute->GetNamedArg(identifiers.Get("min")) == nullptr);
    });
}

TEST_CASE("Attribute Identifier Argument Is Positional", "[Parse]") {
    ExpectedNoDiagnostic diagnostics{};
    Parse(diagnostics, "[Knob(min = 1, max = 2), Range(Log)] float32 v;", [](VCL::CompilerContext&, VCL::AttributeInstance* attribute) {
        REQUIRE(attribute->GetPositionalArgs().empty());
        REQUIRE(attribute->GetArgsCount() == 2);
        REQUIRE(attribute->GetNextAttribute() != nullptr);
        REQUIRE(attribute->GetNextAttribute()->GetPositionalArgs().size() == 1);
        REQUIRE(attribute->GetNextAttribute()->GetArgs()[0]->GetConstantValueClass() == VCL::ConstantValue::ConstantIdentifierClass);
    });
}

TEST_CASE("Attribute Unknown Named Argument", "[Parse]") {
    CheckForError<VCL::Diagnostic::AttributeUnknownArgument>("[Knob(1, 2, uint = \"Hz\")] float32 v;");
}

TEST_CASE("Attribute Without Named Arguments", "[Parse]") {
    CheckForError<VCL::Diagnostic::AttributeUnknownArgument>("[Range(min = 1)] float32 v;");
}

TEST_CASE("Attribute Duplicate Named Argument", "[Parse]") {
    CheckForError<VCL::Diagnostic::AttributeDuplicateArgument>("[Knob(min = 1, min = 2)] float32 v;");
}

TEST_CASE("Attribute Positional After Named Argument", "[Parse]") {
    CheckForError<VCL::Diagnostic::AttributePositionalAfterNamed>("[Knob(min = 1, 2)] float32 v;");
}
