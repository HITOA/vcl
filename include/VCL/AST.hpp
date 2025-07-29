#pragma once

#include <VCL/Definition.hpp>
#include <VCL/Source.hpp>
#include <VCL/Attribute.hpp>

#include <string>
#include <memory>
#include <vector>


namespace VCL {
    class ASTProgram;
    class ASTCompoundStatement;
    class ASTFunctionArgument;
    class ASTFunctionPrototype;
    class ASTFunctionDeclaration;
    class ASTStructureFieldDeclaration;
    class ASTStructureDeclaration;
    class ASTTemplateParameterDeclaration;
    class ASTTemplateDeclaration;
    class ASTTemplateFunctionDeclaration;
    class ASTReturnStatement;
    class ASTIfStatement;
    class ASTWhileStatement;
    class ASTForStatement;
    class ASTBreakStatement;
    class ASTBinaryArithmeticExpression;
    class ASTBinaryLogicalExpression;
    class ASTBinaryComparisonExpression;
    class ASTAssignmentExpression;
    class ASTPrefixArithmeticExpression;
    class ASTPrefixLogicalExpression;
    class ASTPostfixArithmeticExpression;
    class ASTFieldAccessExpression;
    class ASTSubscriptExpression;
    class ASTLiteralExpression;
    class ASTIdentifierExpression;
    class ASTVariableDeclaration;
    class ASTFunctionCall;
    class ASTAggregateExpression;
    class ASTDirective;

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
        virtual void VisitStructureFieldDeclaration(ASTStructureFieldDeclaration* node) {};
        virtual void VisitStructureDeclaration(ASTStructureDeclaration* node) {};
        virtual void VisitTemplateParameterDeclaration(ASTTemplateParameterDeclaration* node) {};
        virtual void VisitTemplateDeclaration(ASTTemplateDeclaration* node) {};
        virtual void VisitTemplateFunctionDeclaration(ASTTemplateFunctionDeclaration* node) {};
        virtual void VisitReturnStatement(ASTReturnStatement* node) {};
        virtual void VisitIfStatement(ASTIfStatement* node) {};
        virtual void VisitWhileStatement(ASTWhileStatement* node) {};
        virtual void VisitForStatement(ASTForStatement* node) {};
        virtual void VisitBreakStatement(ASTBreakStatement* node) {};
        virtual void VisitBinaryArithmeticExpression(ASTBinaryArithmeticExpression* node) {};
        virtual void VisitBinaryLogicalExpression(ASTBinaryLogicalExpression* node) {};
        virtual void VisitBinaryComparisonExpression(ASTBinaryComparisonExpression* node) {};
        virtual void VisitAssignmentExpression(ASTAssignmentExpression* node) {};
        virtual void VisitPrefixArithmeticExpression(ASTPrefixArithmeticExpression* node) {};
        virtual void VisitPrefixLogicalExpression(ASTPrefixLogicalExpression* node) {};
        virtual void VisitPostfixArithmeticExpression(ASTPostfixArithmeticExpression* node) {};
        virtual void VisitFieldAccessExpression(ASTFieldAccessExpression* node) {};
        virtual void VisitSubscriptExpression(ASTSubscriptExpression* node) {};
        virtual void VisitLiteralExpression(ASTLiteralExpression* node) {};
        virtual void VisitIdentifierExpression(ASTIdentifierExpression* node) {};
        virtual void VisitVariableDeclaration(ASTVariableDeclaration* node) {};
        virtual void VisitFunctionCall(ASTFunctionCall* node) {};
        virtual void VisitAggregateExpression(ASTAggregateExpression* node) {};
        virtual void VisitDirective(ASTDirective* node) {};
    };

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

    class ASTDirective : public ASTStatement {
    public:
        virtual ~ASTDirective() = default;

        virtual std::string GetName() const = 0;

        void Accept(ASTVisitor* visitor) override { visitor->VisitDirective(this); }
    };

    /**
     * @brief represents the root of a VCL Program.
     */
    class ASTProgram : public ASTNode {
    public:
        ASTProgram(std::vector<std::unique_ptr<ASTStatement>> statements, std::shared_ptr<Source> source) :
            statements{ std::move(statements) }, source{ source } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitProgram(this); }
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

        void Accept(ASTVisitor* visitor) override { visitor->VisitCompoundStatement(this); }
    public:
        std::vector<std::unique_ptr<ASTStatement>> statements;
    };

    /**
     * @brief represents a function argument declaration.
     */
    class ASTFunctionArgument : public ASTStatement {
    public:
        ASTFunctionArgument(std::shared_ptr<TypeInfo> type, const std::string& name) :
            type{ type }, name{ name } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitFunctionArgument(this); }
    public:
        std::shared_ptr<TypeInfo> type;
        std::string name;
    };

    /**
     * @brief represents a function prototype.
     */
    class ASTFunctionPrototype : public ASTStatement {
    public:
        ASTFunctionPrototype(
            std::shared_ptr<TypeInfo> type, const std::string& name, std::vector<std::unique_ptr<ASTFunctionArgument>> arguments, AttributeSet attributes) :
            type{ type }, name{ name }, arguments{ std::move(arguments) }, attributes{ attributes } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitFunctionPrototype(this); }
    public:
        std::shared_ptr<TypeInfo> type;
        std::string name;
        std::vector<std::unique_ptr<ASTFunctionArgument>> arguments;
        AttributeSet attributes;
    };

    /**
     * @brief represents a function declaration.
     */
    class ASTFunctionDeclaration : public ASTStatement {
    public:
        ASTFunctionDeclaration(std::unique_ptr<ASTFunctionPrototype> prototype, std::unique_ptr<ASTStatement> body) :
            prototype{ std::move(prototype) }, body{ std::move(body) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitFunctionDeclaration(this); }
    public:
        std::unique_ptr<ASTFunctionPrototype> prototype;
        std::unique_ptr<ASTStatement> body;
    };

    /**
     * @brief represents a structure field declaration.
     */
    class ASTStructureFieldDeclaration : public ASTStatement {
    public:
        ASTStructureFieldDeclaration(std::shared_ptr<TypeInfo> type, const std::string& name) : 
            type{ type }, name{ name } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitStructureFieldDeclaration(this); }
    public:
        std::shared_ptr<TypeInfo> type;
        std::string name;
    };

    /**
     * @brief represents a structure declaration.
     */
    class ASTStructureDeclaration : public ASTStatement {
    public:
        ASTStructureDeclaration(const std::string& name, std::vector<std::unique_ptr<ASTStructureFieldDeclaration>> fields) :
            name{ name }, fields{ std::move(fields) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitStructureDeclaration(this); }
    public:
        std::string name;
        std::vector<std::unique_ptr<ASTStructureFieldDeclaration>> fields;
    };

    class ASTTemplateParameterDeclaration : public ASTStatement {
    public:
        ASTTemplateParameterDeclaration(const std::string& name, TemplateArgument::TemplateValueType type) :
            name{ name }, type{ type } {};
        
        void Accept(ASTVisitor* visitor) override { visitor->VisitTemplateParameterDeclaration(this); }
    public:
        std::string name;
        TemplateArgument::TemplateValueType type;
    };

    class ASTTemplateDeclaration : public ASTStructureDeclaration {
    public:
        ASTTemplateDeclaration(const std::string& name, std::vector<std::unique_ptr<ASTTemplateParameterDeclaration>> parameters,
            std::vector<std::unique_ptr<ASTStructureFieldDeclaration>> fields) : 
                ASTStructureDeclaration{ name, std::move(fields) }, parameters{ std::move(parameters) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitTemplateDeclaration(this); }
    public:
        std::vector<std::unique_ptr<ASTTemplateParameterDeclaration>> parameters;
    };

    class ASTTemplateFunctionDeclaration : public ASTStatement {
    public:
        ASTTemplateFunctionDeclaration(std::shared_ptr<TypeInfo> type,
            const std::string& name,
            std::vector<std::unique_ptr<ASTTemplateParameterDeclaration>> parameters,
            std::vector<std::unique_ptr<ASTFunctionArgument>> arguments,
            std::unique_ptr<ASTNode> body) : type{ type }, name{ name }, 
            parameters{ std::move(parameters) }, arguments{ std::move(arguments) }, body{ std::move(body) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitTemplateFunctionDeclaration(this); }
    public:
        std::shared_ptr<TypeInfo> type;
        std::string name;
        std::vector<std::unique_ptr<ASTTemplateParameterDeclaration>> parameters;
        std::vector<std::unique_ptr<ASTFunctionArgument>> arguments;
        std::unique_ptr<ASTNode> body;
    };

    /**
     * @brief represents a return statement.
     */
    class ASTReturnStatement : public ASTStatement {
    public:
        ASTReturnStatement(std::unique_ptr<ASTExpression> expression) :
            expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitReturnStatement(this); }
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

        void Accept(ASTVisitor* visitor) override { visitor->VisitIfStatement(this); }
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

        void Accept(ASTVisitor* visitor) override { visitor->VisitWhileStatement(this); }
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

        void Accept(ASTVisitor* visitor) override { visitor->VisitForStatement(this); }
    public:
        std::unique_ptr<ASTStatement> start;
        std::unique_ptr<ASTExpression> condition;
        std::unique_ptr<ASTStatement> end;
        std::unique_ptr<ASTStatement> thenStmt;
    };

     /**
     * @brief represents a break instruction.
     */
    class ASTBreakStatement : public ASTStatement {
    public:
        ASTBreakStatement() {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitBreakStatement(this); }
    };

    class ASTBinaryArithmeticExpression : public ASTExpression {
    public:
        ASTBinaryArithmeticExpression(Operator::ID op, std::unique_ptr<ASTExpression> lhs, std::unique_ptr<ASTExpression> rhs) :
            op{ op }, lhs{ std::move(lhs) }, rhs{ std::move(rhs) } {};
            
        void Accept(ASTVisitor* visitor) override { visitor->VisitBinaryArithmeticExpression(this); }
    public:
        Operator::ID op;
        std::unique_ptr<ASTExpression> lhs;
        std::unique_ptr<ASTExpression> rhs;
    };

    class ASTBinaryLogicalExpression : public ASTExpression {
    public:
        ASTBinaryLogicalExpression(Operator::ID op, std::unique_ptr<ASTExpression> lhs, std::unique_ptr<ASTExpression> rhs) :
            op{ op }, lhs{ std::move(lhs) }, rhs{ std::move(rhs) } {};
            
        void Accept(ASTVisitor* visitor) override { visitor->VisitBinaryLogicalExpression(this); }
    public:
        Operator::ID op;
        std::unique_ptr<ASTExpression> lhs;
        std::unique_ptr<ASTExpression> rhs;
    };

    class ASTBinaryComparisonExpression : public ASTExpression {
    public:
        ASTBinaryComparisonExpression(Operator::ID op, std::unique_ptr<ASTExpression> lhs, std::unique_ptr<ASTExpression> rhs) :
            op{ op }, lhs{ std::move(lhs) }, rhs{ std::move(rhs) } {};
            
        void Accept(ASTVisitor* visitor) override { visitor->VisitBinaryComparisonExpression(this); }
    public:
        Operator::ID op;
        std::unique_ptr<ASTExpression> lhs;
        std::unique_ptr<ASTExpression> rhs;
    };

    class ASTAssignmentExpression : public ASTExpression {
    public:
        ASTAssignmentExpression(Operator::ID op, std::unique_ptr<ASTExpression> lhs, std::unique_ptr<ASTExpression> rhs) :
            op{ op }, lhs{ std::move(lhs) }, rhs{ std::move(rhs) } {};
            
        void Accept(ASTVisitor* visitor) override { visitor->VisitAssignmentExpression(this); }
    public:
        Operator::ID op;
        std::unique_ptr<ASTExpression> lhs;
        std::unique_ptr<ASTExpression> rhs;
    };

    class ASTPrefixArithmeticExpression : public ASTExpression {
    public:
        ASTPrefixArithmeticExpression(Operator::ID op, std::unique_ptr<ASTExpression> expression) :
            op{ op }, expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitPrefixArithmeticExpression(this); }
    public:
        Operator::ID op;
        std::unique_ptr<ASTExpression> expression;
    };

    class ASTPrefixLogicalExpression : public ASTExpression {
    public:
        ASTPrefixLogicalExpression(Operator::ID op, std::unique_ptr<ASTExpression> expression) :
            op{ op }, expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitPrefixLogicalExpression(this); }
    public:
        Operator::ID op;
        std::unique_ptr<ASTExpression> expression;
    };

    class ASTPostfixArithmeticExpression : public ASTExpression {
    public:
        ASTPostfixArithmeticExpression(Operator::ID op, std::unique_ptr<ASTExpression> expression) :
            op{ op }, expression{ std::move(expression) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitPostfixArithmeticExpression(this); }
    public:
        Operator::ID op;
        std::unique_ptr<ASTExpression> expression;
    };

    class ASTFieldAccessExpression : public ASTExpression {
    public:
        ASTFieldAccessExpression(std::unique_ptr<ASTExpression> expression, const std::string& fieldName) :
            expression{ std::move(expression) }, fieldName{ fieldName } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitFieldAccessExpression(this); }
    public:
        std::unique_ptr<ASTExpression> expression;
        std::string fieldName;
    };

    class ASTSubscriptExpression : public ASTExpression {
    public:
        ASTSubscriptExpression(std::unique_ptr<ASTExpression> expression, std::unique_ptr<ASTExpression> index) :
            expression{ std::move(expression) }, index{ std::move(index) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitSubscriptExpression(this); }
    public:
        std::unique_ptr<ASTExpression> expression;
        std::unique_ptr<ASTExpression> index;
    };

    /**
     * @brief represents a literal numeric value.
     */
    class ASTLiteralExpression : public ASTExpression {
    public:
        ASTLiteralExpression(float value) : type{ TypeInfo::TypeName::Float }, fValue{ value } {};
        ASTLiteralExpression(int value) : type{ TypeInfo::TypeName::Int }, iValue{ value } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitLiteralExpression(this); }
    public:
        TypeInfo::TypeName type;
        union {
            float fValue;
            int iValue;
        };
    };

    /**
     * @brief represents a value with an identifier.
     */
    class ASTIdentifierExpression : public ASTExpression {
    public:
        ASTIdentifierExpression(const std::string& name) : name{ name } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitIdentifierExpression(this); }
    public:
        std::string name;
    };

    /**
     * @brief represents a variable declaration.
     */
    class ASTVariableDeclaration : public ASTExpression {
    public:
        ASTVariableDeclaration(
            std::shared_ptr<TypeInfo> type, const std::string& name, std::unique_ptr<ASTExpression> expression, AttributeSet attributes) :
            type{ type }, name{ name }, expression{ std::move(expression) }, attributes{ attributes } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitVariableDeclaration(this); }
    public:
        std::shared_ptr<TypeInfo> type;
        std::string name;
        std::unique_ptr<ASTExpression> expression;
        AttributeSet attributes;
    };

    /**
     * @brief represents a function call expression.
     */
    class ASTFunctionCall : public ASTExpression {
    public:
        ASTFunctionCall(const std::string& name, std::vector<std::unique_ptr<ASTExpression>> arguments) :
            name{ name }, arguments{ std::move(arguments) }, templateArguments{} {};
        ASTFunctionCall(const std::string& name, std::vector<std::unique_ptr<ASTExpression>> arguments, std::vector<std::shared_ptr<TemplateArgument>> templateArguments) :
            name{ name }, arguments{ std::move(arguments) }, templateArguments{ templateArguments } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitFunctionCall(this); }
    public:
        std::string name;
        std::vector<std::unique_ptr<ASTExpression>> arguments;
        std::vector<std::shared_ptr<TemplateArgument>> templateArguments;
    };

    /**
     * @brief represents an aggregate expression
     */
    class ASTAggregateExpression : public ASTExpression {
    public:
        ASTAggregateExpression(std::vector<std::unique_ptr<ASTExpression>> values) : values{ std::move(values) } {};

        void Accept(ASTVisitor* visitor) override { visitor->VisitAggregateExpression(this); }
    public:
        std::vector<std::unique_ptr<ASTExpression>> values;
    };
}