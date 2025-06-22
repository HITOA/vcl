#pragma once

#include <VCL/Definition.hpp>
#include <VCL/Attribute.hpp>

#include <string>
#include <memory>
#include <vector>


namespace VCL {

    struct VariableInfo {
        std::string name;
        std::shared_ptr<TypeInfo> typeinfo;
        AttributeSet attributes;
    };

    struct FunctionInfo {
        std::string name;
        std::shared_ptr<TypeInfo> returnTypeinfo;
        std::vector<std::shared_ptr<TypeInfo>> argumentsTypeinfo;
        AttributeSet attributes;
    };

    class ModuleInfo {
    public:
        ModuleInfo() = default;

        void AddVariable(std::shared_ptr<VariableInfo> info);
        void AddFunction(std::shared_ptr<FunctionInfo> info);
        
        std::vector<std::shared_ptr<VariableInfo>>& GetVariables();
        std::vector<std::shared_ptr<FunctionInfo>>& GetFunctions();

    private:
        std::vector<std::shared_ptr<VariableInfo>> variables;
        std::vector<std::shared_ptr<FunctionInfo>> functions;
    };

}