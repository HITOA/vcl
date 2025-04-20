#include "ModuleBuilder.hpp"

#include "Utility.hpp"
#include "Callable.hpp"

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
    std::vector<llvm::Type*> elements(node->fields.size());

    for (size_t i = 0; i < node->fields.size(); ++i) {
        Type type = ThrowOnError(Type::Create(node->fields[i]->type, context), node->fields[i]->location);
        elements[i] = type.GetLLVMType();
    }

    llvm::StructType* type = llvm::StructType::create(*context->GetTSContext().getContext(), elements);
    if (!context->GetScopeManager().PushNamedType(node->name, type))
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
    TypeInfo typeInfo{};
    typeInfo.type = TypeInfo::TypeName::BOOLEAN;
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
    TypeInfo typeInfo{};
    typeInfo.type = TypeInfo::TypeName::BOOLEAN;
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
    TypeInfo typeInfo{};
    typeInfo.type = TypeInfo::TypeName::BOOLEAN;
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

void VCL::ModuleBuilder::VisitUnaryExpression(ASTUnaryExpression* node) {
    node->expression->Accept(this);

    Handle<Value> value = ThrowOnError(lastReturnedValue->Load(), node->location);
    llvm::Value* result;
    
    switch (node->op) {
        case UnaryOpType::PLUS:
            result = value->GetLLVMValue();
            break;
        case UnaryOpType::MINUS:
            result = ThrowOnError(DISPATCH_UNARY(
                UNARY_DISPATCH_FUNCTION(FLOAT, CreateFNeg),
                UNARY_DISPATCH_FUNCTION(VFLOAT, CreateFNeg),
                UNARY_DISPATCH_FUNCTION(INT, CreateNeg),
                UNARY_DISPATCH_FUNCTION(VINT, CreateNeg)
            )(value->GetType().GetTypeInfo().type, value->GetLLVMValue()), node->location);
            break;
        case UnaryOpType::NOT:
            result = ThrowOnError(DISPATCH_UNARY(
                UNARY_DISPATCH_FUNCTION(BOOLEAN, CreateNot)
            )(value->GetType().GetTypeInfo().type, value->GetLLVMValue()), node->location);
            break;
        default:
            throw Exception{ "Invalid unary operator", node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(MakeValueVCLFromLLVM(result, context), node->location);
}

void VCL::ModuleBuilder::VisitBinaryExpression(ASTBinaryExpression* node) {
    node->lhs->Accept(this);
    Handle<Value> lhs = ThrowOnError(lastReturnedValue->Load(), node->location);
    node->rhs->Accept(this);
    Handle<Value> rhs = ThrowOnError(lastReturnedValue->Load(), node->location);

    rhs = ThrowOnError(rhs->Cast(lhs->GetType()), node->location);

    llvm::Value* result;
    
    switch (node->op) {
        case BinaryOpType::ADDITION:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFAdd),
                BINARY_DISPATCH_FUNCTION(INT, CreateAdd),
                BINARY_DISPATCH_FUNCTION(VINT, CreateAdd)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::SUBSTRACTION:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFSub),
                BINARY_DISPATCH_FUNCTION(INT, CreateSub),
                BINARY_DISPATCH_FUNCTION(VINT, CreateSub)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::MULTIPLICATION:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFMul),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFMul),
                BINARY_DISPATCH_FUNCTION(INT, CreateMul),
                BINARY_DISPATCH_FUNCTION(VINT, CreateMul)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::DIVISION:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFDiv),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFDiv),
                BINARY_DISPATCH_FUNCTION(INT, CreateSDiv),
                BINARY_DISPATCH_FUNCTION(VINT, CreateSDiv)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        
        case BinaryOpType::SUPERIOR:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOGT),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFCmpOGT),
                BINARY_DISPATCH_FUNCTION(VINT, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(VBOOL, CreateICmpUGT)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::INFERIOR:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOLT),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFCmpOLT),
                BINARY_DISPATCH_FUNCTION(VINT, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(VBOOL, CreateICmpULT)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::SUPERIOREQUAL:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOGE),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFCmpOGE),
                BINARY_DISPATCH_FUNCTION(VINT, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(VBOOL, CreateICmpUGE)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::INFERIOREQUAL:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOLE),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFCmpOLE),
                BINARY_DISPATCH_FUNCTION(VINT, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(VBOOL, CreateICmpULE)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::EQUAL:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOEQ),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFCmpOEQ),
                BINARY_DISPATCH_FUNCTION(VINT, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(VBOOL, CreateICmpEQ)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::NOTEQUAL:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpONE),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFCmpONE),
                BINARY_DISPATCH_FUNCTION(VINT, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(VBOOL, CreateICmpNE)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::LOGICALAND:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateAnd),
                BINARY_DISPATCH_FUNCTION(VBOOL, CreateAnd)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::LOGICALOR:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateOr),
                BINARY_DISPATCH_FUNCTION(VBOOL, CreateOr)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        default:
            throw Exception{ "Invalid binary operator", node->location }; //Should never happen
    }

    lastReturnedValue = ThrowOnError(MakeValueVCLFromLLVM(result, context), node->location);
}

