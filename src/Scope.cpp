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

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::ScopeManager::GetNamedValue(const std::string& name) const {
    for (const Scope& scope : scopes) {
        if (scope.namedValue.count(name))
            return scope.namedValue.at(name);
    }
    return std::unexpected(Error{ std::format("Undefined named value \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedValue(const std::string& name, Handle<Value> value, uint32_t offset) {
    if (scopes[offset].namedValue.count(name))
        return false;
    scopes[offset].namedValue.emplace(name, value);
    return true;
}

std::expected<VCL::Handle<VCL::StructDefinition>, VCL::Error> VCL::ScopeManager::GetNamedType(const std::string& name) const {
    for (const Scope& scope : scopes) {
        if (scope.namedType.count(name))
            return scope.namedType.at(name);
    }
    return std::unexpected(Error{ std::format("Undefined named type \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedType(const std::string& name, Handle<StructDefinition> type, uint32_t offset) {
    if (scopes[offset].namedType.count(name))
        return false;
    scopes[offset].namedType.emplace(name, type);
    return true;
}

std::expected<VCL::Handle<VCL::StructTemplate>, VCL::Error> VCL::ScopeManager::GetNamedStructTemplate(const std::string& name) const {
    for (const Scope& scope : scopes) {
        if (scope.namedStructTemplate.count(name))
            return scope.namedStructTemplate.at(name);
    }
    return std::unexpected(Error{ std::format("Undefined named template \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedStructTemplate(const std::string& name, Handle<StructTemplate> type, uint32_t offset) {
    if (scopes[offset].namedStructTemplate.count(name))
        return false;
    scopes[offset].namedStructTemplate.emplace(name, type);
    return true;
}

std::expected<VCL::Handle<VCL::CallableTemplate>, VCL::Error> VCL::ScopeManager::GetNamedFunctionTemplate(const std::string& name) const {
    for (const Scope& scope : scopes) {
        if (scope.namedFunctionTemplate.count(name))
            return scope.namedFunctionTemplate.at(name);
    }
    return std::unexpected(Error{ std::format("Undefined named function template \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedFunctionTemplate(const std::string& name, Handle<CallableTemplate> type, uint32_t offset) {
    if (scopes[offset].namedFunctionTemplate.count(name))
        return false;
    scopes[offset].namedFunctionTemplate.emplace(name, type);
    return true;
}

std::expected<std::shared_ptr<VCL::TypeInfo>, VCL::Error> VCL::ScopeManager::GetNamedTypeAlias(const std::string& name) const {
    for (const Scope& scope : scopes) {
        if (scope.namedTypeAlias.count(name))
            return scope.namedTypeAlias.at(name);
    }
    return std::unexpected(Error{ std::format("Undefined named type alias \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedTypeAlias(const std::string& name, std::shared_ptr<TypeInfo> type, uint32_t offset) {
    if (scopes[offset].namedTypeAlias.count(name))
        return false;
    scopes[offset].namedTypeAlias.emplace(name, type);
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

void VCL::ScopeManager::SetCurrentDebugInformationScope(llvm::DIScope* scope) {
    scopes[0].diScope = scope;
}

llvm::DIScope* VCL::ScopeManager::GetCurrentDebugInformationScope() {
    for (const Scope& scope : scopes) {
        if (scope.diScope)
            return scope.diScope;
    }
    return nullptr;
}

uint32_t VCL::ScopeManager::GetNamedStructTemplateOffset(const std::string& name) const {
    uint32_t i = 0;
    for (const Scope& scope : scopes) {
        if (scope.namedStructTemplate.count(name))
            return i;
        ++i;
    }
    return 0;
}

uint32_t VCL::ScopeManager::GetNamedFunctionTemplateOffset(const std::string& name) const {
    uint32_t i = 0;
    for (const Scope& scope : scopes) {
        if (scope.namedFunctionTemplate.count(name))
            return i;
        ++i;
    }
    return 0;
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