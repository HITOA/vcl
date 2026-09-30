#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>
#include <VCL/CodeGen/Mangler.hpp>

#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/IR/Verifier.h>


VCL::CodeGenFunction::CodeGenFunction(CodeGenModule& cgm) 
    : cgm{ cgm }, builder{ cgm.GetLLVMContext() }, locals{}, breakBBStack{}, continueBBStack{}, strictIEEE{ false } {
    
}

llvm::Function* VCL::CodeGenFunction::Generate(FunctionDecl* decl, bool imported) {
    llvm::FunctionType* functionType = cgm.GetCGT().ConvertFunctionType(QualType{ decl->GetType() });

    AttributeDefinition* entryPointAD = cgm.GetAttributeTable().GetDefinition(
        cgm.GetIdentifierTable().Get("EntryPoint"));
    AttributeDefinition* noMangleAD = cgm.GetAttributeTable().GetDefinition(
        cgm.GetIdentifierTable().Get("NoMangle"));

    AttributeDefinition* strictIEEEAD = cgm.GetAttributeTable().GetDefinition(
        cgm.GetIdentifierTable().Get("StrictIEEE"));
    AttributeDefinition* allowApproxFuncAD = cgm.GetAttributeTable().GetDefinition(
        cgm.GetIdentifierTable().Get("AllowApproxFunctions"));

    strictIEEE = decl->HasAttribute(strictIEEEAD) != nullptr;
    bool allowApproxFunc = decl->HasAttribute(allowApproxFuncAD) != nullptr;

    ASTContext& context = imported ? cgm.GetImportedDeclModule(decl)->GetCompilerInstance()->GetASTContext() : cgm.GetASTContext();
    std::string functionName = decl->GetIdentifierInfo()->GetName().str();

    if (!decl->HasAttribute(noMangleAD) && !decl->HasAttribute(entryPointAD))
        functionName = Mangler::MangleFunctionDecl(context, decl);

    function = llvm::cast<llvm::Function>(cgm.GetLLVMModule().getOrInsertFunction(functionName, functionType).getCallee());

    // Several CodeGenModules can emit into the same llvm::Module (vcl-graph compiles every node
    // into one module), so an imported template specialization may already have a body.
    if (!function->empty()) {
        Decl* existing = CodeGenModule::GetSymbolDecl(function);
        if (function->getFunctionType() != functionType || (existing != nullptr && existing != decl)) {
            cgm.GetDiagnosticReporter().Error(Diagnostic::SymbolNameCollision, functionName)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        return function;
    }
    CodeGenModule::SetSymbolDecl(function, decl);

    function->setLinkage(llvm::GlobalValue::InternalLinkage);

    bool isSpecialization = decl->HasFunctionFlag(FunctionDecl::IsTemplateSpecialization);
    if (decl->HasAttribute(entryPointAD)) {
        function->setLinkage(llvm::GlobalValue::ExternalLinkage);
        function->setDSOLocal(true);
    } else if (imported && isSpecialization) {
        // Instantiated here from an imported template: every importer emits its own copy.
        function->setLinkage(llvm::GlobalValue::LinkOnceAnyLinkage);
    } else if (imported) {
        // Defined in the imported module, which is linked in later.
        function->setLinkage(llvm::GlobalValue::ExternalLinkage);
    } else if (decl->IsExported() && !isSpecialization) {
        // Importers reference it by name, so it must survive linking.
        function->setLinkage(llvm::GlobalValue::ExternalLinkage);
    }

    if (!strictIEEE) {
        function->addFnAttr("denormal-fp-math", "positive-zero");
        function->addFnAttr("denormal-fp-math-f32", "positive-zero");
        function->addFnAttr("unsafe-fp-math", "true");
        function->addFnAttr("no-infs-fp-math", "true");
        function->addFnAttr("no-nans-fp-math", "true");
        function->addFnAttr("no-signed-zeros-fp-math", "true");
    }

    if (allowApproxFunc) {
        function->addFnAttr("approx-func-fp-math", "true");
    }

    // The function attributes above only reach the backend. The IR optimizers (InstCombine,
    // reassociation, FMA contraction, vectorizers) read the per-instruction flags.
    llvm::FastMathFlags fmf{};
    if (!strictIEEE) {
        fmf.setFast();
        fmf.setApproxFunc(allowApproxFunc);
    }
    builder.setFastMathFlags(fmf);

    int i = 0;
    for (auto it = decl->Begin(); it != decl->End(); ++it) {
        if (it->GetDeclClass() == Decl::ParamDeclClass) {
            ParamDecl* paramDecl = (ParamDecl*)it.Get();
            function->getArg(i)->setName(paramDecl->GetIdentifierInfo()->GetName());
            GenerateParamAttributes(paramDecl, function->getArg(i));
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
                llvm::AllocaInst* alloca = GenerateAllocaInst(paramDecl->GetValueType(), paramDecl->GetIdentifierInfo()->GetName());
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

    if (llvm::verifyFunction(*function, &llvm::errs())) {
        function->dump();
        cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    return function;
}

void VCL::CodeGenFunction::GenerateParamAttributes(ParamDecl* decl, llvm::Argument* arg) {
    Type* type = decl->GetValueType().GetType();
    if (decl->GetCodeGenFlags() == ParamDecl::NoCodeGenFlags || type->GetTypeClass() != Type::ReferenceTypeClass)
        return;
    llvm::LLVMContext& context = cgm.GetLLVMContext();
    if (decl->HasCodeGenFlag(ParamDecl::NoAlias))
        arg->addAttr(llvm::Attribute::NoAlias);
    if (decl->HasCodeGenFlag(ParamDecl::NoCapture))
        arg->addAttr(llvm::Attribute::getWithCaptureInfo(context, llvm::CaptureInfo::none()));
    if (decl->HasCodeGenFlag(ParamDecl::ReadOnly))
        arg->addAttr(llvm::Attribute::ReadOnly);
    if (decl->HasCodeGenFlag(ParamDecl::Aligned) || decl->HasCodeGenFlag(ParamDecl::Dereferenceable)) {
        llvm::Type* referencedType = cgm.GetCGT().ConvertType(((ReferenceType*)type)->GetType());
        const llvm::DataLayout& layout = cgm.GetLLVMModule().getDataLayout();
        if (decl->HasCodeGenFlag(ParamDecl::Aligned))
            arg->addAttr(llvm::Attribute::getWithAlignment(context, layout.getABITypeAlign(referencedType)));
        if (decl->HasCodeGenFlag(ParamDecl::Dereferenceable))
            arg->addAttr(llvm::Attribute::getWithDereferenceableBytes(context, layout.getTypeAllocSize(referencedType)));
    }
}

llvm::AllocaInst* VCL::CodeGenFunction::GenerateAllocaInst(llvm::Type* type, llvm::StringRef name) {
    llvm::IRBuilder<>::InsertPointGuard ipGuard{ builder };
    // Always in the entry block: SROA and mem2reg only promote entry-block allocas, and an alloca
    // anywhere else (e.g. a local declared in a loop body) becomes a dynamic stack allocation.
    llvm::BasicBlock& entryBB = function->getEntryBlock();
    builder.SetInsertPoint(&entryBB, entryBB.getFirstInsertionPt());
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    llvm::AllocaInst* alloca = builder.CreateAlloca(type, nullptr, name);
    return alloca;
}

llvm::AllocaInst* VCL::CodeGenFunction::GenerateAllocaInst(QualType type, llvm::StringRef name) {
    llvm::Type* convertedType = cgm.GetCGT().ConvertType(type);
    llvm::AllocaInst* alloca = GenerateAllocaInst(convertedType, name);
    Type* canonicalType = Type::GetCanonicalType(type.GetType());
    if (canonicalType->GetTypeClass() == Type::LanesTypeClass || canonicalType->GetTypeClass() == Type::RecordTypeClass) {
        alloca->setAlignment(llvm::Align{ cgm.GetTarget().GetVectorWidthInByte() });
    }
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