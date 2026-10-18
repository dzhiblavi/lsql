#pragma once

#include <variant>

namespace lsql::ir {

struct FieldScalar;
struct ValueScalar;
struct FnCallScalar;
struct CoalesceScalar;
struct CachedScalar;

using ScalarNode = std::variant< //
    FieldScalar, //
    ValueScalar, //
    CoalesceScalar, //
    FnCallScalar, //
    CachedScalar //
>;

struct Scalar;

}  // namespace lsql::ir
