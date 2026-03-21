#include <VCL/Sema/Sema.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/Identifier.hpp>
#include <VCL/AST/ExprEvaluator.hpp>
#include <VCL/AST/TypePrinter.hpp>
#include <VCL/Sema/Template.hpp>
#include <VCL/Frontend/ModuleCache.hpp>
#include <VCL/Frontend/CompilerContext.hpp>

#include <llvm/ADT/SmallPtrSet.h>


VCL::Sema::SemaScopeGuard::SemaScopeGuard(Sema& sema, DeclContext* context) : sema{ sema }, context{ context } {}

VCL::Sema::SemaScopeGuard::SemaScopeGuard(SemaScopeGuard&& other) : sema{ other.sema }, context{ std::move(other.context) } {
    other.context = nullptr;
}

VCL::Sema::SemaScopeGuard::~SemaScopeGuard() {
    Release();
}

void VCL::Sema::SemaScopeGuard::Release() {
    if (context)
        sema.PopDeclContextScope(context);
}

VCL::Sema::Sema(CompilerContext& cc, ASTContext& astContext, DiagnosticReporter& diagnosticReporter, IdentifierTable& identifierTable, 
        DirectiveRegistry& directiveRegistry, SymbolTable& exportedSymbols, ModuleTable& importedModules, DefineTable& definedValues) 
        : cc{ cc }, astContext{ astContext }, diagnosticReporter{ diagnosticReporter }, identifierTable{ identifierTable }, 
            directiveRegistry{ directiveRegistry }, exportedSymbols{ exportedSymbols }, importedModules{ importedModules }, definedValues{ definedValues } {
    translationUnitScope = sm.EmplaceScopeFront(astContext.GetTranslationUnitDecl());
    AddIntrinsicTemplateDecl();
}

VCL::TemplateSpecializationType* VCL::Sema::CreateVectorTemplateSpecializationType(Type* ofType) {
    TemplateArgumentList* argList = TemplateArgumentList::Create(astContext, { TemplateArgument{ ofType } }, SourceRange{});
    TemplateDecl* decl = LookupTemplateDecl(SymbolRef{ identifierTable.GetKeyword(TokenKind::Keyword_Vec) });
    return astContext.GetTypeCache().GetOrCreateTemplateSpecializationType(decl, argList);
}

VCL::TemplateSpecializationType* VCL::Sema::CreateLanesTemplateSpecializationType(Type* ofType) {
    TemplateArgumentList* argList = TemplateArgumentList::Create(astContext, { TemplateArgument{ ofType } }, SourceRange{});
    TemplateDecl* decl = LookupTemplateDecl(SymbolRef{ identifierTable.GetKeyword(TokenKind::Keyword_Lanes) });
    return astContext.GetTypeCache().GetOrCreateTemplateSpecializationType(decl, argList);
}

void VCL::Sema::AddIntrinsicTypes() {
    IdentifierInfo* ofTypeIdentifier = identifierTable.Get("T");
    IdentifierInfo* ofSizeIdentifier = identifierTable.Get("Size");
    BuiltinType* ofSizeType = astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt64);

    // Vec
    IntrinsicTypeDecl* vecDecl = IntrinsicTypeDecl::Create(astContext, identifierTable.GetKeyword(TokenKind::Keyword_Vec));
    NamedDecl* vecOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateParameterList* vecParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &vecOfType, 1 }, SourceRange{});
    TemplateDecl* templatedVecDecl = TemplateDecl::Create(astContext, vecParamList, SourceRange{});
    templatedVecDecl->SetTemplatedNamedDecl(vecDecl);
    if (!AddDeclToScopeAndContext(templatedVecDecl))
        return;

    // Vec
    IntrinsicTypeDecl* lanesDecl = IntrinsicTypeDecl::Create(astContext, identifierTable.GetKeyword(TokenKind::Keyword_Lanes));
    NamedDecl* lanesOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateParameterList* lanesParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &lanesOfType, 1 }, SourceRange{});
    TemplateDecl* templatedLanesDecl = TemplateDecl::Create(astContext, lanesParamList, SourceRange{});
    templatedLanesDecl->SetTemplatedNamedDecl(lanesDecl);
    if (!AddDeclToScopeAndContext(templatedLanesDecl))
        return;

    // Array
    IntrinsicTypeDecl* arrayDecl = IntrinsicTypeDecl::Create(astContext, identifierTable.GetKeyword(TokenKind::Keyword_Array));
    NamedDecl* arrayOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    NamedDecl* arrayOfSize = NonTypeTemplateParamDecl::Create(astContext, ofSizeType, ofSizeIdentifier, SourceRange{});
    llvm::SmallVector<NamedDecl*> arrayParams{ arrayOfType, arrayOfSize };
    TemplateParameterList* arrayParamList = TemplateParameterList::Create(astContext, arrayParams, SourceRange{});
    TemplateDecl* templatedArrayDecl = TemplateDecl::Create(astContext, arrayParamList, SourceRange{});
    templatedArrayDecl->SetTemplatedNamedDecl(arrayDecl);
    if (!AddDeclToScopeAndContext(templatedArrayDecl))
        return;

    // Span
    IntrinsicTypeDecl* spanDecl = IntrinsicTypeDecl::Create(astContext, identifierTable.GetKeyword(TokenKind::Keyword_Span));
    NamedDecl* spanOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateParameterList* spanParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &spanOfType, 1 }, SourceRange{});
    TemplateDecl* templatedSpanDecl = TemplateDecl::Create(astContext, spanParamList, SourceRange{});
    templatedSpanDecl->SetTemplatedNamedDecl(spanDecl);
    if (!AddDeclToScopeAndContext(templatedSpanDecl))
        return;
}

void VCL::Sema::AddIntrinsicMathFunction(FunctionDecl::IntrinsicID intrinsicID, llvm::StringRef name, uint32_t argCount) {
    IdentifierInfo* nameIdentifier = identifierTable.Get(name);
    IdentifierInfo* ofTypeIdentifier = identifierTable.Get("T");

    NamedDecl* ofType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateTypeParamType* ofTypeType = astContext.GetTypeCache().GetOrCreateTemplateTypeParamType((TemplateTypeParamDecl*)ofType);
    TemplateParameterList* templateParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &ofType, 1 }, SourceRange{});

    FunctionDecl* functionDecl = FunctionDecl::Create(astContext, nameIdentifier, intrinsicID);

    llvm::SmallVector<QualType> paramsType{};
    for (int i = 0; i < argCount; ++i) {
        paramsType.push_back(ofTypeType);
        std::string paramName = "Arg_" + std::to_string(i);
        IdentifierInfo* paramIdentifier = identifierTable.Get(paramName);
        ParamDecl* paramDecl = ParamDecl::Create(astContext, ofTypeType, paramIdentifier, VarDecl::VarAttrBitfield{}, SourceRange{});
        functionDecl->InsertBack(paramDecl);
    }

    FunctionType* type = astContext.GetTypeCache().GetOrCreateFunctionType(ofTypeType, paramsType);
    functionDecl->SetType(type);

    TemplateDecl* templateDecl = TemplateDecl::Create(astContext, templateParamList, SourceRange{});
    templateDecl->InsertBack(functionDecl);
    templateDecl->SetTemplatedNamedDecl(functionDecl);

    if (!AddDeclToScopeAndContext(templateDecl))
        diagnosticReporter.Error(Diagnostic::MissingImplementation)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
}

void VCL::Sema::AddIntrinsicFunction(FunctionDecl::IntrinsicID intrinsicID, llvm::StringRef name) {
    IdentifierInfo* nameIdentifier = identifierTable.Get(name);
    IdentifierInfo* ofTypeIdentifier = identifierTable.Get("T");

    NamedDecl* ofType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateTypeParamType* ofTypeType = astContext.GetTypeCache().GetOrCreateTemplateTypeParamType((TemplateTypeParamDecl*)ofType);
    TemplateParameterList* templateParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &ofType, 1 }, SourceRange{});

    FunctionDecl* functionDecl = FunctionDecl::Create(astContext, nameIdentifier, intrinsicID);

    Type* returnType = ofTypeType;
    llvm::SmallVector<QualType> paramsType{};

    switch (intrinsicID) {
        case FunctionDecl::IntrinsicID::Unpack: {
            returnType = CreateLanesTemplateSpecializationType(ofTypeType);
            Type* argType = CreateVectorTemplateSpecializationType(ofTypeType);
            argType = astContext.GetTypeCache().GetOrCreateReferenceType(argType);
            paramsType.push_back(argType);
            IdentifierInfo* paramIdentifier = identifierTable.Get("Arg_0");
            ParamDecl* paramDecl = ParamDecl::Create(astContext, argType, paramIdentifier, VarDecl::VarAttrBitfield{}, SourceRange{});
            functionDecl->InsertBack(paramDecl);
            break;
        }
        case FunctionDecl::IntrinsicID::Pack: {
            returnType = CreateVectorTemplateSpecializationType(ofTypeType);
            Type* argType = CreateLanesTemplateSpecializationType(ofTypeType);
            argType = astContext.GetTypeCache().GetOrCreateReferenceType(argType);
            paramsType.push_back(argType);
            IdentifierInfo* paramIdentifier = identifierTable.Get("Arg_0");
            ParamDecl* paramDecl = ParamDecl::Create(astContext, argType, paramIdentifier, VarDecl::VarAttrBitfield{}, SourceRange{});
            functionDecl->InsertBack(paramDecl);
            break;
        }
        case FunctionDecl::IntrinsicID::Length: {
            returnType = astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt64);
            paramsType.push_back(ofTypeType);
            IdentifierInfo* paramIdentifier = identifierTable.Get("Arg_0");
            ParamDecl* paramDecl = ParamDecl::Create(astContext, ofTypeType, paramIdentifier, VarDecl::VarAttrBitfield{}, SourceRange{});
            functionDecl->InsertBack(paramDecl);
            break;
        }
        case FunctionDecl::IntrinsicID::Select: {
            returnType = CreateVectorTemplateSpecializationType(ofTypeType);
            Type* param0Type = astContext.GetTypeCache().GetOrCreateVectorType(astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Bool));
            Type* param1Type = CreateVectorTemplateSpecializationType(ofTypeType);
            Type* param2Type = CreateVectorTemplateSpecializationType(ofTypeType);
            paramsType.push_back(param0Type);
            paramsType.push_back(param1Type);
            paramsType.push_back(param2Type);
            IdentifierInfo* param0Identifier = identifierTable.Get("Arg_0");
            IdentifierInfo* param1Identifier = identifierTable.Get("Arg_1");
            IdentifierInfo* param2Identifier = identifierTable.Get("Arg_2");
            ParamDecl* param0Decl = ParamDecl::Create(astContext, param0Type, param0Identifier, VarDecl::VarAttrBitfield{}, SourceRange{});
            ParamDecl* param1Decl = ParamDecl::Create(astContext, param1Type, param1Identifier, VarDecl::VarAttrBitfield{}, SourceRange{});
            ParamDecl* param2Decl = ParamDecl::Create(astContext, param2Type, param2Identifier, VarDecl::VarAttrBitfield{}, SourceRange{});
            functionDecl->InsertBack(param0Decl);
            functionDecl->InsertBack(param1Decl);
            functionDecl->InsertBack(param2Decl);
            break;
        }
        default:
            diagnosticReporter.Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return;
    }

    FunctionType* type = astContext.GetTypeCache().GetOrCreateFunctionType(returnType, paramsType);
    functionDecl->SetType(type);

    TemplateDecl* templateDecl = TemplateDecl::Create(astContext, templateParamList, SourceRange{});
    templateDecl->InsertBack(functionDecl);
    templateDecl->SetTemplatedNamedDecl(functionDecl);

    if (!AddDeclToScopeAndContext(templateDecl))
        diagnosticReporter.Error(Diagnostic::MissingImplementation)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
}

