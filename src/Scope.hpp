#pragma once

#include <VCL/Error.hpp>

#include "Value.hpp"

#include <deque>
#include <unordered_map>
#include <string>
#include <expected>


namespace VCL {

    /**
     * @brief Scope data structure
     */
    struct Scope {
        llvm::BasicBlock* bb;
        std::unordered_map<std::string, Handle<Value>> namedValue;
        std::unordered_map<std::string, llvm::Type*> namedType;

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
        std::expected<Handle<Value>, Error> GetNamedValue(std::string_view name) const;
        
        /**
         * @brief Add a named value to the current scope.
         *
         * @return true on success or false if a value already exist with this name in the current scope.
         */
        bool PushNamedValue(std::string_view name, Handle<Value> value);

        /**
         * @brief Get a named type by name from the current or upper scope.
         * 
         * @return Either the correct type or an error if the name doesn't match any.
         */
        std::expected<llvm::Type*, Error> GetNamedType(std::string_view name) const;

        /**
         * @brief Add a named type to the current scope.
         * 
         * @return true on success or false if a type already exist with this name in the current scope.
         */
        bool PushNamedType(std::string_view name, llvm::Type* type);

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