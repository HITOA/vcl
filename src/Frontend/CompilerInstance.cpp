#include <VCL/Frontend/CompilerInstance.hpp>

#include <VCL/AST/ASTContext.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/FrontendAction.hpp>


VCL::CompilerInstance::CompilerInstance(CompilerContext& compilerCtx) : compilerCtx{ compilerCtx }, source{ nullptr }, astCtx{ nullptr } {

}

VCL::CompilerInstance::~CompilerInstance() {
    
}

VCL::CompilerContext& VCL::CompilerInstance::GetCompilerContext() {
    return compilerCtx;
}

VCL::ASTContext& VCL::CompilerInstance::GetASTContext() {
    return *astCtx;
}

bool VCL::CompilerInstance::HasASTContext() {
    return astCtx != nullptr;
}

bool VCL::CompilerInstance::CreateASTContext() {
    astCtx = llvm::makeIntrusiveRefCnt<ASTContext>();
    return true;
}

bool VCL::CompilerInstance::BeginSource(Source* source) {
    if (this->source != nullptr)
        return false;
    this->source = source;
    return true;
}

bool VCL::CompilerInstance::EndSource() {
    this->source = nullptr;
    return true;
}

bool VCL::CompilerInstance::HasSource() {
    return source != nullptr;
}

VCL::Source* VCL::CompilerInstance::GetSource() {
    return source;
}

bool VCL::CompilerInstance::ExecuteAction(FrontendAction& action) {
    if (!action.Prepare(this))
        return false;
    return action.Execute();
}