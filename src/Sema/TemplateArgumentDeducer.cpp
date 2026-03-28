#include <VCL/Sema/TemplateArgumentDeducer.hpp>

#include <VCL/AST/Type.hpp>
#include <VCL/AST/Expr.hpp>
#include <VCL/Sema/Sema.hpp>
#include <VCL/Core/Diagnostic.hpp>

#include <llvm/ADT/SmallVector.h>


VCL::TemplateArgumentDeducer::TemplateArgumentDeducer(Sema& sema, TemplateParameterList* parameters) : 
    sema{ sema }, parameters{ parameters }, substitutionMap{} {
    for (size_t i = 0; i < parameters->GetParams().size(); ++i) {
        substitutionMap.insert({ parameters->GetParams()[i], QualType{ nullptr } });
    }
}


bool VCL::TemplateArgumentDeducer::FillSubstitutionMapWithTemplateArgumentList(TemplateArgumentList* args) {
    if (!args)
        return true;
    if (args->GetArgs().size() > parameters->GetParams().size()) {
        sema.GetDiagnosticReporter().Error(Diagnostic::TooManyTemplateArgument)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ args->GetSourceRange() })
            .AddHint(DiagnosticHint{ parameters->GetSourceRange(), DiagnosticHint::Declared })
            .Report();
        return false;
    }
    for (size_t i = 0; i < args->GetArgs().size(); ++i) {
        const TemplateArgument& arg = args->GetArgs()[i];
        NamedDecl* param = parameters->GetParams()[i];
        switch (param->GetDeclClass()) {
            case Decl::TemplateTypeParamDeclClass: {
                if (arg.GetKind() != TemplateArgument::Type) {
                    if (param->GetSourceRange().start.GetPtr() != nullptr) {
                        sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                            .SetCompilerInfo(__FILE__, __func__, __LINE__)
                            .AddHint(DiagnosticHint{ arg.GetSourceRange() })
                            .AddHint(DiagnosticHint{ param->GetSourceRange(), DiagnosticHint::Declared })
                            .Report();
                    } else {
                        sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                            .SetCompilerInfo(__FILE__, __func__, __LINE__)
                            .AddHint(DiagnosticHint{ arg.GetSourceRange() })
                            .Report();
                    }
                    return false;
                }
                break;
            }
            case Decl::NonTypeTemplateParamDeclClass: {
                if (arg.GetKind() != TemplateArgument::Integral) {
                    if (param->GetSourceRange().start.GetPtr() != nullptr) {
                        sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                            .SetCompilerInfo(__FILE__, __func__, __LINE__)
                            .AddHint(DiagnosticHint{ arg.GetSourceRange() })
                            .AddHint(DiagnosticHint{ param->GetSourceRange(), DiagnosticHint::Declared })
                            .Report();
                    } else {
                        sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                            .SetCompilerInfo(__FILE__, __func__, __LINE__)
                            .AddHint(DiagnosticHint{ arg.GetSourceRange() })
                            .Report();
                    }
                    return false;
                }
                break;
            }
            default:
                sema.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ args->GetSourceRange() })
                    .Report();
                return false;
        }
    }
    for (size_t i = 0; i < args->GetArgs().size(); ++i)
        substitutionMap[parameters->GetParams()[i]] = args->GetArgs()[i];
    return true;
}

bool VCL::TemplateArgumentDeducer::IsDeductionComplete() {
    for (const auto& it : substitutionMap) {
        if (it.second.GetKind() == TemplateArgument::Type && it.second.GetType().GetAsOpaquePtr() == 0)
            return false;
    }
    return true;
}
#include <VCL/AST/TypePrinter.hpp>
#include <iostream>
bool VCL::TemplateArgumentDeducer::DeduceForType(Type* baseType, Type* substitutedType) {
    switch (baseType->GetTypeClass())
    {
        case Type::ReferenceTypeClass:
            DeduceForType(((ReferenceType*)baseType)->GetType().GetType(), substitutedType);
        case Type::TemplateTypeParamTypeClass: {
            TemplateTypeParamType* type = (TemplateTypeParamType*)baseType;
            if (!substitutionMap.count(type->GetTemplateTypeParamDecl())) {
                sema.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return false;
            }
            TemplateArgument arg = substitutionMap.at(type->GetTemplateTypeParamDecl());
            if (arg.GetKind() != TemplateArgument::Type) {
                sema.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return false;
            }
            if (arg.GetType().GetAsOpaquePtr() != 0 && !Type::IsCanonicallyEqual(arg.GetType().GetType(), substitutedType)) {
                sema.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return false;
            }
            if (arg.GetType().GetAsOpaquePtr() == 0)
                substitutionMap[type->GetTemplateTypeParamDecl()] = TemplateArgument{ QualType{ substitutedType } };
            return true;
        }
        case Type::TemplateSpecializationTypeClass: {
            if (substitutedType->GetTypeClass() != Type::TemplateSpecializationTypeClass) {
                std::cout << TypePrinter::Print(substitutedType) << std::endl;
                std::cout << substitutedType->GetTypeClass() << std::endl;
                sema.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return false;
            }
            TemplateSpecializationType* baseTypeSpe = (TemplateSpecializationType*)baseType;
            TemplateSpecializationType* substitutedTypeSpe = (TemplateSpecializationType*)substitutedType;
            if (baseTypeSpe->GetTemplateDecl() != substitutedTypeSpe->GetTemplateDecl()) {
                sema.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return false;
            }

            TemplateArgumentList* baseArgs = baseTypeSpe->GetTemplateArgumentList();
            TemplateArgumentList* substitutedArgs = substitutedTypeSpe->GetTemplateArgumentList();
            for (size_t i = 0; i < baseArgs->GetArgs().size(); ++i) {
                TemplateArgument baseArg = baseArgs->GetArgs()[i];
                TemplateArgument substitutedArg = substitutedArgs->GetArgs()[i];
                if (baseArg.GetKind() == TemplateArgument::Type) {
                    if (!DeduceForType(baseArg.GetType().GetType(), substitutedArg.GetType().GetType()))
                        return false;
                } else if (baseArg.GetKind() == TemplateArgument::Expression) {
                    if (baseArg.GetExpr()->GetExprClass() != Expr::DeclRefExprClass)
                        continue;
                    ValueDecl* decl = ((DeclRefExpr*)baseArg.GetExpr())->GetValueDecl();
                    if (!substitutionMap.count(decl))
                        continue;
                    if (substitutionMap.at(decl).GetKind() == TemplateArgument::Type)
                        substitutionMap[decl] = substitutedArg;
                }
            }
        }
        default:
            return true;
    }
}

VCL::TemplateArgumentList* VCL::TemplateArgumentDeducer::BuildTemplateArgumentList(bool makeCanonical) {
    llvm::SmallVector<TemplateArgument> args{};

    for (size_t i = 0; i < parameters->GetParams().size(); ++i) {
        TemplateArgument arg = substitutionMap.at(parameters->GetParams()[i]);
        if (arg.GetKind() == TemplateArgument::Type && arg.GetType().GetAsOpaquePtr() == 0)
            return nullptr;
        args.push_back(arg);
    }

    return sema.ActOnTemplateArgumentList(args, {}, makeCanonical);
}