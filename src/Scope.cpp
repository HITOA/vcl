#include "Scope.hpp"


VCL::ScopeManager::ScopeManager() {
    PushScope(nullptr); //Push global scope
}

VCL::ScopeManager::~ScopeManager() {
    PopScope();
}

void VCL::ScopeManager::PushScope(llvm::BasicBlock* bb) {
    scopes.emplace_front(bb);
}

void VCL::ScopeManager::PopScope() {
    scopes.pop_front();
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::ScopeManager::GetNamedValue(std::string_view name) const {
    std::string nameStr{ name };
    for (const Scope& scope : scopes) {
        if (scope.namedValue.count(nameStr))
            return scope.namedValue.at(nameStr);
    }
    return std::unexpected(Error{ std::format("Undefine named value \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedValue(std::string_view name, Handle<Value> value) {
    std::string nameStr{ name };
    if (scopes.front().namedValue.count(nameStr))
        return false;
    scopes.front().namedValue.emplace(nameStr, value);
    return true;
}

llvm::BasicBlock* VCL::ScopeManager::GetTransferControlBasicBlock() const {
    return scopes.front().bb;
}

bool VCL::ScopeManager::IsCurrentScopeGlobal() const {
    return scopes.size() == 1;
}

VCL::ScopeGuard::ScopeGuard(ScopeManager* sm, llvm::BasicBlock* bb) : sm{ sm }, released{ false } {
    sm->PushScope(bb);
}

VCL::ScopeGuard::~ScopeGuard() {
    if (!released)
        Release();
}

void VCL::ScopeGuard::Release() {
    sm->PopScope();
    released = true;
}