#pragma once

#include <vcl/definition.hpp>

#include <string_view>
#include <memory>
#include <vector>


namespace VCL {

    struct ASTTypeInfo {
#undef DEF
#define DEF(name, symbol, ...) name = __VA_ARGS__,
        enum class QualifierFlag {
            NONE = 0,
            TYPE_QUALIFIER_DEF
        } qualifiers;

#undef DEF
#define DEF(name, symbol, ...) name,
        enum class TypeName {
            NONE,
            TYPE_DEF
            MAX
        } type;
    };

    inline ASTTypeInfo::QualifierFlag operator|(ASTTypeInfo::QualifierFlag a, ASTTypeInfo::QualifierFlag b) {
        return (ASTTypeInfo::QualifierFlag)((int)a | (int)b);
    }

    inline ASTTypeInfo::QualifierFlag operator|=(ASTTypeInfo::QualifierFlag& a, ASTTypeInfo::QualifierFlag b) {
        
        a = (ASTTypeInfo::QualifierFlag)((int)a | (int)b);
        return a;
    }

    inline bool operator&(ASTTypeInfo::QualifierFlag a, ASTTypeInfo::QualifierFlag b) {
        return ((int)a & (int)b) != 0;
    }

    class ASTVisitor;

    class ASTNode {
    public:
        virtual ~ASTNode() = default;

        virtual void Accept(ASTVisitor* visitor) {};
    };

    class ASTStatement : public ASTNode {
    public:
        virtual ~ASTStatement() = default;
    };


    class ASTExpression : public ASTStatement {
    public:
        virtual ~ASTExpression() = default;
    };


    class ASTProgram : public ASTNode {
    public:
        ASTProgram(std::vector<std::unique_ptr<ASTStatement>> statements) :
            statements{ std::move(statements) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::vector<std::unique_ptr<ASTStatement>> statements;
    };

    // Statements
    
    class ASTCompoundStatement : public ASTStatement {
    public:
        ASTCompoundStatement(std::vector<std::unique_ptr<ASTStatement>> statements) :
            statements{ std::move(statements) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::vector<std::unique_ptr<ASTStatement>> statements;
    };

    class ASTVariableAssignment : public ASTStatement {
    public:
        ASTVariableAssignment(std::string_view name, std::unique_ptr<ASTExpression> expression) :
            name{ name }, expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::string_view name;
        std::unique_ptr<ASTExpression> expression;
    };

    class ASTVariableDeclaration : public ASTStatement {
    public:
        ASTVariableDeclaration(ASTTypeInfo type, std::string_view name, std::unique_ptr<ASTExpression> expression) :
            type{ type }, name{ name }, expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        ASTTypeInfo type;
        std::string_view name;
        std::unique_ptr<ASTExpression> expression;
    };

    class ASTFunctionArgument : public ASTStatement {
    public:
        ASTFunctionArgument(ASTTypeInfo type, std::string_view name) :
            type{ type }, name{ name } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        ASTTypeInfo type;
        std::string_view name;
    };

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

    class ASTFunctionDeclaration : public ASTStatement {
    public:
        ASTFunctionDeclaration(std::unique_ptr<ASTFunctionPrototype> prototype, std::unique_ptr<ASTNode> body) :
            prototype{ std::move(prototype) }, body{ std::move(body) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::unique_ptr<ASTFunctionPrototype> prototype;
        std::unique_ptr<ASTNode> body;
    };

    class ASTReturnStatement : public ASTStatement {
    public:
        ASTReturnStatement(std::unique_ptr<ASTExpression> expression) :
            expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::unique_ptr<ASTExpression> expression;
    };

    // Expressions

    class ASTUnaryExpression : public ASTExpression {
    public:
        ASTUnaryExpression(uint8_t op, std::unique_ptr<ASTExpression> expression) :
            op{ op }, expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        uint8_t op;
        std::unique_ptr<ASTExpression> expression;
    };

    class ASTBinaryExpression : public ASTExpression {
    public:
        ASTBinaryExpression(uint8_t op, std::unique_ptr<ASTExpression> lhs, std::unique_ptr<ASTExpression> rhs) :
            op{ op }, lhs{ std::move(lhs) }, rhs{ std::move(rhs) } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        uint8_t op;
        std::unique_ptr<ASTExpression> lhs;
        std::unique_ptr<ASTExpression> rhs;
    };

    class ASTLiteralExpression : public ASTExpression {
    public:
        ASTLiteralExpression(float value) : type{ ASTTypeInfo::TypeName::FLOAT }, fValue{ value } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        ASTTypeInfo::TypeName type;
        union {
            float fValue;
        };
    };

    class ASTVariableExpression : public ASTExpression {
    public:
        ASTVariableExpression(std::string_view name) : name{ name } {};

        void Accept(ASTVisitor* visitor) override;
    public:
        std::string_view name;
    };

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

    class ASTVisitor {
    public:
        virtual ~ASTVisitor() = default;

        virtual void VisitProgram(ASTProgram* node) {};
        virtual void VisitCompoundStatement(ASTCompoundStatement* node) {};
        virtual void VisitVariableAssignment(ASTVariableAssignment* node) {};
        virtual void VisitVariableDeclaration(ASTVariableDeclaration* node) {};
        virtual void VisitFunctionArgument(ASTFunctionArgument* node) {};
        virtual void VisitFunctionPrototype(ASTFunctionPrototype* node) {};
        virtual void VisitFunctionDeclaration(ASTFunctionDeclaration* node) {};
        virtual void VisitReturnStatement(ASTReturnStatement* node) {};
        virtual void VisitUnaryExpression(ASTUnaryExpression* node) {};
        virtual void VisitBinaryExpression(ASTBinaryExpression* node) {};
        virtual void VisitLiteralExpression(ASTLiteralExpression* node) {};
        virtual void VisitVariableExpression(ASTVariableExpression* node) {};
        virtual void VisitFunctionCall(ASTFunctionCall* node) {};
    };
}