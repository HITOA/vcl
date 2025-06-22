#include <VCL/ModuleInfo.hpp>



void VCL::ModuleInfo::AddVariable(std::shared_ptr<VariableInfo> info) {
   variables.push_back(info); 
}

void VCL::ModuleInfo::AddFunction(std::shared_ptr<FunctionInfo> info) {
   functions.push_back(info);
}

std::vector<std::shared_ptr<VCL::VariableInfo>>& VCL::ModuleInfo::GetVariables() {
   return variables;
}

std::vector<std::shared_ptr<VCL::FunctionInfo>>& VCL::ModuleInfo::GetFunctions() {
   return functions;
}