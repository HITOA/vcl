#include "CallableTemplate.hpp"

#include <VCL/Debug.hpp>

#include "ModuleBuilder.hpp"
#include "ModuleContext.hpp"
#include "TemplateUtils.hpp"

#include <iostream>


VCL::CallableTemplate::CallableTemplate(std::string_view name, 
    std::shared_ptr<TypeInfo> returnType,
    std::vector<std::pair<std::string_view, std::shared_ptr<TypeInfo>>> functionArguments,
    std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>> templateParameters,
    std::unique_ptr<ASTNode> body, llvm::DIFile* file, uint32_t line, ModuleContext* context) : name{ name },
        returnType{ returnType }, functionArguments{ functionArguments }, templateParameters{ templateParameters },
        body{ std::move(body) }, file{ file }, line{ line }, context{ context } {}

std::expected<std::string, VCL::Error> VCL::CallableTemplate::Mangle(std::vector<std::shared_ptr<TemplateArgument>>& args, std::vector<std::shared_ptr<TypeInfo>>& resolvedArguments) {
    TemplateArgumentMapper mapper{};
    mapper.SetName(name);
    if (auto err = mapper.Map(args, templateParameters))
        return std::unexpected{ err.value() };

    std::vector<std::shared_ptr<TypeInfo>> templatedArguments( functionArguments.size() );
    for (size_t i = 0; i < functionArguments.size(); ++i)
        templatedArguments[i] = functionArguments[i].second;

    if (auto err = mapper.Infer(templateParameters, templatedArguments, resolvedArguments))
        return std::unexpected{ err.value() };

    if (auto err = mapper.CheckParameters(templateParameters))
        return std::unexpected{ err.value() };

    return mapper.Mangle();
}

