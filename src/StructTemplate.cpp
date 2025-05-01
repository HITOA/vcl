#include "StructTemplate.hpp"

#include "ModuleContext.hpp"
#include <VCL/Debug.hpp>

#include <iostream>


VCL::StructTemplate::StructTemplate(std::string_view name, std::vector<std::pair<std::string_view, TypeInfo>>& structTemplate, 
    std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters, ModuleContext* context) :
    name{ name }, structTemplate{ structTemplate }, templateParameters{ templateParameters }, context{ context } {}

std::string VCL::StructTemplate::Mangle(std::vector<TemplateArgument>& args) {
    std::string mangledName{ name };

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].type == TemplateArgument::TemplateValueType::Int) {
            mangledName += "_" + std::to_string(args[i].value.intValue);
            continue;
        }

        switch (args[i].value.typeValue) {
            case TemplateArgument::ValueUnion::TypeName::Float:
                mangledName += "_float";
                break;
            case TemplateArgument::ValueUnion::TypeName::Bool:
                mangledName += "_bool";
                break;
            case TemplateArgument::ValueUnion::TypeName::Int:
                mangledName += "_int";
                break;
            case TemplateArgument::ValueUnion::TypeName::Void:
                mangledName += "_void";
                break;
            case TemplateArgument::ValueUnion::TypeName::VectorFloat:
                mangledName += "_vfloat";
                break;
            case TemplateArgument::ValueUnion::TypeName::VectorBool:
                mangledName += "_vbool";
                break;
            case TemplateArgument::ValueUnion::TypeName::VectorInt:
                mangledName += "_vint";
                break;
            default:
                mangledName += "_none";
                break;
        }
    }

    return mangledName;
}

std::expected<VCL::Handle<VCL::StructDefinition>, VCL::Error> VCL::StructTemplate::Resolve(std::vector<TemplateArgument>& args) {
    if (args.size() != templateParameters.size())
        return std::unexpected{ Error{ std::format("In template `{}`, {} arguments were given but {} were expected.", 
            name, args.size(), templateParameters.size()) } };
    std::string mangledName = Mangle(args);
    std::unordered_map<std::string, TemplateArgument> templateArgsMap{};

    std::vector<std::pair<std::string, TypeInfo>> elements{};

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].type != templateParameters[i].second)
            return std::unexpected{ Error{ "Wrong template parameter type." } };
        templateArgsMap[std::string{ templateParameters[i].first }] = args[i];
    }

    for (size_t i = 0; i < structTemplate.size(); ++i) {
        std::string fieldName{ structTemplate[i].first };
        TypeInfo typeInfo = structTemplate[i].second;
        std::string n{ typeInfo.name };
        if (typeInfo.type == TypeInfo::TypeName::Custom) {
            if (templateArgsMap.count(n)) {
                TemplateArgument arg = templateArgsMap[n];
                if (arg.type != TemplateArgument::TemplateValueType::Typename)
                    return std::unexpected{ Error{ std::format("Template parameter `{}` is not a typename.", typeInfo.name) } };
                switch (arg.value.typeValue) {
                    case TemplateArgument::ValueUnion::TypeName::Float:
                        typeInfo.type = TypeInfo::TypeName::Float;
                        break;
                    case TemplateArgument::ValueUnion::TypeName::Bool:
                        typeInfo.type = TypeInfo::TypeName::Bool;
                        break;
                    case TemplateArgument::ValueUnion::TypeName::Int:
                        typeInfo.type = TypeInfo::TypeName::Int;
                        break;
                    case TemplateArgument::ValueUnion::TypeName::Void:
                        typeInfo.type = TypeInfo::TypeName::Void;
                        break;
                    case TemplateArgument::ValueUnion::TypeName::VectorFloat:
                        typeInfo.type = TypeInfo::TypeName::VectorFloat;
                        break;
                    case TemplateArgument::ValueUnion::TypeName::VectorBool:
                        typeInfo.type = TypeInfo::TypeName::VectorBool;
                        break;
                    case TemplateArgument::ValueUnion::TypeName::VectorInt:
                        typeInfo.type = TypeInfo::TypeName::VectorInt;
                        break;
                    case TemplateArgument::ValueUnion::TypeName::Custom:
                        typeInfo.type = TypeInfo::TypeName::Custom;
                        typeInfo.name = arg.name;
                        break;
                    default:
                        return std::unexpected{ Error{ "Invalid typename." } };
                }
            }
        }
        for (uint32_t j = 0; j < typeInfo.templateArgsCount; ++j) {
            TemplateArgument ta = typeInfo.arguments[j];
            if (ta.type != TemplateArgument::TemplateValueType::Typename)
                continue;
            if (ta.value.typeValue != TemplateArgument::ValueUnion::TypeName::Custom)
                continue;
            n = std::string{ ta.name };
            if (!templateArgsMap.count(n))
                continue;
            TemplateArgument arg = templateArgsMap[n];
            typeInfo.arguments[j] = arg;
        }
        elements.push_back(std::make_pair(fieldName, typeInfo));
    }

    if (auto t = StructDefinition::Create(mangledName, elements, context); t.has_value()) {
        if (!context->GetScopeManager().PushNamedType(mangledName, *t))
            return std::unexpected{ Error{ std::format("redefinition of `{}`", mangledName) } };
        return *t;
    } else 
        return std::unexpected{ t.error() };
}

std::expected<VCL::Handle<VCL::StructTemplate>, VCL::Error> VCL::StructTemplate::Create(std::string_view name, 
    std::vector<std::pair<std::string_view, TypeInfo>>& structTemplate, 
    std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters, ModuleContext* context) {
    return MakeHandle<StructTemplate>(name, structTemplate, templateParameters, context);
}