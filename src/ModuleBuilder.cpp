#include "ModuleBuilder.hpp"

#include "Utility.hpp"
#include "Callable.hpp"
#include "Aggregate.hpp"
#include "Intrinsic.hpp"
#include "StructDefinition.hpp"
#include "StructTemplate.hpp"
#include "CallableTemplate.hpp"
#include "Cast.hpp"

#include <VCL/Debug.hpp>
#include <VCL/Directive.hpp>

#include <iostream>


struct BasicBlockOnNeed {
    llvm::LLVMContext* context = nullptr;
    const char* name = nullptr;
    llvm::Function* function = nullptr;
    llvm::BasicBlock* bb = nullptr;
    bool created = false;

    BasicBlockOnNeed() = delete;
    BasicBlockOnNeed(llvm::LLVMContext* context, const char* name, llvm::Function* function) :
        context{ context }, name{ name }, function{ function }, bb{ nullptr }, created{ false } {};

    llvm::BasicBlock* Get() {
        if (!created) {
            bb = llvm::BasicBlock::Create(*context, name, function);
            created = true;
        }
        return bb;
    }
};

void SetCurrentDebugLocation(VCL::ModuleContext* context, VCL::ASTStatement* node) {
    if (llvm::DIScope* scope = context->GetScopeManager().GetCurrentDebugInformationScope()) {
        llvm::DILocation* loc = llvm::DILocation::get(*(context->GetTSContext().getContext()),
            node->location.line + 1, node->location.position + 1, scope);
        context->GetIRBuilder().SetCurrentDebugLocation(loc);
    }
}

void CreateDebugScope(VCL::ModuleContext* context, VCL::ASTStatement* node, llvm::DIFile* file) {
    if (llvm::DIScope* parentScope = context->GetScopeManager().GetCurrentDebugInformationScope()) {
        llvm::DIScope* scope = context->GetDIBuilder().createLexicalBlock(
            parentScope, file, node->location.line + 1, node->location.position + 1);
        context->GetScopeManager().SetCurrentDebugInformationScope(scope);
    }
}

VCL::ModuleBuilder::ModuleBuilder(ModuleContext* context) : context{ context }, file{ nullptr } {

}

VCL::ModuleBuilder::~ModuleBuilder() {
    
}

void VCL::ModuleBuilder::VisitProgram(ASTProgram* node) {
    if (diSettings.generateDebugInformation) {
        file = context->GetDIBuilder().createFile(
            node->source->path.filename().string(),
            node->source->path.parent_path().string());
        llvm::DICompileUnit* cu = context->GetDIBuilder().createCompileUnit(
            llvm::dwarf::DW_LANG_C,
            file,
            "VCL JIT Compiler", 
            diSettings.optimized, 
            "", 
            0
        );
        context->GetScopeManager().SetCurrentDebugInformationScope(file);
    }

    for (auto& statement : node->statements)
        statement->Accept(this);
}

void VCL::ModuleBuilder::VisitCompoundStatement(ASTCompoundStatement* node) {
    ScopeGuard scopeGuard{ &context->GetScopeManager() };

    CreateDebugScope(context, node, file);

    for (auto& statement : node->statements)
        statement->Accept(this);

    scopeGuard.Release();
}

void VCL::ModuleBuilder::VisitFunctionPrototype(ASTFunctionPrototype* node) {
    if (auto function = context->GetScopeManager().GetNamedValue(node->name); function.has_value()) {
        lastReturnedValue = *function;
        return;
    }

    Type returnType = ThrowOnError(Type::Create(node->type, context), node->location);

    std::vector<std::shared_ptr<TypeInfo>> argsTypeinfo( node->arguments.size() );
    std::vector<Function::ArgInfo> argsInfo( node->arguments.size() );

    for (size_t i = 0; i < node->arguments.size(); ++i) {
        Type argType = ThrowOnError(Type::Create(node->arguments[i]->type, context), 
            node->arguments[i]->location);
        argsInfo[i] = Function::ArgInfo{ argType, node->arguments[i]->name };
        argsTypeinfo[i] = node->arguments[i]->type;
    }

    Handle<Function> function = ThrowOnError(Function::Create(returnType, argsInfo, node->name, context), node->location);

    if (llvm::DIScope* parentScope = context->GetScopeManager().GetCurrentDebugInformationScope()) {
        context->GetDIBuilder().createFunction(
            parentScope, node->name, node->name,
            file, node->location.line, function->GetDIType(), node->location.line, 
            llvm::DINode::DIFlags::FlagPrototyped, llvm::DISubprogram::DISPFlags::SPFlagZero
        );
    }

    std::shared_ptr<FunctionInfo> info = std::make_shared<FunctionInfo>();
    info->name = node->name;
    info->returnTypeinfo = node->type;
    info->argumentsTypeinfo = argsTypeinfo;
    info->attributes = node->attributes;
    context->GetModuleInfo()->AddFunction(info);

    context->GetScopeManager().PushNamedValue(node->name, function);
    lastReturnedValue = function;
}

