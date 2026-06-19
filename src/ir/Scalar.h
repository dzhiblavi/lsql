#pragma once

#include <variant>

namespace lsql::ir {

struct FieldScalar;
struct ValueScalar;
struct FnCallScalar;
struct CoalesceScalar;

using ScalarNode = std::variant< //
    FieldScalar, //
    ValueScalar, //
    CoalesceScalar, //
    FnCallScalar //
>;

struct Scalar;

}  // namespace lsql::ir
