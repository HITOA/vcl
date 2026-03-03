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
            ReturnStmtClass,
            IfStmtClass,
            WhileStmtClass,
            ForStmtClass,
            BreakStmtClass,
            ContinueStmtClass
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
            size_t size = additionalSizeToAlloc<Stmt*>(stmts.size());
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
    public:
        IfStmt(Expr* condition, Stmt* thenStmt, Stmt* elseStmt) 
                : condition{ condition }, thenStmt{ thenStmt }, elseStmt{ elseStmt }, Stmt{ Stmt::IfStmtClass } {}
        ~IfStmt() = default;

        inline Expr* GetCondition() { return condition; }
        inline Stmt* GetThenStmt() { return thenStmt; }
        inline Stmt* GetElseStmt() { return elseStmt; }

        static inline IfStmt* Create(ASTContext& context, Expr* condition, Stmt* thenStmt, Stmt* elseStmt, SourceRange range) {
            IfStmt* stmt = context.AllocateNode<IfStmt>(condition, thenStmt, elseStmt);
            stmt->SetSourceRange(range);
            return stmt;
        }

    private:
        Expr* condition;
        Stmt* thenStmt;
        Stmt* elseStmt;
    };

    class WhileStmt : public Stmt {
    public:
        WhileStmt(Expr* condition, Stmt* thenStmt) : condition{ condition }, thenStmt{ thenStmt }, Stmt{ Stmt::WhileStmtClass } {}
        ~WhileStmt() = default;

        inline Expr* GetCondition() { return condition; }
        inline Stmt* GetThenStmt() { return thenStmt; }

        static inline WhileStmt* Create(ASTContext& context, Expr* condition, Stmt* thenStmt, SourceRange range) {
            WhileStmt* stmt = context.AllocateNode<WhileStmt>(condition, thenStmt);
            stmt->SetSourceRange(range);
            return stmt;
        }

    private:
        Expr* condition;
        Stmt* thenStmt;
    };

    class ForStmt : public Stmt {
    public:
        ForStmt(Stmt* startStmt, Expr* condition, Expr* loopExpr, Stmt* thenStmt)
                : startStmt{ startStmt }, condition{ condition }, loopExpr{ loopExpr }, thenStmt{ thenStmt}, Stmt{ Stmt::ForStmtClass } {}
        ~ForStmt() = default;

        inline Stmt* GetStartStmt() { return startStmt; }
        inline Expr* GetCondition() { return condition; }
        inline Expr* GetLoopExpr() { return loopExpr; }
        inline Stmt* GetThenStmt() { return thenStmt; }
        
        static inline ForStmt* Create(ASTContext& context, Stmt* startStmt, Expr* condition, Expr* loopExpr, Stmt* thenStmt, SourceRange range) {
            ForStmt* stmt = context.AllocateNode<ForStmt>(startStmt, condition, loopExpr, thenStmt);
            stmt->SetSourceRange(range);
            return stmt;
        }

    private:
        Stmt* startStmt;
        Expr* condition;
        Expr* loopExpr;
        Stmt* thenStmt;
    };

    class BreakStmt : public Stmt {
    public:
        BreakStmt() : Stmt{ Stmt::BreakStmtClass } {}
        ~BreakStmt() = default;

        static inline BreakStmt* Create(ASTContext& context, SourceRange range) {
            BreakStmt* stmt = context.AllocateNode<BreakStmt>();
            stmt->SetSourceRange(range);
            return stmt;
        }
    };

    class ContinueStmt : public Stmt {
    public:
        ContinueStmt() : Stmt{ Stmt::ContinueStmtClass } {}
        ~ContinueStmt() = default;

        static inline ContinueStmt* Create(ASTContext& context, SourceRange range) {
            ContinueStmt* stmt = context.AllocateNode<ContinueStmt>();
            stmt->SetSourceRange(range);
            return stmt;
        }
    };

}