void VCL::ModuleBuilder::VisitFunctionDeclaration(ASTFunctionDeclaration* node) {
    node->prototype->Accept(this);
    Handle<Function> function = HandleCast<Function, Value>(lastReturnedValue);
    if (function->HasStorage())
        throw Exception{ "Function redefinition", node->location };

    llvm::Function* llvmFunction = function->GetLLVMFunction();
    llvm::DISubprogram* subprogram = nullptr;
    
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "entry", llvmFunction);
    context->GetIRBuilder().SetInsertPoint(bb);

    ScopeGuard scopeGuard{ &context->GetScopeManager() };

    if (llvm::DIScope* parentScope = context->GetScopeManager().GetCurrentDebugInformationScope()) {
        subprogram = context->GetDIBuilder().createFunction(
            parentScope, node->prototype->name, node->prototype->name,
            file, node->location.line + 1, function->GetDIType(), node->body->location.line + 1, 
            llvm::DINode::DIFlags::FlagPrototyped, llvm::DISubprogram::DISPFlags::SPFlagDefinition
        );
        llvmFunction->setSubprogram(subprogram);
        context->GetScopeManager().SetCurrentDebugInformationScope(subprogram);
        llvm::DILocation* loc = llvm::DILocation::get(*(context->GetTSContext().getContext()),
            node->location.line + 1, node->location.position + 1, subprogram);
        context->GetIRBuilder().SetCurrentDebugLocation(loc);
    }
    
    for (size_t i = 0; i < llvmFunction->arg_size(); ++i) {
        Type argType = function->GetArgsInfo()[i].type;
        const std::string& argName = function->GetArgsInfo()[i].name;
        std::string argNameStr{ argName };
        Handle<Value> argValue = ThrowOnError(Value::Create(llvmFunction->getArg(i), argType, context), node->location);
        if (argType.GetTypeInfo()->IsGivenByReference()) {
            context->GetScopeManager().PushNamedValue(argName, argValue);
        } else {
            Handle<Value> arg = ThrowOnError(Value::CreateLocalVariable(argType, argValue, context, argNameStr.c_str(), 
                file, node->location.line, node->location.position), node->location);
            context->GetScopeManager().PushNamedValue(argName, arg);
        }
    }

    node->body->Accept(this);

    scopeGuard.Release();

    for (llvm::BasicBlock& bb : *llvmFunction) {
        llvm::Instruction* terminator = bb.getTerminator();
        if (terminator) continue;
        if (llvmFunction->getReturnType()->isVoidTy()) {
            context->GetIRBuilder().SetInsertPoint(&bb);
            context->GetIRBuilder().CreateRetVoid();
        } else {
            throw Exception{ "Missing return statement.", node->location };
        }
    }

    if (subprogram)
        context->GetDIBuilder().finalizeSubprogram(subprogram);

    function->Verify();
}

void VCL::ModuleBuilder::VisitStructureDeclaration(ASTStructureDeclaration* node) {
    std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>> elements(node->fields.size());
    
    for (size_t i = 0; i < node->fields.size(); ++i) {
        std::string fieldName{ node->fields[i]->name };
        elements[i] = std::make_pair(fieldName, node->fields[i]->type);
    }

    Handle<StructDefinition> structDefinition = ThrowOnError(StructDefinition::Create(node->name, elements, context), node->location);
    
    if (!context->GetScopeManager().PushNamedType(node->name, structDefinition))
        throw Exception{ std::format("redefinition of `{}`", node->name), node->location };
}

void VCL::ModuleBuilder::VisitTemplateDeclaration(ASTTemplateDeclaration* node) {
    const std::string& name = node->name;
    std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>> structTemplate{};
    std::vector<std::pair<std::string, TemplateArgument::TemplateValueType>> templateParameters{};

    for (size_t i = 0; i < node->fields.size(); ++i)
        structTemplate.push_back(std::make_pair(node->fields[i]->name, node->fields[i]->type));
    for (size_t i = 0; i < node->parameters.size(); ++i)
        templateParameters.push_back(std::make_pair(node->parameters[i]->name, node->parameters[i]->type));

    Handle<StructTemplate> st = ThrowOnError(StructTemplate::Create(name, structTemplate, templateParameters, context), node->location);

    if (!context->GetScopeManager().PushNamedStructTemplate(node->name, st))
        throw Exception{ std::format("redefinition of `{}`", node->name), node->location };
}

void VCL::ModuleBuilder::VisitTemplateFunctionDeclaration(ASTTemplateFunctionDeclaration* node) {
    const std::string& name = node->name;
    std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>> functionArguments{};
    std::vector<std::pair<std::string, TemplateArgument::TemplateValueType>> templateParameters{};

    for (size_t i = 0; i < node->arguments.size(); ++i)
        functionArguments.push_back(std::make_pair(node->arguments[i]->name, node->arguments[i]->type));
    for (size_t i = 0; i < node->parameters.size(); ++i)
        templateParameters.push_back(std::make_pair(node->parameters[i]->name, node->parameters[i]->type));

    Handle<CallableTemplate> ft = ThrowOnError(CallableTemplate::Create(name, node->type,
        functionArguments, templateParameters, std::move(node->body), file, node->location.line, context), node->location);
    
    if (!context->GetScopeManager().PushNamedFunctionTemplate(node->name, ft))
        throw Exception{ std::format("redefinition of `{}`", node->name), node->location };
}

