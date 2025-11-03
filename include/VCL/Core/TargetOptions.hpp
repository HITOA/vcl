#pragma once

#include <llvm/TargetParser/Host.h>
#include <llvm/ADT/StringRef.h>

#include <string>


namespace VCL {

    class TargetOptions {
    public:
        TargetOptions();
        TargetOptions(const TargetOptions& other) = default;
        TargetOptions(TargetOptions&& other) = default;
        ~TargetOptions() = default;
        
        TargetOptions& operator=(const TargetOptions& other) = default;
        TargetOptions& operator=(TargetOptions&& other) = default;

        inline llvm::StringRef GetTriple() { return triple; }
        inline void SetTriple(std::string& triple) { this->triple = triple; }

        inline llvm::StringRef GetCPU() { return cpu; }
        inline void SetCPU(std::string& cpu) { this->cpu = cpu; }

        inline llvm::StringRef GetFeatures() { return features; }
        inline void SetFeatures(std::string& features) { this->features = features; }

    private:
        std::string triple;
        std::string cpu;
        std::string features;
    };

}