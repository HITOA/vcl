#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/Source.hpp>
#include <VCL/Core/SourceManager.hpp>
#include <VCL/Lex/Lexer.hpp>
#include <VCL/Lex/TokenStream.hpp>
#include <VCL/Parse/Parser.hpp>
#include <VCL/Sema/Sema.hpp>
#include <VCL/Sema/TreeTransform.hpp>
#include <VCL/CodeGen/CodeGenModule.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>

#include <llvm/IR/Module.h>

#include <cstdint>
#include <functional>
#include <memory>

#include "../Common/ExpectedDiagnostic.hpp"
#include "../Common/MakeModule.hpp"


// TreeTransform (P2.1), emitting a transformed translation unit (P2.2) and parameter codegen flags
// (P2.3). Each test parses a source with a live Sema, as vcl-graph does, copies its variables and
// functions into a new translation unit through a transform, and emits only the copy.

using MakeTransform = std::function<std::unique_ptr<VCL::TreeTransform>(VCL::Sema& sema, VCL::TranslationUnitDecl* source)>;

static llvm::orc::ThreadSafeModule EmitTransformedSource(const char* src, MakeTransform makeTransform) {
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

    VCL::Source* source = cc.GetSourceManager().LoadFromMemory(src, "buff");
    REQUIRE(source != nullptr);

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    instance->CreateASTContext();
    instance->CreateExportSymbolTable();
    instance->CreateImportModuleTable();
    instance->CreateDefineTable();

    VCL::Lexer lexer{ source->GetBufferRef(), cc.GetDiagnosticReporter(), cc.GetIdentifierTable() };
    VCL::TokenStream stream{ lexer };
    VCL::Sema sema{ cc, instance->GetASTContext(), cc.GetDiagnosticReporter(), cc.GetIdentifierTable(),
        cc.GetDirectiveRegistry(), instance->GetExportSymbolTable(), instance->GetImportModuleTable(), instance->GetDefineTable() };
    VCL::Parser parser{ stream, sema, cc.GetAttributeTable() };
    REQUIRE(parser.Parse());

    VCL::TranslationUnitDecl* root = instance->GetASTContext().GetTranslationUnitDecl();
    VCL::TranslationUnitDecl* copy = VCL::TranslationUnitDecl::Create(instance->GetASTContext());
    {
        std::unique_ptr<VCL::TreeTransform> transform = makeTransform(sema, root);
        auto guard = sema.EnterTranslationUnit(copy);
        for (auto it = root->Begin(); it != root->End(); ++it) {
            if (it->GetDeclClass() != VCL::Decl::VarDeclClass && it->GetDeclClass() != VCL::Decl::FunctionDeclClass)
                continue; // records, templates... are used where they are
            REQUIRE(transform->TransformDecl(it.Get()) != nullptr);
        }
    }

    llvm::orc::ThreadSafeModule module{
        cc.GetLLVMContext().withContextDo([](llvm::LLVMContext* context) {
            return std::make_unique<llvm::Module>("module", *context);
        }),
        cc.GetLLVMContext() };
    module.withModuleDo([&](llvm::Module& m) {
        VCL::CodeGenModule cgm{ m, instance->GetASTContext(), cc.GetDiagnosticReporter(), cc.GetTarget(),
            instance->GetImportModuleTable(), cc.GetAttributeTable(), cc.GetIdentifierTable() };
        REQUIRE(cgm.Emit(copy));
    });

    instance->EndSource();
    return module;
}

static VCL::NamedDecl* FindTopLevelDecl(VCL::TranslationUnitDecl* tu, llvm::StringRef name) {
    for (auto it = tu->Begin(); it != tu->End(); ++it)
        if (it->IsNamedDecl() && ((VCL::NamedDecl*)it.Get())->GetIdentifierInfo()->GetName() == name)
            return (VCL::NamedDecl*)it.Get();
    return nullptr;
}

static MakeTransform Identity() {
    return [](VCL::Sema& sema, VCL::TranslationUnitDecl*) { return std::make_unique<VCL::TreeTransform>(sema); };
}

// The P2.1 test subclass: every reference to `from` becomes a reference to `to`.
class RenameTransform : public VCL::TreeTransform {
public:
    RenameTransform(VCL::Sema& sema, VCL::ValueDecl* from, VCL::ValueDecl* to) : TreeTransform{ sema }, from{ from }, to{ to } {}

    VCL::Expr* TransformDeclRefExpr(VCL::DeclRefExpr* expr) override {
        if (expr->GetValueDecl() != from)
            return TreeTransform::TransformDeclRefExpr(expr);
        VCL::Decl* copy = GetTransformedDecl(to);
        return GetSema().ActOnDeclRefExpr((VCL::ValueDecl*)(copy ? copy : to), expr->GetSourceRange());
    }

private:
    VCL::ValueDecl* from;
    VCL::ValueDecl* to;
};