void VCL::ModuleBuilder::VisitReturnStatement(ASTReturnStatement* node) {
    if (context->GetScopeManager().IsCurrentScopeGlobal())
        throw new Exception{ "return statement cannot be outside of a function's body.", node->location };

    SetCurrentDebugLocation(context, node);
    if (node->expression) {
        node->expression->Accept(this);
        context->GetIRBuilder().CreateRet(ThrowOnError(lastReturnedValue->Load(), node->location)->GetLLVMValue());
    } else {
        context->GetIRBuilder().CreateRetVoid();
    }
}

void VCL::ModuleBuilder::VisitIfStatement(ASTIfStatement* node) {
    if (context->GetScopeManager().IsCurrentScopeGlobal())
        throw new Exception{ "if statement cannot be outside of a function's body.", node->location };

    node->condition->Accept(this);
    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
    typeInfo->type = TypeInfo::TypeName::Bool;
    Type type = ThrowOnError(Type::Create(typeInfo, context), node->location);
    Handle<Value> conditionValue = ThrowOnError(lastReturnedValue->Cast(type), node->condition->location);
    
    llvm::Value* r = context->GetIRBuilder().CreateICmpNE(conditionValue->GetLLVMValue(), 
        llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context->GetTSContext().getContext()), 0));

    llvm::Function* function = context->GetIRBuilder().GetInsertBlock()->getParent();

    if (!function)
        throw Exception{ "A if statement may only be used withing a function's body.", node->location };

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "then", function);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "else", function);
    BasicBlockOnNeed endBB{ context->GetTSContext().getContext(), "end", function };

    ScopeGuard scopeGuard{ &context->GetScopeManager() };

    CreateDebugScope(context, node, file);
    SetCurrentDebugLocation(context, node);

    context->GetIRBuilder().CreateCondBr(r, thenBB, elseBB);

    context->GetIRBuilder().SetInsertPoint(thenBB);
    node->thenStmt->Accept(this);
    if (!context->GetIRBuilder().GetInsertBlock()->getTerminator())
        context->GetIRBuilder().CreateBr(endBB.Get());

    context->GetIRBuilder().SetInsertPoint(elseBB);
    if (node->elseStmt)
        node->elseStmt->Accept(this);
    if (!context->GetIRBuilder().GetInsertBlock()->getTerminator())
        context->GetIRBuilder().CreateBr(endBB.Get());

    if (endBB.created)
        context->GetIRBuilder().SetInsertPoint(endBB.Get());

    scopeGuard.Release();
}

void VCL::ModuleBuilder::VisitWhileStatement(ASTWhileStatement* node) {
    if (context->GetScopeManager().IsCurrentScopeGlobal())
        throw new Exception{ "while statement cannot be outside of a function's body.", node->location };

    llvm::Function* function = context->GetIRBuilder().GetInsertBlock()->getParent();

    if (!function)
        throw Exception{ "A while loop may only be used withing a function's body.", node->location };

    llvm::BasicBlock* conditionBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "condition", function);
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "loop", function);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "end", function);

    ScopeGuard scopeGuard{ &context->GetScopeManager(), endBB };

    CreateDebugScope(context, node, file);
    SetCurrentDebugLocation(context, node);

    context->GetIRBuilder().CreateBr(conditionBB);

    context->GetIRBuilder().SetInsertPoint(conditionBB);
    node->condition->Accept(this);
    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
    typeInfo->type = TypeInfo::TypeName::Bool;
    Type type = ThrowOnError(Type::Create(typeInfo, context), node->location);
    Handle<Value> conditionValue = ThrowOnError(lastReturnedValue->Cast(type), node->condition->location);
    
    llvm::Value* r = context->GetIRBuilder().CreateICmpNE(conditionValue->GetLLVMValue(), 
        llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context->GetTSContext().getContext()), 0));

    context->GetIRBuilder().CreateCondBr(r, thenBB, endBB);

    context->GetIRBuilder().SetInsertPoint(thenBB);
    node->thenStmt->Accept(this);
    if (!context->GetIRBuilder().GetInsertBlock()->getTerminator())
        context->GetIRBuilder().CreateBr(conditionBB);

    context->GetIRBuilder().SetInsertPoint(endBB);

    scopeGuard.Release();
}