void VCL::Sema::AddIntrinsicTemplateDecl() {
    AddIntrinsicTypes();

    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Sin, "sin", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Cos, "cos", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Tan, "tan", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Sinh, "sinh", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Cosh, "cosh", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Tanh, "tanh", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::ASin, "asin", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::ACos, "acos", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::ATan, "atan", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::ATan2, "atan2", 1);

    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Sqrt, "sqrt", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Log, "log", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Log2, "log2", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Log10, "log10", 1);

    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Exp, "exp", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Exp2, "exp2", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Floor, "floor", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Ceil, "ceil", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Round, "round", 1);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Abs, "abs", 1);
    
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Pow, "pow", 2);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Min, "min", 2);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Max, "max", 2);
    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::FMod, "fmod", 2);

    AddIntrinsicMathFunction(FunctionDecl::IntrinsicID::Fma, "fma", 3);

    AddIntrinsicFunction(FunctionDecl::IntrinsicID::Unpack, "unpack");
    AddIntrinsicFunction(FunctionDecl::IntrinsicID::Pack, "pack");
    AddIntrinsicFunction(FunctionDecl::IntrinsicID::Length, "length");
    AddIntrinsicFunction(FunctionDecl::IntrinsicID::Select, "select");
}

VCL::Sema::SemaScopeGuard VCL::Sema::PushScope(DeclContext* context, bool loopScope) {
    if (!context)
        context = astContext.AllocateNode<DeclContext>(DeclContext::TransientDeclContext);
    PushDeclContextScope(context, loopScope);
    return SemaScopeGuard{ *this, context };
}

bool VCL::Sema::PushDeclContextScope(DeclContext* context, bool loopScope) {
    Scope* parent = sm.GetScopeFront();
    Scope* scope = sm.EmplaceScopeFront(context);
    if (loopScope) {
        scope->SetBreakScope(parent);
        scope->SetContinueScope(parent);
    }
    return true;
}

bool VCL::Sema::PopDeclContextScope(DeclContext* context) {
    if (sm.GetScopeFront()->GetDeclContext() != context)  {
        diagnosticReporter.Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }
    sm.PopScopeFront(context);
    return true;
}

bool VCL::Sema::AddDeclToScope(Decl* decl) {
    return sm.GetScopeFront()->AddDeclInScope(decl);
}

bool VCL::Sema::AddDeclToContext(Decl* decl) {
    Scope* currentScope = sm.GetScopeFront();
    while (currentScope->GetDeclContext() == nullptr) {
        currentScope = currentScope->GetParentScope();
        if (!currentScope)
            return false;
    }
    currentScope->GetDeclContext()->InsertBack(decl);
    return true;
}

bool VCL::Sema::AddDeclToScopeAndContext(Decl* decl) {
    bool r = AddDeclToScope(decl) && AddDeclToContext(decl);
    if (!r) {
        diagnosticReporter.Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }
    return true;
}

