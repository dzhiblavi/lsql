#pragma once

#include <variant>

namespace lsql::ir {

struct FieldScalar;
struct ValueScalar;
struct FnCallScalar;
struct UnaryScalar;
struct BinaryScalar;

using ScalarNode = std::variant< //
    FieldScalar, //
    ValueScalar, //
    FnCallScalar, //
    UnaryScalar, //
    BinaryScalar //
>;

struct Scalar;

}  // namespace lsql::ir
