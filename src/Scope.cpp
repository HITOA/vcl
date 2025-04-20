#include "Scope.hpp"


VCL::ScopeManager::ScopeManager() : scopes{} {
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
    return std::unexpected(Error{ std::format("Undefined named value \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedValue(std::string_view name, Handle<Value> value) {
    std::string nameStr{ name };
    if (scopes.front().namedValue.count(nameStr))
        return false;
    scopes.front().namedValue.emplace(nameStr, value);
    return true;
}

std::expected<llvm::Type*, VCL::Error> VCL::ScopeManager::GetNamedType(std::string_view name) const {
    std::string nameStr{ name };
    for (const Scope& scope : scopes) {
        if (scope.namedType.count(nameStr))
            return scope.namedType.at(nameStr);
    }
    return std::unexpected(Error{ std::format("Undefined named type \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedType(std::string_view name, llvm::Type* type) {
    std::string nameStr{ name };
    if (scopes.front().namedType.count(nameStr))
        return false;
    scopes.front().namedType.emplace(nameStr, type);
    return true;
}

llvm::BasicBlock* VCL::ScopeManager::GetTransferControlBasicBlock() const {
    for (const Scope& scope : scopes) {
        if (scope.bb != nullptr)
            return scope.bb;
    }
    return nullptr;
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