bool VCL::Sema::ExportSymbol(Decl* decl, SourceRange range) {
    if (!decl->IsNamedDecl() && !decl->IsTemplateDecl()) {
        diagnosticReporter.Error(Diagnostic::CannotExportUnnamedDecl)
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }
    IdentifierInfo* identifierInfo = nullptr;

    if (decl->IsNamedDecl())
        identifierInfo = ((NamedDecl*)decl)->GetIdentifierInfo();
    else
        identifierInfo = ((TemplateDecl*)decl)->GetTemplatedNamedDecl()->GetIdentifierInfo();

    if (Decl* exportedDecl = exportedSymbols.Get(identifierInfo); exportedDecl != nullptr) {
        diagnosticReporter.Error(Diagnostic::AlreadyExported, identifierInfo->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ exportedDecl->GetSourceRange(), DiagnosticHint::Declared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
    }
    decl->SetExported(true);
    return exportedSymbols.Add(identifierInfo, decl);
}

bool VCL::Sema::ImportModule(Module* module, IdentifierInfo* identifierInfo) {
    if (importedModules.Get(identifierInfo)) {
        if (module == importedModules.Get(identifierInfo))
            return true;
        diagnosticReporter.Error(Diagnostic::ImportDuplicatedNamespace)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }

    importedModules.Add(identifierInfo, module);

    for (auto dep : module->GetCompilerInstance()->GetImportModuleTable())
        ImportModule(dep.second, dep.first);

    return true;
}

bool VCL::Sema::ValidateIntrinsicFunctionDeclSpecialization(FunctionDecl* decl) {
    FunctionType* functionType = decl->GetType();

    switch (decl->GetIntrinsicID()) {
        // Math Intrinsic (one type either float or vector of float)
        case FunctionDecl::IntrinsicID::Sin:
        case FunctionDecl::IntrinsicID::Cos:
        case FunctionDecl::IntrinsicID::Tan:
        case FunctionDecl::IntrinsicID::Sinh:
        case FunctionDecl::IntrinsicID::Cosh:
        case FunctionDecl::IntrinsicID::Tanh:
        case FunctionDecl::IntrinsicID::ASin:
        case FunctionDecl::IntrinsicID::ACos:
        case FunctionDecl::IntrinsicID::ATan:
        case FunctionDecl::IntrinsicID::ATan2:
        case FunctionDecl::IntrinsicID::Sqrt:
        case FunctionDecl::IntrinsicID::Log:
        case FunctionDecl::IntrinsicID::Log2:
        case FunctionDecl::IntrinsicID::Log10:
        case FunctionDecl::IntrinsicID::Exp:
        case FunctionDecl::IntrinsicID::Exp2:
        case FunctionDecl::IntrinsicID::Floor:
        case FunctionDecl::IntrinsicID::Ceil:
        case FunctionDecl::IntrinsicID::Round:
        case FunctionDecl::IntrinsicID::FMod:
        case FunctionDecl::IntrinsicID::Pow: {
            Type* type = functionType->GetReturnType().GetType();
            type = Type::GetCanonicalType(type);
            if (type->GetTypeClass() == Type::VectorTypeClass) {
                type = ((VectorType*)type)->GetElementType().GetType();
            }

            if (type->GetTypeClass() == Type::BuiltinTypeClass) {
                BuiltinType* builtinType = (BuiltinType*)type;
                BuiltinType::Kind kind = builtinType->GetKind();
                if (kind == BuiltinType::Float32 || kind == BuiltinType::Float64)
                    return true;
            }
            
            diagnosticReporter.Error(Diagnostic::MathIntrinsicBadSpecialization, decl->GetIdentifierInfo()->GetName().str(), TypePrinter::Print(type))
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return false;
        }
        // Math Intrinsic (one type either scalar or vector of scalar)
        case FunctionDecl::IntrinsicID::Min:
        case FunctionDecl::IntrinsicID::Max:
        case FunctionDecl::IntrinsicID::Fma:
        case FunctionDecl::IntrinsicID::Abs: {
            Type* type = functionType->GetReturnType().GetType();
            type = Type::GetCanonicalType(type);
            if (type->GetTypeClass() == Type::VectorTypeClass)
                type = ((VectorType*)type)->GetElementType().GetType();

            if (type->GetTypeClass() == Type::BuiltinTypeClass) {
                BuiltinType* builtinType = (BuiltinType*)type;
                BuiltinType::Kind kind = builtinType->GetKind();
                if (kind != BuiltinType::Void || kind != BuiltinType::Bool)
                    return true;
            }
            
            diagnosticReporter.Error(Diagnostic::MathIntrinsicBadSpecialization, decl->GetIdentifierInfo()->GetName().str(), TypePrinter::Print(type))
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return false;
        }
        case FunctionDecl::IntrinsicID::Unpack:
        case FunctionDecl::IntrinsicID::Pack:
            return true;
        case FunctionDecl::IntrinsicID::Length: {
            Type* type = Type::GetCanonicalType(functionType->GetParamsType()[0].GetType());
            Type::TypeClass typeClass = type->GetTypeClass();
            if (typeClass != Type::VectorTypeClass && typeClass != Type::LanesTypeClass && 
                    typeClass != Type::ArrayTypeClass && typeClass != Type::SpanTypeClass) {
                diagnosticReporter.Error(Diagnostic::LengthIntrinsicBadSpecialization, TypePrinter::Print(type))
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return false;
            }
            return true;
        }
        case FunctionDecl::IntrinsicID::Select:
            return true;
        default:
            diagnosticReporter.Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return false;
    }
}

VCL::NamedDecl* VCL::Sema::LookupNamedDecl(SymbolRef symbolRef, int depth) {
    Scope* currentScope = sm.GetScopeFront();

    if (symbolRef.IsLocal()) {
        int currentDepth = 0;
        while (currentScope != nullptr && (currentDepth < depth || depth == -1)) {
            for (Decl* decl : *currentScope) {
                if (decl->GetDeclClass() == Decl::TemplateDeclClass) {
                    TemplateDecl* templateDecl = (TemplateDecl*)decl;
                    if (templateDecl->GetTemplatedNamedDecl() == nullptr)
                        continue;
                    if (symbolRef.GetSymbolName() == templateDecl->GetTemplatedNamedDecl()->GetIdentifierInfo())
                        return templateDecl->GetTemplatedNamedDecl();
                } else {
                    if (!decl->IsNamedDecl())
                        continue;
                    NamedDecl* namedDecl = (NamedDecl*)decl;
                    if (symbolRef.GetSymbolName() == namedDecl->GetIdentifierInfo())
                        return namedDecl;
                }
            }
            currentScope = currentScope->GetParentScope();
            ++currentDepth;
        }
    } else {
        Module* module = importedModules.Get(symbolRef.GetModuleName());
        if (module == nullptr) {
            diagnosticReporter.Error(Diagnostic::ModuleNameDoesNotExists, symbolRef.GetModuleName()->GetName().str())
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        Decl* decl = module->GetCompilerInstance()->GetExportSymbolTable().Get(symbolRef.GetSymbolName());
        if (!decl)
            return nullptr;
        if (decl->GetDeclClass() == Decl::TemplateDeclClass) {
            TemplateDecl* templateDecl = (TemplateDecl*)decl;
            if (templateDecl->GetTemplatedNamedDecl() == nullptr)
                return nullptr;
            if (symbolRef.GetSymbolName() == templateDecl->GetTemplatedNamedDecl()->GetIdentifierInfo())
                return templateDecl->GetTemplatedNamedDecl();
        } else {
            if (!decl->IsNamedDecl())
                return nullptr;
            NamedDecl* namedDecl = (NamedDecl*)decl;
            if (symbolRef.GetSymbolName() == namedDecl->GetIdentifierInfo())
                return namedDecl;
        }
    }
    
    return nullptr;
}

VCL::TemplateDecl* VCL::Sema::LookupTemplateDecl(SymbolRef symbolRef, int depth) {
    Scope* currentScope = sm.GetScopeFront();

    if (symbolRef.IsLocal()) {
        int currentDepth = 0;
        while (currentScope != nullptr && (currentDepth < depth || depth == -1)) {
            for (Decl* decl : *currentScope) {
                if (decl->GetDeclClass() == Decl::TemplateDeclClass) {
                    TemplateDecl* templateDecl = (TemplateDecl*)decl;
                    if (templateDecl->GetTemplatedNamedDecl() == nullptr)
                        continue;
                    if (symbolRef.GetSymbolName() == templateDecl->GetTemplatedNamedDecl()->GetIdentifierInfo())
                        return templateDecl;
                }
            }
            currentScope = currentScope->GetParentScope();
            ++currentDepth;
        }
    } else {
        if (!importedModules.Get(symbolRef.GetModuleName())) {
            diagnosticReporter.Error(Diagnostic::ModuleNameDoesNotExists, symbolRef.GetModuleName()->GetName().str())
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        Module* module = importedModules.Get(symbolRef.GetModuleName());
        Decl* decl = module->GetCompilerInstance()->GetExportSymbolTable().Get(symbolRef.GetSymbolName());
        if (!decl)
            return nullptr;
        if (decl->GetDeclClass() == Decl::TemplateDeclClass) {
            TemplateDecl* templateDecl = (TemplateDecl*)decl;
            if (templateDecl->GetTemplatedNamedDecl() == nullptr)
                return nullptr;
            if (symbolRef.GetSymbolName() == templateDecl->GetTemplatedNamedDecl()->GetIdentifierInfo())
                return templateDecl;
        }
    }
    
    return nullptr;
}

VCL::CompoundStmt* VCL::Sema::ActOnCompoundStmt(llvm::ArrayRef<Stmt*> stmts, SourceRange range) {
    return CompoundStmt::Create(astContext, stmts, range);
}

VCL::DirectiveDecl* VCL::Sema::ActOnDirectiveDecl(IdentifierInfo* identifierInfo, llvm::ArrayRef<ConstantValue*> args, SourceRange range) {
    DirectiveHandler* handler = directiveRegistry.GetDirectiveHandler(identifierInfo);
    if (!handler) {
        bool r = diagnosticReporter.Error(Diagnostic::DirectiveDoesNotExist, identifierInfo->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        if (r)
            return nullptr;
        return DirectiveDecl::Create(astContext, identifierInfo, args, range);
    }

    DirectiveDecl* decl = DirectiveDecl::Create(astContext, identifierInfo, args, range);
    if (!handler->OnSema(*this, decl)) {
        diagnosticReporter.Error(Diagnostic::DirectiveSemaError)
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    return decl;
}

VCL::TypeAliasDecl* VCL::Sema::ActOnTypeAliasDecl(IdentifierInfo* identifierInfo, Type* type, SourceRange range) {
    TemplateInstantiator instantiator{ *this };
    if (!instantiator.MakeTypeComplete(type)) {
        diagnosticReporter.Error(Diagnostic::TypeAliasCannotBeTemplated)
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    TypeAliasDecl* instance = TypeAliasDecl::Create(astContext, identifierInfo, type, range);
    type = astContext.GetTypeCache().GetOrCreateTypeAliasType(Type::GetCanonicalType(type), instance);
    instance->SetType(type);
    if (!AddDeclToScopeAndContext(instance))
        return nullptr;
    return instance;
}

VCL::DeclStmt* VCL::Sema::ActOnDeclStmt(Decl* decl, SourceRange range) {
    return DeclStmt::Create(astContext, decl, range);
}

VCL::TemplateDecl* VCL::Sema::ActOnTemplateDecl(TemplateParameterList* parameters, SourceRange range) {
    TemplateDecl* decl = TemplateDecl::Create(astContext, parameters, range);
    if (!AddDeclToScopeAndContext(decl))
        return nullptr;
    return decl;
}

VCL::RecordDecl* VCL::Sema::ActOnRecordDecl(IdentifierInfo* identifier, SourceRange range) {
    NamedDecl* decl = LookupNamedDecl(SymbolRef{ identifier });
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        diagnosticReporter.Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    decl = RecordDecl::Create(astContext, identifier, range);
    if (!AddDeclToScopeAndContext(decl))
        return nullptr;
    return (RecordDecl*)decl;
}

VCL::FieldDecl* VCL::Sema::ActOnFieldDecl(QualType type, IdentifierInfo* identifier, SourceRange range) {
    NamedDecl* decl = LookupNamedDecl(SymbolRef{ identifier }, 1);
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        diagnosticReporter.Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    if (!type.GetType()->IsDependent()) {
        TemplateInstantiator instantiator{ *this };
        if (!instantiator.MakeTypeComplete(type.GetType()))
            return nullptr;
    }

    decl = FieldDecl::Create(astContext, identifier, type, range);
    if (!AddDeclToScopeAndContext(decl))
        return nullptr;
    return (FieldDecl*)decl;
}

VCL::FunctionDecl* VCL::Sema::ActOnFunctionDecl(FunctionDecl* decl, QualType returnType, TemplateArgumentList* args, SourceRange range) {
    NamedDecl* lookupDecl = LookupNamedDecl(SymbolRef{ decl->GetIdentifierInfo() }, 1);
    if (lookupDecl && !args) {
        SourceRange redeclRange = lookupDecl->GetSourceRange();
        diagnosticReporter.Error(Diagnostic::Redeclaration, lookupDecl->GetIdentifierInfo()->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    
    llvm::SmallVector<QualType> paramsType{};
    for (auto it = decl->Begin(); it != decl->End(); ++it) {
        if (it->GetDeclClass() != Decl::ParamDeclClass)
            continue;
        ParamDecl* paramDecl = (ParamDecl*)it.Get();
        paramsType.push_back(paramDecl->GetValueType());
    }

    FunctionType* type = astContext.GetTypeCache().GetOrCreateFunctionType(returnType, paramsType);
    decl->SetType(type);
    decl->SetSourceRange(range);

    if (!type->IsDependent()) {
        TemplateInstantiator instantiator{ *this };
        if (!instantiator.MakeTypeComplete(decl->GetType()->GetReturnType().GetType()))
            return nullptr;
    }

    if (decl->HasFunctionFlag(FunctionDecl::IsIntrinsic))
        if (!ValidateIntrinsicFunctionDeclSpecialization(decl))
            return nullptr;

    if (!args && sm.GetScopeFront()->GetDeclContext() != decl) {
        if (!AddDeclToScopeAndContext(decl))
            return nullptr;
    } else if (args) {
        TemplateDecl* templateDecl = LookupTemplateDecl(SymbolRef{ decl->GetIdentifierInfo() }, 1);
        if (!templateDecl) {
            diagnosticReporter.Error(Diagnostic::MissingTemplateDecl)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ range })
                .Report();
            return nullptr;
        }

        TemplateInstantiator instantiator{ *this };
        if (!instantiator.CheckTemplateArgumentsParametersMatch(args, templateDecl->GetTemplateParametersList()))
            return nullptr;

        for (auto it = templateDecl->Begin(); it != templateDecl->End(); ++it) {
            if (it->GetDeclClass() != Decl::TemplateSpecializationDeclClass)
                continue;
            TemplateSpecializationDecl* specializationDecl = (TemplateSpecializationDecl*)it.Get();
            if (MatchTemplateArgumentList(specializationDecl->GetTemplateArgumentList(), args)) {
                diagnosticReporter.Error(Diagnostic::SpecializationAlreadyExist)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ range })
                    .AddHint(DiagnosticHint{ specializationDecl->GetNamedDecl()->GetSourceRange(), DiagnosticHint::PreviouslyDeclared })
                    .Report();
                return nullptr;
            }
        }

        TemplateSpecializationDecl* specializationDecl = TemplateSpecializationDecl::Create(astContext, args, decl);
        templateDecl->InsertBack(specializationDecl);
    }

    return decl;
}

VCL::ParamDecl* VCL::Sema::ActOnParamDecl(Decl::VarAttrBitfield attr, QualType type, IdentifierInfo* identifier, SourceRange range) {
    NamedDecl* decl = LookupNamedDecl(SymbolRef{ identifier }, 1);
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        diagnosticReporter.Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    if (!type.GetType()->IsDependent()) {
        TemplateInstantiator instantiator{ *this };
        if (!instantiator.MakeTypeComplete(type.GetType()))
            return nullptr;

        bool isPassedByReference = attr.hasOutAttribute || (!attr.hasInAttribute && TypePreferByReference(type.GetType()));
        if (isPassedByReference) {
            Type* refType = astContext.GetTypeCache().GetOrCreateReferenceType(type);
            type = QualType{ refType, type.GetQualifiers() };
        }
    }

    decl = ParamDecl::Create(astContext, type, identifier, attr, range);
    if (!AddDeclToScopeAndContext(decl))
        return nullptr;
    return (ParamDecl*)decl;
}

VCL::CompoundStmt* VCL::Sema::ActOnFunctionBody(FunctionDecl* function, CompoundStmt* body) {
    if (!ActRecOnFunctionBodyReturnStmt(function, body->GetStmts()))
        return nullptr;
    return body;
}

bool VCL::Sema::ActRecOnFunctionBodyReturnStmt(FunctionDecl* function, llvm::ArrayRef<Stmt*> stmts) {
    for (size_t i = 0; i < stmts.size(); ++i) {
        Stmt* stmt = stmts[i];
        if (stmt->GetStmtClass() == Stmt::ReturnStmtClass) {
            if (i + 1 < stmts.size()) {
                Stmt* ignoredStmt = stmts[i + 1];
                diagnosticReporter.Warn(Diagnostic::StatementNeverReached)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ ignoredStmt->GetSourceRange() })
                    .Report();
            }
            return true;
        }
    }

    bool isFunctionVoid = false;
    Type* returnType = function->GetType()->GetReturnType().GetType();
    if (returnType->GetTypeClass() == Type::BuiltinTypeClass)
        isFunctionVoid = ((BuiltinType*)returnType)->GetKind() == BuiltinType::Void;

    if (!isFunctionVoid) {
        diagnosticReporter.Error(Diagnostic::MissingReturnStmt)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ function->GetSourceRange() })
            .Report();
    }

    return isFunctionVoid;
}

