#pragma once

#include <VCL/AST/Type.hpp>
#include <VCL/AST/Stmt.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/ConstantValue.hpp>
#include <VCL/AST/Operator.hpp>
#include <VCL/AST/ASTContext.hpp>


namespace VCL {

    class Expr : public ValueStmt {
    public:
        enum ExprClass {
            NumericLiteralExprClass,
            LoadExprClass,
            SplatExprClass,
            DeclRefExprClass,
            CastExprClass,
            BinaryExprClass,
            UnaryExprClass,
            CallExprClass,
            FieldAccessExprClass,
            SubscriptExprClass,
            NullExprClass,
            AggregateExprClass
        };

        enum ValueCategory {
            LValue,
            RValue
        };

    public:
        Expr() = delete;
        Expr(ExprClass exprClass) : exprClass{ exprClass }, resultType{}, constantValue{ }, valueCategory{ RValue } {}
        Expr(ExprClass exprClass, QualType resultType) : exprClass{ exprClass }, resultType{ resultType }, constantValue{ }, valueCategory{ RValue } {}
        Expr(const Expr& other) = delete;
        Expr(Expr&& other) = delete;
        ~Expr() = default;

        Expr& operator=(const Expr& other) = delete;
        Expr& operator=(Expr&& other) = delete;

        inline ExprClass GetExprClass() const { return exprClass; }
        inline ValueCategory GetValueCategory() const { return valueCategory; }
        inline void SetValueCategory(ValueCategory valueCategory) { this->valueCategory =valueCategory; }
        inline QualType GetResultType() const { return resultType; }
        inline void SetResultType(QualType resultType) { this->resultType = resultType; }
        inline ConstantValue* GetConstantValue() const { return constantValue; }
        inline void SetConstantValue(ConstantValue* constantValue) { this->constantValue = constantValue; }

    private:
        ExprClass exprClass;
        ValueCategory valueCategory;
        QualType resultType;
        ConstantValue* constantValue;
    };

    class NumericLiteralExpr : public Expr {
    public:
        NumericLiteralExpr(ConstantScalar scalar) : scalar{ scalar }, Expr{ Expr::NumericLiteralExprClass } {
            SetConstantValue(&(this->scalar));
        }
        ~NumericLiteralExpr() = default;

        inline ConstantScalar* GetValue() { return &scalar; }

        static inline NumericLiteralExpr* Create(ASTContext& context, ConstantScalar scalar, SourceRange range) {
            NumericLiteralExpr* instance = context.AllocateNode<NumericLiteralExpr>(scalar);
            BuiltinType* resultType = context.GetTypeCache().GetOrCreateBuiltinType(instance->scalar.GetKind());
            instance->SetResultType(resultType);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        ConstantScalar scalar;
    };

    // LValue2RValue
    class LoadExpr : public Expr {
    public:
        LoadExpr(Expr* expr): expr{ expr }, Expr{ Expr::LoadExprClass } {}
        ~LoadExpr() = default;

        inline Expr* GetExpr() { return expr; }

        static inline LoadExpr* Create(ASTContext& context, Expr* expr, SourceRange range) {
            QualType resultType = expr->GetResultType();
            if (resultType.GetType()->GetTypeClass() == Type::ReferenceTypeClass)
                resultType = ((ReferenceType*)resultType.GetType())->GetType();
            LoadExpr* instance = context.AllocateNode<LoadExpr>(expr);
            instance->SetResultType(resultType);
            instance->SetSourceRange(range);
            return instance;
        }
        
    private:
        Expr* expr;
    };

    class DeclRefExpr : public Expr {
    public:
        DeclRefExpr(ValueDecl* decl) : decl{ decl }, Expr{ Expr::DeclRefExprClass } {}
        ~DeclRefExpr() = default;    

        inline ValueDecl* GetValueDecl() const { return decl; }

        static inline DeclRefExpr* Create(ASTContext& context, ValueDecl* decl, SourceRange range) {
            DeclRefExpr* instance = context.AllocateNode<DeclRefExpr>(decl);
            instance->SetResultType(decl->GetValueType());
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        ValueDecl* decl;
    };
    
