#pragma once

#include <VCL/AST.hpp>
#include <VCL/Lexer.hpp>

#include <unordered_map>
#include <typeindex>


namespace VCL {
    class Parser;
    class ModuleContext;
    
    class MetaComponent {};

    class MetaState {
    public:
        MetaState() = default;
        ~MetaState() = default;

        template<typename T>
        std::shared_ptr<T> GetOrCreate() {
            std::type_index index = typeid(T);
            if (components.count(index))
                return std::static_pointer_cast<T>(components.at(index));
            std::shared_ptr<T> component = std::make_shared<T>();
            components.insert({ index, component });
            return component;
        }

        static std::shared_ptr<MetaState> Create();

    private:
        std::unordered_map<std::type_index, std::shared_ptr<MetaComponent>> components{};
    };

    class DirectiveHandler {
    public:
        virtual ~DirectiveHandler() = default;

        virtual std::string GetDirectiveName() = 0;
        virtual std::unique_ptr<ASTDirective> Parse(Lexer& lexer, Parser* parser) = 0;
        virtual void Run(ModuleContext* context, ASTDirective* directive, ASTVisitor* visitor) = 0;
    };

    class DirectiveRegistry {
    public:
        DirectiveRegistry() = default;
        ~DirectiveRegistry() = default;
        
        void RegisterDirective(std::shared_ptr<DirectiveHandler> handler);
        std::shared_ptr<DirectiveHandler> GetDirective(std::string name);

        void RegisterDefaultDirectives();

        static std::shared_ptr<DirectiveRegistry> Create();

    private:
        std::unordered_map<std::string, std::shared_ptr<DirectiveHandler>> handlers{};
    };
}