#include "ModuleBuilder.hpp"

#include "Utility.hpp"
#include "Callable.hpp"
#include "Intrinsic.hpp"
#include "StructDefinition.hpp"
#include "StructTemplate.hpp"
#include "CallableTemplate.hpp"

#include <VCL/Debug.hpp>

#include <iostream>


VCL::ModuleBuilder::ModuleBuilder(ModuleContext* context) : context{ context } {

}

VCL::ModuleBuilder::~ModuleBuilder() {
    
}

void VCL::ModuleBuilder::VisitProgram(ASTProgram* node) {
    for (auto& statement : node->statements)
        statement->Accept(this);
}

void VCL::ModuleBuilder::VisitCompoundStatement(ASTCompoundStatement* node) {
    ScopeGuard scopeGuard{ &context->GetScopeManager() };

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

    std::vector<Function::ArgInfo> argsInfo( node->arguments.size() );
    for (size_t i = 0; i < node->arguments.size(); ++i) {
        Type argType = ThrowOnError(Type::Create(node->arguments[i]->type, context), 
            node->arguments[i]->location);
        argsInfo[i] = Function::ArgInfo{ argType, node->arguments[i]->name };
    }

    Handle<Function> function = ThrowOnError(Function::Create(returnType, argsInfo, node->name, context), node->location);
    context->GetScopeManager().PushNamedValue(node->name, function);
    lastReturnedValue = function;
}

void VCL::ModuleBuilder::VisitFunctionDeclaration(ASTFunctionDeclaration* node) {
    node->prototype->Accept(this);
    Handle<Function> function = HandleCast<Function, Value>(lastReturnedValue);
    if (function->HasStorage())
        throw Exception{ "Function redefinition", node->location };

    llvm::Function* llvmFunction = function->GetLLVMFunction();
    
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "entry", llvmFunction);
    context->GetIRBuilder().SetInsertPoint(bb);

    ScopeGuard scopeGuard{ &context->GetScopeManager() };
    
    for (size_t i = 0; i < llvmFunction->arg_size(); ++i) {
        Type argType = function->GetArgsInfo()[i].type;
        std::string_view argName = function->GetArgsInfo()[i].name;
        std::string argNameStr{ argName };
        Handle<Value> argValue = ThrowOnError(Value::Create(llvmFunction->getArg(i), argType, context), node->location);
        Handle<Value> arg = ThrowOnError(Value::CreateLocalVariable(argType, argValue, context, argNameStr.c_str()), node->location);
        context->GetScopeManager().PushNamedValue(argName, arg);
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
    std::string_view name = node->name;
    std::vector<std::pair<std::string_view, std::shared_ptr<TypeInfo>>> structTemplate{};
    std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>> templateParameters{};

    for (size_t i = 0; i < node->fields.size(); ++i)
        structTemplate.push_back(std::make_pair(node->fields[i]->name, node->fields[i]->type));
    for (size_t i = 0; i < node->parameters.size(); ++i)
        templateParameters.push_back(std::make_pair(node->parameters[i]->name, node->parameters[i]->type));

    Handle<StructTemplate> st = ThrowOnError(StructTemplate::Create(name, structTemplate, templateParameters, context), node->location);

    if (!context->GetScopeManager().PushNamedStructTemplate(node->name, st))
        throw Exception{ std::format("redefinition of `{}`", node->name), node->location };
}

void VCL::ModuleBuilder::VisitTemplateFunctionDeclaration(ASTTemplateFunctionDeclaration* node) {
    std::string_view name = node->name;
    std::vector<std::pair<std::string_view, std::shared_ptr<TypeInfo>>> functionArguments{};
    std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>> templateParameters{};

    for (size_t i = 0; i < node->arguments.size(); ++i)
        functionArguments.push_back(std::make_pair(node->arguments[i]->name, node->arguments[i]->type));
    for (size_t i = 0; i < node->parameters.size(); ++i)
        templateParameters.push_back(std::make_pair(node->parameters[i]->name, node->parameters[i]->type));

    Handle<CallableTemplate> ft = ThrowOnError(CallableTemplate::Create(name, node->type,
        functionArguments, templateParameters, std::move(node->body), context), node->location);
    
    if (!context->GetScopeManager().PushNamedFunctionTemplate(node->name, ft))
        throw Exception{ std::format("redefinition of `{}`", node->name), node->location };
}

