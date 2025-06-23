#pragma once

#include <VCL/Definition.hpp>
#include <VCL/Error.hpp>

#include "Handle.hpp"
#include "StructDefinition.hpp"

#include <vector>
#include <string>
#include <string_view>
#include <utility>


namespace VCL {
    class ModuleContext;

    class StructTemplate {
    public:
        StructTemplate() = delete;
        StructTemplate(const std::string& name, std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>>& structTemplate, 
            std::vector<std::pair<std::string, TemplateArgument::TemplateValueType>>& templateParameters, ModuleContext* context);
        StructTemplate(const StructTemplate& value) = default;
        StructTemplate(StructTemplate&& value) noexcept = default;
        virtual ~StructTemplate() = default;

        StructTemplate& operator=(const StructTemplate& value) = default;
        StructTemplate& operator=(StructTemplate&& value) noexcept = default;

        std::string Mangle(std::vector<std::shared_ptr<TemplateArgument>>& args);
        std::expected<Handle<StructDefinition>, Error> Resolve(std::vector<std::shared_ptr<TemplateArgument>>& args);

        static std::expected<Handle<StructTemplate>, Error> Create(const std::string& name, 
            std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>>& structTemplate, 
            std::vector<std::pair<std::string, TemplateArgument::TemplateValueType>>& templateParameters, ModuleContext* context);
    private:
        std::string name;
        std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>> structTemplate{};
        std::vector<std::pair<std::string, TemplateArgument::TemplateValueType>> templateParameters{};
        ModuleContext* context;
    };

}