void VCL::ModuleBuilder::VisitForStatement(ASTForStatement* node) {
    if (context->GetScopeManager().IsCurrentScopeGlobal())
        throw new Exception{ "for statement cannot be outside of a function's body.", node->location };
        
    llvm::Function* function = context->GetIRBuilder().GetInsertBlock()->getParent();

    if (!function)
        throw Exception{ "A for loop may only be used withing a function's body.", node->location };

    llvm::BasicBlock* conditionBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "condition", function);
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "loop", function);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "end", function);

    ScopeGuard scopeGuard{ &context->GetScopeManager(), endBB };

    CreateDebugScope(context, node, file);
    SetCurrentDebugLocation(context, node);

    node->start->Accept(this);
    context->GetIRBuilder().CreateBr(conditionBB);
    
    context->GetIRBuilder().SetInsertPoint(conditionBB);
    node->condition->Accept(this);
    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
    typeInfo->type = TypeInfo::TypeName::Bool;
    Type type = ThrowOnError(Type::Create(typeInfo, context), node->location);
    Handle<Value> conditionValue = ThrowOnError(lastReturnedValue->Cast(type), node->condition->location);
    
    llvm::Value* r = context->GetIRBuilder().CreateICmpNE(conditionValue->GetLLVMValue(), 
        llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context->GetTSContext().getContext()), 0));

    context->GetIRBuilder().CreateCondBr(r, thenBB, endBB);

    context->GetIRBuilder().SetInsertPoint(thenBB);
    node->thenStmt->Accept(this);
    node->end->Accept(this);
    if (!context->GetIRBuilder().GetInsertBlock()->getTerminator())
        context->GetIRBuilder().CreateBr(conditionBB);

    context->GetIRBuilder().SetInsertPoint(endBB);

    scopeGuard.Release();
}

void VCL::ModuleBuilder::VisitBreakStatement(ASTBreakStatement* node) {
    llvm::BasicBlock* dest = context->GetScopeManager().GetTransferControlBasicBlock();
    if (dest == nullptr)
        throw Exception{ "A break statement may only be used within a loop.", node->location };

    SetCurrentDebugLocation(context, node);

    context->GetIRBuilder().CreateBr(dest);
}

void VCL::ModuleBuilder::VisitBinaryArithmeticExpression(ASTBinaryArithmeticExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Numeric | IntrinsicArgumentPolicy::Vector };

    node->lhs->Accept(this);
    Handle<Value> lhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    node->rhs->Accept(this);
    Handle<Value> rhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    SetCurrentDebugLocation(context, node);

    CastResult castResult = ThrowOnError(ArithmeticImplicitCast(lhs, rhs), node->location);
    lhs = castResult.lhs;
    rhs = castResult.rhs;

    if (!policy(lhs->GetType()) || !policy(rhs->GetType()))
        throw Exception{ std::format("Invalid operands to arithmetic operator `{}`: left operand is `{}`, right operand is `{}`.",
            ToString(node->op), ToString(lhs->GetType().GetTypeInfo()), ToString(rhs->GetType().GetTypeInfo())), node->location };

    llvm::Value* result;

    switch (node->op) {
        case Operator::ID::Add:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(Int, CreateAdd),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateAdd),
                BINARY_DISPATCH_FUNCTION(Double, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFAdd)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Sub:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFSub),
                BINARY_DISPATCH_FUNCTION(Int, CreateSub),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateSub),
                BINARY_DISPATCH_FUNCTION(Double, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFSub)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Mul:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFMul),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFMul),
                BINARY_DISPATCH_FUNCTION(Int, CreateMul),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateMul),
                BINARY_DISPATCH_FUNCTION(Double, CreateFMul),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFMul)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Div:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFDiv),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFDiv),
                BINARY_DISPATCH_FUNCTION(Int, CreateSDiv),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateSDiv),
                BINARY_DISPATCH_FUNCTION(Double, CreateFDiv),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFDiv)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Remainder:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Int, CreateSRem),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateSRem)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        default:
            throw Exception{ std::format("Invalid binary arithmetic operator `{}`.", ToString(node->op)), node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(Value::Create(result, lhs->GetType(), context), node->location);
}

void VCL::ModuleBuilder::VisitBinaryLogicalExpression(ASTBinaryLogicalExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Condition | IntrinsicArgumentPolicy::Mask };

    node->lhs->Accept(this);
    Handle<Value> lhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    node->rhs->Accept(this);
    Handle<Value> rhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    SetCurrentDebugLocation(context, node);

    CastResult castResult = ThrowOnError(ArithmeticImplicitCast(lhs, rhs), node->location);
    lhs = castResult.lhs;
    rhs = castResult.rhs;

    if (!policy(lhs->GetType()) || !policy(rhs->GetType()))
        throw Exception{ std::format("Logical operator `{}` requires both operands to be of type `bool` or `vbool`, but got `{}` and `{}`.",
            ToString(node->op), ToString(lhs->GetType().GetTypeInfo()), ToString(rhs->GetType().GetTypeInfo())), node->location };

    llvm::Value* result;

    switch (node->op) {
        case Operator::ID::LogicalAnd:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Bool, CreateAnd),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateAnd)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::LogicalOr:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Bool, CreateOr),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateOr)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        default:
            throw Exception{ std::format("Invalid binary logical operator `{}`.", ToString(node->op)), node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(Value::Create(result, lhs->GetType(), context), node->location);
}

