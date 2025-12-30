#pragma once

#include <VCL/AST/Decl.hpp>

#include <llvm/Support/TrailingObjects.h>


namespace VCL {

    class Stmt {
    public:
        enum StmtClass {
            ValueStmtClass,
            DeclStmtClass,
            CompoundStmtClass,
            ReturnStmtClass
        };

    public:
        Stmt() = delete;
        Stmt(StmtClass stmtClass) : stmtClass{ stmtClass } {}
        Stmt(const Stmt& other) = delete;
        Stmt(Stmt&& other) = delete;
        ~Stmt() = default;

        Stmt& operator=(const Stmt& other) = delete;
        Stmt& operator=(Stmt&& other) = delete;

        inline StmtClass GetStmtClass() const { return stmtClass; }
        inline SourceRange GetSourceRange() const { return range; }
        inline void SetSourceRange(SourceRange range) { this->range = range; }

    private:
        StmtClass stmtClass;
        SourceRange range;
    };

    class ValueStmt : public Stmt {
    public:
        ValueStmt() : Stmt{ Stmt::ValueStmtClass } {}
        ~ValueStmt() = default;
    };

    class DeclStmt : public Stmt {
    public:
        DeclStmt(Decl* decl) : decl{ decl }, Stmt{ Stmt::DeclStmtClass } {}
        ~DeclStmt() = default;

        inline Decl* GetDecl() { return decl; }

        static DeclStmt* Create(ASTContext& context, Decl* decl, SourceRange range) {
            DeclStmt* stmt = context.AllocateNode<DeclStmt>(decl);
            stmt->SetSourceRange(range);
            return stmt;
        }

    private:
        Decl* decl;
    };

    class CompoundStmt final : public Stmt, private llvm::TrailingObjects<CompoundStmt, Stmt*> {
        friend TrailingObjects;
    
    public:
        CompoundStmt(llvm::ArrayRef<Stmt*> stmts) : stmtCount{ stmts.size() }, Stmt{ Stmt::CompoundStmtClass } {
            std::uninitialized_copy(stmts.begin(), stmts.end(), getTrailingObjects());
        }
        ~CompoundStmt() = delete;

        inline llvm::ArrayRef<Stmt*> GetStmts() { return { getTrailingObjects(), stmtCount }; }

        static inline CompoundStmt* Create(ASTContext& context, llvm::ArrayRef<Stmt*> stmts, SourceRange range) {
            size_t size = totalSizeToAlloc<Stmt*>(stmts.size());
            void* ptr = context.Allocate(sizeof(CompoundStmt) + size);
            CompoundStmt* stmt = new(ptr) CompoundStmt{ stmts };
            stmt->SetSourceRange(range);
            return stmt;
        }

    private:
        size_t stmtCount;
    };

    class ReturnStmt : public Stmt {
    public:
        ReturnStmt(Expr* expr) : expr{ expr }, Stmt{ Stmt::ReturnStmtClass } {}
        ~ReturnStmt() = default;
        
        inline Expr* GetExpr() { return expr; }
        inline void SetExpr(Expr* expr) { this->expr = expr; }

        static inline ReturnStmt* Create(ASTContext& context, Expr* expr, SourceRange range) {
            ReturnStmt* stmt = context.AllocateNode<ReturnStmt>(expr);
            stmt->SetSourceRange(range);
            return stmt;
        }

    private:
        Expr* expr;
    };

    class IfStmt : public Stmt {

    };

    class WhileStmt : public Stmt {

    };

    class ForStmt : public Stmt {

    };

    class BreakStmt : public Stmt {

    };

}