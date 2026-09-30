#pragma once

#include <VCL/Core/Directive.hpp>
#include <VCL/Lex/Token.hpp>
#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Type.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/AST/Stmt.hpp>
#include <VCL/AST/Expr.hpp>
#include <VCL/AST/Operator.hpp>
#include <VCL/AST/Template.hpp>
#include <VCL/AST/SymbolRef.hpp>
#include <VCL/Sema/Scope.hpp>
#include <VCL/Sema/ScopeManager.hpp>
#include <VCL/Sema/SymbolTable.hpp>
#include <VCL/Sema/ModuleTable.hpp>
#include <VCL/Sema/DefineTable.hpp>

#include <llvm/ADT/SmallSet.h>

#include <variant>
#include <stack>


namespace VCL {
    class CompilerContext;
    class DiagnosticReporter;
    class IdentifierTable;
    class Module;

    /**
     * This is the semantic analyzer. It is called by the parser to 
     * act on each semantic and build the ast. 
     */
    class Sema {
    public:
        class SemaScopeGuard {
        public:
            SemaScopeGuard(Sema& sema, DeclContext* context);
            SemaScopeGuard(const SemaScopeGuard& other) = delete;
            SemaScopeGuard(SemaScopeGuard&& other);
            ~SemaScopeGuard();
            
            inline void Release();

        private:
            Sema& sema;
            DeclContext* context;
        };

        class SemaContextGuard {
        public:
            SemaContextGuard(Sema& sema, ASTContext& context);
            SemaContextGuard(const SemaContextGuard& other) = delete;
            SemaContextGuard(SemaContextGuard&& other) = delete;
            ~SemaContextGuard();

            inline void Release();

        private:
            Sema& sema;
            ASTContext* context;
        };

        /** Restores the scope that was current before Sema::EnterTranslationUnit. */
        class TranslationUnitScopeGuard {
        public:
            TranslationUnitScopeGuard(Sema& sema, TranslationUnitDecl* translationUnit, Scope* previous);
            TranslationUnitScopeGuard(const TranslationUnitScopeGuard& other) = delete;
            TranslationUnitScopeGuard(TranslationUnitScopeGuard&& other);
            ~TranslationUnitScopeGuard();

            void Release();

        private:
            Sema& sema;
            TranslationUnitDecl* translationUnit;
            Scope* previous;
        };

    public:
        Sema() = delete;
        Sema(CompilerContext& cc, ASTContext& astContext, DiagnosticReporter& diagnosticReporter, IdentifierTable& identifierTable, 
                DirectiveRegistry& directiveRegistry, SymbolTable& exportedSymbols, ModuleTable& importedModules, DefineTable& defineTable);
        Sema(const Sema& other) = delete;
        Sema(Sema&& other) = delete;
        ~Sema() = default;

        Sema& operator=(const Sema& other) = delete;
        Sema& operator=(Sema&& other) = delete;

    public:
        inline CompilerContext& GetCompilerContext() { return cc; }
        inline ASTContext& GetASTContext() { return *astContextStack.top(); }
        inline DiagnosticReporter& GetDiagnosticReporter() { return diagnosticReporter; }
        inline IdentifierTable& GetIdentifierTable() { return identifierTable; }
        inline SymbolTable& GetExportedSymbols() { return exportedSymbols; }
        inline ModuleTable& GetImportedModules() { return importedModules; }
        inline DefineTable& GetDefineTable() { return definedValues; }
        
        TemplateSpecializationType* CreateVectorTemplateSpecializationType(Type* ofType);
        TemplateSpecializationType* CreateLanesTemplateSpecializationType(Type* ofType);
        void AddIntrinsicTypes();
        void AddIntrinsicMathFunction(FunctionDecl::IntrinsicID intrinsicID, llvm::StringRef name, uint32_t argCount);
        void AddIntrinsicFunction(FunctionDecl::IntrinsicID intrinsicID, llvm::StringRef name);
        void AddIntrinsicTemplateDecl();
        
        void PushASTContext(ASTContext& astContext);
        void PopASTContext(ASTContext& astContext);

