#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>

#include "../Common/ExpectedDiagnostic.hpp"


template<VCL::Diagnostic::DiagnosticMsg Message>
void CheckForError(const char* src) {
    ExpectedDiagnostic<Message> consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateAttributeTable();
    cc.CreateDirectiveRegistry();
    cc.CreateTypeCache();
    cc.CreateSourceManager();
    
    VCL::Source* source = cc.GetSourceManager().LoadFromMemory(src, "buff");
    REQUIRE(source != nullptr);

    VCL::ParseSyntaxOnlyAction act{};

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    instance->ExecuteAction(act);
    instance->EndSource();

    consumer.Require();
}

TEST_CASE("Redeclaration", "[Sema]") { 
    CheckForError<VCL::Diagnostic::Redeclaration>("float32 var; float32 var;");
}

TEST_CASE("Reserved Identifier", "[Sema]") { 
    CheckForError<VCL::Diagnostic::ReservedIdentifier>("float32 return;");
}

TEST_CASE("Too Many Template Argument", "[Sema]") { 
    CheckForError<VCL::Diagnostic::TooManyTemplateArgument>("Vec<float32, float32> var;");
}

TEST_CASE("Not Enough Template Argument", "[Sema]") { 
    CheckForError<VCL::Diagnostic::NotEnoughTemplateArgument>("Array<float32> var;");
}

TEST_CASE("Missing Template Argument", "[Sema]") { 
    CheckForError<VCL::Diagnostic::MissingTemplateArgument>("Vec var;");
}

TEST_CASE("Wrong Type Of Template Argument", "[Sema]") { 
    CheckForError<VCL::Diagnostic::TemplateArgumentWrongType>("Vec<2> var;");
}

TEST_CASE("Builtin Type Is Not Templated", "[Sema]") { 
    CheckForError<VCL::Diagnostic::BuiltinTypeIsNotTemplated>("float32<2> var;");
}

TEST_CASE("Identifier Undefined", "[Sema]") { 
    CheckForError<VCL::Diagnostic::IdentifierUndefined>("ayaya var;");
}

TEST_CASE("Invalid Cast", "[Sema]") { 
    CheckForError<VCL::Diagnostic::InvalidCast>("Span<float32> v1 = 3.0;");
}

TEST_CASE("Invalid Vector Cast", "[Sema]") { 
    CheckForError<VCL::Diagnostic::InvalidVectorCast>("Vec<float32> v1 = 3.0; float32 v2 = v1;");
}

TEST_CASE("Does Not Take Template Argument List", "[Sema]") { 
    CheckForError<VCL::Diagnostic::DoesNotTakeTemplateArgList>("struct MyStruct { float32 v; } MyStruct<float32> var;");
}

TEST_CASE("Not A Type Declaration", "[Sema]") { 
    CheckForError<VCL::Diagnostic::NotTypeDecl>("void MyFunc(); MyFunc var;");
}

TEST_CASE("Template Redeclared", "[Sema]") { 
    CheckForError<VCL::Diagnostic::TemplateRedeclared>("template<typename T, typename T> struct MyStruct { float32 v; }");
}

TEST_CASE("Expression Does Not Evaluate At Compile Time", "[Sema]") { 
    CheckForError<VCL::Diagnostic::ExprDoesNotEvaluate>("float32 MyFunc(); float32 v = MyFunc();");
}

TEST_CASE("Attribute Invalid Use", "[Sema]") { 
    CheckForError<VCL::Diagnostic::AttrInvalidUse>("void MyFunc() { in float32 v; }");
}

TEST_CASE("Initializer Input Variable", "[Sema]") { 
    CheckForError<VCL::Diagnostic::InitializerInputVarDecl>("in float32 v = 1.0;");
}

TEST_CASE("Assignment Not L-Value", "[Sema]") { 
    CheckForError<VCL::Diagnostic::AssignmentNotLValue>("void MyFunc() { 4.0 = 2.0; }");
}

TEST_CASE("Assignment To Const", "[Sema]") { 
    CheckForError<VCL::Diagnostic::AssignmentConstValue>("void MyFunc() { const float32 a = 2.0; a = 3.0; }");
}

TEST_CASE("Assignment Input Variable", "[Sema]") { 
    CheckForError<VCL::Diagnostic::AssignmentInputValue>("in float32 v; void MyFunc() { v = 2.0; }");
}

TEST_CASE("Missing Return", "[Sema]") { 
    CheckForError<VCL::Diagnostic::MissingReturnStmt>("float32 MyFunc() { }");
}

TEST_CASE("Invalid Return Stmt No Expression", "[Sema]") { 
    CheckForError<VCL::Diagnostic::InvalidReturnStmtNoExpr>("float32 MyFunc() { return; }");
}

