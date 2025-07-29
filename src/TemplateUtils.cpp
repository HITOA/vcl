#include "TemplateUtils.hpp"

#include <unordered_set>
#include <VCL/Debug.hpp>
#include <VCL/Directive.hpp>
#include <iostream>


std::optional<VCL::Error> VCL::TemplateArgumentMapper::Map(std::vector<std::shared_ptr<TemplateArgument>>& arguments, 
    std::vector<std::pair<std::string, TemplateArgument::TemplateValueType>>& templateParameters) {

    if (arguments.size() > templateParameters.size())
        return Error{ std::format("In template  `{}`, {} arguments were given but {} were expected.", 
            name, arguments.size(), templateParameters.size()) };

    for (size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i]->type != templateParameters[i].second)
            return Error{ "Wrong template parameter type." };
        map[std::string{ templateParameters[i].first }] = arguments[i];
    }

    return {};
}

std::optional<VCL::Error> VCL::TemplateArgumentMapper::InferOne(std::unordered_map<std::string, TemplateArgument::TemplateValueType>& templateParameters,
    std::shared_ptr<TypeInfo> templatedArgument, std::shared_ptr<TypeInfo> resolvedArguments) {
    
    std::string templatedTypeInfoName{ templatedArgument->name };
    if (templateParameters.count(templatedTypeInfoName) && !map.count(templatedTypeInfoName)) {
        std::shared_ptr<TemplateArgument> templateArgument = std::make_shared<TemplateArgument>();
        templateArgument->type = TemplateArgument::TemplateValueType::Typename;
        templateArgument->typeInfo = resolvedArguments;
        map[templatedTypeInfoName] = templateArgument;
    }

    if (templatedArgument->arguments.size() > resolvedArguments->arguments.size())
        return Error{ std::format("Given arguments doesn't match templated function arguments in `{}`.", name) };

    for (size_t i = 0; i < templatedArgument->arguments.size(); ++i) {
        if (templatedArgument->arguments[i]->type != TemplateArgument::TemplateValueType::Typename)
            continue;
        if (resolvedArguments->arguments[i]->type == TemplateArgument::TemplateValueType::Int) {
            templatedTypeInfoName = std::string{ templatedArgument->arguments[i]->typeInfo->name };
            if (templateParameters.count(templatedTypeInfoName) && !map.count(templatedTypeInfoName)) {
                std::shared_ptr<TemplateArgument> templateArgument = std::make_shared<TemplateArgument>();
                templateArgument->type = TemplateArgument::TemplateValueType::Int;
                templateArgument->intValue = resolvedArguments->arguments[i]->intValue;
                map[templatedTypeInfoName] = templateArgument;
            }
            continue;
        }
        if (auto err = InferOne(templateParameters, templatedArgument->arguments[i]->typeInfo, resolvedArguments->arguments[i]->typeInfo))
            return err;
    }

    return {};
}

std::optional<VCL::Error> VCL::TemplateArgumentMapper::Infer(std::vector<std::pair<std::string, TemplateArgument::TemplateValueType>>& templateParameters,
    std::vector<std::shared_ptr<TypeInfo>>& templatedArguments, std::vector<std::shared_ptr<TypeInfo>>& resolvedArguments) {

    if (templatedArguments.size() != resolvedArguments.size())
        return Error{ "Missing number of arguments. {} were given but {} were expected.", 
            resolvedArguments.size(), templatedArguments.size() };

    std::unordered_map<std::string, TemplateArgument::TemplateValueType> templateParametersMap{};

    for (size_t i = 0; i < templateParameters.size(); ++i) {
        std::string parameterName{ templateParameters[i].first };
        templateParametersMap.insert(std::make_pair(parameterName, templateParameters[i].second));
    }

    for (size_t i = 0; i < resolvedArguments.size(); ++i)
        if (auto err = InferOne(templateParametersMap, templatedArguments[i], resolvedArguments[i]))
            return err.value();

    return {};
}

std::optional<VCL::Error> VCL::TemplateArgumentMapper::CheckParameters(std::vector<std::pair<std::string, TemplateArgument::TemplateValueType>>& templateParameters) {
    for (size_t i = 0; i < templateParameters.size(); ++i) {
        std::string strn{ templateParameters[i].first };
        if (!map.count(strn))
            return Error{ std::format("In template `{}`, parameter `{}` is missing.", name, strn) };
    }
    return {};
}

std::expected<std::shared_ptr<VCL::TypeInfo>, VCL::Error> VCL::TemplateArgumentMapper::ResolveType(std::shared_ptr<TypeInfo> typeInfo) {
    std::shared_ptr<TypeInfo> newTypeInfo = std::make_shared<TypeInfo>(*typeInfo);
    std::string n{ typeInfo->name };
    if (typeInfo->type == TypeInfo::TypeName::Custom) {
        if (map.count(n)) {
            std::shared_ptr<TemplateArgument> argument = map[n];
            if (argument->type != TemplateArgument::TemplateValueType::Typename)
                return std::unexpected{ Error{ std::format("Template parameter `{}` is not a typename.", typeInfo->name) } };
            newTypeInfo->type = argument->typeInfo->type;
            newTypeInfo->name = argument->typeInfo->name;
            newTypeInfo->qualifiers = argument->typeInfo->qualifiers;
            newTypeInfo->arguments = argument->typeInfo->arguments;
        }
    }
    for (size_t i = 0; i < newTypeInfo->arguments.size(); ++i) {
        std::shared_ptr<TemplateArgument> ta = newTypeInfo->arguments[i];
        std::shared_ptr<TemplateArgument> newta = std::make_shared<TemplateArgument>(*ta);
        if (newta->type != TemplateArgument::TemplateValueType::Typename)
            continue;
        n = std::string{newta->typeInfo->name};
        if (map.count(n)) {
            std::shared_ptr<TemplateArgument> argument = map[n];
            if (argument->type == TemplateArgument::TemplateValueType::Int) {
                newta->type = TemplateArgument::TemplateValueType::Int;
                newta->typeInfo = nullptr;
                newta->intValue = argument->intValue;
            } else {
                if (auto r = ResolveType(ta->typeInfo); r.has_value())
                    newta->typeInfo = *r;
                else
                    return std::unexpected{ r.error() };
            }
        }
        newTypeInfo->arguments[i] = newta;
    }
    return newTypeInfo;
}

std::string VCL::TemplateArgumentMapper::Mangle() {
    std::string mangledName{ name };
    for (auto v : map) {
        mangledName += "_" + ToString(v.second);
    }
    return mangledName;
}

void VCL::ResolveDefinedTemplateArgumentAsInt(std::shared_ptr<TemplateArgument> argument, std::shared_ptr<MetaState> state) {
    if (argument->type == TemplateArgument::TemplateValueType::Typename) {
        std::shared_ptr<DefineDirective::DefineDirectiveMetaComponent> component = state->GetOrCreate<DefineDirective::DefineDirectiveMetaComponent>();
        
        if (component->Defined(argument->typeInfo->name)) {
            ASTLiteralExpression* expression = component->GetDefine(argument->typeInfo->name);
            if (expression->type != TypeInfo::TypeName::Int)
                throw std::runtime_error{ std::format("Template numeric arguments must be int. But `{}` is a float", argument->typeInfo->name) };
            argument->type = TemplateArgument::TemplateValueType::Int;
            argument->typeInfo = nullptr;
            argument->intValue = expression->iValue;
        }
    }
}