        SemaScopeGuard PushScope(DeclContext* context, bool loopScope = false);
        /**
         * Makes `translationUnit` (e.g. one created with TranslationUnitDecl::Create to receive a
         * transformed copy of the source) the current scope and context, until the guard is
         * released. Its scope doesn't see the source's: declarations may reuse the source's names,
         * and none of its variables are shadowed. Names aren't looked up there, so build its
         * content from already resolved declarations (see TreeTransform).
         */
        TranslationUnitScopeGuard EnterTranslationUnit(TranslationUnitDecl* translationUnit);

        bool PushDeclContextScope(DeclContext* context, bool loopScope = false);
        bool PopDeclContextScope(DeclContext* context);
        bool AddDeclToScope(Decl* decl);
        bool AddDeclToContext(Decl* decl);
        bool AddDeclToScopeAndContext(Decl* decl);

        bool ExportSymbol(Decl* decl, SourceRange range);
        bool ImportModule(Module* module, IdentifierInfo* identifierInfo);

        bool ValidateIntrinsicFunctionDeclSpecialization(FunctionDecl* decl);

        NamedDecl* LookupNamedDecl(SymbolRef symbolRef, int depth = -1);
        /**
         * Reports `Shadowing` when `identifier` names a variable or a parameter visible from an
         * enclosing scope. Call after the current scope's `Redeclaration` check.
         */
        bool CheckShadowing(IdentifierInfo* identifier, SourceRange range);
        TemplateDecl* LookupTemplateDecl(SymbolRef symbolRef, int depth = -1);

        CompoundStmt* ActOnCompoundStmt(llvm::ArrayRef<Stmt*> stmts, SourceRange range);

        DirectiveDecl* ActOnDirectiveDecl(IdentifierInfo* identifierInfo, llvm::ArrayRef<ConstantValue*> args, SourceRange range);

        TypeAliasDecl* ActOnTypeAliasDecl(IdentifierInfo* identifierInfo, Type* type, SourceRange range);

        DeclStmt* ActOnDeclStmt(Decl* decl, SourceRange range);

        TemplateDecl* ActOnTemplateDecl(TemplateParameterList* parameters, SourceRange range);

        RecordDecl* ActOnRecordDecl(IdentifierInfo* identifier, SourceRange range);

        FieldDecl* ActOnFieldDecl(QualType type, IdentifierInfo* identifier, SourceRange range);

        FunctionDecl* ActOnFunctionDecl(FunctionDecl* decl, QualType returnType, TemplateArgumentList* args, SourceRange range);

        ParamDecl* ActOnParamDecl(VarDecl::VarAttrBitfield attr, QualType type, IdentifierInfo* identifier, SourceRange range);
        
        CompoundStmt* ActOnFunctionBody(FunctionDecl* function, CompoundStmt* body);
        bool ActRecOnFunctionBodyReturnStmt(FunctionDecl* function, llvm::ArrayRef<Stmt*> stmts);

        ReturnStmt* ActOnReturnStmt(Expr* expr, SourceRange range);
        IfStmt* ActOnIfStmt(Expr* condition, Stmt* thenStmt, Stmt* elseStmt, SourceRange range);
        WhileStmt* ActOnWhileStmt(Expr* condition, Stmt* thenStmt, SourceRange range);
        ForStmt* ActOnForStmt(Stmt* startStmt, Expr* condition, Expr* loopExpr, Stmt* thenStmt, SourceRange range);
        BreakStmt* ActOnBreakStmt(SourceRange range);
        ContinueStmt* ActOnContinueStmt(SourceRange range);

        VarDecl* ActOnVarDecl(QualType type, IdentifierInfo* identifier, VarDecl::VarAttrBitfield varAttrBitfield, Expr* initializer, SourceRange range);
        
        QualType ActOnQualType(Type* type, Qualifier qualifiers, SourceRange range);
        Type* ActOnType(SymbolRef symbolRef, TemplateArgumentList* list, SourceRange range);
        
        TemplateParameterList* ActOnTemplateParameterList(llvm::ArrayRef<NamedDecl*> params, SourceRange range);
        TemplateArgumentList* ActOnTemplateArgumentList(llvm::ArrayRef<TemplateArgument> args, SourceRange range, bool canonicalize = false);

        TemplateTypeParamDecl* ActOnTemplateTypeParamDecl(IdentifierInfo* identifier, SourceRange range);
        NonTypeTemplateParamDecl* ActOnNonTypeTemplateParamDecl(BuiltinType* type, IdentifierInfo* identifier, SourceRange range);

