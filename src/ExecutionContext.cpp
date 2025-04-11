#include "ExecutionContext.hpp"


VCL::ExecutionContext::ExecutionContext() : context{} {

}

VCL::ExecutionContext::~ExecutionContext() {

}

llvm::LLVMContext& VCL::ExecutionContext::GetContext() {
    return context;
}