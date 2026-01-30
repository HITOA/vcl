#pragma once

#include <VCL/Core/Identifier.hpp>

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/TrailingObjects.h>


namespace VCL {

    class AttributeDefinition {
    public: 
        AttributeDefinition(IdentifierInfo* identifierInfo, uint32_t minArgs, uint32_t maxArgs)
            : identifierInfo{ identifierInfo }, minArgs{ minArgs }, maxArgs{ maxArgs } {}    
        AttributeDefinition() = delete;
        AttributeDefinition(const AttributeDefinition& other) = delete;
        AttributeDefinition(AttributeDefinition&& other) = delete;
        ~AttributeDefinition() = default;

        AttributeDefinition& operator=(const AttributeDefinition& other) = delete;
        AttributeDefinition& operator=(AttributeDefinition&& other) = delete;
        
        inline IdentifierInfo* GetIdentifierInfo() { return identifierInfo; }
        inline uint32_t GetMinArgs() const { return minArgs; }
        inline uint32_t GetMaxArgs() const { return maxArgs; }

    private:
        IdentifierInfo* identifierInfo;
        uint32_t minArgs;
        uint32_t maxArgs;

        friend class AttributeTable;
    };

    class AttributeTable : public llvm::RefCountedBase<AttributeTable> {
    public:
        AttributeTable() = default;
        AttributeTable(const AttributeTable& other) = delete;
        AttributeTable(AttributeTable&& other) = delete;
        ~AttributeTable() = default;

        AttributeTable& operator=(const AttributeTable& other) = delete;
        AttributeTable& operator=(AttributeTable&& other) = delete;

        inline AttributeDefinition* GetDefinition(IdentifierInfo* identifierInfo) {
            if (attributes.count(identifierInfo))
                return attributes[identifierInfo];
            return nullptr;
        }

        inline AttributeDefinition* AddDefinition(IdentifierInfo* identifierInfo, uint32_t minArgs, uint32_t maxArgs) {
            if (attributes.count(identifierInfo))
                return nullptr;
            void* attributeDefinition = attributeDefinitionAllocator.Allocate<AttributeDefinition>();
            new (attributeDefinition) AttributeDefinition{ identifierInfo, minArgs, maxArgs };
            attributes.insert({ identifierInfo, (AttributeDefinition*)attributeDefinition });
            return (AttributeDefinition*)attributeDefinition;
        }

        void AddDefaults(IdentifierTable& table);

    private:
        llvm::BumpPtrAllocator attributeDefinitionAllocator{};
        llvm::DenseMap<IdentifierInfo*, AttributeDefinition*> attributes{};
    };

}