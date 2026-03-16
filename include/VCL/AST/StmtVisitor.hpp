#pragma once

#include <VCL/AST/Stmt.hpp>


namespace VCL {

    template<typename T>
    class StmtVisitor {
    public:
        inline T VisitStmt(Stmt* stmt) {
            switch (stmt->GetStmtClass()) {
                case Stmt::ValueStmtClass: return VisitValueStmt((ValueStmt*)stmt);
                case Stmt::DeclStmtClass: return VisitDeclStmt((DeclStmt*)stmt);
                case Stmt::CompoundStmtClass: return VisitCompoundStmt((CompoundStmt*)stmt);
                case Stmt::ReturnStmtClass: return VisitReturnStmt((ReturnStmt*)stmt);
                case Stmt::IfStmtClass: return VisitIfStmt((IfStmt*)stmt);
                case Stmt::WhileStmtClass: return VisitWhileStmt((WhileStmt*)stmt);
                case Stmt::ForStmtClass: return VisitForStmt((ForStmt*)stmt);
                case Stmt::BreakStmtClass: return VisitBreakStmt((BreakStmt*)stmt);
                case Stmt::ContinueStmtClass: return VisitContinueStmt((ContinueStmt*)stmt);
                default: return T{};
            }
        }

    protected:
        virtual T VisitValueStmt(ValueStmt* stmt) = 0;
        virtual T VisitDeclStmt(DeclStmt* stmt) = 0;
        virtual T VisitCompoundStmt(CompoundStmt* stmt) = 0;
        virtual T VisitReturnStmt(ReturnStmt* stmt) = 0;
        virtual T VisitIfStmt(IfStmt* stmt) = 0;
        virtual T VisitWhileStmt(WhileStmt* stmt) = 0;
        virtual T VisitForStmt(ForStmt* stmt) = 0;
        virtual T VisitBreakStmt(BreakStmt* stmt) = 0;
        virtual T VisitContinueStmt(ContinueStmt* stmt) = 0;
    };

}