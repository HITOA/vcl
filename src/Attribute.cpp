#include <VCL/Attribute.hpp>



size_t VCL::Attribute::Hasher::operator()(const Attribute& attribute) const {
    return std::hash<std::string>()(attribute.name);
}

bool VCL::Attribute::operator==(const Attribute& attribute) const {
    return attribute.name == name;
}

bool VCL::AttributeSet::HasAttributeByName(std::string_view name) {
    for (auto& attribute : *this) {
        if (attribute.name == name)
            return true;
    }
    return false;
}