VCL::ReturnStmt* VCL::Sema::ActOnReturnStmt(Expr* expr, SourceRange range) {
    FunctionDecl* function = GetFrontmostFunctionDecl();
    if (!function) {
        diagnosticReporter.Error(Diagnostic::ReturnStmtOutsideOfBody)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    Type* returnType = function->GetType()->GetReturnType().GetType();

    bool isExprDependent = expr ? expr->IsDependent() : false;

    if (!returnType->IsDependent() && !isExprDependent) {
        bool isFunctionVoid = false;
        
        if (returnType->GetTypeClass() == Type::BuiltinTypeClass)
            isFunctionVoid = ((BuiltinType*)returnType)->GetKind() == BuiltinType::Void;

        if (expr != nullptr && isFunctionVoid) {
            diagnosticReporter.Error(Diagnostic::InvalidReturnStmtExpr)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ range })
                .Report();
            return nullptr;
        } else if (expr == nullptr && !isFunctionVoid) {
            diagnosticReporter.Error(Diagnostic::InvalidReturnStmtNoExpr)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ range })
                .Report();
            return nullptr;
        }

        if (expr) {
            expr = ActOnCast(ActOnLoad(expr), function->GetType()->GetReturnType(), range);
            if (!expr)
                return nullptr;
        }
    }

    return ReturnStmt::Create(astContext, expr, range);
}

VCL::IfStmt* VCL::Sema::ActOnIfStmt(Expr* condition, Stmt* thenStmt, Stmt* elseStmt, SourceRange range) {
    condition = ActOnCast(ActOnLoad(condition), astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Bool), condition->GetSourceRange());
    if (!condition)
        return nullptr;
    return IfStmt::Create(astContext, condition, thenStmt, elseStmt, range);
}

VCL::WhileStmt* VCL::Sema::ActOnWhileStmt(Expr* condition, Stmt* thenStmt, SourceRange range) {
    condition = ActOnCast(ActOnLoad(condition), astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Bool), condition->GetSourceRange());
    if (!condition)
        return nullptr;
    return WhileStmt::Create(astContext, condition, thenStmt, range);
}

VCL::ForStmt* VCL::Sema::ActOnForStmt(Stmt* startStmt, Expr* condition, Expr* loopExpr, Stmt* thenStmt, SourceRange range) {
    if (condition) {
        condition = ActOnCast(ActOnLoad(condition), astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Bool), condition->GetSourceRange());
        if (!condition)
            return nullptr;
    }
    return ForStmt::Create(astContext, startStmt, condition, loopExpr, thenStmt, range);
}

