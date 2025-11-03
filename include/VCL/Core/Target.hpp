#pragma once

#include <VCL/Core/TargetOptions.hpp>

#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>

#include <cstdint>
#include <expected>


namespace VCL {

    class Target : public llvm::RefCountedBase<Target> {
    public:
        Target();
        Target(std::unique_ptr<llvm::TargetMachine> tm);
        Target(TargetOptions& options);
        Target(const Target& other) = delete;
        Target(Target&& other) = delete;
        ~Target() = default;

        Target& operator=(const Target& other) = delete;
        Target& operator=(Target&& other) = delete;

        inline llvm::TargetMachine* GetTargetMachine() { return tm.get(); }

        /** Always the same number of element in a vector, let llvm do its magic while lowering */
        inline uint32_t GetVectorWidthInElement() const { return vectorWidthInByte / 4; }

    private:
        void CacheTargetMetadata();
    
    private:
        std::unique_ptr<llvm::TargetMachine> tm;
        uint32_t vectorWidthInByte;
    };

}