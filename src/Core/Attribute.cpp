#include <VCL/Core/Attribute.hpp>



void VCL::AttributeTable::AddDefaults(IdentifierTable& table) {
    AddDefinition(table.Get("EntryPoint"), 0, 0);
    AddDefinition(table.Get("NoMangle"), 0, 0);
    AddDefinition(table.Get("StrictIEEE"), 0, 0);
    AddDefinition(table.Get("AllowApproxFunctions"), 0, 0);
}