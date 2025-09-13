#pragma once

#include <VCL/Core/TokenKind.hpp>

#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringMapEntry.h>
#include <llvm/Support/Allocator.h>


namespace VCL {

    class IdentifierInfo {
    public:
        IdentifierInfo() = default;
        IdentifierInfo(const IdentifierInfo& other) = delete;
        IdentifierInfo(IdentifierInfo&& other) = delete;
        ~IdentifierInfo() = default;

        IdentifierInfo& operator=(const IdentifierInfo& other) = delete;
        IdentifierInfo& operator=(IdentifierInfo&& other) = delete;

        inline llvm::StringRef GetName() const { return entry->getKey(); }
        inline bool IsKeyword() const { return isKeyword; }
        inline TokenKind GetTokenKind() const { return kind; }

        
    private:
        llvm::StringMapEntry<IdentifierInfo*>* entry = nullptr;
        bool isKeyword = false;
        TokenKind kind = TokenKind::Unknown;

        friend class IdentifierTable;
    };

    class IdentifierTable {
    public:
        IdentifierTable() = default;
        IdentifierTable(const IdentifierTable& other) = delete;
        IdentifierTable(IdentifierTable&& other) = delete;
        ~IdentifierTable() = default;

        IdentifierTable& operator=(const IdentifierTable& other) = delete;
        IdentifierTable& operator=(IdentifierTable&& other) = delete;

        IdentifierInfo* Get(llvm::StringRef name) {
            llvm::StringMapEntry<IdentifierInfo*>& entry = *identifiers.try_emplace(name, nullptr).first;

            IdentifierInfo*& info = entry.second;
            if (info)
                return info;

            void* mem = identifiers.getAllocator().Allocate<IdentifierInfo>();
            info = new (mem) IdentifierInfo{};

            info->entry = &entry;

            return info;
        }

        IdentifierInfo* GetKeyword(TokenKind kind);
        void AddKeyword(llvm::StringRef name, TokenKind kind);
        void AddKeywords();

    private:
        llvm::StringMap<IdentifierInfo*, llvm::BumpPtrAllocator> identifiers{};
    };

}