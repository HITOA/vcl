#pragma once

#include <VCL/AST/Decl.hpp>


namespace VCL {

    class Stmt {
    public:
        enum StmtClass {
            ValueStmtStmt
        };

    public:
        Stmt() = delete;
        Stmt(StmtClass stmtClass) : stmtClass{ stmtClass } {}
        Stmt(const Stmt& other) = delete;
        Stmt(Stmt&& other) = delete;
        ~Stmt() = default;

        Stmt& operator=(const Stmt& other) = delete;
        Stmt& operator=(Stmt&& other) = delete;

    private:
        StmtClass stmtClass;
    };

    class ValueStmt : public Stmt {
    public:
        ValueStmt() : Stmt{ Stmt::ValueStmtStmt } {}
        ~ValueStmt() = default;
    };

}