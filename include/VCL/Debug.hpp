#pragma once

#include <VCL/Definition.hpp>

#include <string>
#include <memory>


namespace VCL {

    std::string ToString(Operator::ID id);
    std::string ToString(std::shared_ptr<TypeInfo> type);
    std::string ToString(std::shared_ptr<TemplateArgument> argument);

}