std::expected<VCL::Handle<VCL::Callable>, VCL::Error> VCL::CallableTemplate::Resolve(std::vector<std::shared_ptr<TemplateArgument>>& args, 
    std::vector<std::shared_ptr<TypeInfo>>& resolvedArguments) {
    std::string mangledName;

    if (auto err = Mangle(args, resolvedArguments); err.has_value())
        mangledName = *err;
    else
        std::unexpected{ err.error() };

    TemplateArgumentMapper mapper{};
    mapper.SetName(name);
    if (auto err = mapper.Map(args, templateParameters))
        return std::unexpected{ err.value() };

    std::vector<std::shared_ptr<TypeInfo>> templatedArguments( functionArguments.size() );
    for (size_t i = 0; i < functionArguments.size(); ++i)
        templatedArguments[i] = functionArguments[i].second;
    
    if (auto err = mapper.Infer(templateParameters, templatedArguments, resolvedArguments))
        return std::unexpected{ err.value() };
    
    if (auto err = mapper.CheckParameters(templateParameters))
        return std::unexpected{ err.value() };

    llvm::IRBuilder<>::InsertPointGuard ipGuard{ context->GetIRBuilder() };
    ScopeGuard scopeGuard{ &context->GetScopeManager() };

    for (auto& arg : mapper) {
        if (arg.second->type == TemplateArgument::TemplateValueType::Typename)
            context->GetScopeManager().PushNamedTypeAlias(arg.first, arg.second->typeInfo);
        if (arg.second->type == TemplateArgument::TemplateValueType::Int)
            if (auto r = Value::CreateConstantInt32(arg.second->intValue, context); r.has_value())
                context->GetScopeManager().PushNamedValue(arg.first, *r);
            else
                return std::unexpected{ r.error() };
    }
    
    Type returnType;
    if (auto t = mapper.ResolveType(this->returnType); t.has_value()) {
        if (auto r = Type::Create(*t, context); r.has_value())
            returnType = *r;
        else
            return std::unexpected{ r.error() };
    }
    else
        return std::unexpected{ t.error() };

    std::vector<Function::ArgInfo> argsInfo( functionArguments.size() );
    for (size_t i = 0; i < functionArguments.size(); ++i) {
        std::shared_ptr<TypeInfo> argTypeInfo = functionArguments[i].second;
        if (auto r = mapper.ResolveType(argTypeInfo); r.has_value())
            argTypeInfo = *r;
        else
            return std::unexpected{ r.error() };
        Type argType;
        if (auto r = Type::Create(argTypeInfo, context); r.has_value())
            argType = *r;
        else
            return std::unexpected{ r.error() };
        
        argsInfo[i] = Function::ArgInfo{ argType, functionArguments[i].first };
    }

    Handle<Function> function;
    if (auto r = Function::Create(returnType, argsInfo, mangledName, context); r.has_value())
        function = *r;
    else
        return std::unexpected{ r.error() };
    
    uint32_t offset = context->GetScopeManager().GetNamedFunctionTemplateOffset(name);
    context->GetScopeManager().PushNamedValue(mangledName, function, offset);
    
    llvm::Function* llvmFunction = function->GetLLVMFunction();
    llvm::DISubprogram* subprogram = nullptr;
    
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(*context->GetTSContext().getContext(), "entry", llvmFunction);
    context->GetIRBuilder().SetInsertPoint(bb);

    if (file) {
        subprogram = context->GetDIBuilder().createFunction(
            file, mangledName, mangledName,
            file, line + 1, function->GetDIType(), line + 1, 
            llvm::DINode::DIFlags::FlagPrototyped, llvm::DISubprogram::DISPFlags::SPFlagDefinition
        );
        llvmFunction->setSubprogram(subprogram);
        context->GetScopeManager().SetCurrentDebugInformationScope(subprogram);
        llvm::DILocation* loc = llvm::DILocation::get(*(context->GetTSContext().getContext()),
            line + 1, 0, subprogram);
        context->GetIRBuilder().SetCurrentDebugLocation(loc);
    }

    for (size_t i = 0; i < llvmFunction->arg_size(); ++i) {
        Type argType = function->GetArgsInfo()[i].type;
        std::string_view argName = function->GetArgsInfo()[i].name;
        std::string argNameStr{ argName };
        Handle<Value> argValue;
        if (auto r = Value::Create(llvmFunction->getArg(i), argType, context); r.has_value())
            argValue = *r;
        else
            return std::unexpected{ r.error() };

        if (argType.GetTypeInfo()->IsGivenByReference()) {
            context->GetScopeManager().PushNamedValue(argName, argValue);
        } else {
            Handle<Value> arg;
            if (auto r = Value::CreateLocalVariable(argType, argValue, context, argNameStr.c_str(), file, line, 0); r.has_value())
                arg = *r;
            else
                return std::unexpected{ r.error() };
            context->GetScopeManager().PushNamedValue(argName, arg);
        }
    }

    if (body) {
        ModuleBuilder builder{ context };
        builder.file = file;
        body->Accept(&builder);
    } else {
        return std::unexpected{ std::format("Missing body in templated function `{}`", name) };
    }

    scopeGuard.Release();

    for (llvm::BasicBlock& bb : *llvmFunction) {
        llvm::Instruction* terminator = bb.getTerminator();
        if (terminator) continue;
        if (llvmFunction->getReturnType()->isVoidTy()) {
            context->GetIRBuilder().SetInsertPoint(&bb);
            context->GetIRBuilder().CreateRetVoid();
        } else {
            return std::unexpected{ Error{ "Missing return statement." } };
        }
    }

    if (subprogram)
        context->GetDIBuilder().finalizeSubprogram(subprogram);

    function->Verify();

    return function;
}

std::expected<VCL::Handle<VCL::CallableTemplate>, VCL::Error> VCL::CallableTemplate::Create(std::string_view name, 
            std::shared_ptr<TypeInfo> returnType,
            std::vector<std::pair<std::string_view, std::shared_ptr<TypeInfo>>> functionArguments,
            std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>> templateParameters,
            std::unique_ptr<ASTNode> body, llvm::DIFile* file, uint32_t line, ModuleContext* context) {
    return MakeHandle<CallableTemplate>(name, returnType, functionArguments, templateParameters, std::move(body), file, line, context);
}