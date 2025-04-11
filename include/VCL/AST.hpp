#pragma once

#include <VCL/Definition.hpp>
#include <VCL/Source.hpp>

#include <string_view>
#include <memory>
#include <vector>


namespace VCL {
    using ASTTypeInfo = TypeInfo;

    class ASTVisitor;

    /**
     * @brief represents a common base node in an abstract syntax tree.
     */
    class ASTNode {
    public:
        virtual ~ASTNode() = default;

        virtual void Accept(ASTVisitor* visitor) {};
    };

    /**
     * @brief represents an ASTNode that have an effect.
     */
    class ASTStatement : public ASTNode {
    public:
        virtual ~ASTStatement() = default;

    public:
        SourceLocation location;
    };

    /**
     * @brief represents an ASTStatement that also express (return) a value.
     */
    class ASTExpression : public ASTStatement {
    public:
        virtual ~ASTExpression() = default;
    };

    /**
     * @brief represents the root of a VCL Program.
     */
    class ASTProgram : public ASTNode {
    public:
        ASTProgram(std::vector<std::unique_ptr<ASTStatement>> statements, std::shared_ptr<Source> source) :
            statements{ std::move(statements) }, source{ source } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::vector<std::unique_ptr<ASTStatement>> statements;
        std::shared_ptr<Source> source;
    };

    /**
     * @brief represents a compound of statements.
     */
    class ASTCompoundStatement : public ASTStatement {
    public:
        ASTCompoundStatement(std::vector<std::unique_ptr<ASTStatement>> statements) :
            statements{ std::move(statements) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::vector<std::unique_ptr<ASTStatement>> statements;
    };

    /**
     * @brief represents a function argument declaration.
     */
    class ASTFunctionArgument : public ASTStatement {
    public:
        ASTFunctionArgument(ASTTypeInfo type, std::string_view name) :
            type{ type }, name{ name } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        ASTTypeInfo type;
        std::string_view name;
    };

    /**
     * @brief represents a function prototype.
     */
    class ASTFunctionPrototype : public ASTStatement {
    public:
        ASTFunctionPrototype(ASTTypeInfo type, std::string_view name, std::vector<std::unique_ptr<ASTFunctionArgument>> arguments) :
            type{ type }, name{ name }, arguments{ std::move(arguments) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        ASTTypeInfo type;
        std::string_view name;
        std::vector<std::unique_ptr<ASTFunctionArgument>> arguments;
    };

    /**
     * @brief represents a function declaration.
     */
    class ASTFunctionDeclaration : public ASTStatement {
    public:
        ASTFunctionDeclaration(std::unique_ptr<ASTFunctionPrototype> prototype, std::unique_ptr<ASTNode> body) :
            prototype{ std::move(prototype) }, body{ std::move(body) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::unique_ptr<ASTFunctionPrototype> prototype;
        std::unique_ptr<ASTNode> body;
    };

    /**
     * @brief represents a return statement.
     */
    class ASTReturnStatement : public ASTStatement {
    public:
        ASTReturnStatement(std::unique_ptr<ASTExpression> expression) :
            expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::unique_ptr<ASTExpression> expression;
    };

    /**
     * @brief represents a if control flow statement.
     */
    class ASTIfStatement : public ASTStatement {
    public:
        ASTIfStatement(std::unique_ptr<ASTExpression> condition,
            std::unique_ptr<ASTStatement> thenStmt,
            std::unique_ptr<ASTStatement> elseStmt) :
            condition{ std::move(condition) },
            thenStmt{ std::move(thenStmt) },
            elseStmt{ std::move(elseStmt) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::unique_ptr<ASTExpression> condition;
        std::unique_ptr<ASTStatement> thenStmt;
        std::unique_ptr<ASTStatement> elseStmt;
    };

    /**
     * @brief represents a while loop control flow statement.
     */
    class ASTWhileStatement : public ASTStatement {
    public:
        ASTWhileStatement(std::unique_ptr<ASTExpression> condition,
            std::unique_ptr<ASTStatement> thenStmt) : 
            condition{ std::move(condition) },
            thenStmt{ std::move(thenStmt) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::unique_ptr<ASTExpression> condition;
        std::unique_ptr<ASTStatement> thenStmt;
    };