void VCL::ModuleBuilder::VisitBinaryComparisonExpression(ASTBinaryComparisonExpression* node) {
    node->lhs->Accept(this);
    Handle<Value> lhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    node->rhs->Accept(this);
    Handle<Value> rhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    SetCurrentDebugLocation(context, node);

    CastResult castResult = ThrowOnError(ArithmeticImplicitCast(lhs, rhs), node->location);
    lhs = castResult.lhs;
    rhs = castResult.rhs;

    llvm::Value* result;

    switch (node->op) {
        case Operator::ID::Greater:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOGT),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOGT),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(Double, CreateFCmpOGT),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFCmpOGT)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Less:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOLT),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOLT),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(Double, CreateFCmpOLT),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFCmpOLT)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::GreaterEqual:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOGE),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOGE),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(Double, CreateFCmpOGE),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFCmpOGE)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::LessEqual:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOLE),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOLE),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(Double, CreateFCmpOLE),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFCmpOLE)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Equal:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOEQ),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOEQ),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(Double, CreateFCmpOEQ),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFCmpOEQ)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::NotEqual:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpONE),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpONE),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(Double, CreateFCmpONE),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFCmpONE)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        default:
            throw Exception{ std::format("Invalid binary comparison operator `{}`.", ToString(node->op)), node->location }; //Should never happen
    }

    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();

    if (lhs->GetType().GetTypeInfo()->IsVector())
        typeInfo->type = TypeInfo::TypeName::VectorBool;
    else
        typeInfo->type = TypeInfo::TypeName::Bool;

    Type resultType = ThrowOnError(Type::Create(typeInfo, context), node->location);

    lastReturnedValue = ThrowOnError(Value::Create(result, resultType, context), node->location);
}

void VCL::ModuleBuilder::VisitAssignmentExpression(ASTAssignmentExpression* node) {
    node->lhs->Accept(this);
    Handle<Value> lhs = lastReturnedValue;

    node->rhs->Accept(this);
    Handle<Value> rhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    if (!lhs->IsAssignable())
        throw Exception{ std::format("Left-hand side of assignment must be a variable or a memory location (l-value)."), node->location };
    
    rhs = ThrowOnError(rhs->Cast(lhs->GetType()), node->rhs->location);

    if (auto err = lhs->Store(rhs); err.has_value())
        throw Exception{ err.value(), node->location };

    lastReturnedValue = lhs;
}

void VCL::ModuleBuilder::VisitPrefixArithmeticExpression(ASTPrefixArithmeticExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Numeric | IntrinsicArgumentPolicy::Vector };

    node->expression->Accept(this);
    Handle<Value> expression = lastReturnedValue;

    SetCurrentDebugLocation(context, node);

    if (!policy(expression->GetType()))
        throw Exception{ std::format("Arithmetic unary operator `{}` expects a numeric operand, but got `{}`.",
            ToString(node->op), ToString(expression->GetType().GetTypeInfo())), node->location };

    Handle<Value> one = ThrowOnError(Value::CreateConstantInt32(1, context), node->location);
    one = ThrowOnError(one->Cast(expression->GetType()), node->location);

    llvm::Value* result;

    switch (node->op)
    {
    case Operator::ID::Plus:
        lastReturnedValue = ThrowOnError(expression->Load(), node->location);
        return;
    case Operator::ID::Minus:
        expression = ThrowOnError(expression->Load(), node->location);
        result = ThrowOnError(DISPATCH_UNARY(
                UNARY_DISPATCH_FUNCTION(Float, CreateFNeg),
                UNARY_DISPATCH_FUNCTION(VectorFloat, CreateFNeg),
                UNARY_DISPATCH_FUNCTION(Int, CreateNeg),
                UNARY_DISPATCH_FUNCTION(VectorInt, CreateNeg),
                UNARY_DISPATCH_FUNCTION(Double, CreateFNeg),
                UNARY_DISPATCH_FUNCTION(VectorDouble, CreateFNeg)
            )(expression->GetType().GetTypeInfo()->type, expression->GetLLVMValue()), node->location);
        break;
    case Operator::ID::PreIncrement:
        if (!expression->IsAssignable())
            throw Exception{ std::format("Increment/decrement operator `{}` requires a numeric l-value.",
                ToString(node->op)), node->location };
        {
            Handle<Value> loadedExpression = ThrowOnError(expression->Load(), node->location);
            llvm::Value* incrementedValueResult = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(Int, CreateAdd),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateAdd),
                BINARY_DISPATCH_FUNCTION(Double, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFAdd)
            )(loadedExpression->GetType().GetTypeInfo()->type, loadedExpression->GetLLVMValue(), one->GetLLVMValue()), node->location);
            Handle<Value> incrementedValue = ThrowOnError(Value::Create(incrementedValueResult, expression->GetType(), context), node->location);
            
            if (auto err = expression->Store(incrementedValue); err.has_value())
                throw Exception{ err.value(), node->location };
            lastReturnedValue = expression;
            return;
        }
        break;
    case Operator::ID::PreDecrement:
        if (!expression->IsAssignable())
            throw Exception{ std::format("Increment/decrement operator `{}` requires a numeric l-value.",
                ToString(node->op)), node->location };
        {
            Handle<Value> loadedExpression = ThrowOnError(expression->Load(), node->location);
            llvm::Value* decrementedValueResult = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFSub),
                BINARY_DISPATCH_FUNCTION(Int, CreateSub),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateSub),
                BINARY_DISPATCH_FUNCTION(Double, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFSub)
            )(loadedExpression->GetType().GetTypeInfo()->type, loadedExpression->GetLLVMValue(), one->GetLLVMValue()), node->location);
            Handle<Value> decrementedValue = ThrowOnError(Value::Create(decrementedValueResult, expression->GetType(), context), node->location);
            if (auto err = expression->Store(decrementedValue); err.has_value())
                throw Exception{ err.value(), node->location };
            lastReturnedValue = expression;
            return;
        }
        break;
    default:
        throw Exception{ std::format("Invalid unary operator `{}`.", ToString(node->op)), node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(Value::Create(result, expression->GetType(), context), node->location);
}

