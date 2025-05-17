#include "StructDefinition.hpp"

#include "ModuleContext.hpp"
#include "Type.hpp"


VCL::StructDefinition::StructDefinition(llvm::StructType* type, llvm::DIType* diType, std::unordered_map<std::string, uint32_t>& fields) :
    type{ type }, diType{ diType }, fields{ fields } {}

llvm::StructType* VCL::StructDefinition::GetType() {
    return type;
}

llvm::DIType* VCL::StructDefinition::GetDIType() {
    return diType;
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
    std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>>& elements, ModuleContext* context) {
    
    std::unordered_map<std::string, uint32_t> fields{};
    std::vector<llvm::Type*> elementsType(elements.size());
    std::vector<llvm::Metadata*> elementsDIType(elements.size());

    for (size_t i = 0; i < elements.size(); ++i) {
        if (elements[i].second->IsExtern())
            return std::unexpected(std::format("Struct cannot contain extern field `{}`.", elements[i].first));
        if (auto t = Type::Create(elements[i].second, context); t.has_value()) {
            fields[elements[i].first] = i;
            elementsType[i] = t->GetLLVMType();
            elementsDIType[i] = t->GetDIType();
        } else {
            return std::unexpected(t.error());
        }
    }

    auto elementsDITypesArray = context->GetDIBuilder().getOrCreateArray(elementsDIType);
    
    llvm::StructType* type = llvm::StructType::create(*context->GetTSContext().getContext(), elementsType);
    type->setName(name);

    uint64_t structSizeInBits = context->GetTSModule().getModuleUnlocked()->getDataLayout().getTypeSizeInBits(type);
    uint64_t structAlignInBits = context->GetTSModule().getModuleUnlocked()->getDataLayout().getABITypeAlign(type).value() * 8;
                
    llvm::DIType* diType = context->GetDIBuilder().createStructType(context->GetScopeManager().GetCurrentDebugInformationScope(),
        name, nullptr, 0, structSizeInBits, structAlignInBits, llvm::DINode::FlagZero, nullptr, elementsDITypesArray);

    return MakeHandle<VCL::StructDefinition>(type, diType, fields);
}
