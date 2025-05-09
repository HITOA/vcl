#include "TemplateUtils.hpp"

#include <unordered_set>
#include <VCL/Debug.hpp>
#include <iostream>


std::optional<VCL::Error> VCL::TemplateArgumentMapper::Map(std::vector<std::shared_ptr<TemplateArgument>>& arguments, 
    std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters) {

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
        if (resolvedArguments->arguments[i]->type != TemplateArgument::TemplateValueType::Typename)
            continue;
        if (auto err = InferOne(templateParameters, templatedArgument->arguments[i]->typeInfo, resolvedArguments->arguments[i]->typeInfo))
            return err;
    }

    return {};
}

std::optional<VCL::Error> VCL::TemplateArgumentMapper::Infer(std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters,
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

std::optional<VCL::Error> VCL::TemplateArgumentMapper::CheckParameters(std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters) {
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
    for (size_t j = 0; j < newTypeInfo->arguments.size(); ++j) {
        std::shared_ptr<TemplateArgument> ta = newTypeInfo->arguments[j];
        if (ta->type != TemplateArgument::TemplateValueType::Typename)
            continue;
        if (auto r = ResolveType(ta->typeInfo); r.has_value())
            ta->typeInfo = *r;
        else
            return std::unexpected{ r.error() };
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