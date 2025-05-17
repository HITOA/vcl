#pragma once

#include <VCL/Definition.hpp>
#include <VCL/Error.hpp>
#include <VCL/AST.hpp>

#include "Handle.hpp"
#include "Callable.hpp"

#include <vector>
#include <string>
#include <string_view>
#include <utility>


namespace VCL {
    class ModuleContext;

    class CallableTemplate {
    public:
        CallableTemplate() = delete;
        CallableTemplate(std::string_view name, 
            std::shared_ptr<TypeInfo> returnType,
            std::vector<std::pair<std::string_view, std::shared_ptr<TypeInfo>>> functionArguments,
            std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>> templateParameters,
            std::unique_ptr<ASTNode> body, llvm::DIFile* file, uint32_t line, ModuleContext* context);
        CallableTemplate(const CallableTemplate& value) = default;
        CallableTemplate(CallableTemplate&& value) noexcept = default;
        virtual ~CallableTemplate() = default;

        CallableTemplate& operator=(const CallableTemplate& value) = default;
        CallableTemplate& operator=(CallableTemplate&& value) noexcept = default;

        std::expected<std::string, Error> Mangle(std::vector<std::shared_ptr<TemplateArgument>>& args,
            std::vector<std::shared_ptr<TypeInfo>>& resolvedArguments);
        std::expected<Handle<Callable>, Error> Resolve(std::vector<std::shared_ptr<TemplateArgument>>& args, 
            std::vector<std::shared_ptr<TypeInfo>>& resolvedArguments);

        static std::expected<Handle<CallableTemplate>, Error> Create(std::string_view name, 
            std::shared_ptr<TypeInfo> returnType,
            std::vector<std::pair<std::string_view, std::shared_ptr<TypeInfo>>> functionArguments,
            std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>> templateParameters,
            std::unique_ptr<ASTNode> body, llvm::DIFile* file, uint32_t line, ModuleContext* context);
    private:
        std::string_view name;
        std::shared_ptr<TypeInfo> returnType;
        std::vector<std::pair<std::string_view, std::shared_ptr<TypeInfo>>> functionArguments{};
        std::vector<std::pair<std::string_view, TemplateArgument::TemplateValueType>> templateParameters{};
        std::unique_ptr<ASTNode> body;
        llvm::DIFile* file;
        uint32_t line;
        ModuleContext* context;
    };

}