#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>
#include <VCL/CodeGen/Mangler.hpp>

#include <llvm/Transforms/Utils/BasicBlockUtils.h>

VCL::CodeGenFunction::CodeGenFunction(CodeGenModule& cgm) : cgm{ cgm }, builder{ cgm.GetLLVMContext() }, locals{}, breakBBStack{}, continueBBStack{} {
    
}

llvm::Function* VCL::CodeGenFunction::Generate(FunctionDecl* decl, bool imported) {
    llvm::FunctionType* functionType = cgm.GetCGT().ConvertFunctionType(QualType{ decl->GetType() });

    AttributeDefinition* entryPointAD = cgm.GetAttributeTable().GetDefinition(
        cgm.GetIdentifierTable().Get("EntryPoint"));
    AttributeDefinition* noMangleAD = cgm.GetAttributeTable().GetDefinition(
        cgm.GetIdentifierTable().Get("NoMangle"));

    ASTContext& context = imported ? cgm.GetImportedDeclModule(decl)->GetCompilerInstance()->GetASTContext() : cgm.GetASTContext();
    std::string functionName = decl->GetIdentifierInfo()->GetName().str();

    if (!decl->HasAttribute(noMangleAD) && !decl->HasAttribute(entryPointAD))
        functionName = Mangler::MangleFunctionDecl(context, decl);

    function = llvm::cast<llvm::Function>(cgm.GetLLVMModule().getOrInsertFunction(functionName, functionType).getCallee());
    function->setLinkage(llvm::GlobalValue::LinkOnceODRLinkage);

    if (decl->HasAttribute(entryPointAD)) {
        function->setLinkage(llvm::GlobalValue::ExternalLinkage);
        function->setDSOLocal(true);
    }

    int i = 0;
    for (auto it = decl->Begin(); it != decl->End(); ++it) {
        if (it->GetDeclClass() == Decl::ParamDeclClass) {
            ParamDecl* paramDecl = (ParamDecl*)it.Get();
            function->getArg(i)->setName(paramDecl->GetIdentifierInfo()->GetName());
            ++i;
        }
    }

    if (decl->GetBody() == nullptr || (imported && !decl->HasFunctionFlag(FunctionDecl::IsTemplateSpecialization)))
        return function;

    llvm::BasicBlock* bb = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "entry", function);
    builder.SetInsertPoint(bb);

    i = 0;
    for (auto it = decl->Begin(); it != decl->End(); ++it) {
        if (it->GetDeclClass() == Decl::ParamDeclClass) {
            ParamDecl* paramDecl = (ParamDecl*)it.Get();
            llvm::Argument* arg = function->getArg(i);
            if (paramDecl->GetValueType().GetType()->GetTypeClass() == Type::ReferenceTypeClass) {
                locals.insert(std::make_pair(paramDecl, arg));
            } else {
                llvm::AllocaInst* alloca = GenerateAllocaInst(arg->getType(), paramDecl->GetIdentifierInfo()->GetName());
                builder.CreateStore(arg, alloca);
                locals.insert(std::make_pair(paramDecl, alloca));
            }
            ++i;
        }
    }

    if (!GenerateStmt(decl->GetBody()))
        return nullptr;
    
    bool isFunctionVoid = false;
    Type* returnType = decl->GetType()->GetReturnType().GetType();
    if (returnType->GetTypeClass() == Type::BuiltinTypeClass)
        isFunctionVoid = ((BuiltinType*)returnType)->GetKind() == BuiltinType::Void;

    for (llvm::BasicBlock& bb : *function) {
        if (bb.getTerminator() == nullptr) {
            builder.SetInsertPoint(&bb);
            if (isFunctionVoid)
                builder.CreateRetVoid();
            else
                builder.CreateUnreachable();
        }
    }

    llvm::EliminateUnreachableBlocks(*function);

    return function;
}

llvm::AllocaInst* VCL::CodeGenFunction::GenerateAllocaInst(llvm::Type* type, llvm::StringRef name) {
    llvm::IRBuilder<>::InsertPointGuard ipGuard{ builder };
    builder.SetInsertPoint(builder.GetInsertBlock()->getFirstInsertionPt());
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    llvm::AllocaInst* alloca = builder.CreateAlloca(type, nullptr, name);
    return alloca;
}

llvm::Value* VCL::CodeGenFunction::GetDeclValue(Decl* decl) {
    if (locals.count(decl))
        return locals.at(decl);
    return cgm.GetGlobalDeclValue(decl);
}

void VCL::CodeGenFunction::PushBreakBB(llvm::BasicBlock* breakBB) {
    breakBBStack.push_back(breakBB);
}

void VCL::CodeGenFunction::PopBreakBB() {
    breakBBStack.pop_back();
}

void VCL::CodeGenFunction::PushContinueBB(llvm::BasicBlock* continueBB) {
    continueBBStack.push_back(continueBB);
}

void VCL::CodeGenFunction::PopContinueBB() {
    continueBBStack.pop_back();
}