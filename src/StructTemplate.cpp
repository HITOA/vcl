#include "StructTemplate.hpp"

#include <VCL/Debug.hpp>

#include "ModuleContext.hpp"
#include "TemplateUtils.hpp"

#include <iostream>


VCL::StructTemplate::StructTemplate(std::string_view name, std::vector<std::pair<std::string_view, std::shared_ptr<TypeInfo>>>& structTemplate, 
    std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters, ModuleContext* context) :
    name{ name }, structTemplate{ structTemplate }, templateParameters{ templateParameters }, context{ context } {}

std::string VCL::StructTemplate::Mangle(std::vector<std::shared_ptr<TemplateArgument>>& args) {
    std::string mangledName{ name };

    for (size_t i = 0; i < args.size(); ++i)
        mangledName += "_" + ToString(args[i]);

    return mangledName;
}

std::expected<VCL::Handle<VCL::StructDefinition>, VCL::Error> VCL::StructTemplate::Resolve(std::vector<std::shared_ptr<TemplateArgument>>& args) {
    std::string mangledName = Mangle(args);

    TemplateArgumentMapper mapper{};
    mapper.SetName(name);
    if (auto err = mapper.Map(args, templateParameters))
        return std::unexpected{ err.value() };

    if (auto err = mapper.CheckParameters(templateParameters))
        return std::unexpected{ err.value() };
    
    std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>> elements{};

    for (size_t i = 0; i < structTemplate.size(); ++i) {
        std::string fieldName{ structTemplate[i].first };
        std::shared_ptr<TypeInfo> typeInfo = structTemplate[i].second;
        if (auto r = mapper.ResolveType(typeInfo); r.has_value())
            typeInfo = *r;
        else
            return std::unexpected{ r.error() };
        elements.push_back(std::make_pair(fieldName, typeInfo));
    }

    if (auto t = StructDefinition::Create(mangledName, elements, context); t.has_value()) {
        uint32_t offset = context->GetScopeManager().GetNamedStructTemplateOffset(name);
        if (!context->GetScopeManager().PushNamedType(mangledName, *t, offset))
            return std::unexpected{ Error{ std::format("redefinition of `{}`", mangledName) } };
        return *t;
    } else 
        return std::unexpected{ t.error() };
}

std::expected<VCL::Handle<VCL::StructTemplate>, VCL::Error> VCL::StructTemplate::Create(std::string_view name, 
    std::vector<std::pair<std::string_view, std::shared_ptr<TypeInfo>>>& structTemplate, 
    std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters, ModuleContext* context) {
    return MakeHandle<StructTemplate>(name, structTemplate, templateParameters, context);
}