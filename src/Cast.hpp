#pragma once

#include <VCL/Error.hpp>

#include "Value.hpp"

#include <expected>


namespace VCL {

    struct CastResult {
        Handle<Value> lhs;
        Handle<Value> rhs;
    };

    std::expected<CastResult, Error> ArithmeticImplicitCast(Handle<Value> lhs, Handle<Value> rhs);

}