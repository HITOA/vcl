#include "StructDefinition.hpp"

#include "ModuleContext.hpp"
#include "Type.hpp"


VCL::StructDefinition::StructDefinition(llvm::StructType* type, std::unordered_map<std::string, uint32_t>& fields) :
    type{ type }, fields{ fields } {}

llvm::StructType* VCL::StructDefinition::GetType() {
    return type;
}

uint32_t VCL::StructDefinition::GetFieldCount() {
    return fields.size();
}

uint32_t VCL::StructDefinition::GetFieldIndex(std::string_view name) {
    std::string str{ name };
    return fields[str];
}

bool VCL::StructDefinition::HasField(std::string_view name) {
    std::string str{ name };
    return fields.count(str); 
}

std::expected<VCL::Handle<VCL::StructDefinition>, VCL::Error> VCL::StructDefinition::Create(std::string_view name, 
    std::vector<std::pair<std::string, TypeInfo>>& elements, ModuleContext* context) {
    
    std::unordered_map<std::string, uint32_t> fields{};
    std::vector<llvm::Type*> elementsType(elements.size());

    for (size_t i = 0; i < elements.size(); ++i) {
        if (auto t = Type::Create(elements[i].second, context); t.has_value()) {
            fields[elements[i].first] = i;
            elementsType[i] = t->GetLLVMType();
        } else {
            return std::unexpected(t.error());
        }
    }
    
    llvm::StructType* type = llvm::StructType::create(*context->GetTSContext().getContext(), elementsType);
    type->setName(name);

    return MakeHandle<VCL::StructDefinition>(type, fields);
}
