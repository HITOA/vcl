#include <VCL/Meta.hpp>

#include <VCL/Directive.hpp>


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
    RegisterDirective(std::make_shared<MacroDirective>());
    //RegisterDirective(std::make_shared<ConditionalDirective>());
}

std::shared_ptr<VCL::DirectiveRegistry> VCL::DirectiveRegistry::Create() {
    return std::make_shared<DirectiveRegistry>();
}