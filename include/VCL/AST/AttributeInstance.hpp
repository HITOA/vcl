#pragma once

#include <VCL/Core/Attribute.hpp>
#include <VCL/Core/SourceLocation.hpp>
#include <VCL/AST/ConstantValue.hpp>
#include <VCL/AST/ASTContext.hpp>

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/TrailingObjects.h>


namespace VCL {
    class AttributeInstance final : private llvm::TrailingObjects<AttributeInstance, ConstantValue*, IdentifierInfo*> {
        friend TrailingObjects;

    public:
        AttributeInstance() = delete;
        /** `argNames` is empty, or names each argument (nullptr: given by position). */
        AttributeInstance(AttributeDefinition* definition, llvm::ArrayRef<ConstantValue*> args, llvm::ArrayRef<IdentifierInfo*> argNames,
                SourceRange range)
            : definition{ definition }, argsCount{ args.size() }, range{ range }, next{ nullptr } {
            std::uninitialized_copy(args.begin(), args.end(), getTrailingObjects<ConstantValue*>());
            IdentifierInfo** names = getTrailingObjects<IdentifierInfo*>();
            if (argNames.empty())
                std::uninitialized_fill_n(names, args.size(), nullptr);
            else
                std::uninitialized_copy(argNames.begin(), argNames.end(), names);
        }
        AttributeInstance(const AttributeInstance& other) = delete;
        AttributeInstance(AttributeInstance&& other) = delete;
        ~AttributeInstance() = default;

        AttributeInstance& operator=(const AttributeInstance& other) = delete;
        AttributeInstance& operator=(AttributeInstance&& other) = delete;

        inline AttributeDefinition* GetDefinition() const { return definition; }
        inline SourceRange GetSourceRange() const { return range; }
        /** All the arguments, as written: the positional ones first, then the named ones. */
        inline llvm::ArrayRef<ConstantValue*> GetArgs() const { return { getTrailingObjects<ConstantValue*>(), argsCount }; }
        inline ConstantValue** GetData() { return getTrailingObjects<ConstantValue*>(); }
        inline size_t GetArgsCount() { return argsCount; }
        /** The name of each argument of GetArgs, nullptr for a positional one. */
        inline llvm::ArrayRef<IdentifierInfo*> GetArgNames() const { return { getTrailingObjects<IdentifierInfo*>(), argsCount }; }
        /** The arguments given by position. */
        inline llvm::ArrayRef<ConstantValue*> GetPositionalArgs() const {
            llvm::ArrayRef<IdentifierInfo*> names = GetArgNames();
            return GetArgs().take_front(std::find_if(names.begin(), names.end(), [](IdentifierInfo* name) { return name != nullptr; }) - names.begin());
        }
        /** The argument given as `name = value`, or nullptr. */
        inline ConstantValue* GetNamedArg(IdentifierInfo* name) const {
            llvm::ArrayRef<IdentifierInfo*> names = GetArgNames();
            for (size_t i = 0; i < names.size(); ++i)
                if (names[i] != nullptr && names[i] == name)
                    return GetArgs()[i];
            return nullptr;
        }

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

        inline static AttributeInstance* Create(ASTContext& context, AttributeDefinition* definition, llvm::ArrayRef<ConstantValue*> args,
                SourceRange range, llvm::ArrayRef<IdentifierInfo*> argNames = {}) {
            size_t size = totalSizeToAlloc<ConstantValue*, IdentifierInfo*>(args.size(), args.size());
            void* ptr = context.Allocate(size);
            return new(ptr) AttributeInstance{ definition, args, argNames, range };
        }

    private:
        inline size_t numTrailingObjects(OverloadToken<ConstantValue*>) const { return argsCount; }

    private:
        AttributeDefinition* definition;
        SourceRange range;
        size_t argsCount;

        AttributeInstance* next;
    };

}