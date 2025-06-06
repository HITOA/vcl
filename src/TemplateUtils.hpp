#pragma once

#include <VCL/Error.hpp>
#include <VCL/Definition.hpp>

#include <unordered_map>
#include <string>
#include <optional>
#include <expected>


namespace VCL {

    class TemplateArgumentMapper {
    public:
        TemplateArgumentMapper() = default;
        ~TemplateArgumentMapper() = default;

        inline void SetName(std::string_view name) { this->name = name; }

        std::optional<Error> Map(std::vector<std::shared_ptr<TemplateArgument>>& arguments, 
            std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters);

        std::optional<Error> InferOne(std::unordered_map<std::string, TemplateArgument::TemplateValueType>& templateParameters,
            std::shared_ptr<TypeInfo> templatedArgument, std::shared_ptr<TypeInfo> resolvedArguments);

        std::optional<Error> Infer(std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters,
            std::vector<std::shared_ptr<TypeInfo>>& templatedArguments, std::vector<std::shared_ptr<TypeInfo>>& resolvedArguments);

        std::optional<Error> CheckParameters(std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>>& templateParameters);

        std::expected<std::shared_ptr<TypeInfo>, Error> ResolveType(std::shared_ptr<TypeInfo> typeInfo);
        
        std::string Mangle();

        std::unordered_map<std::string, std::shared_ptr<TemplateArgument>>::iterator begin() { return map.begin(); }
        std::unordered_map<std::string, std::shared_ptr<TemplateArgument>>::iterator end() { return map.end(); }

    private:
        std::unordered_map<std::string, std::shared_ptr<TemplateArgument>> map;
        std::string_view name;
    };

}