VCL::BreakStmt* VCL::Sema::ActOnBreakStmt(SourceRange range) {
    if (!IsWithinALoop()) {
        diagnosticReporter.Error(Diagnostic::BreakOutsideOfLoop)
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    return BreakStmt::Create(astContext, range);
}

VCL::ContinueStmt* VCL::Sema::ActOnContinueStmt(SourceRange range) {
    if (!IsWithinALoop()) {
        diagnosticReporter.Error(Diagnostic::ContinueOutsideOfLoop)
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    return ContinueStmt::Create(astContext, range);
}

VCL::VarDecl* VCL::Sema::ActOnVarDecl(QualType type, IdentifierInfo* identifier, VarDecl::VarAttrBitfield varAttrBitfield, Expr* initializer, SourceRange range) {
    if (identifier->IsKeyword()) {
        diagnosticReporter.Error(Diagnostic::ReservedIdentifier, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    NamedDecl* decl = LookupNamedDecl(SymbolRef{ identifier }, 1);
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        diagnosticReporter.Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    if ((varAttrBitfield.hasInAttribute || varAttrBitfield.hasOutAttribute) && !IsCurrentScopeGlobal()) {
        diagnosticReporter.Error(Diagnostic::AttrInvalidUse)
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    if (IsCurrentScopeGlobal() && initializer) {
        if (varAttrBitfield.hasInAttribute) {
            diagnosticReporter.Error(Diagnostic::InitializerInputVarDecl)
                .AddHint(DiagnosticHint{ range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
    }

    bool isInitializerDependent = initializer ? initializer->IsDependent() : false;
    if (!type.GetType()->IsDependent() && !isInitializerDependent) {
        TemplateInstantiator instantiator{ *this };
        if (!instantiator.MakeTypeComplete(type.GetType()))
            return nullptr;
            
        if (IsCurrentScopeGlobal() && initializer) {
            if (initializer->GetResultType().GetType() != type.GetType()) {
                Expr* castedInitializer = ActOnCast(initializer, type, initializer->GetSourceRange());
                if (!castedInitializer)
                    return nullptr;
                initializer = castedInitializer;
            }
            ExprEvaluator exprEvaluator{ astContext };
            ConstantValue* value = exprEvaluator.Visit(initializer);
            if (!value) {
                diagnosticReporter.Error(Diagnostic::ExprDoesNotEvaluate)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ initializer->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            initializer->SetConstantValue(value);
        } else if (initializer) {
            initializer = ActOnCast(ActOnLoad(initializer), type, initializer->GetSourceRange());
            if (!initializer)
                return nullptr;
        }
    }

    decl = VarDecl::Create(astContext, type, identifier, varAttrBitfield, range);
    ((VarDecl*)decl)->SetInitializer(initializer);
    if (!AddDeclToScopeAndContext(decl))
        return nullptr;
    return (VarDecl*)decl;
}

VCL::QualType VCL::Sema::ActOnQualType(Type* type, Qualifier qualifiers, SourceRange range) {
    return QualType(type, qualifiers);
}

#define ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list) if (list != nullptr) { \
                diagnosticReporter.Error(Diagnostic::BuiltinTypeIsNotTemplated, symbolRef.GetSymbolName()->GetName().str()) \
                    .AddHint(DiagnosticHint{ range })\
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)\
                    .Report(); \
                return nullptr; }
                
VCL::Type* VCL::Sema::ActOnType(SymbolRef symbolRef, TemplateArgumentList* list, SourceRange range) {
    if (!symbolRef.GetSymbolName()->IsKeyword()) {
        NamedDecl* decl = LookupNamedDecl(symbolRef);
        if (decl == nullptr) {
            diagnosticReporter.Error(Diagnostic::IdentifierUndefined, symbolRef.GetSymbolName()->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        TemplateDecl* templateDecl = LookupTemplateDecl(symbolRef);
        if (list && !templateDecl) {
            diagnosticReporter.Error(Diagnostic::DoesNotTakeTemplateArgList, symbolRef.GetSymbolName()->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        if (!list && templateDecl) {
            diagnosticReporter.Error(Diagnostic::MissingTemplateArgument, symbolRef.GetSymbolName()->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        if (!decl->IsTypeDecl()) {
            diagnosticReporter.Error(Diagnostic::NotTypeDecl, symbolRef.GetSymbolName()->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }

        if (!list)
            return ((TypeDecl*)decl)->GetType();
        else
            return astContext.GetTypeCache().GetOrCreateTemplateSpecializationType(templateDecl, list);
    }
    
    switch (symbolRef.GetSymbolName()->GetTokenKind()) {
        case TokenKind::Keyword_void:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Void);
        case TokenKind::Keyword_bool:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Bool);
        case TokenKind::Keyword_uint8:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt8);
        case TokenKind::Keyword_uint16:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt16);
        case TokenKind::Keyword_uint32:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt32);
        case TokenKind::Keyword_uint64:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt64);
        case TokenKind::Keyword_int8:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Int8);
        case TokenKind::Keyword_int16:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Int16);
        case TokenKind::Keyword_int32:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Int32);
        case TokenKind::Keyword_int64:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Int64);
        case TokenKind::Keyword_float32: {
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            Type* type = astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Float32);
            return type;
        }
        case TokenKind::Keyword_float64:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Float64);
        default:
            TemplateDecl* decl = LookupTemplateDecl(symbolRef);
            if (decl == nullptr) {
                diagnosticReporter.Error(Diagnostic::ReservedIdentifier, symbolRef.GetSymbolName()->GetName().str())
                    .AddHint(DiagnosticHint{ range })
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return nullptr;
            }
            if (!list) {
                diagnosticReporter.Error(Diagnostic::MissingTemplateArgument, symbolRef.GetSymbolName()->GetName().str())
                    .AddHint(DiagnosticHint{ range })
                    .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return nullptr;
            }
            return astContext.GetTypeCache().GetOrCreateTemplateSpecializationType(decl, list);
    }
}

VCL::TemplateParameterList* VCL::Sema::ActOnTemplateParameterList(llvm::ArrayRef<NamedDecl*> params, SourceRange range) {
    llvm::SmallPtrSet<IdentifierInfo*, 4> set{};
    
    for (auto param : params) {
        if (set.count(param->GetIdentifierInfo())) {
            NamedDecl* previous = nullptr;
            for (size_t i = 0; i < params.size(); ++i) {
                if (params[i]->GetIdentifierInfo() == param->GetIdentifierInfo()) {
                    previous = params[i];
                    break;
                }
            }
            assert(previous != nullptr);
            diagnosticReporter.Error(Diagnostic::TemplateRedeclared, param->GetIdentifierInfo()->GetName().str())
                .AddHint(DiagnosticHint{ param->GetSourceRange() })
                .AddHint(DiagnosticHint{ previous->GetSourceRange(), DiagnosticHint::PreviouslyDeclared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        set.insert(param->GetIdentifierInfo());
    }

    return TemplateParameterList::Create(astContext, params, range);
}

VCL::TemplateArgumentList* VCL::Sema::ActOnTemplateArgumentList(llvm::ArrayRef<TemplateArgument> args, SourceRange range, bool canonicalize) {
    llvm::SmallVector<TemplateArgument> argsEval{};

    for (auto arg : args) {
        if (arg.IsDependent()) {
            argsEval.push_back(arg);
            continue;
        }
        switch (arg.GetKind()) {
            case TemplateArgument::Type: {
                if (!canonicalize) {
                    argsEval.push_back(arg);
                    break;
                }
                TemplateInstantiator instantiator{ *this };
                if (!instantiator.MakeTypeComplete(arg.GetType().GetType()))
                    return nullptr;
                VCL::Type* type = Type::GetCanonicalType(arg.GetType().GetType());
                QualType qualType{ type, arg.GetType().GetQualifiers() };
                argsEval.push_back(qualType);
                break;
            }
            case TemplateArgument::Expression: {
                ExprEvaluator eval{ astContext };
                ConstantValue* value = eval.Visit(arg.GetExpr());
                arg.GetExpr()->SetConstantValue(value);
                if (!canonicalize) {
                    argsEval.push_back(arg);
                    break;
                } else if (!value || value->GetConstantValueClass() != ConstantValue::ConstantScalarClass) {
                    diagnosticReporter.Error(Diagnostic::ExprDoesNotEvaluateScalar)
                        .SetCompilerInfo(__FILE__, __func__, __LINE__)
                        .AddHint(DiagnosticHint{ arg.GetSourceRange() })
                        .Report();
                    return nullptr;
                }
                argsEval.push_back(TemplateArgument{ *(ConstantScalar*)value });
                break;
            }
            default:
                argsEval.push_back(arg);
                break;
        }
    }

    return TemplateArgumentList::Create(astContext, argsEval, range);
}

VCL::TemplateTypeParamDecl* VCL::Sema::ActOnTemplateTypeParamDecl(IdentifierInfo* identifier, SourceRange range) {
    return TemplateTypeParamDecl::Create(astContext, identifier, range);
}

VCL::NonTypeTemplateParamDecl* VCL::Sema::ActOnNonTypeTemplateParamDecl(BuiltinType* type, IdentifierInfo* identifier, SourceRange range) {
    return NonTypeTemplateParamDecl::Create(astContext, type, identifier, range);
}

VCL::Expr* VCL::Sema::ActOnBinaryExpr(Expr* lhs, Expr* rhs, BinaryOperator::Kind op) {
    if (lhs->GetResultType().GetType()->IsDependent() || rhs->GetResultType().GetType()->IsDependent()) {
        Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
        expr->SetResultType(astContext.GetTypeCache().GetOrCreateDependentType());
        switch (op) {
            case BinaryOperator::Assignment:
            case BinaryOperator::AssignmentAdd:
            case BinaryOperator::AssignmentSub:
            case BinaryOperator::AssignmentMul:
            case BinaryOperator::AssignmentDiv:
            case BinaryOperator::AssignmentRemainder:
            case BinaryOperator::AssignmentBitwiseAnd:
            case BinaryOperator::AssignmentBitwiseXor:
            case BinaryOperator::AssignmentBitwiseOr:
            case BinaryOperator::AssignmentLeftShift:
            case BinaryOperator::AssignmentRightShift:
                expr->SetValueCategory(Expr::LValue);
            default:
                expr->SetValueCategory(Expr::RValue);
        }
        return expr;
    }
    
    switch (op) {
        // Arithmetic
        case BinaryOperator::Add:
        case BinaryOperator::Sub:
        case BinaryOperator::Mul:
        case BinaryOperator::Div: {
            lhs = ActOnLoad(lhs);
            rhs = ActOnLoad(rhs);
            std::pair<Expr*, Expr*> r = ActOnImplicitBinaryArithmeticCast(lhs, rhs);
            lhs = r.first;
            rhs = r.second;
            if (!lhs || !rhs)
                return nullptr;
            if (!Type::IsTypeNumeric(lhs->GetResultType().GetType())) {
                diagnosticReporter.Error(Diagnostic::NotNumericType, TypePrinter::Print(lhs->GetResultType()))
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
            expr->SetValueCategory(Expr::RValue);
            return expr;
        }
        case BinaryOperator::Remainder: {
            lhs = ActOnLoad(lhs);
            rhs = ActOnLoad(rhs);    
            std::pair<Expr*, Expr*> r = ActOnImplicitBinaryArithmeticCast(lhs, rhs);
            lhs = r.first;
            rhs = r.second;
            if (!lhs || !rhs)
                return nullptr;
            if (!Type::IsTypeIntegral(lhs->GetResultType().GetType())) {
                diagnosticReporter.Error(Diagnostic::NotIntegralType, TypePrinter::Print(lhs->GetResultType()))
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
            expr->SetValueCategory(Expr::RValue);
            return expr;
        }
        //Logical
        case BinaryOperator::Greater:
        case BinaryOperator::Lesser:
        case BinaryOperator::GreaterEqual:
        case BinaryOperator::LesserEqual:
        case BinaryOperator::Equal:
        case BinaryOperator::NotEqual: {
            lhs = ActOnLoad(lhs);
            rhs = ActOnLoad(rhs);
            std::pair<Expr*, Expr*> r = ActOnImplicitBinaryArithmeticCast(lhs, rhs);
            lhs = r.first;
            rhs = r.second;
            if (!lhs || !rhs)
                return nullptr;
            if (!Type::IsTypeNumeric(lhs->GetResultType().GetType())) {
                diagnosticReporter.Error(Diagnostic::NotNumericType, TypePrinter::Print(lhs->GetResultType()))
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            Type* trueType = Type::GetCanonicalType(lhs->GetResultType().GetType());
            Type* resultType = astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Bool);
            if (trueType->GetTypeClass() == Type::VectorTypeClass)
                resultType = astContext.GetTypeCache().GetOrCreateVectorType(resultType);
            Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
            expr->SetValueCategory(Expr::RValue);
            expr->SetResultType(resultType);
            return expr;
        }
        case BinaryOperator::LogicalAnd:
        case BinaryOperator::LogicalOr: {
            lhs = ActOnLoad(lhs);
            rhs = ActOnLoad(rhs);
            Type* trueType = Type::GetCanonicalType(lhs->GetResultType().GetType());
            Type* resultType = astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Bool);
            if (trueType->GetTypeClass() == Type::VectorTypeClass)
                resultType = astContext.GetTypeCache().GetOrCreateVectorType(resultType);
            lhs = ActOnCast(lhs, resultType, lhs->GetSourceRange());
            rhs = ActOnCast(rhs, resultType, rhs->GetSourceRange());
            if (!lhs || !rhs)
                return nullptr;
            Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
            expr->SetValueCategory(Expr::RValue);
            expr->SetResultType(resultType);
            return expr;
        }
        case BinaryOperator::BitwiseAnd:
        case BinaryOperator::BitwiseXor:
        case BinaryOperator::BitwiseOr:
        case BinaryOperator::LeftShift:
        case BinaryOperator::RightShift: {
            lhs = ActOnLoad(lhs);
            rhs = ActOnLoad(rhs);
            std::pair<Expr*, Expr*> r = ActOnImplicitBinaryArithmeticCast(lhs, rhs);
            lhs = r.first;
            rhs = r.second;
            if (!lhs || !rhs)
                return nullptr;
            if (!Type::IsTypeIntegral(lhs->GetResultType().GetType())) {
                diagnosticReporter.Error(Diagnostic::NotIntegralType)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
                    .Report();
                return nullptr;
            }

            Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
            expr->SetValueCategory(Expr::RValue);
            return expr;
        }
        // Assignment
        case BinaryOperator::Assignment: {
            rhs = ActOnLoad(rhs);
            if (!IsExprAssignable(lhs))
                return nullptr;
            rhs = ActOnCast(rhs, lhs->GetResultType(), SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end });
            if (!rhs) return nullptr;
            Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
            expr->SetValueCategory(Expr::LValue);
            return expr;
        }
        case BinaryOperator::AssignmentAdd:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::Add);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentSub:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::Sub);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentMul:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::Mul);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentDiv:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::Div);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentRemainder:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::Remainder);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentBitwiseAnd:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::BitwiseAnd);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentBitwiseXor:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::BitwiseXor);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentBitwiseOr:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::BitwiseOr);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentLeftShift:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::LeftShift);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentRightShift:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::RightShift);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        default: {
            diagnosticReporter.Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
    }
}