void VCL::ModuleBuilder::VisitReturnStatement(ASTReturnStatement* node) {
    if (node->expression) {
        node->expression->Accept(this);
        context->GetIRBuilder().CreateRet(ThrowOnError(lastReturnedValue->Load(), node->location)->GetLLVMValue());
    } else {
        context->GetIRBuilder().CreateRetVoid();
    }
}

void VCL::ModuleBuilder::VisitIfStatement(ASTIfStatement* node) {
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
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "end", function);

    context->GetIRBuilder().CreateCondBr(r, thenBB, elseBB);

    context->GetIRBuilder().SetInsertPoint(thenBB);
    node->thenStmt->Accept(this);
    if (!context->GetIRBuilder().GetInsertBlock()->getTerminator())
        context->GetIRBuilder().CreateBr(endBB);

    context->GetIRBuilder().SetInsertPoint(elseBB);
    if (node->elseStmt)
        node->elseStmt->Accept(this);
    context->GetIRBuilder().CreateBr(endBB);

    context->GetIRBuilder().SetInsertPoint(endBB);
}

void VCL::ModuleBuilder::VisitWhileStatement(ASTWhileStatement* node) {
    llvm::Function* function = context->GetIRBuilder().GetInsertBlock()->getParent();

    if (!function)
        throw Exception{ "A while loop may only be used withing a function's body.", node->location };

    llvm::BasicBlock* conditionBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "condition", function);
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "loop", function);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "end", function);

    ScopeGuard scopeGuard{ &context->GetScopeManager(), endBB };

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
    llvm::Function* function = context->GetIRBuilder().GetInsertBlock()->getParent();

    if (!function)
        throw Exception{ "A for loop may only be used withing a function's body.", node->location };

    llvm::BasicBlock* conditionBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "condition", function);
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "loop", function);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "end", function);

    ScopeGuard scopeGuard{ &context->GetScopeManager(), endBB };

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

    context->GetIRBuilder().CreateBr(dest);
}

void VCL::ModuleBuilder::VisitBinaryArithmeticExpression(ASTBinaryArithmeticExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Numeric | IntrinsicArgumentPolicy::Vector };

    node->lhs->Accept(this);
    Handle<Value> lhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    node->rhs->Accept(this);
    Handle<Value> rhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    rhs = ThrowOnError(rhs->Cast(lhs->GetType()), node->rhs->location);

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
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateAdd)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Sub:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFSub),
                BINARY_DISPATCH_FUNCTION(Int, CreateSub),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateSub)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Mul:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFMul),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFMul),
                BINARY_DISPATCH_FUNCTION(Int, CreateMul),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateMul)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Div:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFDiv),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFDiv),
                BINARY_DISPATCH_FUNCTION(Int, CreateSDiv),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateSDiv)
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

    lastReturnedValue = ThrowOnError(MakeValueVCLFromLLVM(result, context), node->location);
}

void VCL::ModuleBuilder::VisitBinaryLogicalExpression(ASTBinaryLogicalExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Condition | IntrinsicArgumentPolicy::Mask };

    node->lhs->Accept(this);
    Handle<Value> lhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    node->rhs->Accept(this);
    Handle<Value> rhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    rhs = ThrowOnError(rhs->Cast(lhs->GetType()), node->rhs->location);

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

    lastReturnedValue = ThrowOnError(MakeValueVCLFromLLVM(result, context), node->location);
}

