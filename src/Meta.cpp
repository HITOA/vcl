#include <VCL/Meta.hpp>

#include <VCL/Directive.hpp>


void VCL::MetaState::CopyTo(std::shared_ptr<MetaState> state) {
    for (auto& entry : components) {
        if (state->components.count(entry.first)) {
            entry.second->CopyTo(state->components[entry.first]);
        } else {
            state->components[entry.first] = entry.second->Clone();
        }
    }
}

std::shared_ptr<VCL::MetaState> VCL::MetaState::Clone() {
    std::shared_ptr<VCL::MetaState> state = VCL::MetaState::Create();
    CopyTo(state);
    return state;
}

std::shared_ptr<VCL::MetaState> VCL::MetaState::Create() {
    return std::make_shared<MetaState>();
}

void VCL::DirectiveRegistry::RegisterDirective(std::shared_ptr<DirectiveHandler> handler) {
    handlers[handler->GetDirectiveName()] = handler;
}

std::shared_ptr<VCL::DirectiveHandler> VCL::DirectiveRegistry::GetDirective(std::string name) {
    if (handlers.count(name))
        return handlers[name];
    return nullptr;
}

void VCL::DirectiveRegistry::RegisterDefaultDirectives() {
    RegisterDirective(std::make_shared<ImportDirective>());
    RegisterDirective(std::make_shared<DefineDirective>());
    RegisterDirective(std::make_shared<ConditionalDirective>());
}

std::shared_ptr<VCL::DirectiveRegistry> VCL::DirectiveRegistry::Create() {
    return std::make_shared<DirectiveRegistry>();
}