TEST_CASE("Invalid Return Stmt With Expression", "[Sema]") { 
    CheckForError<VCL::Diagnostic::InvalidReturnStmtExpr>("void MyFunc() { return 1.0; }");
}

TEST_CASE("Too Many Arguments", "[Sema]") { 
    CheckForError<VCL::Diagnostic::TooManyArgument>("void MyFunc1(float32 v); void MyFunc2() { MyFunc1(1.0, 2.0); }");
}

TEST_CASE("Missing Arguments", "[Sema]") { 
    CheckForError<VCL::Diagnostic::MissingArgument>("void MyFunc1(float32 v); void MyFunc2() { MyFunc1(); }");
}

TEST_CASE("Expression Must Be A L-Value", "[Sema]") { 
    CheckForError<VCL::Diagnostic::MustBeLValue>("void MyFunc1(out float32 v); void MyFunc2() { MyFunc1(1.0); }");
}

TEST_CASE("Incorrect Type", "[Sema]") { 
    CheckForError<VCL::Diagnostic::IncorrectType>("void MyFunc1(out float32 v); void MyFunc2() { int32 v; MyFunc1(v); }");
}

TEST_CASE("Qualifier Dropped", "[Sema]") { 
    CheckForError<VCL::Diagnostic::QualifierDropped>("void MyFunc1(out float32 v); void MyFunc2() { const float32 v; MyFunc1(v); }");
}

TEST_CASE("Must Be A Struct", "[Sema]") { 
    CheckForError<VCL::Diagnostic::MustHaveStructType>("void MyFunc1() { float32 v; v.a = 1.0; }");
}

TEST_CASE("Missing Member", "[Sema]") { 
    CheckForError<VCL::Diagnostic::MissingMember>("struct MyStruct { float32 v; } void MyFunc1() { MyStruct s; s.v2 = 1.0; }");
}

TEST_CASE("Statement Never Reached", "[Sema]") {
    CheckForError<VCL::Diagnostic::StatementNeverReached>("void MyFunc() { return; float32 v = 1.0; }");
}

TEST_CASE("Field Assignment On Non-LValue", "[Sema]") {
    CheckForError<VCL::Diagnostic::MustBeLValue>("struct MyStruct { float32 v; } MyStruct GetStruct(); void MyFunc() { GetStruct().v = 1.0; }");
}

TEST_CASE("Cast To Void Type", "[Sema]") {
    CheckForError<VCL::Diagnostic::InvalidCast>("void MyFunc() { float32 v = 1.0; void result = v; }");
}

TEST_CASE("Cast From Void Type", "[Sema]") {
    CheckForError<VCL::Diagnostic::InvalidCast>("void MyFunc(); void MyFunc2() { float32 v = MyFunc(); }");
}

TEST_CASE("Assignment To Function Call Result", "[Sema]") {
    CheckForError<VCL::Diagnostic::AssignmentNotLValue>("float32 GetValue(); void MyFunc() { GetValue() = 1.0; }");
}

TEST_CASE("InOut Parameter With Const Variable", "[Sema]") {
    CheckForError<VCL::Diagnostic::QualifierDropped>("void MyFunc(inout float32 p); void TestFunc() { const float32 v = 1.0; MyFunc(v); }");
}

TEST_CASE("Global Variable With Non-Constant Initializer", "[Sema]") {
    CheckForError<VCL::Diagnostic::ExprDoesNotEvaluate>("float32 GetValue(); float32 global = GetValue();");
}

TEST_CASE("Vector Cast To Scalar In Assignment", "[Sema]") {
    CheckForError<VCL::Diagnostic::InvalidVectorCast>("void MyFunc() { Vec<float32> v; float32 s = v; }");
}

TEST_CASE("Struct Member Access On Vector", "[Sema]") {
    CheckForError<VCL::Diagnostic::MustHaveStructType>("void MyFunc() { Vec<float32> v; v.x = 1.0; }");
}

TEST_CASE("Template Argument On Regular Struct", "[Sema]") {
    CheckForError<VCL::Diagnostic::DoesNotTakeTemplateArgList>("struct MyStruct { float32 v; } MyStruct<float32> instance;");
}

TEST_CASE("Missing Both Template Arguments For Array", "[Sema]") {
    CheckForError<VCL::Diagnostic::MissingTemplateArgument>("Array arr;");
}

TEST_CASE("Function With Undefined Return Type", "[Sema]") {
    CheckForError<VCL::Diagnostic::IdentifierUndefined>("UndefinedType MyFunc() { }");
}

TEST_CASE("Variable Declaration With Undefined Type In Template", "[Sema]") {
    CheckForError<VCL::Diagnostic::IdentifierUndefined>("UndefinedStruct<float32> var;");
}