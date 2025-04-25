#pragma once

#include <llvm/IR/DerivedTypes.h>

#include <unordered_map>
#include <string>


namespace VCL {

    struct StructDefinition {
        llvm::StructType* type;
        std::unordered_map<std::string, uint32_t> fields;
    };

}