static MakeTransform Rename(llvm::StringRef from, llvm::StringRef to) {
    return [=](VCL::Sema& sema, VCL::TranslationUnitDecl* source) {
        auto* fromDecl = (VCL::ValueDecl*)FindTopLevelDecl(source, from);
        auto* toDecl = (VCL::ValueDecl*)FindTopLevelDecl(source, to);
        REQUIRE(fromDecl != nullptr);
        REQUIRE(toDecl != nullptr);
        return std::make_unique<RenameTransform>(sema, fromDecl, toDecl);
    };
}

// Sets codegen flags on parameters, by parameter name.
class FlagTransform : public VCL::TreeTransform {
public:
    using Flags = std::vector<std::pair<std::string, uint32_t>>;

    FlagTransform(VCL::Sema& sema, Flags flags) : TreeTransform{ sema }, flags{ std::move(flags) } {}

    VCL::Decl* TransformParamDecl(VCL::ParamDecl* decl) override {
        auto* newDecl = (VCL::ParamDecl*)TreeTransform::TransformParamDecl(decl);
        if (newDecl == nullptr)
            return nullptr;
        for (auto& [name, flag] : flags)
            if (decl->GetIdentifierInfo()->GetName() == name)
                newDecl->SetCodeGenFlag((VCL::ParamDecl::CodeGenFlags)flag);
        return newDecl;
    }

private:
    Flags flags;
};

// Everything the transform rebuilds: globals with initializers, a const, records, arrays, helpers
// with `inout` and read-only aggregate parameters, a template, an intrinsic, casts, every statement.
static const char* programSource =
    "struct Pair { float32 x; float32 y; }\n"
    "Array<float32, 4> history;\n"
    "Pair state;\n"
    "uint32 count = 1;\n"
    "const Array<float32, 2> weights = { 0.5, 2.0 };\n"
    "out float32 o_sum;\n"
    "out float32 o_pair;\n"
    "out uint32 o_count;\n"
    "template<typename T>\n"
    "T Twice(T value) { return value + value; }\n"
    "void Push(inout Pair p, float32 v) {\n"
    "    p.x = p.y;\n"
    "    p.y = v;\n"
    "}\n"
    "float32 Sum(Array<float32, 4> values) {\n"
    "    float32 total = 0.0;\n"
    "    for (int32 i = 0; i < 4; ++i) {\n"
    "        if (values[i] < 1.0)\n"
    "            continue;\n"
    "        total += values[i];\n"
    "    }\n"
    "    return total;\n"
    "}\n"
    "[EntryPoint] void Main() {\n"
    "    history[count % 4] = Twice((float32)count) * weights[0];\n"
    "    Push(state, max(state.y, (float32)count * weights[1]));\n"
    "    uint32 k = 0;\n"
    "    while (k < 10) {\n"
    "        k += 1;\n"
    "        if (k >= 3) {\n"
    "            break;\n"
    "        } else {\n"
    "            int32 unused = 0;\n"
    "        }\n"
    "    }\n"
    "    count = count + k - 2;\n"
    "    o_sum = Sum(history);\n"
    "    o_pair = state.x + state.y;\n"
    "    o_count = count;\n"
    "}\n";

struct ProgramOutputs {
    float sum;
    float pair;
    uint32_t count;
    bool operator==(const ProgramOutputs&) const = default;
};

static std::vector<ProgramOutputs> RunProgram(llvm::orc::ThreadSafeModule module, int calls) {
    VCL::ExecutionSession session{};
    REQUIRE(session.SubmitModule(std::move(module)));
    auto* main = (void(*)())session.Lookup("Main");
    auto* sum = (float*)session.Lookup("o_sum");
    auto* pair = (float*)session.Lookup("o_pair");
    auto* count = (uint32_t*)session.Lookup("o_count");
    REQUIRE(main != nullptr);
    REQUIRE(sum != nullptr);
    REQUIRE(pair != nullptr);
    REQUIRE(count != nullptr);
    std::vector<ProgramOutputs> outputs{};
    for (int i = 0; i < calls; ++i) {
        main();
        outputs.push_back({ *sum, *pair, *count });
    }
    return outputs;
}

TEST_CASE("Tree Transform", "[Sema][TreeTransform]") {
    SECTION("The identity transform gives the same program") {
        std::vector<ProgramOutputs> expected = RunProgram(MakeModuleFromSource(programSource, false), 6);
        std::vector<ProgramOutputs> actual = RunProgram(EmitTransformedSource(programSource, Identity()), 6);
        REQUIRE(expected.back().count == 7);
        REQUIRE(actual == expected);
    }
    SECTION("A subclass redirects references") {
        const char* src =
            "float32 a = 2.0;\n"
            "float32 b = 5.0;\n"
            "out float32 o_value;\n"
            "float32 PlusA(float32 x) { return x + a; }\n"
            "[EntryPoint] void Main() { o_value = PlusA(a) * 10.0; }\n";

        VCL::ExecutionSession session{};
        REQUIRE(session.SubmitModule(EmitTransformedSource(src, Rename("a", "b"))));
        auto* main = (void(*)())session.Lookup("Main");
        auto* value = (float*)session.Lookup("o_value");
        REQUIRE(main != nullptr);
        REQUIRE(value != nullptr);
        main();
        REQUIRE(*value == 100.0f); // (b + b) * 10, where the source computes (a + a) * 10 = 40
    }
    SECTION("Only the transformed translation unit is emitted, under the source's names") {
        const char* src =
            "out float32 o_value;\n"
            "float32 Helper(float32 x) { return x * 3.0; }\n"
            "[EntryPoint] void Main() { o_value = Helper(2.0); }\n";

        llvm::orc::ThreadSafeModule module = EmitTransformedSource(src, Identity());
        module.withModuleDo([](llvm::Module& m) {
            int helpers = 0;
            for (llvm::Function& function : m)
                if (function.getName().contains("Helper"))
                    ++helpers;
            REQUIRE(helpers == 1);
            REQUIRE(m.getFunction("Main") != nullptr);
        });

        VCL::ExecutionSession session{};
        REQUIRE(session.SubmitModule(std::move(module)));
        auto* main = (void(*)())session.Lookup("Main");
        REQUIRE(main != nullptr);
        main();
        REQUIRE(*(float*)session.Lookup("o_value") == 6.0f);
    }
}