VCL::Expr* VCL::Sema::ActOnUnaryExpr(Expr* expr, UnaryOperator op, SourceRange range) {
    if (expr->GetResultType().GetType()->IsDependent())
        return UnaryExpr::Create(astContext, expr, op, range);

    switch (op) {
        case UnaryOperator::PrefixIncrement:
        case UnaryOperator::PrefixDecrement:
        case UnaryOperator::PostfixIncrement:
        case UnaryOperator::PostfixDecrement: {
            if (!IsExprAssignable(expr))
                return nullptr;
            if (!Type::IsTypeNumeric(expr->GetResultType().GetType())) {
                diagnosticReporter.Error(Diagnostic::NotNumericType, TypePrinter::Print(expr->GetResultType()))
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            return UnaryExpr::Create(astContext, expr, op, range);
        }
        case UnaryOperator::Plus:
        case UnaryOperator::Minus: {
            expr = ActOnLoad(expr);
            if (!Type::IsTypeNumeric(expr->GetResultType().GetType())) {
                diagnosticReporter.Error(Diagnostic::NotNumericType, TypePrinter::Print(expr->GetResultType()))
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            return UnaryExpr::Create(astContext, expr, op, range);
        }
        case UnaryOperator::BitwiseNot:
        case UnaryOperator::LogicalNot: {
            expr = ActOnLoad(expr);
            if (!Type::IsTypeIntegral(expr->GetResultType().GetType())) {
                diagnosticReporter.Error(Diagnostic::NotIntegralType, TypePrinter::Print(expr->GetResultType()))
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            return UnaryExpr::Create(astContext, expr, op, range);
        }
        default: {
            diagnosticReporter.Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
    }
}

bool VCL::Sema::IsExprAssignable(Expr* expr) {
    if (expr->GetValueCategory() != Expr::LValue) {
        diagnosticReporter.Error(Diagnostic::AssignmentNotLValue)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return false;
    }

    if (expr->GetResultType().HasQualifier(Qualifier::Const)) {
        diagnosticReporter.Error(Diagnostic::AssignmentConstValue)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return false;
    }

    if (expr->GetExprClass() == Expr::DeclRefExprClass) {
        ValueDecl* decl = ((DeclRefExpr*)expr)->GetValueDecl();
        switch (decl->GetDeclClass()) {
            case Decl::VarDeclClass:
                if (((VarDecl*)decl)->HasInAttribute()) {
                    diagnosticReporter.Error(Diagnostic::AssignmentInputValue)
                        .SetCompilerInfo(__FILE__, __func__, __LINE__)
                        .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                        .Report();
                    return false;
                }
                return true;
            default:
                return true;
        }
    }
    return true;
}

VCL::Expr* VCL::Sema::ActOnFieldAccessExpr(Expr* lhs, IdentifierInfo* field, SourceRange range) {
    if (lhs->GetValueCategory() != Expr::LValue) {
        diagnosticReporter.Error(Diagnostic::MustBeLValue)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    Type* type = lhs->GetResultType().GetType();
    if (type->IsDependent())
        return DependentFieldAccessExpr::Create(astContext, lhs, field, range);

    if (type->GetTypeClass() == Type::ReferenceTypeClass)
        type = ((ReferenceType*)type)->GetType().GetType();
    if (type->GetTypeClass() == Type::TemplateSpecializationTypeClass)
        type = ((TemplateSpecializationType*)type)->GetInstantiatedType();
    if (type->GetTypeClass() != Type::RecordTypeClass) {
        diagnosticReporter.Error(Diagnostic::MustHaveStructType, TypePrinter::Print(type))
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    RecordDecl* recordDecl = ((RecordType*)type)->GetRecordDecl();

    FieldDecl* correctFieldDecl = nullptr;

    uint32_t fieldIdx = 0;

    for (auto it = recordDecl->Begin(); it != recordDecl->End(); ++it) {
        if (it->GetDeclClass() != Decl::FieldDeclClass)
            continue;
        FieldDecl* fieldDecl = (FieldDecl*)it.Get();
        if (fieldDecl->GetIdentifierInfo() == field) {
            correctFieldDecl = fieldDecl;
            break;
        }
        ++fieldIdx;
    }

    if (!correctFieldDecl) {
        diagnosticReporter.Error(Diagnostic::MissingMember, recordDecl->GetIdentifierInfo()->GetName().str(), field->GetName().str())
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ recordDecl->GetSourceRange(), DiagnosticHint::Declared })
            .Report();
        return nullptr;
    }

    QualType returnedFieldType = correctFieldDecl->GetType();
    if (lhs->GetResultType().HasQualifier(Qualifier::Const))
        returnedFieldType.AddQualifier(Qualifier::Const);

    return FieldAccessExpr::Create(astContext, lhs, (RecordType*)type, fieldIdx, returnedFieldType, range);
}

VCL::Expr* VCL::Sema::ActOnSubscriptExpr(Expr* expr, Expr* index, SourceRange range) {
    index = ActOnLoad(index);
    if (!index)
        return nullptr;

    if (!index->GetResultType().GetType()->IsDependent() && !Type::IsTypeIntegral(index->GetResultType().GetType())) {
        diagnosticReporter.Error(Diagnostic::NotIntegralType, TypePrinter::Print(index->GetResultType()))
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ index->GetSourceRange() })
            .Report();
        return nullptr;
    }

    if (expr->GetResultType().GetType()->IsDependent()) {
        QualType resultType = astContext.GetTypeCache().GetOrCreateDependentType();
        return SubscriptExpr::Create(astContext, expr, index, resultType, range);
    }

    Type* exprTrueType = Type::GetCanonicalType(expr->GetResultType().GetType());
    switch (exprTrueType->GetTypeClass()) {
        case Type::LanesTypeClass: {
            QualType resultType = ((LanesType*)exprTrueType)->GetElementType();
            return SubscriptExpr::Create(astContext, expr, index, resultType, range);
        }
        case Type::ArrayTypeClass: {
            QualType resultType = ((ArrayType*)exprTrueType)->GetElementType();
            return SubscriptExpr::Create(astContext, expr, index, resultType, range);
        }
        case Type::SpanTypeClass: {
            QualType resultType = ((SpanType*)exprTrueType)->GetElementType();
            return SubscriptExpr::Create(astContext, expr, index, resultType, range);
        }
        default:
            diagnosticReporter.Error(Diagnostic::MustBeSubscriptable)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                .Report();
            return nullptr;
    }
}

VCL::Expr* VCL::Sema::ActOnLoad(Expr* expr) {
    if (expr->GetValueCategory() != Expr::LValue)
        return expr;
    return LoadExpr::Create(astContext, expr, expr->GetSourceRange());
}

std::pair<VCL::Expr*, VCL::Expr*> VCL::Sema::ActOnImplicitBinaryArithmeticCast(Expr* lhs, Expr* rhs) {
    if (lhs->GetResultType().GetType()->IsDependent() || rhs->GetResultType().GetType()->IsDependent())
        return std::make_pair(lhs, rhs);
    Type* lhsType = Type::GetCanonicalType(lhs->GetResultType().GetType());
    Type* rhsType = Type::GetCanonicalType(rhs->GetResultType().GetType());

    if (lhsType->GetTypeClass() != Type::BuiltinTypeClass && lhsType->GetTypeClass() != Type::VectorTypeClass) {
        diagnosticReporter.Error(Diagnostic::InvalidArithmeticConversion, TypePrinter::Print(lhsType))
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
            .Report();
        return std::make_pair(nullptr, nullptr);
    }
    if (rhsType->GetTypeClass() != Type::BuiltinTypeClass && rhsType->GetTypeClass() != Type::VectorTypeClass) {
        diagnosticReporter.Error(Diagnostic::InvalidArithmeticConversion, TypePrinter::Print(rhsType))
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ rhs->GetSourceRange() })
            .Report();
        return std::make_pair(nullptr, nullptr);
    }

    bool lhsVec = lhsType->GetTypeClass() == Type::VectorTypeClass;
    bool rhsVec = rhsType->GetTypeClass() == Type::VectorTypeClass;

    BuiltinType::Kind lhsKind = GetScalarKindFromBuiltinOrVectorType(lhsType);
    BuiltinType::Kind rhsKind = GetScalarKindFromBuiltinOrVectorType(rhsType);

    uint32_t lhsBitWidth = BuiltinType::GetKindBitWidth(lhsKind);
    BuiltinType::Category lhsCategory = BuiltinType::GetKindCategory(lhsKind);

    uint32_t rhsBitWidth = BuiltinType::GetKindBitWidth(rhsKind);
    BuiltinType::Category rhsCategory = BuiltinType::GetKindCategory(rhsKind);

    if (lhsType == rhsType && lhsBitWidth != 1)
        return std::make_pair(lhs, rhs);

    Type* toType = lhsType; // both operands are casted to this type wich may or may not be the type of one of the operand

    // trying to ~follow usual arithmetic conversions : https://en.cppreference.com/w/cpp/language/usual_arithmetic_conversions.html

    if (!lhsVec && rhsVec) { // non-vec to vec
        toType = rhsType;
    } else if (lhsVec == rhsVec) { // both operand are either vec or non-vec
        // if either operand is of floating point type
        if (lhsCategory == BuiltinType::FloatingPointKind || rhsCategory == BuiltinType::FloatingPointKind) {
            if (lhsCategory != BuiltinType::FloatingPointKind) { // non-floating point to floating point
                toType = rhsType;
            } else if (rhsBitWidth > lhsBitWidth) { // lesser to greater rank
                toType = rhsType;
            }
        } else { // otherwise both operands are of integer types
            if (lhsCategory == rhsCategory && rhsBitWidth > lhsBitWidth) { // both signed or unsigned so lesser to greater rank
                toType = rhsType;
            } else if (lhsCategory == rhsCategory && rhsBitWidth == lhsBitWidth && lhsBitWidth == 1) { // is both are boolean then cast to int8
                toType = astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Int8);
                if (lhsVec)
                    toType = astContext.GetTypeCache().GetOrCreateVectorType(toType);
            } else { // unsigned/signed conversion
                if (rhsCategory == BuiltinType::UnsignedKind && rhsBitWidth >= lhsBitWidth) { // if U is greater or equal than S then cast to U
                    toType = rhsType;
                } else if (rhsCategory == BuiltinType::SignedKind && rhsBitWidth > lhsBitWidth) { // if S can contain U then cast to S
                    toType = rhsType;
                }
            }
        }
    }

    // check for lose of precision with non-vec to vec
    if (lhsVec != rhsVec) {
        Type* nonVecType = lhsVec ? rhsType : lhsType;
        SourceRange range = lhsVec ? rhs->GetSourceRange() : lhs->GetSourceRange();

        BuiltinType::Kind nonVecKind = GetScalarKindFromBuiltinOrVectorType(nonVecType);
        uint32_t nonVecBitWidth = BuiltinType::GetKindBitWidth(nonVecKind);
        BuiltinType::Category nonVecCategory = BuiltinType::GetKindCategory(nonVecKind);

        BuiltinType::Kind toKind = GetScalarKindFromBuiltinOrVectorType(toType);
        uint32_t toBitWidth = BuiltinType::GetKindBitWidth(toKind);
        BuiltinType::Category toCategory = BuiltinType::GetKindCategory(toKind);

        if (nonVecCategory != toCategory || nonVecBitWidth > toBitWidth) {
            Diagnostic::DiagnosticMsg msg = 
                    nonVecCategory == BuiltinType::FloatingPointKind ? Diagnostic::LoseOfFloatingPointPrecision : Diagnostic::LoseOfIntegerPrecision;
            bool r = diagnosticReporter.Warn(msg, TypePrinter::Print(nonVecType), TypePrinter::Print(toType))
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ range })
                .Report();
            if (r)
                return std::make_pair(nullptr, nullptr);
        }
    }

    return std::make_pair(ActOnCast(lhs, toType, lhs->GetSourceRange()), ActOnCast(rhs, toType, rhs->GetSourceRange()));
}