void VCL::ModuleBuilder::VisitLiteralExpression(ASTLiteralExpression* node) {
    switch (node->type)
    {
    case TypeInfo::TypeName::FLOAT:
        lastReturnedValue = ThrowOnError(Value::CreateConstantFloat(node->fValue, context), node->location);
        break;
    case TypeInfo::TypeName::INT:
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

void VCL::ModuleBuilder::VisitVariableAssignment(ASTVariableAssignment* node) {
    Handle<Value> variable = ThrowOnError(
        context->GetScopeManager().GetNamedValue(node->name), node->location);
    if (variable->GetType().GetTypeInfo().IsConst())
        throw Exception{ "You cannot assign to a variable that is const", node->location };
    node->expression->Accept(this);
    Handle<Value> value = lastReturnedValue;
    variable->Store(value);
    lastReturnedValue = variable;
}

void VCL::ModuleBuilder::VisitVariableDeclaration(ASTVariableDeclaration* node) {
    Type type = ThrowOnError(Type::Create(node->type, context), node->location);
    Handle<Value> variable{};
    Handle<Value> initializer{};

    std::string name{ node->name };
    
    if (node->expression) {
        node->expression->Accept(this);
        initializer = lastReturnedValue;
    }

    if (context->GetScopeManager().IsCurrentScopeGlobal())
        variable = ThrowOnError(Value::CreateGlobalVariable(type, initializer, context, name.c_str()), node->location);
    else
        variable = ThrowOnError(Value::CreateLocalVariable(type, initializer, context, name.c_str()), node->location);

    if (!context->GetScopeManager().PushNamedValue(node->name, variable))
        throw Exception{ std::format("redefinition of `{}`", node->name), node->location };

    lastReturnedValue = variable;
}

void VCL::ModuleBuilder::VisitFunctionCall(ASTFunctionCall* node) {
    Handle<Value> value = ThrowOnError(context->GetScopeManager().GetNamedValue(node->name), node->location);

    if (!value->GetType().GetTypeInfo().IsCallable())
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

    std::vector<Handle<Value>> argsv( (size_t)node->arguments.size() );
    
    for (uint32_t i = 0; i < node->arguments.size(); ++i) {
        node->arguments[i]->Accept(this);
        Handle<Value> argValue = ThrowOnError(lastReturnedValue->Load(), node->location);

        if (!callee->CheckArgType(i, argValue->GetType()))
            if (callee->GetCallableType() == CallableType::Function)
                argValue = ThrowOnError(argValue->Cast(HandleCast<Function>(callee)->GetArgType(i)), node->location);
            else
                throw Exception{ std::format("Argument number {} is of wrong type", i), node->location };
        argsv[i] = argValue;
    }

    lastReturnedValue = ThrowOnError(callee->Call(argsv), node->location);
}