    class SplatExpr : public Expr {
    public:
        SplatExpr(Expr* expr) : expr{ expr }, Expr{ Expr::SplatExprClass } {}
        ~SplatExpr() = default;    

        inline Expr* GetExpr() const { return expr; }

        static inline SplatExpr* Create(ASTContext& context, Expr* expr, SourceRange range) {
            SplatExpr* instance = context.AllocateNode<SplatExpr>(expr);
            Type* elemType = expr->GetResultType().GetType();
            VectorType* vectorType = context.GetTypeCache().GetOrCreateVectorType(QualType{ elemType });
            instance->SetResultType(QualType{ vectorType, expr->GetResultType().GetQualifiers() });
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        Expr* expr;
    };

    class CastExpr : public Expr {
    public:
        enum CastKind {
            FloatingCastExt = 0,
            FloatingCastTrunc,
            FloatingToSigned,
            FloatingToUnsigned,
            SignedCastExt,
            SignedCastTrunc,
            SignedToFloating,
            SignedToUnsigned,
            UnsignedCastExt,
            UnsignedCastTrunc,
            UnsignedToFloating,
            UnsignedToSigned
        }; 

    public:
        CastExpr(Expr* expr, CastKind kind) : expr{ expr }, kind{ kind }, Expr{ Expr::CastExprClass } {}
        ~CastExpr() = default;

        inline Expr* GetExpr() { return expr; }
        inline CastKind GetCastKind() const { return kind; }

        static inline CastExpr* Create(ASTContext& context, Expr* expr, CastKind kind, QualType type, SourceRange range) {
            CastExpr* instance = context.AllocateNode<CastExpr>(expr, kind);
            instance->SetResultType(type);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        Expr* expr;
        CastKind kind;
    };

    class BinaryExpr : public Expr {
    public:
        BinaryExpr(Expr* lhs, Expr* rhs, BinaryOperator::Kind op) :
            lhs{ lhs }, rhs{ rhs }, op{ op }, Expr{ Expr::BinaryExprClass } {}
        ~BinaryExpr() = default;

        inline Expr* GetLHS() { return lhs; }
        inline Expr* GetRHS() { return rhs; }
        inline BinaryOperator::Kind GetOperatorKind() const { return op; }

        static inline BinaryExpr* Create(ASTContext& context, Expr* lhs, Expr* rhs, BinaryOperator& op) {
            BinaryExpr* instance = context.AllocateNode<BinaryExpr>(lhs, rhs, op.kind);
            instance->SetResultType(lhs->GetResultType());
            instance->SetSourceRange(SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end });
            return instance;
        }

        static inline BinaryExpr* Create(ASTContext& context, Expr* lhs, Expr* rhs, BinaryOperator::Kind op) {
            BinaryExpr* instance = context.AllocateNode<BinaryExpr>(lhs, rhs, op);
            instance->SetResultType(lhs->GetResultType());
            instance->SetSourceRange(SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end });
            return instance;
        }

    private:
        Expr* lhs;
        Expr* rhs;
        BinaryOperator::Kind op;
    };

    class UnaryExpr : public Expr {
    public:
        UnaryExpr(Expr* expr, UnaryOperator op) : 
            expr{ expr }, op{ op }, Expr{ Expr::UnaryExprClass } {}
        ~UnaryExpr() = default;

        inline Expr* GetExpr() { return expr; }
        inline UnaryOperator GetOperator() const { return op; }

        static inline UnaryExpr* Create(ASTContext& context, Expr* expr, UnaryOperator op, SourceRange range) {
            UnaryExpr* instance = context.AllocateNode<UnaryExpr>(expr, op);
            instance->SetResultType(expr->GetResultType());
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        Expr* expr;
        UnaryOperator op;
    };

    class CallExpr final : public Expr, private llvm::TrailingObjects<CallExpr, Expr*> {
        friend TrailingObjects;

    public:
        CallExpr(FunctionDecl* decl, llvm::ArrayRef<Expr*> args)
                : decl{ decl }, argsCount{ args.size() }, Expr{ Expr::CallExprClass } {
            std::uninitialized_copy(args.begin(), args.end(), getTrailingObjects());
        }
        ~CallExpr() = default;

