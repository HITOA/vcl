#include "ModuleVerifier.hpp"


VCL::ModuleVerifier::ModuleVerifier(ModuleVerifierSettings settings) : settings{ settings } {
    
}

VCL::ModuleVerifier::~ModuleVerifier() {

}

void VCL::ModuleVerifier::Verify(ModuleContext* context) {
    std::stringstream sstream{};
    llvm::raw_os_ostream llvmStream{ sstream };
    bool brokenDebugInfo;
    
    if (llvm::verifyModule(*context->GetTSModule().getModuleUnlocked(), &llvmStream, &brokenDebugInfo)) {
        llvmStream.flush();
        throw std::runtime_error{ sstream.str() };
    }
    if (brokenDebugInfo && context->GetLogger())
        if (settings.brokenDebugInformationAsError)
            throw std::runtime_error{ sstream.str() };
        else
            context->GetLogger()->Warning("{}", sstream.str());

    llvm::CallGraph graph{ *context->GetTSModule().getModuleUnlocked() };
    
    if (settings.enableSelectRecursionCheck)
        VerifySelectRecursion(context, graph);
}

std::unique_ptr<VCL::ModuleVerifier> VCL::ModuleVerifier::Create(ModuleVerifierSettings settings) {
    return std::make_unique<ModuleVerifier>(settings);
}

void VCL::ModuleVerifier::VerifySelectRecursion(ModuleContext* context, llvm::CallGraph& graph) {
    for (auto& node : graph) {
        if (node.first == nullptr)
            continue;
        for (auto& bb : *node.first) {
            for (auto& inst : bb) {
                if (auto* selectInst = llvm::dyn_cast<llvm::SelectInst>(&inst)) {
                    llvm::Value* trueExpr = selectInst->getOperand(1);
                    llvm::Value* falseExpr = selectInst->getOperand(2);

                    if (IsExpressionRecursive(trueExpr, node.first, graph) || IsExpressionRecursive(falseExpr, node.first, graph)) {
                        std::string functionName{ node.first->getName() };
                        std::string errormsg = std::format("`select` intrinsic in function `{}` have recursive expression.\n{}", functionName,
                            "select always evaluate both expression wich will likely result in infinite recursion.");
                        if (settings.selectRecursionAsError)
                            throw std::runtime_error{ errormsg };
                        else
                            context->GetLogger()->Warning("{}", errormsg);
                    }
                }
            }
        }
    }
}

bool VCL::ModuleVerifier::IsExpressionRecursive(llvm::Value* value, const llvm::Function* function, llvm::CallGraph& graph) {
    if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(value)) {
        llvm::Function* callee = callInst->getFunction(); 
        for (auto& node : graph) {
            if (node.first != callee)
                continue;
            if (IsRecursive(function, node.second.get()))
                return true;
        }
    }

    if (auto* inst = llvm::dyn_cast<llvm::Instruction>(value)) {
        for (llvm::Value* op : inst->operands()) {
            return IsExpressionRecursive(op, function, graph);
        }
    }

    return false;
}

bool VCL::ModuleVerifier::IsRecursive(const llvm::Function* function, llvm::CallGraphNode* node, int depth) {
    if (depth <= 0)
        return false;
    for (auto v : *node) {
        if (v.second->getFunction() == function)
            return true;
        if (IsRecursive(function, v.second, depth - 1))
            return true;
    }
    return false;
}