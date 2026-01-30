#pragma once

#include <VCL/Core/Attribute.hpp>
#include <VCL/Core/SourceLocation.hpp>
#include <VCL/AST/ConstantValue.hpp>
#include <VCL/AST/ASTContext.hpp>

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/TrailingObjects.h>


namespace VCL {
    class AttributeInstance final : private llvm::TrailingObjects<AttributeInstance, ConstantValue*> {
        friend TrailingObjects;

    public:
        AttributeInstance() = delete;
        AttributeInstance(AttributeDefinition* definition, llvm::ArrayRef<ConstantValue*> args, SourceRange range)
            : definition{ definition }, argsCount{ args.size() }, range{ range }, next{ nullptr } {
            std::uninitialized_copy(args.begin(), args.end(), getTrailingObjects());
        }
        AttributeInstance(const AttributeInstance& other) = delete;
        AttributeInstance(AttributeInstance&& other) = delete;
        ~AttributeInstance() = default;

        AttributeInstance& operator=(const AttributeInstance& other) = delete;
        AttributeInstance& operator=(AttributeInstance&& other) = delete;

        inline AttributeDefinition* GetDefinition() const { return definition; }
        inline llvm::ArrayRef<ConstantValue*> GetArgs() const { return { getTrailingObjects(), argsCount }; }
        inline ConstantValue** GetData() { return getTrailingObjects(); }
        inline size_t GetArgsCount() { return argsCount; }

        inline AttributeInstance* GetNextAttribute() const { return next; }
        inline void SetNextAttribute(AttributeInstance* attribute) { next = attribute; }
        inline void PushAttribute(AttributeInstance* attribute) {
            if (!next) {
                next = attribute;
                return;
            }
            AttributeInstance* current = next;
            while (current->GetNextAttribute() != nullptr)
                current = current->GetNextAttribute();
            current->SetNextAttribute(attribute);
        }

        inline static AttributeInstance* Create(ASTContext& context, AttributeDefinition* definition, llvm::ArrayRef<ConstantValue*> args, SourceRange range) {
            size_t size = totalSizeToAlloc<ConstantValue*>(args.size());
            void* ptr = context.Allocate(sizeof(AttributeInstance) + size);
            return new(ptr) AttributeInstance{ definition, args, range };
        }

    private:
        AttributeDefinition* definition;
        SourceRange range;
        size_t argsCount;

        AttributeInstance* next;
    };

}