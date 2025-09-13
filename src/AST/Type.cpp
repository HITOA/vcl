#include <VCL/AST/Type.hpp>

#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/AST/Template.hpp>


void VCL::TemplateSpecializationType::Profile(llvm::FoldingSetNodeID& id, TemplateDecl* decl, TemplateArgumentList* args) {
    id.AddPointer(decl);
    for (auto& arg : args->GetArgs()) {
        switch (arg.GetKind()) {
            case TemplateArgument::Type:
                id.AddInteger((uintptr_t)arg.GetType().GetAsOpaquePtr());
                break;
            case TemplateArgument::Integral:
                id.AddInteger(arg.GetIntegral().Get<uint64_t>());
                break;
            case TemplateArgument::Expression:
                id.AddPointer(arg.GetExpr());
                break;
            default:
                id.AddPointer(&arg);
                break;
        }
    }
}