VCL::Expr* VCL::Sema::ActOnCast(Expr* expr, QualType toType, SourceRange range) {
    if (expr->GetExprClass() == Expr::AggregateExprClass) {
        AggregateExpr* aggregateExpr = (AggregateExpr*)expr;
        aggregateExpr->SetResultType(toType);
        return ActOnAggregateExpr(aggregateExpr) ? aggregateExpr : nullptr;
    }

    if (expr->GetResultType().GetType()->IsDependent() || toType.GetType()->IsDependent())
        return expr;

    TemplateInstantiator instantiator{ *this };
    if (!instantiator.MakeTypeComplete(toType.GetType()))
        return nullptr;

    Type* srcType = Type::GetCanonicalType(expr->GetResultType().GetType());
    Type* dstType = Type::GetCanonicalType(toType.GetType());

    if (!srcType || !dstType) {
        diagnosticReporter.Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }

    BuiltinType::Kind srcKind;
    BuiltinType::Kind dstKind;

    while (dstType->GetTypeClass() == Type::ReferenceTypeClass)
        dstType = ((ReferenceType*)dstType)->GetType().GetType();
    
    if (srcType == dstType)
        return expr;

    if (!CheckTypeCastability(srcType) || !CheckTypeCastability(dstType)) {
        diagnosticReporter.Error(Diagnostic::InvalidCast, TypePrinter::Print(srcType), TypePrinter::Print(dstType))
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }

    if (srcType->GetTypeClass() == Type::VectorTypeClass && dstType->GetTypeClass() != Type::VectorTypeClass) {
        diagnosticReporter.Error(Diagnostic::InvalidVectorCast, TypePrinter::Print(srcType), TypePrinter::Print(dstType))
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }

    if (dstType->GetTypeClass() == Type::VectorTypeClass && srcType->GetTypeClass() == Type::BuiltinTypeClass) {
        expr = ActOnSplat(expr, expr->GetSourceRange());
        if (!expr)
            return nullptr;
    }
    
    srcKind = GetScalarKindFromBuiltinOrVectorType(srcType);
    dstKind = GetScalarKindFromBuiltinOrVectorType(dstType);

    uint32_t srcBitWidth = BuiltinType::GetKindBitWidth(srcKind);
    BuiltinType::Category srcCategory = BuiltinType::GetKindCategory(srcKind);

    uint32_t dstBitWidth = BuiltinType::GetKindBitWidth(dstKind);
    BuiltinType::Category dstCategory = BuiltinType::GetKindCategory(dstKind);

    CastExpr::CastKind kind;

    if (srcCategory == dstCategory) {
        uint32_t o = srcCategory == BuiltinType::FloatingPointKind ? CastExpr::FloatingCastExt : CastExpr::SignedCastExt;
        if (srcCategory == BuiltinType::UnsignedKind)
            o = CastExpr::UnsignedCastExt;

        if (srcBitWidth < dstBitWidth) {
            kind = (CastExpr::CastKind)o;
        } else {
            kind = (CastExpr::CastKind)(o + 1);
        }
    } else {
        if (srcCategory == BuiltinType::FloatingPointKind) {
            if (dstCategory == BuiltinType::SignedKind)
                kind = CastExpr::FloatingToSigned;
            else
                kind = CastExpr::FloatingToUnsigned;
        } else if (srcCategory == BuiltinType::SignedKind) {
            if (dstCategory == BuiltinType::FloatingPointKind)
                kind = CastExpr::SignedToFloating;
            else
                kind = CastExpr::SignedToUnsigned;
        } else if (srcCategory == BuiltinType::UnsignedKind) {
            if (dstCategory == BuiltinType::FloatingPointKind)
                kind = CastExpr::UnsignedToFloating;
            else
                kind = CastExpr::UnsignedToSigned;
        }
    }

    Expr* castExpr = CastExpr::Create(astContext, ActOnLoad(expr), kind, toType, range);
    return castExpr;
}

VCL::Expr* VCL::Sema::ActOnSplat(Expr* expr, SourceRange range) {
    Type* type = expr->GetResultType().GetType();
    if (type->IsDependent())
        return expr;
    if (type->GetTypeClass() != Type::BuiltinTypeClass) {
        diagnosticReporter.Error(Diagnostic::InvalidSplat, TypePrinter::Print(type))
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }
    BuiltinType* builtinType = (BuiltinType*)type;
    if (builtinType->GetKind() == BuiltinType::Void) {
        diagnosticReporter.Error(Diagnostic::InvalidSplat, TypePrinter::Print(type))
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }
    return SplatExpr::Create(astContext, ActOnLoad(expr), range);
}

VCL::Expr* VCL::Sema::ActOnNumericConstant(Token* value) {
    llvm::StringRef valueStr{ value->range.start.GetPtr(), (size_t)(value->range.end.GetPtr() - value->range.start.GetPtr()) };
    if (value->isFloatingPoint) {
        double v = std::stod(valueStr.str());
        return NumericLiteralExpr::Create(astContext, ConstantScalar{ v }, value->range);
    } else {
        int64_t v = std::stoll(valueStr.str());

        if (v < std::numeric_limits<int8_t>::max())
            return NumericLiteralExpr::Create(astContext, ConstantScalar{ (int8_t)v }, value->range);
        if (v < std::numeric_limits<int16_t>::max())
            return NumericLiteralExpr::Create(astContext, ConstantScalar{ (int16_t)v }, value->range);
        if (v < std::numeric_limits<int32_t>::max())
            return NumericLiteralExpr::Create(astContext, ConstantScalar{ (int32_t)v }, value->range);

        return NumericLiteralExpr::Create(astContext, ConstantScalar{ v }, value->range);
    }
}

VCL::Expr* VCL::Sema::ActOnIdentifierExpr(SymbolRef symbolRef, SourceRange range) {
    NamedDecl* decl = LookupNamedDecl(symbolRef);
    if (decl == nullptr || !decl->IsValueDecl()) {
        diagnosticReporter.Error(Diagnostic::IdentifierUndefined, symbolRef.GetSymbolName()->GetName().str())
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }
    DeclRefExpr* expr = DeclRefExpr::Create(astContext, (ValueDecl*)decl, range);
    expr->SetValueCategory(Expr::LValue);
    return expr;
}

VCL::Expr* VCL::Sema::ActOnCallExpr(SymbolRef symbolRef, llvm::ArrayRef<Expr*> args, TemplateArgumentList* templateArgs, SourceRange range) {
    FunctionDecl* decl = nullptr;
    for (Expr* arg : args)
        if (arg->GetExprClass() != Expr::AggregateExprClass && arg->GetResultType().GetType()->IsDependent())
            return DependentCallExpr::Create(astContext, symbolRef, args, templateArgs, range);
    if (TemplateDecl* templateDecl = LookupTemplateDecl(symbolRef)) {
        FunctionDecl* templatedFunctionDecl = (FunctionDecl*)templateDecl->GetTemplatedNamedDecl();

        TemplateParameterList* parameters = templateDecl->GetTemplateParametersList();
        templateArgs = DeduceTemplateArgumentFromCall(templatedFunctionDecl, args, templateArgs, parameters);
        if (!templateArgs)
            return nullptr;
        
        if (!templateArgs->IsCanonical()) {
            templateArgs = ActOnTemplateArgumentList(templateArgs->GetArgs(), templateArgs->GetSourceRange(), true);
            if (!templateArgs)
                return nullptr;
        }

        if (templateArgs->IsDependent())
            return DependentCallExpr::Create(astContext, symbolRef, args, templateArgs, range);

        for (auto it = templateDecl->Begin(); it != templateDecl->End(); ++it) {
            if (it->GetDeclClass() != Decl::TemplateSpecializationDeclClass)
                continue;
            TemplateSpecializationDecl* specializationDecl = (TemplateSpecializationDecl*)it.Get();
            if (MatchTemplateArgumentList(specializationDecl->GetTemplateArgumentList(), templateArgs)) {
                decl = (FunctionDecl*)specializationDecl->GetNamedDecl();
                break;
            }
        }

        if (!decl) {
            TemplateInstantiator instantiator{ *this };
            if (!instantiator.AddTemplateArgumentListAndDecl(templateArgs, templateDecl))
                return nullptr;
            decl = instantiator.InstantiateTemplatedFunctionDecl(templateDecl);
            if (!decl)
                return nullptr;

            TemplateSpecializationDecl* specializationDecl = TemplateSpecializationDecl::Create(astContext, templateArgs, decl);
            templateDecl->InsertBack(specializationDecl);
        }
    } else if (NamedDecl* namedDecl = LookupNamedDecl(symbolRef)) {
        if (namedDecl->GetDeclClass() == Decl::FunctionDeclClass) {
            decl = (FunctionDecl*)namedDecl;
        } else {
            diagnosticReporter.Error(Diagnostic::IdentifierUndefined, symbolRef.GetSymbolName()->GetName().str())
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ range })
                .Report();
            return nullptr;
        }
    } else {
        diagnosticReporter.Error(Diagnostic::IdentifierUndefined, symbolRef.GetSymbolName()->GetName().str())
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    FunctionType* type = decl->GetType();
    if (type->GetParamsType().size() < args.size()) {
        diagnosticReporter.Error(Diagnostic::TooManyArgument)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    llvm::SmallVector<Expr*> trueArgs{};
    for (size_t i = 0; i < args.size(); ++i) {
        Expr* arg = args[i];
        QualType paramType = type->GetParamsType()[i];

        if (arg->GetExprClass() == Expr::AggregateExprClass)
            arg = ActOnCast(arg, paramType, arg->GetSourceRange());

        if (paramType.GetType()->IsDependent() || arg->GetResultType().GetType()->IsDependent()) {
            trueArgs.push_back(arg);
            continue;
        }
        
        if (!Type::IsCanonicallyEqual(arg->GetResultType().GetType(), paramType.GetType())) {
            diagnosticReporter.Error(Diagnostic::IncorrectType, TypePrinter::Print(arg->GetResultType()), TypePrinter::Print(paramType))
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ arg->GetSourceRange() })
                .Report();
            return nullptr;
        }

        if (paramType.GetType()->GetTypeClass() == Type::ReferenceTypeClass) {
            if (arg->GetValueCategory() != Expr::LValue) {
                diagnosticReporter.Error(Diagnostic::MustBeLValue)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ arg->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            if ((arg->GetResultType().GetQualifiers() & paramType.GetQualifiers()) != arg->GetResultType().GetQualifiers()) {
                diagnosticReporter.Error(Diagnostic::QualifierDropped, TypePrinter::Print(arg->GetResultType()), TypePrinter::Print(paramType))
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ arg->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            trueArgs.push_back(arg);
        } else {
            Expr* castedArg = ActOnCast(ActOnLoad(arg), paramType, arg->GetSourceRange());
            if (!castedArg)
                return nullptr;
            trueArgs.push_back(castedArg);
        }
    }

    if (trueArgs.size() < type->GetParamsType().size()) {
        diagnosticReporter.Error(Diagnostic::MissingArgument)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    return CallExpr::Create(astContext, decl, trueArgs, type->GetReturnType(), range);
}

VCL::Expr* VCL::Sema::ActOnAggregateExpr(llvm::ArrayRef<Expr*> elems, SourceRange range) {
    return AggregateExpr::Create(astContext, elems, range);
}

bool VCL::Sema::ActOnAggregateExpr(AggregateExpr* aggregate) {
    if (aggregate->GetResultType().GetType()->IsDependent())
        return true;
    Type* trueType = Type::GetCanonicalType(aggregate->GetResultType().GetType());
    
    switch (trueType->GetTypeClass())
    {
    case Type::ArrayTypeClass: {
        QualType ofType = ((ArrayType*)trueType)->GetElementType();
        uint64_t ofSize = ((ArrayType*)trueType)->GetElementCount();
        size_t elementCount = aggregate->GetElements().size();
        if (elementCount > ofSize) {
            diagnosticReporter.Error(Diagnostic::TooManyInitializerValue)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ aggregate->GetElements()[ofSize]->GetSourceRange() })
                .Report();
            return false;
        }
        for (size_t i = 0; i < ofSize; ++i) {
            if (i < elementCount) {
                Expr* expr = ActOnLoad(aggregate->GetElements()[i]);
                aggregate->SetElement(ActOnCast(expr, ofType, expr->GetSourceRange()), i);
                if (expr == nullptr)
                    return false;
            } else {
                aggregate->AddElement(NullExpr::Create(astContext, ofType, aggregate->GetSourceRange()));
            }
        }
        aggregate->SetResultType(trueType);
        return true;
    }
    case Type::RecordTypeClass: {
        RecordDecl* recordDecl = ((RecordType*)trueType)->GetRecordDecl();
        uint32_t i = 0;
        size_t elementCount = aggregate->GetElements().size();
        for (auto it = recordDecl->Begin(); it != recordDecl->End(); ++it) {
            if (it->GetDeclClass() != Decl::FieldDeclClass)
                continue;
            FieldDecl* fieldDecl = (FieldDecl*)it.Get();
            QualType ofType = fieldDecl->GetType();
            if (elementCount <= i) {
                aggregate->AddElement(NullExpr::Create(astContext, ofType, aggregate->GetSourceRange()));
            } else {
                Expr* expr = ActOnLoad(aggregate->GetElements()[i]);
                aggregate->SetElement(ActOnCast(expr, ofType, expr->GetSourceRange()), i);
                if (expr == nullptr)
                    return false;
            }
            ++i;
        }
        if (i < elementCount) {
            diagnosticReporter.Error(Diagnostic::TooManyInitializerValue)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ aggregate->GetElements()[i]->GetSourceRange() })
                .Report();
            return false;
        }
        aggregate->SetResultType(trueType);
        return true;
    }
    default:
        diagnosticReporter.Error(Diagnostic::InvalidAggregateType, TypePrinter::Print(aggregate->GetResultType()))
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ aggregate->GetSourceRange() })
            .Report();
        return false;
    }
}

