#pragma once

#include <VCL/Error.hpp>

#include "Value.hpp"
#include "StructDefinition.hpp"
#include "StructTemplate.hpp"
#include "CallableTemplate.hpp"

#include <llvm/IR/DIBuilder.h>

#include <deque>
#include <unordered_map>
#include <string>
#include <expected>
#include <optional>


namespace VCL {

    /**
     * @brief Scope data structure
     */
    struct Scope {
        llvm::BasicBlock* bb = nullptr;
        llvm::DIScope* diScope = nullptr;
        std::unordered_map<std::string, Handle<Value>> namedValue;
        std::unordered_map<std::string, Handle<StructDefinition>> namedType;
        std::unordered_map<std::string, Handle<StructTemplate>> namedStructTemplate;
        std::unordered_map<std::string, Handle<CallableTemplate>> namedFunctionTemplate;
        std::unordered_map<std::string, std::shared_ptr<TypeInfo>> namedTypeAlias;

        Scope() = delete;
        Scope(llvm::BasicBlock* bb) : bb{ bb }, namedValue{} {};
    };

    /**
     * @brief Main class for managing scope and symbol table.
     */
    class ScopeManager {
    public:
        ScopeManager();
        ~ScopeManager();

        /**
         * @brief Push a new scope into the scopes stack.
         *
         * @param bb Given a non nullptr BasicBlock this specify where the control will continue after transfer.
         */
        void PushScope(llvm::BasicBlock* bb = nullptr);

        /**
         * @brief Pop the top scope of the scopes stack.
         */
        void PopScope();

        /**
         * @brief Get a named value by name from the current or upper scope.
         *
         * @return Either the correct value or an error if the name doesn't match any.
         */
        std::expected<Handle<Value>, Error> GetNamedValue(const std::string& name) const;
        
        /**
         * @brief Add a named value to the current scope.
         *
         * @return true on success or false if a value already exist with this name in the current scope.
         */
        bool PushNamedValue(const std::string& name, Handle<Value> value, uint32_t offset = 0);

        /**
         * @brief Get a named type by name from the current or upper scope.
         * 
         * @return Either the correct type or an error if the name doesn't match any.
         */
        std::expected<Handle<StructDefinition>, Error> GetNamedType(const std::string& name) const;

        /**
         * @brief Add a named type to the current scope.
         * 
         * @return true on success or false if a type already exist with this name in the current scope.
         */
        bool PushNamedType(const std::string& name, Handle<StructDefinition> type, uint32_t offset = 0);


        /**
         * @brief Get a named template by name from the current or upper scope.
         * 
         * @return Either the correct template or an error if the name doesn't match any.
         */
        std::expected<Handle<StructTemplate>, Error> GetNamedStructTemplate(const std::string& name) const;

        /**
         * @brief Add a named template to the current scope.
         * 
         * @return true on success or false if a template already exist with this name in the current scope.
         */
        bool PushNamedStructTemplate(const std::string& name, Handle<StructTemplate> type, uint32_t offset = 0);

        /**
         * @brief Get a named function template by name from the current or upper scope.
         * 
         * @return Either the correct function template or an error if the name doesn't match any.
         */
        std::expected<Handle<CallableTemplate>, Error> GetNamedFunctionTemplate(const std::string& name) const;

        /**
         * @brief Add a named function template to the current scope.
         * 
         * @return true on success or false if a function template already exist with this name in the current scope.
         */
        bool PushNamedFunctionTemplate(const std::string& name, Handle<CallableTemplate> type, uint32_t offset = 0);

        /**
         * @brief Get a named type alias by name from the current or upper scope.
         * 
         * @return Either the correct type alias or an error if the name doesn't match any.
         */
        std::expected<std::shared_ptr<TypeInfo>, Error> GetNamedTypeAlias(const std::string& name) const;

        /**
         * @brief Add a named type alias to the current scope.
         * 
         * @return true on success or false if a type alias already exist with this name in the current scope.
         */
        bool PushNamedTypeAlias(const std::string& name, std::shared_ptr<TypeInfo> type, uint32_t offset = 0);

        /**
         * @brief Get BasicBlock for transfer control if any exist.
         *
         * @return A ptr to a BasicBlock or nullptr if none exist.
         */
        llvm::BasicBlock* GetTransferControlBasicBlock() const;

        /**
         * @brief Check if the current scope is global.
         * 
         * @return true if the scope stack size is one. false otherwise.
         */
        bool IsCurrentScopeGlobal() const;

        void SetCurrentDebugInformationScope(llvm::DIScope* scope);
        llvm::DIScope* GetCurrentDebugInformationScope();

        uint32_t GetNamedStructTemplateOffset(const std::string& name) const;
        uint32_t GetNamedFunctionTemplateOffset(const std::string& name) const;

    private:
        /**
         * @brief The scope stack. It can't contain less than one scope wich represent the global scope.
         */
        std::deque<Scope> scopes;
    };

    /**
     * @brief Utility class for pushing and popping scope with RAII.
     */
    class ScopeGuard {
    public:
        ScopeGuard() = delete;
        ScopeGuard(ScopeManager* sm, llvm::BasicBlock* bb = nullptr);
        ~ScopeGuard();

        /**
         * @brief Pop the scope that this class is guarding.
         */
        void Release();

    private:
        ScopeManager* sm;
        bool released;
    };

}