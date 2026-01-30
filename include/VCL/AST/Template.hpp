#pragma once

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
                case Kind::Expression: return GetExpr()->GetResultType().GetType()->IsDependent();
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

        static inline TemplateArgumentList* Create(ASTContext& context, llvm::ArrayRef<TemplateArgument> args, SourceRange range) {
            size_t size = totalSizeToAlloc<TemplateArgument>(args.size());
            void* ptr = context.Allocate(sizeof(TemplateArgumentList) + size);
            return new(ptr) TemplateArgumentList{ args, range };
        }

    private:
        size_t argCount;
        SourceRange range;
    };

}