        Expr* ActOnBinaryExpr(Expr* lhs, Expr* rhs, BinaryOperator::Kind op);
        Expr* ActOnUnaryExpr(Expr* expr, UnaryOperator op, SourceRange range);
        bool IsExprAssignable(Expr* expr);
        
        Expr* ActOnFieldAccessExpr(Expr* lhs, IdentifierInfo* field, SourceRange range);
        Expr* ActOnSubscriptExpr(Expr* expr, Expr* index, SourceRange range);

        Expr* ActOnLoad(Expr* expr);

        std::pair<Expr*, Expr*> ActOnImplicitBinaryArithmeticCast(Expr* lhs, Expr* rhs);
        Expr* ActOnCast(Expr* expr, QualType toType, SourceRange range);
        Expr* ActOnExplicitCast(Expr* expr, QualType toType, SourceRange range);
        Expr* ActOnSplat(Expr* expr, SourceRange range);
        
        Expr* ActOnNumericConstant(Token* value);
        Expr* ActOnIdentifierExpr(SymbolRef symbolRef, SourceRange range);
        Expr* ActOnDeclRefExpr(ValueDecl* decl, SourceRange range);
        Expr* ActOnCallExpr(SymbolRef symbolRef, llvm::ArrayRef<Expr*> args, TemplateArgumentList* templateArgs, SourceRange range);
        /** Call to an already resolved callee (a TemplateDecl or a FunctionDecl). */
        Expr* ActOnResolvedCallExpr(Decl* callee, SymbolRef symbolRef, llvm::ArrayRef<Expr*> args, 
            TemplateArgumentList* templateArgs, SourceRange range);
        /** Resolve a callee name without reporting anything if it isn't visible. */
        Decl* LookupCalleeQuietly(SymbolRef symbolRef);

        Expr* ActOnAggregateExpr(llvm::ArrayRef<Expr*> elems, SourceRange range);
        bool ActOnAggregateExpr(AggregateExpr* aggregate);
        
        FunctionDecl* GetFrontmostFunctionDecl();
        bool IsWithinALoop();

        bool TypePreferByReference(Type* type);
        bool CheckTypeCastability(Type* type);
        BuiltinType::Kind GetScalarKindFromBuiltinOrVectorType(Type* type);
        bool IsCurrentScopeGlobal();
        
        std::variant<Type*, ConstantScalar> RecursivelyDeduceTemplateArgument(NamedDecl* parameter, 
            TemplateSpecializationType* a, TemplateSpecializationType* b);
        TemplateArgumentList* DeduceTemplateArgumentFromCall(
            FunctionDecl* functionDecl, llvm::ArrayRef<Expr*> args, TemplateArgumentList* templateArgs, TemplateParameterList* parameters);
        bool MatchTemplateArgumentList(TemplateArgumentList* args1, TemplateArgumentList* args2);
        FunctionDecl* InstantiateFunctionTemplateSpecialization(TemplateArgumentList* templateArgs, TemplateDecl* templateDecl);

        /**
         * The ASTContext an instantiation of `templateDecl` for `args` belongs in: the template's
         * own, unless an argument type is owned by another ASTContext's cache (e.g. a struct of the
         * importing module). The instantiation must not outlive that type, so it then stays in the
         * current compilation instead of being stored with a (possibly cached) library.
         */
        ASTContext& GetInstantiationContext(TemplateDecl* templateDecl, TemplateArgumentList* args);

    private:
        CompilerContext& cc;
        std::stack<ASTContext*> astContextStack;
        DiagnosticReporter& diagnosticReporter;
        IdentifierTable& identifierTable;
        DirectiveRegistry& directiveRegistry;
        SymbolTable& exportedSymbols;
        ModuleTable& importedModules;
        DefineTable& definedValues;
        ScopeManager sm{};
        Scope* translationUnitScope;
        // Nesting of function template instantiations. Their scopes hang off the call site, and
        // their declarations were checked for shadowing with the template.
        int instantiationDepth = 0;
        // Function template specializations kept in this compilation (see GetInstantiationContext).
        llvm::SmallVector<std::pair<TemplateDecl*, TemplateSpecializationDecl*>> localSpecializations{};
    };

}