void VCL::ModuleBuilder::VisitBinaryComparisonExpression(ASTBinaryComparisonExpression* node) {
    node->lhs->Accept(this);
    Handle<Value> lhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    node->rhs->Accept(this);
    Handle<Value> rhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    rhs = ThrowOnError(rhs->Cast(lhs->GetType()), node->rhs->location);

    llvm::Value* result;

    switch (node->op) {
        case Operator::ID::Greater:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOGT),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOGT),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpUGT)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Less:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOLT),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOLT),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpULT)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::GreaterEqual:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOGE),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOGE),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpUGE)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::LessEqual:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOLE),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOLE),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpULE)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::Equal:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpOEQ),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpOEQ),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpEQ)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case Operator::ID::NotEqual:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFCmpONE),
                BINARY_DISPATCH_FUNCTION(Int, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(Bool, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFCmpONE),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(VectorBool, CreateICmpNE)
            )(lhs->GetType().GetTypeInfo()->type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        default:
            throw Exception{ std::format("Invalid binary comparison operator `{}`.", ToString(node->op)), node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(MakeValueVCLFromLLVM(result, context), node->location);
}

void VCL::ModuleBuilder::VisitAssignmentExpression(ASTAssignmentExpression* node) {
    if (!node->lhs->IsLValue())
        throw Exception{ std::format("Left-hand side of assignment must be a variable or a memory location (l-value)."), node->location };

    node->lhs->Accept(this);
    Handle<Value> lhs = lastReturnedValue;

    node->rhs->Accept(this);
    Handle<Value> rhs = ThrowOnError(lastReturnedValue->Load(), node->location);
    
    rhs = ThrowOnError(rhs->Cast(lhs->GetType()), node->rhs->location);

    if (lhs->GetType().GetTypeInfo()->IsConst())
        throw Exception{ "You cannot assign to a variable that is const.", node->location };
    
    lhs->Store(rhs);
    lastReturnedValue = lhs;
}

void VCL::ModuleBuilder::VisitPrefixArithmeticExpression(ASTPrefixArithmeticExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Numeric | IntrinsicArgumentPolicy::Vector };

    node->expression->Accept(this);
    Handle<Value> expression = lastReturnedValue;

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
                UNARY_DISPATCH_FUNCTION(VectorInt, CreateNeg)
            )(expression->GetType().GetTypeInfo()->type, expression->GetLLVMValue()), node->location);
        break;
    case Operator::ID::PreIncrement:
        if (!node->expression->IsLValue())
            throw Exception{ std::format("Increment/decrement operator `{}` requires a numeric l-value.",
                ToString(node->op)), node->location };
        {
            Handle<Value> loadedExpression = ThrowOnError(expression->Load(), node->location);
            Handle<Value> incrementedValue = ThrowOnError(MakeValueVCLFromLLVM(ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(Int, CreateAdd),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateAdd)
            )(loadedExpression->GetType().GetTypeInfo()->type, loadedExpression->GetLLVMValue(), one->GetLLVMValue()), node->location),
            context), node->location);
            expression->Store(incrementedValue);
            result = incrementedValue->GetLLVMValue();
        }
        break;
    case Operator::ID::PreDecrement:
        if (node->expression->IsLValue())
            throw Exception{ std::format("Increment/decrement operator `{}` requires a numeric l-value.",
                ToString(node->op)), node->location };
        {
            Handle<Value> loadedExpression = ThrowOnError(expression->Load(), node->location);
            Handle<Value> decrementedValue = ThrowOnError(MakeValueVCLFromLLVM(ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFSub),
                BINARY_DISPATCH_FUNCTION(Int, CreateSub),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateSub)
            )(loadedExpression->GetType().GetTypeInfo()->type, loadedExpression->GetLLVMValue(), one->GetLLVMValue()), node->location),
            context), node->location);
            expression->Store(decrementedValue);
            result = decrementedValue->GetLLVMValue();
        }
        break;
    default:
        throw Exception{ std::format("Invalid unary operator `{}`.", ToString(node->op)), node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(MakeValueVCLFromLLVM(result, context), node->location);
}

void VCL::ModuleBuilder::VisitPrefixLogicalExpression(ASTPrefixLogicalExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Condition | IntrinsicArgumentPolicy::Mask };

    node->expression->Accept(this);
    Handle<Value> expression = ThrowOnError(lastReturnedValue->Load(), node->location);

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

    lastReturnedValue = ThrowOnError(MakeValueVCLFromLLVM(result, context), node->location);
}