void VCL::ModuleBuilder::VisitPrefixLogicalExpression(ASTPrefixLogicalExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Condition | IntrinsicArgumentPolicy::Mask };

    node->expression->Accept(this);
    Handle<Value> expression = ThrowOnError(lastReturnedValue->Load(), node->location);

    SetCurrentDebugLocation(context, node);

    if (!policy(expression->GetType()))
        throw Exception{ std::format("Logical unary operator `{}` expects a `bool` or `vbool` operand, but got `{}`.",
            ToString(node->op), ToString(expression->GetType().GetTypeInfo())), node->location };

    llvm::Value* result;

    switch (node->op)
    {
    case Operator::ID::Not:
        result = ThrowOnError(DISPATCH_UNARY(
            UNARY_DISPATCH_FUNCTION(Bool, CreateNot)
        )(expression->GetType().GetTypeInfo()->type, expression->GetLLVMValue()), node->location);
        break;
    default:
        throw Exception{ std::format("Invalid unary operator `{}`.", ToString(node->op)), node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(Value::Create(result, expression->GetType(), context), node->location);
}

void VCL::ModuleBuilder::VisitPostfixArithmeticExpression(ASTPostfixArithmeticExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Numeric | IntrinsicArgumentPolicy::Vector };

    node->expression->Accept(this);
    Handle<Value> expression = lastReturnedValue;

    SetCurrentDebugLocation(context, node);

    if (!policy(expression->GetType()))
        throw Exception{ std::format("Arithmetic unary operator `{}` expects a numeric operand, but got `{}`.",
            ToString(node->op), ToString(expression->GetType().GetTypeInfo())), node->location };

    Handle<Value> one = ThrowOnError(Value::CreateConstantInt32(1, context), node->location);
    one = ThrowOnError(one->Cast(expression->GetType()), node->location);

    llvm::Value* result;

    switch (node->op)
    {
    case Operator::ID::PostIncrement:
        if (!expression->IsAssignable())
            throw Exception{ std::format("Increment/decrement operator `{}` requires a numeric l-value.",
                ToString(node->op)), node->location };
        {
            Handle<Value> loadedExpression = ThrowOnError(expression->Load(), node->location);
            llvm::Value* incrementedValueResult = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(Int, CreateAdd),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateAdd),
                BINARY_DISPATCH_FUNCTION(Double, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFAdd)
            )(loadedExpression->GetType().GetTypeInfo()->type, loadedExpression->GetLLVMValue(), one->GetLLVMValue()), node->location);
            Handle<Value> incrementedValue = ThrowOnError(Value::Create(incrementedValueResult, expression->GetType(), context), node->location);
            if (auto err = expression->Store(incrementedValue); err.has_value())
                throw Exception{ err.value(), node->location };
            lastReturnedValue = loadedExpression;
            return;
        }
        break;
    case Operator::ID::PostDecrement:
        if (!expression->IsAssignable())
            throw Exception{ std::format("Increment/decrement operator `{}` requires a numeric l-value.",
                ToString(node->op)), node->location };
        {
            Handle<Value> loadedExpression = ThrowOnError(expression->Load(), node->location);
            llvm::Value* decrementedValueResult = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFSub),
                BINARY_DISPATCH_FUNCTION(Int, CreateSub),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateSub),
                BINARY_DISPATCH_FUNCTION(Double, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VectorDouble, CreateFSub)
            )(loadedExpression->GetType().GetTypeInfo()->type, loadedExpression->GetLLVMValue(), one->GetLLVMValue()), node->location);
            Handle<Value> decrementedValue = ThrowOnError(Value::Create(decrementedValueResult, expression->GetType(), context), node->location);
            if (auto err = expression->Store(decrementedValue); err.has_value())
                throw Exception{ err.value(), node->location };
            lastReturnedValue = loadedExpression;
            return;
        }
        break;
    default:
        throw Exception{ std::format("Invalid unary operator `{}`.", ToString(node->op)), node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(Value::Create(result, expression->GetType(), context), node->location);
}

