#pragma once

#include <VCL/Core/Hasher.hpp>
#include <VCL/AST/Type.hpp>
#include <VCL/AST/Expr.hpp>
#include <VCL/AST/ConstantValue.hpp>

#include <llvm/Support/TrailingObjects.h>

namespace VCL {

    class TemplateArgument {
    public:
        enum Kind {
            None,
            Type,
            Integral,
            Expression
        };
    
    public:
        TemplateArgument() : kind{ Kind::None } {}
        TemplateArgument(QualType type) : type{ type }, kind{ Kind::Type } {}
        TemplateArgument(ConstantScalar integral) : integral{ integral }, kind{ Kind::Integral } {}
        TemplateArgument(Expr* expr) : expr{ expr }, kind{ Kind::Expression } {}
        TemplateArgument(const TemplateArgument& other) = default;
        TemplateArgument(TemplateArgument&& other) = default;
        ~TemplateArgument() = default;

        TemplateArgument& operator=(const TemplateArgument& other) = default;
        TemplateArgument& operator=(TemplateArgument&& other) = default;

        inline Kind GetKind() const { return kind; }

        inline QualType GetType() const { return type; }
        inline ConstantScalar GetIntegral() const { return integral; }
        inline Expr* GetExpr() const { return expr; }

        inline bool IsDependent() const { 
            switch (kind) {
                case Kind::Type: return GetType().GetType()->IsDependent();
                case Kind::Expression: return GetExpr()->IsDependent();
                default: return false;
            }
        }

        inline SourceRange GetSourceRange() const { return range; }
        inline void SetSourceRange(SourceRange range) { this->range = range; }

    private:
        union {
            QualType type;
            ConstantScalar integral;
            Expr* expr;
        };
        Kind kind;
        SourceRange range;
    };

    class TemplateArgumentList final : private llvm::TrailingObjects<TemplateArgumentList, TemplateArgument> {
        friend TrailingObjects;

    public:
        TemplateArgumentList() = delete;
        TemplateArgumentList(llvm::ArrayRef<TemplateArgument> args, SourceRange range) : argCount{ args.size() }, range{ range } {
            std::uninitialized_copy(args.begin(), args.end(), getTrailingObjects());
        }
        TemplateArgumentList(const TemplateArgumentList& other) = delete;
        TemplateArgumentList(TemplateArgumentList&& other) = delete;
        ~TemplateArgumentList() = default;

        TemplateArgumentList& operator=(const TemplateArgumentList& other) = delete;
        TemplateArgumentList& operator=(TemplateArgumentList&& other) = delete;

        inline llvm::ArrayRef<TemplateArgument> GetArgs() const { return { getTrailingObjects(), argCount }; }
        inline SourceRange GetSourceRange() const { return range; }

        inline TemplateArgument* GetData() { return getTrailingObjects(); }
        inline size_t GetCount() const { return argCount; }

        inline bool IsDependent() const {
            for (TemplateArgument arg : GetArgs())
                if (arg.IsDependent())
                    return true;
            return false;
        }

        inline bool IsCanonical() const {
            for (TemplateArgument arg : GetArgs()) {
                if (arg.GetKind() == TemplateArgument::Expression)
                    return false;
                if (arg.GetKind() == TemplateArgument::Type)
                    if (Type::GetCanonicalType(arg.GetType().GetType()) != arg.GetType().GetType())
                        return false;
            }
            return true;
        }

        inline uint64_t GetHash() const {
            Hasher hasher{};
            for (TemplateArgument arg : GetArgs()) {
                switch (arg.GetKind()) {
                    case TemplateArgument::Type:
                        hasher.Hash((uint64_t)arg.GetType().GetAsOpaquePtr());
                        break;
                    case TemplateArgument::Integral:
                        hasher.Hash(arg.GetIntegral().Get<uint64_t>());
                        break;
                    default:
                        hasher.Hash((uint64_t)arg.GetExpr());
                        break;
                }
            }
            return hasher.Get();
        }

        static inline TemplateArgumentList* Create(ASTContext& context, llvm::ArrayRef<TemplateArgument> args, SourceRange range) {
            size_t size = additionalSizeToAlloc<TemplateArgument>(args.size());
            void* ptr = context.Allocate(sizeof(TemplateArgumentList) + size);
            return new(ptr) TemplateArgumentList{ args, range };
        }

    private:
        size_t argCount;
        SourceRange range;
    };

}