void VCL::ModuleBuilder::VisitPostfixArithmeticExpression(ASTPostfixArithmeticExpression* node) {
    IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Numeric | IntrinsicArgumentPolicy::Vector };

    node->expression->Accept(this);
    Handle<Value> expression = lastReturnedValue;

    if (!policy(expression->GetType()))
        throw Exception{ std::format("Arithmetic unary operator `{}` expects a numeric operand, but got `{}`.",
            ToString(node->op), ToString(expression->GetType().GetTypeInfo())), node->location };

    Handle<Value> one = ThrowOnError(Value::CreateConstantInt32(1, context), node->location);
    one = ThrowOnError(one->Cast(expression->GetType()), node->location);

    llvm::Value* result;

    switch (node->op)
    {
    case Operator::ID::PostIncrement:
        if (node->expression->IsLValue())
            throw Exception{ std::format("Increment/decrement operator `{}` requires a numeric l-value.",
                ToString(node->op)), node->location };
        {
            Handle<Value> loadedExpression = ThrowOnError(expression->Load(), node->location);
            Handle<Value> incrementedValue = ThrowOnError(MakeValueVCLFromLLVM(ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(Int, CreateAdd),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateAdd)
            )(loadedExpression->GetType().GetTypeInfo()->type, loadedExpression->GetLLVMValue(), one->GetLLVMValue()), node->location),
            context), node->location);
            expression->Store(incrementedValue);
            result = loadedExpression->GetLLVMValue();
        }
        break;
    case Operator::ID::PostDecrement:
        if (node->expression->IsLValue())
            throw Exception{ std::format("Increment/decrement operator `{}` requires a numeric l-value.",
                ToString(node->op)), node->location };
        {
            Handle<Value> loadedExpression = ThrowOnError(expression->Load(), node->location);
            Handle<Value> decrementedValue = ThrowOnError(MakeValueVCLFromLLVM(ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(Float, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VectorFloat, CreateFSub),
                BINARY_DISPATCH_FUNCTION(Int, CreateSub),
                BINARY_DISPATCH_FUNCTION(VectorInt, CreateSub)
            )(loadedExpression->GetType().GetTypeInfo()->type, loadedExpression->GetLLVMValue(), one->GetLLVMValue()), node->location),
            context), node->location);
            expression->Store(decrementedValue);
            result = loadedExpression->GetLLVMValue();
        }
        break;
    default:
        throw Exception{ std::format("Invalid unary operator `{}`.", ToString(node->op)), node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(MakeValueVCLFromLLVM(result, context), node->location);
}

void VCL::ModuleBuilder::VisitFieldAccessExpression(ASTFieldAccessExpression* node) {
    node->expression->Accept(this);
    Handle<Value> expression = lastReturnedValue;

    if (expression->GetType().GetTypeInfo()->type != TypeInfo::TypeName::Custom)
        throw Exception{ std::format("Cannot access field `{}` on a non-struct type ‘{}’.", 
            node->fieldName, ToString(expression->GetType().GetTypeInfo())), node->location }; 
    
    if (auto type = context->GetScopeManager().GetNamedType(expression->GetType().GetLLVMType()->getStructName()); type.has_value()) {
        Handle<StructDefinition> structDefinition = *type;
        std::string fieldName{ node->fieldName };
        if (!structDefinition->HasField(fieldName))
            throw Exception{ std::format("Type `{}` has no member named `{}`.", 
                ToString(expression->GetType().GetTypeInfo()), fieldName), node->location };
        uint32_t fieldIndex = structDefinition->GetFieldIndex(fieldName);
        llvm::Value* r = context->GetIRBuilder().CreateStructGEP(structDefinition->GetType(), expression->GetLLVMValue(), fieldIndex, fieldName);
        Type fieldType = ThrowOnError(Type::CreateFromLLVMType(structDefinition->GetType()->getTypeAtIndex(fieldIndex), context), node->location);
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

void VCL::ModuleBuilder::VisitVariableExpression(ASTVariableExpression* node) {
    lastReturnedValue = ThrowOnError(
        context->GetScopeManager().GetNamedValue(node->name), node->location);
}

void VCL::ModuleBuilder::VisitVariableDeclaration(ASTVariableDeclaration* node) {
    Type type = ThrowOnError(Type::Create(node->type, context), node->location);
    Handle<Value> variable{};
    Handle<Value> initializer{};

    std::string name{ node->name };
    
    if (node->expression) {
        node->expression->Accept(this);
        initializer = ThrowOnError(lastReturnedValue->Load(), node->expression->location);;
    }

    if (context->GetScopeManager().IsCurrentScopeGlobal())
        variable = ThrowOnError(Value::CreateGlobalVariable(type, initializer, context, name.c_str()), node->location);
    else
        variable = ThrowOnError(Value::CreateLocalVariable(type, initializer, context, name.c_str()), node->location);

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
        Handle<Value> argValue = ThrowOnError(lastReturnedValue->Load(), node->location);
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
        if (!callee->CheckArgType(i, argsv[i]->GetType()))
            if (callee->GetCallableType() == CallableType::Function)
                argsv[i] = ThrowOnError(argsv[i]->Cast(HandleCast<Function>(callee)->GetArgType(i)), node->location);
            else
                throw Exception{ std::format("Argument number {} is of wrong type", i), node->location };
    }

    lastReturnedValue = ThrowOnError(callee->Call(argsv), node->location);
}