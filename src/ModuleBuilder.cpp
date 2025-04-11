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

    llvm::BasicBlock* bb = llvm::BasicBlock::Create(context->GetContext(), "entry", llvmFunction);
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

void VCL::ModuleBuilder::VisitReturnStatement(ASTReturnStatement* node) {
    if (node->expression) {
        node->expression->Accept(this);
        context->GetIRBuilder().CreateRet(lastReturnedValue->GetLLVMValue());
    } else {
        context->GetIRBuilder().CreateRetVoid();
    }
}

void VCL::ModuleBuilder::VisitIfStatement(ASTIfStatement* node) {

}

void VCL::ModuleBuilder::VisitWhileStatement(ASTWhileStatement* node) {

}

void VCL::ModuleBuilder::VisitForStatement(ASTForStatement* node) {

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
                UNARY_DISPATCH_FUNCTION(INT, CreateNeg)
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
                BINARY_DISPATCH_FUNCTION(INT, CreateAdd)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::SUBSTRACTION:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFSub),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFSub),
                BINARY_DISPATCH_FUNCTION(INT, CreateSub)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::MULTIPLICATION:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFMul),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFMul),
                BINARY_DISPATCH_FUNCTION(INT, CreateMul)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::DIVISION:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFDiv),
                BINARY_DISPATCH_FUNCTION(VFLOAT, CreateFDiv),
                BINARY_DISPATCH_FUNCTION(INT, CreateSDiv)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        
        case BinaryOpType::SUPERIOR:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOGT),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpUGT),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpUGT)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::INFERIOR:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOLT),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpULT),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpULT)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::SUPERIOREQUAL:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOGE),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpUGE),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpUGE)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::INFERIOREQUAL:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOLE),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpULE),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpULE)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::EQUAL:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpOEQ),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpEQ),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpEQ)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::NOTEQUAL:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(FLOAT, CreateFCmpONE),
                BINARY_DISPATCH_FUNCTION(INT, CreateICmpNE),
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateICmpNE)
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::LOGICALAND:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateAnd),
            )(lhs->GetType().GetTypeInfo().type, lhs->GetLLVMValue(), rhs->GetLLVMValue()), node->location);
            break;
        case BinaryOpType::LOGICALOR:
            result = ThrowOnError(DISPATCH_BINARY(
                BINARY_DISPATCH_FUNCTION(BOOLEAN, CreateOr),
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

    context->GetScopeManager().PushNamedValue(node->name, variable);
    lastReturnedValue = variable;
}

void VCL::ModuleBuilder::VisitFunctionCall(ASTFunctionCall* node) {
    Handle<Value> value = ThrowOnError(context->GetScopeManager().GetNamedValue(node->name), node->location);

    if (!value->GetType().GetTypeInfo().IsCallable())
        throw Exception{ std::format("`{}` isn't a function", node->name), node->location };
    
    Handle<Callable> callee = HandleCast<Callable, Value>(value);
    
    if (callee->GetArgCount() > node->arguments.size())
        throw Exception{ std::format("`{}` calling with too few arguments", node->name), node->location };
    if (callee->GetArgCount() < node->arguments.size())
        throw Exception{ std::format("`{}` calling with too many arguments", node->name), node->location };

    std::vector<Handle<Value>> argsv( (size_t)callee->GetArgCount() );
    
    for (uint32_t i = 0; i < callee->GetArgCount(); ++i) {
        node->arguments[i]->Accept(this);
        Handle<Value> argValue = ThrowOnError(lastReturnedValue->Load(), node->location);
        if (!callee->CheckArgType(i, argValue->GetType()))
            argValue = ThrowOnError(argValue->Cast(callee->GetArgType(i)), node->location);
        argsv[i] = argValue;
    }

    lastReturnedValue = ThrowOnError(callee->Call(argsv), node->location);
}