VCL::FunctionDecl* VCL::Sema::GetFrontmostFunctionDecl() {
    Scope* currentScope = sm.GetScopeFront();

    while (currentScope != nullptr) {
        DeclContext* declContext = currentScope->GetDeclContext();
        
        if (declContext && declContext->GetDeclContextClass() == DeclContext::FunctionDeclContextClass)
            return (FunctionDecl*)declContext;

        currentScope = currentScope->GetParentScope();
    }

    return nullptr;
}

bool VCL::Sema::IsWithinALoop() {
    Scope* currentScope = sm.GetScopeFront();

    while (currentScope != nullptr) {
        if (currentScope->GetBreakScope() != nullptr)
            return true;
        currentScope = currentScope->GetParentScope();
    }

    return false;
}

bool VCL::Sema::TypePreferByReference(Type* type) {
    type = Type::GetCanonicalType(type);
    if (type->GetTypeClass() == Type::BuiltinTypeClass 
            || type->GetTypeClass() == Type::VectorTypeClass)
        return false;
    return true;
}

bool VCL::Sema::CheckTypeCastability(Type* type) {
    switch (type->GetTypeClass()) {
        case Type::BuiltinTypeClass:
            return ((BuiltinType*)type)->GetKind() != BuiltinType::Void;
        case Type::VectorTypeClass:
            return true;
        default:
            return false;
    }
}

VCL::BuiltinType::Kind VCL::Sema::GetScalarKindFromBuiltinOrVectorType(Type* type) {
    if (type->GetTypeClass() == Type::BuiltinTypeClass)
        return ((BuiltinType*)type)->GetKind();
    VectorType* vectorType = (VectorType*)type;
    return ((BuiltinType*)vectorType->GetElementType().GetType())->GetKind();
}

bool VCL::Sema::IsCurrentScopeGlobal() {
    DeclContext* frontDeclContext = sm.GetScopeFront()->GetDeclContext();
    if (!frontDeclContext)
        return false;
    return frontDeclContext->GetDeclContextClass() == DeclContext::TranslationUnitDeclContextClass;
}

std::variant<VCL::Type*, VCL::ConstantScalar> VCL::Sema::RecursivelyDeduceTemplateArgument(NamedDecl* parameter, TemplateSpecializationType* a, TemplateSpecializationType* b) {
    TemplateSpecializationType* aSpe = (TemplateSpecializationType*)a;
    TemplateSpecializationType* bSpe = (TemplateSpecializationType*)b;
    IdentifierInfo* aId = aSpe->GetTemplateDecl()->GetTemplatedNamedDecl()->GetIdentifierInfo();
    IdentifierInfo* bId = bSpe->GetTemplateDecl()->GetTemplatedNamedDecl()->GetIdentifierInfo();
    if (aSpe->GetTemplateDecl() != bSpe->GetTemplateDecl() && !(aId == bId && aId->IsKeyword() == true))
        return { nullptr };

    for (int i = 0; i < aSpe->GetTemplateArgumentList()->GetCount(); ++i) {
        TemplateArgument aTemplateArg = aSpe->GetTemplateArgumentList()->GetArgs()[i];
        TemplateArgument bTemplateArg = bSpe->GetTemplateArgumentList()->GetArgs()[i];

        if (aTemplateArg.GetKind() != TemplateArgument::Type) {
            switch (bTemplateArg.GetKind()) {
                case TemplateArgument::Integral:
                    return bTemplateArg.GetIntegral();
                case TemplateArgument::Expression: {
                    ExprEvaluator eval{ astContext };
                    ConstantValue* value = eval.Visit(bTemplateArg.GetExpr());
                    if (!value)
                        return { nullptr };
                    if (value->GetConstantValueClass() != ConstantValue::ConstantScalarClass)
                        return { nullptr };
                    return *((ConstantScalar*)value);
                }
                default:
                    return { nullptr };
            }
        }
            
        Type* aTemplateArgType = aTemplateArg.GetType().GetType();
        if (aTemplateArgType->GetTypeClass() == Type::TemplateTypeParamTypeClass) {
            TemplateTypeParamType* aTemplateParamType = (TemplateTypeParamType*)aTemplateArgType;
            if (aTemplateParamType->GetTemplateTypeParamDecl() == parameter)
                return bTemplateArg.GetType().GetType();
        }

        if (aTemplateArgType->GetTypeClass() == Type::TemplateSpecializationTypeClass) {
            if (bTemplateArg.GetKind() != TemplateArgument::Type)
                continue;
            Type* bTemplateArgType = bTemplateArg.GetType().GetType();
            if (aTemplateArgType->GetTypeClass() != bTemplateArgType->GetTypeClass())
                continue;
            std::variant<Type*, ConstantScalar> r = RecursivelyDeduceTemplateArgument(parameter, 
                (TemplateSpecializationType*)aTemplateArgType, (TemplateSpecializationType*)bTemplateArgType);
            if (std::holds_alternative<Type*>(r)) {
                Type* resultType = std::get<Type*>(r);
                if (!resultType)
                    continue;
                return resultType;
            } else {
                return r;
            }
        }
    }

    return { nullptr };
}

VCL::TemplateArgumentList* VCL::Sema::DeduceTemplateArgumentFromCall(
        FunctionDecl* functionDecl, llvm::ArrayRef<Expr*> args, TemplateArgumentList* templateArgs, TemplateParameterList* parameters) {
    llvm::ArrayRef<NamedDecl*> paramArray = parameters->GetParams();
    FunctionType* functionType = functionDecl->GetType();

    llvm::SmallVector<TemplateArgument> newTemplateArgs{};

    for (size_t i = 0; i < parameters->GetParams().size(); ++i) {
        if (templateArgs && templateArgs->GetCount() > i) {
            newTemplateArgs.push_back(templateArgs->GetArgs()[i]);
            continue;
        }

        bool deduced = false;

        for (size_t j = 0; j < functionType->GetParamsType().size(); ++j) {
            if (args[j]->GetExprClass() == Expr::AggregateExprClass)
                continue;

            Type* paramType = functionType->GetParamsType()[j].GetType();
            Type* argType = args[j]->GetResultType().GetType();

            if (paramType->GetTypeClass() == Type::ReferenceTypeClass)
                paramType = ((ReferenceType*)paramType)->GetType().GetType();

            if (paramType->GetTypeClass() != Type::TemplateTypeParamTypeClass && 
                    paramType->GetTypeClass() == Type::TemplateSpecializationTypeClass) {
                if (paramType->GetTypeClass() != argType->GetTypeClass())
                    continue;

                TemplateSpecializationType* paramTypeSpe = (TemplateSpecializationType*)paramType;
                TemplateSpecializationType* argTypeSpe = (TemplateSpecializationType*)argType;
                
                std::variant<Type*, ConstantScalar> r = RecursivelyDeduceTemplateArgument(paramArray[i], paramTypeSpe, argTypeSpe);
                if (std::holds_alternative<Type*>(r)) {
                    Type* resultType = std::get<Type*>(r);
                    if (!resultType)
                        continue;
                    newTemplateArgs.push_back(TemplateArgument{ resultType });
                    deduced = true;
                    break;
                } else {
                    newTemplateArgs.push_back(TemplateArgument{ std::get<ConstantScalar>(r) });
                    deduced = true;
                    break;
                }
            }

            if (paramType->GetTypeClass() != Type::TemplateTypeParamTypeClass)
                continue;

            TemplateTypeParamType* templateParamType = (TemplateTypeParamType*)paramType;
            if (templateParamType->GetTemplateTypeParamDecl() == paramArray[i]) {
                newTemplateArgs.push_back(TemplateArgument{ argType });
                deduced = true;
                break;
            }
        }

        if (!deduced) {
            diagnosticReporter.Error(Diagnostic::CannotDeduceTemplateArgument, functionDecl->GetIdentifierInfo()->GetName().str())
                .AddHint(DiagnosticHint{ functionDecl->GetSourceRange() })
                .AddHint(DiagnosticHint{ paramArray[i]->GetSourceRange(), DiagnosticHint::Declared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
    }

    return TemplateArgumentList::Create(astContext, newTemplateArgs, SourceRange{});
}

bool VCL::Sema::MatchTemplateArgumentList(TemplateArgumentList* args1, TemplateArgumentList* args2) {
    if (args1->GetCount() != args2->GetCount())
        return false;
    
    for (size_t i = 0; i < args1->GetCount(); ++i) {
        TemplateArgument arg1 = args1->GetArgs()[i];
        TemplateArgument arg2 = args2->GetArgs()[i];

        if (arg1.GetKind() != arg2.GetKind())
            return false;

        switch (arg1.GetKind()) {
            case TemplateArgument::Type: {
                if (!Type::IsCanonicallyEqual(arg1.GetType().GetType(), arg2.GetType().GetType()))
                    return false;
                break;
            }
            case TemplateArgument::Integral: {
                if (arg1.GetIntegral() != arg2.GetIntegral())
                    return false;
                break;
            }
            default: return false;
        }
    }

    return true;
}