        inline FunctionDecl* GetFunctionDecl() { return decl; }
        inline llvm::ArrayRef<Expr*> GetArgs() { return { getTrailingObjects(), argsCount }; }

        static inline CallExpr* Create(ASTContext& context, FunctionDecl* decl, llvm::ArrayRef<Expr*> args, QualType returnType, SourceRange range) {
            size_t size = totalSizeToAlloc<Expr*>(args.size());
            void* ptr = context.Allocate(sizeof(CallExpr) + size);
            CallExpr* instance = new (ptr) CallExpr{ decl, args };
            instance->SetResultType(returnType);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        FunctionDecl* decl;
        size_t argsCount;
    };

    class FieldAccessExpr : public Expr {
    public:
        FieldAccessExpr(Expr* expr, RecordType* recordType, uint32_t fieldIndex) 
                : expr{ expr }, recordType{ recordType }, fieldIndex{ fieldIndex }, Expr{ Expr::FieldAccessExprClass } {
            SetValueCategory(Expr::LValue);
        }
        ~FieldAccessExpr() = default;

        inline Expr* GetExpr() { return expr; }
        inline RecordType* GetRecordType() { return recordType; }
        inline uint32_t GetFieldIndex() { return fieldIndex; }

        static inline FieldAccessExpr* Create(ASTContext& context, Expr* expr, RecordType* recordType, 
                uint32_t fieldIndex, QualType resultType, SourceRange range) {
            FieldAccessExpr* instance = context.AllocateNode<FieldAccessExpr>(expr, recordType, fieldIndex);
            instance->SetResultType(resultType);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        Expr* expr;
        RecordType* recordType;
        uint32_t fieldIndex;
    };

    class SubscriptExpr : public Expr {
    public:
        SubscriptExpr(Expr* expr, Expr* index, bool isSpan) 
                : expr{ expr }, index{ index }, isSpan{ isSpan }, Expr{ Expr::SubscriptExprClass } {
            SetValueCategory(Expr::LValue);
        }
        ~SubscriptExpr() = default;

        inline Expr* GetExpr() { return expr; }
        inline Expr* GetIndex() { return index; }
        inline bool IsSpan() const { return isSpan; }

        static inline SubscriptExpr* Create(ASTContext& context, Expr* expr, Expr* index, QualType resultType, SourceRange range) {
            bool isSpan = Type::GetCanonicalType(expr->GetResultType().GetType())->GetTypeClass() == Type::SpanTypeClass;
            SubscriptExpr* instance = context.AllocateNode<SubscriptExpr>(expr, index, isSpan);
            instance->SetResultType(resultType);
            instance->SetSourceRange(range);
            return instance;
        }
        
    private:
        Expr* expr;
        Expr* index;
        bool isSpan;
    };

    class NullExpr : public Expr {
    public:
        NullExpr() : Expr{ Expr::NullExprClass } {}
        ~NullExpr() = default;

        static inline NullExpr* Create(ASTContext& context, QualType type, SourceRange range) {
            NullExpr* instance = context.AllocateNode<NullExpr>();
            instance->SetResultType(type);
            instance->SetSourceRange(range);
            return instance;
        }
    };

    class AggregateExpr : public Expr {
    public:
        AggregateExpr(llvm::ArrayRef<Expr*> elems) : elements{ elems }, Expr{ Expr::AggregateExprClass } {}
        ~AggregateExpr() = default;

        llvm::ArrayRef<Expr*> GetElements() { return { elements.begin(), elements.size() }; }
        Expr* GetElement(size_t index) { return elements[index]; }
        void SetElement(Expr* expr, size_t index) { elements[index] = expr; }
        void AddElement(Expr* expr) { elements.push_back(expr); }
        size_t GetElementCount() const { return elements.size(); }

        static inline AggregateExpr* Create(ASTContext& context, llvm::ArrayRef<Expr*> elements, SourceRange range) {
            AggregateExpr* instance = context.AllocateNode<AggregateExpr>(elements);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        llvm::SmallVector<Expr*> elements{};
    };

}