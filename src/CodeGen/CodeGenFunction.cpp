#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>

VCL::CodeGenFunction::CodeGenFunction(CodeGenModule& cgm) : cgm{ cgm }, builder{ cgm.GetLLVMContext() } {

}

llvm::Function* VCL::CodeGenFunction::Generate(FunctionDecl* decl) {
    llvm::FunctionType* functionType = cgm.GetCGT().ConvertFunctionType(QualType{ decl->GetType() });
    function = llvm::Function::Create(
        functionType, llvm::GlobalValue::ExternalLinkage, decl->GetIdentifierInfo()->GetName(), cgm.GetLLVMModule());
    
    int i = 0;
    for (auto it = decl->Begin(); it != decl->End(); ++it) {
        if (it->GetDeclClass() == Decl::ParamDeclClass) {
            ParamDecl* paramDecl = (ParamDecl*)it.Get();
            function->getArg(i)->setName(paramDecl->GetIdentifierInfo()->GetName());
            ++i;
        }
    }

    if (decl->GetBody() == nullptr)
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
    if (isFunctionVoid && bb->getTerminator() == nullptr) {
        builder.SetInsertPoint(bb);
        builder.CreateRetVoid();
    }

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