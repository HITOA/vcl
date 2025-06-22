#pragma once

#include <string>
#include <memory>
#include <vector>
#include <any>
#include <unordered_set>


namespace VCL {
    using AttributeValue = std::any;

    struct Attribute {
        struct Hasher {
            size_t operator()(const Attribute& attribute) const;
        };

        std::string name;
        std::vector<AttributeValue> values;

        bool operator==(const Attribute& attribute) const;
    };

    class AttributeSet : public std::unordered_set<Attribute, Attribute::Hasher> {
    public:
        bool HasAttributeByName(std::string_view name);
    };

}