    /**
     * @brief represents a for loop control flow statement.
     */
    class ASTForStatement : public ASTStatement {
    public:
        ASTForStatement(std::unique_ptr<ASTStatement> start,
            std::unique_ptr<ASTExpression> condition,
            std::unique_ptr<ASTStatement> end,
            std::unique_ptr<ASTStatement> thenStmt) :
            start{ std::move(start) }, condition{ std::move(condition) }, 
            end{ std::move(end) }, thenStmt{ std::move(thenStmt) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::unique_ptr<ASTStatement> start;
        std::unique_ptr<ASTExpression> condition;
        std::unique_ptr<ASTStatement> end;
        std::unique_ptr<ASTStatement> thenStmt;
    };

    /**
     * @brief represents a unary expression operation.
     */
    class ASTUnaryExpression : public ASTExpression {
    public:
        ASTUnaryExpression(UnaryOpType op, std::unique_ptr<ASTExpression> expression) :
            op{ op }, expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        UnaryOpType op;
        std::unique_ptr<ASTExpression> expression;
    };

    /**
     * @brief represents a binary expression operation.
     */
    class ASTBinaryExpression : public ASTExpression {
    public:
        ASTBinaryExpression(BinaryOpType op, std::unique_ptr<ASTExpression> lhs, std::unique_ptr<ASTExpression> rhs) :
            op{ op }, lhs{ std::move(lhs) }, rhs{ std::move(rhs) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        BinaryOpType op;
        std::unique_ptr<ASTExpression> lhs;
        std::unique_ptr<ASTExpression> rhs;
    };

    /**
     * @brief represents a literal numeric value.
     */
    class ASTLiteralExpression : public ASTExpression {
    public:
        ASTLiteralExpression(float value) : type{ ASTTypeInfo::TypeName::FLOAT }, fValue{ value } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        ASTTypeInfo::TypeName type;
        union {
            float fValue;
            int iValue;
        };
    };

    /**
     * @brief represents a variable value.
     */
    class ASTVariableExpression : public ASTExpression {
    public:
        ASTVariableExpression(std::string_view name) : name{ name } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::string_view name;
    };

    /**
     * @brief represents a variable assignment.
     */
    class ASTVariableAssignment : public ASTExpression {
    public:
        ASTVariableAssignment(std::string_view name, std::unique_ptr<ASTExpression> expression) :
            name{ name }, expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::string_view name;
        std::unique_ptr<ASTExpression> expression;
    };

    /**
     * @brief represents a variable declaration.
     */
    class ASTVariableDeclaration : public ASTExpression {
    public:
        ASTVariableDeclaration(ASTTypeInfo type, std::string_view name, std::unique_ptr<ASTExpression> expression) :
            type{ type }, name{ name }, expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        ASTTypeInfo type;
        std::string_view name;
        std::unique_ptr<ASTExpression> expression;
    };

    /**
     * @brief represents a function call expression.
     */
    class ASTFunctionCall : public ASTExpression {
    public:
        ASTFunctionCall(std::string_view name, std::vector<std::unique_ptr<ASTExpression>> arguments) :
            name{ name }, arguments{ std::move(arguments) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::string_view name;
        std::vector<std::unique_ptr<ASTExpression>> arguments;
    };

    // Visitor

    /**
     * @brief This is the base class to visite a whole AST from top to bottom.
     */
    class ASTVisitor {
    public:
        virtual ~ASTVisitor() = default;

        virtual void VisitProgram(ASTProgram* node) {};
        virtual void VisitCompoundStatement(ASTCompoundStatement* node) {};
        virtual void VisitFunctionArgument(ASTFunctionArgument* node) {};
        virtual void VisitFunctionPrototype(ASTFunctionPrototype* node) {};
        virtual void VisitFunctionDeclaration(ASTFunctionDeclaration* node) {};
        virtual void VisitReturnStatement(ASTReturnStatement* node) {};
        virtual void VisitIfStatement(ASTIfStatement* node) {};
        virtual void VisitWhileStatement(ASTWhileStatement* node) {};
        virtual void VisitForStatement(ASTForStatement* node) {};
        virtual void VisitUnaryExpression(ASTUnaryExpression* node) {};
        virtual void VisitBinaryExpression(ASTBinaryExpression* node) {};
        virtual void VisitLiteralExpression(ASTLiteralExpression* node) {};
        virtual void VisitVariableExpression(ASTVariableExpression* node) {};
        virtual void VisitVariableAssignment(ASTVariableAssignment* node) {};
        virtual void VisitVariableDeclaration(ASTVariableDeclaration* node) {};
        virtual void VisitFunctionCall(ASTFunctionCall* node) {};
    };
}