void VCL::ModuleBuilder::VisitFieldAccessExpression(ASTFieldAccessExpression* node) {
    node->expression->Accept(this);
    Handle<Value> expression = lastReturnedValue;

    SetCurrentDebugLocation(context, node);

    if (expression->GetType().GetTypeInfo()->type != TypeInfo::TypeName::Custom)
        throw Exception{ std::format("Cannot access field `{}` on a non-struct type ‘{}’.", 
            node->fieldName, ToString(expression->GetType().GetTypeInfo())), node->location }; 

    
    std::string mangledName{ ToString(expression->GetType().GetTypeInfo()) };
    
    if (auto type = context->GetScopeManager().GetNamedType(mangledName); type.has_value()) {
        Handle<StructDefinition> structDefinition = *type;
        std::string fieldName{ node->fieldName };
        if (!structDefinition->HasField(fieldName))
            throw Exception{ std::format("Type `{}` has no member named `{}`.", 
                ToString(expression->GetType().GetTypeInfo()), fieldName), node->location };
        uint32_t fieldIndex = structDefinition->GetFieldIndex(fieldName);
        llvm::Value* r = context->GetIRBuilder().CreateStructGEP(structDefinition->GetType(), expression->GetLLVMValue(), fieldIndex, fieldName);
        
        Type fieldType = structDefinition->GetFieldType(fieldIndex);
        lastReturnedValue = ThrowOnError(Value::Create(r, fieldType, context), node->location);
    } else {
        throw std::runtime_error{ "Compiler internal error." };
    }
}

void VCL::ModuleBuilder::VisitSubscriptExpression(ASTSubscriptExpression* node) {
    node->expression->Accept(this);
    Handle<Value> expression = lastReturnedValue;
    node->index->Accept(this);
    Handle<Value> index = ThrowOnError(lastReturnedValue->Load(), node->index->location);

    SetCurrentDebugLocation(context, node);
    
    {
        std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
        typeInfo->type = TypeInfo::TypeName::Int;
        Type type = ThrowOnError(Type::Create(typeInfo, context), node->location);
        index = ThrowOnError(index->Cast(type), node->location);
    }

    llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context->GetTSContext().getContext()), 0);

    if (expression->GetType().GetTypeInfo()->type == TypeInfo::TypeName::Array) {
        llvm::Value* value = context->GetIRBuilder().CreateGEP(expression->GetType().GetLLVMType(), expression->GetLLVMValue(),
            { zero, index->GetLLVMValue() });
        Type type = ThrowOnError(Type::Create(expression->GetType().GetTypeInfo()->arguments[0]->typeInfo, context), node->location);
        lastReturnedValue = ThrowOnError(Value::Create(value, type, context), node->location);
    } else if (expression->GetType().GetTypeInfo()->type == TypeInfo::TypeName::Span) {
        llvm::StructType* structType = llvm::cast<llvm::StructType>(expression->GetType().GetLLVMType());
        Type elementType = ThrowOnError(Type::Create(expression->GetType().GetTypeInfo()->arguments[0]->typeInfo, context), node->location);
        llvm::Value* bufferPtr = context->GetIRBuilder().CreateLoad(structType->getElementType(0),
            context->GetIRBuilder().CreateStructGEP(structType, expression->GetLLVMValue(), 0));
        llvm::Value* value = context->GetIRBuilder().CreateGEP(elementType.GetLLVMType(), bufferPtr, index->GetLLVMValue());
        lastReturnedValue = ThrowOnError(Value::Create(value, elementType, context), node->location);
    } else {
        throw Exception{ std::format("Cannot subscript variable of type `{}`.", 
            ToString(expression->GetType().GetTypeInfo())), node->location };
    }
}

void VCL::ModuleBuilder::VisitLiteralExpression(ASTLiteralExpression* node) {
    SetCurrentDebugLocation(context, node);
    switch (node->type)
    {
    case TypeInfo::TypeName::Float:
        lastReturnedValue = ThrowOnError(Value::CreateConstantFloat(node->fValue, context), node->location);
        break;
    case TypeInfo::TypeName::Int:
        lastReturnedValue = ThrowOnError(Value::CreateConstantInt32(node->iValue, context), node->location);
        break;
    default:
        throw Exception{ "Unsupported literal expression", node->location };
    }
}

void VCL::ModuleBuilder::VisitIdentifierExpression(ASTIdentifierExpression* node) {
    SetCurrentDebugLocation(context, node);
    std::shared_ptr<DefineDirective::DefineDirectiveMetaComponent> component = 
        context->GetMetaState()->GetOrCreate<DefineDirective::DefineDirectiveMetaComponent>();
    if (component->Defined(node->name)) {
        ASTLiteralExpression* expression = component->GetDefine(node->name);
        if (expression)
            expression->Accept(this);
        else
            throw Exception{ "Define name is a flag and not an expression", node->location };
    } else {
        lastReturnedValue = ThrowOnError(
            context->GetScopeManager().GetNamedValue(node->name), node->location);
    }
}


