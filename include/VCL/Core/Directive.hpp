#pragma once

#include <VCL/Core/Identifier.hpp>

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/TrailingObjects.h>


namespace VCL {
    class DirectiveDecl;
    class Sema;

    class DirectiveHandler {
    public:
        DirectiveHandler() = default;
        DirectiveHandler(const DirectiveHandler& other) = delete;
        DirectiveHandler(DirectiveHandler&& other) = delete;
        ~DirectiveHandler() = default;

        DirectiveHandler& operator=(const DirectiveHandler& other) = delete;
        DirectiveHandler& operator=(DirectiveHandler&& other) = delete;

        virtual bool OnSema(Sema& sema, DirectiveDecl* decl) { return true; }
    };

    class DirectiveRegistry : public llvm::RefCountedBase<DirectiveRegistry> {
    public:
        DirectiveRegistry() = default;
        DirectiveRegistry(const DirectiveRegistry& other) = delete;
        DirectiveRegistry(DirectiveRegistry&& other) = delete;
        ~DirectiveRegistry() = default;

        DirectiveRegistry& operator=(const DirectiveRegistry& other) = delete;
        DirectiveRegistry& operator=(DirectiveRegistry&& other) = delete;

        template<typename T, typename... Args>
        inline T* CreateDirectiveHandler(IdentifierInfo* identifierInfo, Args&&... args) {
            T* handler = directiveAllocator.Allocate<T>();
            new (handler) T{ std::forward<Args>(args)... };
            directives.insert({ identifierInfo, handler });
            return handler;
        }

        inline DirectiveHandler* GetDirectiveHandler(IdentifierInfo* identifierInfo) {
            if (!directives.count(identifierInfo))
                return nullptr;
            return directives.at(identifierInfo);
        }
    
    private:
        llvm::BumpPtrAllocator directiveAllocator{};
        llvm::DenseMap<IdentifierInfo*, DirectiveHandler*> directives{};
    };
}