TEST_CASE("Template With Non Dependent Aggregate Parameter", "[Sema][TreeTransform][Template]") {
    // Instantiation rebuilds parameters from the type as written. An aggregate that isn't dependent
    // is already a reference in the template: it used to be wrapped a second time (which happened to
    // work) and now stays a single reference.
    VCL::ExecutionSession session{};
    REQUIRE(session.SubmitModule(MakeModuleFromSource(
        "Array<float32, 4> table = { 1.0, 2.0, 3.0, 4.0 };\n"
        "out float32 o_value;\n"
        "template<typename T>\n"
        "T Scaled(Array<float32, 4> values, T scale) { return (T)values[2] * scale; }\n"
        "[EntryPoint] void Main() { o_value = Scaled(table, 2.0); }\n")));
    auto* main = (void(*)())session.Lookup("Main");
    REQUIRE(main != nullptr);
    main();
    REQUIRE(*(float*)session.Lookup("o_value") == 6.0f);
}

TEST_CASE("Parameter CodeGen Flags", "[Sema][TreeTransform][CodeGen]") {
    const char* src =
        "struct State { Array<float32, 8> buffer; uint32 index; }\n"
        "void Process(inout State self, Array<float32, 8> input, out float32 result, float32 gain) {\n"
        "    self.buffer[self.index] = input[self.index] * gain;\n"
        "    result = self.buffer[0];\n"
        "}\n"
        "void Plain(inout State state, out float32 value) { value = state.buffer[1]; }\n";

    using F = VCL::ParamDecl;
    FlagTransform::Flags flags{
        { "self", F::NoAlias | F::NoCapture | F::Aligned | F::Dereferenceable },
        { "input", F::NoAlias | F::NoCapture | F::ReadOnly },
        { "result", F::NoAlias | F::NoCapture },
        { "gain", F::NoAlias }, // passed by value: ignored
    };
    llvm::orc::ThreadSafeModule module = EmitTransformedSource(src, [&](VCL::Sema& sema, VCL::TranslationUnitDecl*) {
        return std::make_unique<FlagTransform>(sema, flags);
    });

    module.withModuleDo([](llvm::Module& m) {
        llvm::Function* process = nullptr;
        llvm::Function* plain = nullptr;
        for (llvm::Function& function : m) {
            if (function.getName().ends_with(".Process"))
                process = &function;
            if (function.getName().ends_with(".Plain"))
                plain = &function;
        }
        REQUIRE(process != nullptr);
        REQUIRE(plain != nullptr);

        llvm::Argument* self = process->getArg(0);
        REQUIRE(self->hasNoAliasAttr());
        REQUIRE(self->hasNoCaptureAttr());
        REQUIRE(!self->onlyReadsMemory());
        // State: 8 floats and a uint32.
        REQUIRE(self->getDereferenceableBytes() == 36);
        REQUIRE(self->getParamAlign().has_value());
        REQUIRE(self->getParamAlign()->value() == 4);

        llvm::Argument* input = process->getArg(1);
        REQUIRE(input->hasNoAliasAttr());
        REQUIRE(input->hasNoCaptureAttr());
        REQUIRE(input->hasAttribute(llvm::Attribute::ReadOnly));
        REQUIRE(input->getDereferenceableBytes() == 0);
        REQUIRE(!input->getParamAlign().has_value());

        llvm::Argument* result = process->getArg(2);
        REQUIRE(result->hasNoAliasAttr());
        REQUIRE(result->hasNoCaptureAttr());
        REQUIRE(!result->hasAttribute(llvm::Attribute::ReadOnly));

        llvm::Argument* gain = process->getArg(3);
        REQUIRE(gain->getType()->isFloatTy());
        REQUIRE(!gain->hasAttribute(llvm::Attribute::NoAlias));

        // Nothing without flags: a function the transform didn't flag, as all code today.
        for (llvm::Argument& arg : plain->args()) {
            REQUIRE(!arg.hasNoAliasAttr());
            REQUIRE(!arg.hasNoCaptureAttr());
            REQUIRE(arg.getDereferenceableBytes() == 0);
        }
    });
}
