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

bool VCL::ScopeManager::PushNamedValue(std::string_view name, Handle<Value> value, uint32_t offset) {
    std::string nameStr{ name };
    if (scopes[offset].namedValue.count(nameStr))
        return false;
    scopes[offset].namedValue.emplace(nameStr, value);
    return true;
}

std::expected<VCL::Handle<VCL::StructDefinition>, VCL::Error> VCL::ScopeManager::GetNamedType(std::string_view name) const {
    std::string nameStr{ name };
    for (const Scope& scope : scopes) {
        if (scope.namedType.count(nameStr))
            return scope.namedType.at(nameStr);
    }
    return std::unexpected(Error{ std::format("Undefined named type \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedType(std::string_view name, Handle<StructDefinition> type, uint32_t offset) {
    std::string nameStr{ name };
    if (scopes[offset].namedType.count(nameStr))
        return false;
    scopes[offset].namedType.emplace(nameStr, type);
    return true;
}

std::expected<VCL::Handle<VCL::StructTemplate>, VCL::Error> VCL::ScopeManager::GetNamedStructTemplate(std::string_view name) const {
    std::string nameStr{ name };
    for (const Scope& scope : scopes) {
        if (scope.namedStructTemplate.count(nameStr))
            return scope.namedStructTemplate.at(nameStr);
    }
    return std::unexpected(Error{ std::format("Undefined named template \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedStructTemplate(std::string_view name, Handle<StructTemplate> type, uint32_t offset) {
    std::string nameStr{ name };
    if (scopes[offset].namedStructTemplate.count(nameStr))
        return false;
    scopes[offset].namedStructTemplate.emplace(nameStr, type);
    return true;
}

std::expected<VCL::Handle<VCL::CallableTemplate>, VCL::Error> VCL::ScopeManager::GetNamedFunctionTemplate(std::string_view name) const {
    std::string nameStr{ name };
    for (const Scope& scope : scopes) {
        if (scope.namedFunctionTemplate.count(nameStr))
            return scope.namedFunctionTemplate.at(nameStr);
    }
    return std::unexpected(Error{ std::format("Undefined named function template \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedFunctionTemplate(std::string_view name, Handle<CallableTemplate> type, uint32_t offset) {
    std::string nameStr{ name };
    if (scopes[offset].namedFunctionTemplate.count(nameStr))
        return false;
    scopes[offset].namedFunctionTemplate.emplace(nameStr, type);
    return true;
}

std::expected<std::shared_ptr<VCL::TypeInfo>, VCL::Error> VCL::ScopeManager::GetNamedTypeAlias(std::string_view name) const {
    std::string nameStr{ name };
    for (const Scope& scope : scopes) {
        if (scope.namedTypeAlias.count(nameStr))
            return scope.namedTypeAlias.at(nameStr);
    }
    return std::unexpected(Error{ std::format("Undefined named type alias \'{}\'", name) });
}

bool VCL::ScopeManager::PushNamedTypeAlias(std::string_view name, std::shared_ptr<TypeInfo> type, uint32_t offset) {
    std::string nameStr{ name };
    if (scopes[offset].namedTypeAlias.count(nameStr))
        return false;
    scopes[offset].namedTypeAlias.emplace(nameStr, type);
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

uint32_t VCL::ScopeManager::GetNamedStructTemplateOffset(std::string_view name) const {
    std::string nameStr{ name };
    uint32_t i = 0;
    for (const Scope& scope : scopes) {
        if (scope.namedStructTemplate.count(nameStr))
            return i;
        ++i;
    }
    return 0;
}

uint32_t VCL::ScopeManager::GetNamedFunctionTemplateOffset(std::string_view name) const {
    std::string nameStr{ name };
    uint32_t i = 0;
    for (const Scope& scope : scopes) {
        if (scope.namedFunctionTemplate.count(nameStr))
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