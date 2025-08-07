#include <VCL/Directive.hpp>

#include <VCL/Error.hpp>
#include <VCL/Parser.hpp>

#include "ModuleContext.hpp"
#include "String.hpp"
#include "StaticExpressionVisitor.hpp"

#include <iostream>


std::string VCL::ImportDirective::GetDirectiveName() {
    return "import";
}

std::unique_ptr<VCL::ASTDirective> VCL::ImportDirective::Parse(Lexer& lexer, Parser* parser) {
    Token importPathToken = lexer.Consume();
    if (importPathToken.type != TokenType::LiteralString)
        throw new Exception{ "@import directive should be followed by the path of the file to import", importPathToken.location };
    
    if (Token token = lexer.Consume(); token.type != TokenType::Semicolon)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting semicolon", token.name), token.location };

    std::unique_ptr<ASTImportDirective> directive = std::make_unique<ASTImportDirective>(importPathToken.name);
    directive->location = importPathToken.location;
    return std::move(directive);
}

void VCL::ImportDirective::Run(ModuleContext* context, ASTDirective* directive, ASTVisitor* visitor) {
    ASTImportDirective* importDirective = (ASTImportDirective*)directive;
    std::string importPath = ParseRawString(importDirective->importPath);
    std::shared_ptr<Source> source = nullptr;

    if (auto v = LoadSource(importPath); v.has_value())
        source = *v;
    else
        throw Exception{ std::format("@import error: {}", v.error()), directive->location };

    std::shared_ptr<ImportDirectiveMetaComponent> state = context->GetMetaState()->GetOrCreate<ImportDirectiveMetaComponent>();
    if (!state->TrackImport(importPath)) {
        std::unique_ptr<Parser> parser = Parser::Create(context->GetLogger());
        parser->SetDirectiveRegistry(context->GetDirectiveRegistry());
        std::unique_ptr<ASTProgram> program = parser->Parse(source);
        program->Accept(visitor);
    }
}

std::expected<std::shared_ptr<VCL::Source>, VCL::Error> VCL::ImportDirective::LoadSource(const std::string& path) {
    std::filesystem::path importPath{ path };
    return VCL::Source::LoadFromDisk(importPath);
}

bool VCL::ImportDirective::ImportDirectiveMetaComponent::TrackImport(const std::string& path) {
    if (imports.count(path))
        return true;
    imports.insert(path);
    return false;
}

std::string VCL::DefineDirective::GetDirectiveName() {
    return "define";
}

std::unique_ptr<VCL::ASTDirective> VCL::DefineDirective::Parse(Lexer& lexer, Parser* parser) {
    Token defineNameToken = lexer.Consume();
    if (defineNameToken.type != TokenType::Identifier)
        throw new Exception{ "@define is missing a name", defineNameToken.location };

    std::unique_ptr<ASTLiteralExpression> expression = nullptr;

    if (Token token = lexer.Consume(); token.type == TokenType::Assignment) {
        expression = parser->ParseLiteralExpression(lexer);
        if (token = lexer.Consume(); token.type != TokenType::Semicolon)
            throw Exception{ std::format("Unexpected token \'{}\'. Expecting semicolon", token.name), token.location };
    } else if (token.type != TokenType::Semicolon) {
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting semicolon", token.name), token.location };
    }

    return std::make_unique<ASTDefineDirective>(defineNameToken.name, std::move(expression));
}

void VCL::DefineDirective::Run(ModuleContext* context, ASTDirective* directive, ASTVisitor* visitor) {
    ASTDefineDirective* defineDirective = (ASTDefineDirective*)directive;
    std::shared_ptr<DefineDirectiveMetaComponent> component = context->GetMetaState()->GetOrCreate<DefineDirectiveMetaComponent>();
    component->AddDefine(defineDirective->name, std::move(defineDirective->expression));
}

bool VCL::DefineDirective::DefineDirectiveMetaComponent::Defined(const std::string& name) {
    return defines.count(name) > 0;
}

VCL::ASTLiteralExpression* VCL::DefineDirective::DefineDirectiveMetaComponent::GetDefine(const std::string& name) {
    if (!Defined(name))
        return nullptr;
    return defines.at(name).get();
}

bool VCL::DefineDirective::DefineDirectiveMetaComponent::AddDefine(const std::string& name, std::unique_ptr<ASTLiteralExpression> value) {
    if (Defined(name))
        defines[name] = std::move(value);
    defines.insert({ name, std::move(value) });
    return true;
}

bool VCL::DefineDirective::DefineDirectiveMetaComponent::AddDefineFlag(const std::string& name) {
    return AddDefine(name, nullptr);
}

bool VCL::DefineDirective::DefineDirectiveMetaComponent::AddDefineInt(const std::string& name, int value) {
    std::unique_ptr<ASTLiteralExpression> expression = std::make_unique<ASTLiteralExpression>(value);
    return AddDefine(name, std::move(expression));
}

bool VCL::DefineDirective::DefineDirectiveMetaComponent::AddDefineFloat(const std::string& name, float value) {
    std::unique_ptr<ASTLiteralExpression> expression = std::make_unique<ASTLiteralExpression>(value);
    return AddDefine(name, std::move(expression));
}


std::string VCL::ConditionalDirective::GetDirectiveName() {
    return "if";
}

std::unique_ptr<VCL::ASTDirective> VCL::ConditionalDirective::Parse(Lexer& lexer, Parser* parser) {
    std::unique_ptr<ASTExpression> expression = parser->ParseExpression(lexer);
    std::unique_ptr<ASTStatement> thenStmt = nullptr;
    std::unique_ptr<ASTStatement> elseStmt = nullptr;

    if (lexer.Peek().type == TokenType::LBracket) {
        thenStmt = parser->ParseCompoundStatement(lexer);
    } else {    
        thenStmt = parser->ParseStatement(lexer);
    }

    if (lexer.Peek().type == TokenType::Else) {
        lexer.Consume();
        if (lexer.Peek().type == TokenType::If) {
            lexer.Consume();
            elseStmt = Parse(lexer, parser);
        } else if (lexer.Peek().type == TokenType::LBracket) {
            elseStmt = parser->ParseCompoundStatement(lexer);
        } else {
            elseStmt = parser->ParseStatement(lexer);
        }
    }

    return std::make_unique<ASTConditionalDirective>(std::move(expression), std::move(thenStmt), std::move(elseStmt));
}

void VCL::ConditionalDirective::Run(ModuleContext* context, ASTDirective* directive, ASTVisitor* visitor) {
    ASTConditionalDirective* conditionalDirective = (ASTConditionalDirective*)directive;
    StaticExpressionVisitor staticExpressionVisitor{};
    staticExpressionVisitor.state = context->GetMetaState();
    if (staticExpressionVisitor.Evaluate(conditionalDirective->expression.get())) {
        conditionalDirective->thenStmt->Accept(visitor);
    } else if (conditionalDirective->elseStmt) {
        conditionalDirective->elseStmt->Accept(visitor);
    }
}