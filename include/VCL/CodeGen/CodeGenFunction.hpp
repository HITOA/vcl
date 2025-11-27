#pragma once

#include <VCL/AST/Decl.hpp>
#include <VCL/AST/Expr.hpp>

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>


namespace VCL {
    class CodeGenModule;

    class CodeGenFunction {
    public:
        CodeGenFunction() = delete;
        CodeGenFunction(CodeGenModule& cgm);
        CodeGenFunction(const CodeGenFunction& other) = delete;
        CodeGenFunction(CodeGenFunction&& other) = delete;
        ~CodeGenFunction() = default;

        CodeGenFunction& operator=(const CodeGenFunction& other) = delete;
        CodeGenFunction& operator=(CodeGenFunction&& other) = delete;

        llvm::Function* Generate(FunctionDecl* decl);

        llvm::AllocaInst* GenerateAllocaInst(llvm::Type* type, llvm::StringRef name);

        llvm::Value* GetDeclValue(Decl* decl);

        // CGStmt

        bool GenerateStmt(Stmt* stmt);
        bool GenerateValueStmt(ValueStmt* stmt);
        bool GenerateDeclStmt(DeclStmt* stmt);
        bool GenerateCompoundStmt(CompoundStmt* stmt);
        bool GenerateReturnStmt(ReturnStmt* stmt);

        // CGDecl

        bool GenerateDecl(Decl* decl);
        bool GenerateVarDecl(VarDecl* decl);

        // CGExpr

        llvm::Value* GenerateExpr(Expr* expr);
        llvm::Value* GenerateNumericLiteralExpr(NumericLiteralExpr* expr);
        llvm::Value* GenerateLoadExpr(LoadExpr* expr);
        llvm::Value* GenerateDeclRefExpr(DeclRefExpr* expr);
        llvm::Value* GenerateCastExpr(CastExpr* expr);
        llvm::Value* GenerateSplatExpr(SplatExpr* expr);
        llvm::Value* GenerateBinaryExpr(BinaryExpr* expr);
        llvm::Value* GenerateUnaryExpr(UnaryExpr* expr);
        llvm::Value* GenerateCallExpr(CallExpr* expr);
        llvm::Value* GenerateFieldAccessExpr(FieldAccessExpr* expr);
        llvm::Value* GenerateSubscriptExpr(SubscriptExpr* expr);

        llvm::Value* DispatchBinaryArithmeticOp(Expr* lhs, Expr* rhs, llvm::Instruction::BinaryOps signedop, 
            llvm::Instruction::BinaryOps unsignedop, llvm::Instruction::BinaryOps floatop);
        llvm::Value* DispatchBinaryArithmeticOp(Expr* lhs, Expr* rhs, llvm::Instruction::BinaryOps signedop, 
            llvm::Instruction::BinaryOps floatop);

        llvm::Value* DispatchBinaryComparisonOp(Expr* lhs, Expr* rhs, llvm::CmpInst::Predicate signedPredicate,
            llvm::CmpInst::Predicate unsignedPredicate, llvm::CmpInst::Predicate floatPredicate);
        llvm::Value* DispatchBinaryComparisonOp(Expr* lhs, Expr* rhs, llvm::CmpInst::Predicate signedPredicate,
            llvm::CmpInst::Predicate floatPredicate);

    private:
        CodeGenModule& cgm;
        
        llvm::Function* function;
        llvm::IRBuilder<> builder;

        llvm::DenseMap<Decl*, llvm::Value*> locals;
    };
    
}