void VCL::ModuleBuilder::VisitVariableDeclaration(ASTVariableDeclaration* node) {
    Type type = ThrowOnError(Type::Create(node->type, context), node->location);
    Handle<Value> variable{};
    Handle<Value> initializer{};

    std::string name{ node->name };
    
    if (context->GetScopeManager().IsCurrentScopeGlobal() && type.GetTypeInfo()->IsExtern() && node->expression)
        throw Exception{ "global variable with 'in' and 'out' storage imply external linkage and cannot be initialized.", node->location };

    if (node->expression) {
        node->expression->Accept(this);
        initializer = ThrowOnError(lastReturnedValue->Load(), node->expression->location);
        initializer = ThrowOnError(initializer->Cast(type), node->expression->location);
    }

    SetCurrentDebugLocation(context, node);

    if (context->GetScopeManager().IsCurrentScopeGlobal()) {
        variable = ThrowOnError(Value::CreateGlobalVariable(type, initializer, context, name.c_str(), 
            file, node->location.line, node->location.position), node->location);
            
        std::shared_ptr<VariableInfo> info = std::make_shared<VariableInfo>();
        info->name = name;
        info->typeinfo = type.GetTypeInfo();
        info->attributes = node->attributes;
        context->GetModuleInfo()->AddVariable(info);
    } else {
        variable = ThrowOnError(Value::CreateLocalVariable(type, initializer, context, name.c_str(),
            file, node->location.line, node->location.position), node->location);
    }

    if (!context->GetScopeManager().PushNamedValue(node->name, variable))
        throw Exception{ std::format("Redefinition of `{}`", node->name), node->location };

    lastReturnedValue = variable;
}

void VCL::ModuleBuilder::VisitFunctionCall(ASTFunctionCall* node) {
    Handle<Value> value;

    std::vector<Handle<Value>> argsv( (size_t)node->arguments.size() );
    std::vector<std::shared_ptr<TypeInfo>> argst( (size_t)node->arguments.size() );
    
    for (uint32_t i = 0; i < node->arguments.size(); ++i) {
        node->arguments[i]->Accept(this);
        Handle<Value> argValue = lastReturnedValue;
        argsv[i] = argValue;
        argst[i] = argValue->GetType().GetTypeInfo();
    }

    if (auto r = context->GetScopeManager().GetNamedFunctionTemplate(node->name); r.has_value()) {
        Handle<CallableTemplate> callableTemplate = *r;
        std::string name = ThrowOnError(callableTemplate->Mangle(node->templateArguments, argst), node->location);

        if (auto s = context->GetScopeManager().GetNamedValue(name); s.has_value())
            value = *s;
        else
            value = ThrowOnError(callableTemplate->Resolve(node->templateArguments, argst), node->location);
    } else {
        value = ThrowOnError(context->GetScopeManager().GetNamedValue(node->name), node->location);
    }

    if (!value->GetType().GetTypeInfo()->IsCallable())
        throw Exception{ std::format("`{}` isn't callable", node->name), node->location };
    
    Handle<Callable> callee = HandleCast<Callable, Value>(value);
    
    if (callee->GetCallableType() == CallableType::Function) {
        uint32_t argCount = HandleCast<Function>(callee)->GetArgCount();

        if (argCount > node->arguments.size())
            throw Exception{ std::format("`{}` calling with too few argument(s), expecting {} argument(s)", node->name, argCount), node->location };
        if (argCount < node->arguments.size())
            throw Exception{ std::format("`{}` calling with too many argument(s), expecting {} argument(s)", node->name, argCount), node->location };
    } else if (!callee->CheckArgCount(node->arguments.size())) {
        throw Exception{ std::format("`{}` wrong number of arguments given.", node->name), node->location };
    }

    for (uint32_t i = 0; i < argsv.size(); ++i) {
        if (!callee->CheckArgType(i, argsv[i]->GetType())) {
            if (callee->GetCallableType() == CallableType::Function) {
                if (HandleCast<Function>(callee)->GetArgType(i).GetTypeInfo()->IsGivenByReference() && !argsv[i]->GetType().IsPointerType()) {
                    throw Exception{ "Cannot pass r-value by reference.", node->location };
                } else if (HandleCast<Function>(callee)->GetArgType(i).GetTypeInfo()->IsGivenByValue()) {
                    argsv[i] = ThrowOnError(argsv[i]->Cast(HandleCast<Function>(callee)->GetArgType(i)), node->location);
                } else {
                    throw Exception{ std::format("Argument number {} is of wrong type", i), node->location };
                }
            } else {
                throw Exception{ std::format("Argument number {} is of wrong type", i), node->location };
            }
        }
    }
    
    SetCurrentDebugLocation(context, node);

    lastReturnedValue = ThrowOnError(callee->Call(argsv), node->location);
}

void VCL::ModuleBuilder::VisitAggregateExpression(ASTAggregateExpression* node) {
    std::vector<Handle<Value>> values{};

    for (size_t i = 0; i < node->values.size(); ++i) {
        node->values[i]->Accept(this);
        values.push_back(lastReturnedValue);
    }
    
    lastReturnedValue = ThrowOnError(Aggregate::Create(values, context), node->location);
}

void VCL::ModuleBuilder::VisitDirective(ASTDirective* node) {
    std::shared_ptr<DirectiveRegistry> registry = context->GetDirectiveRegistry();
    if (!registry)
        throw Exception{ "Tried to use compiler directive but no directive registry has been given to the module context", node->location };
    std::shared_ptr<DirectiveHandler> handler = registry->GetDirective(node->GetName());
    if (!handler)
        throw Exception{ "Tried to use compiler directive that is not contained in the given directive registry", node->location };
    std::shared_ptr<MetaState> state = context->GetMetaState();
    if (!state)
        throw Exception{ "Tried to use compiler directive without any MetaState class instance given to the module context", node->location };
    handler->Run(context, node, this);
}