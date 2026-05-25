#pragma once

#include <variant>

namespace lsql::ir {

struct FieldScalar;
struct ValueScalar;
struct CoalesceScalar;
struct CastScalar;
struct LikeScalar;
struct RSubstrScalar;
struct UnaryScalar;
struct BinaryScalar;

using ScalarNode = std::variant< //
    FieldScalar,
    ValueScalar, //
    CoalesceScalar, //
    CastScalar, //
    LikeScalar, //
    RSubstrScalar, //
    UnaryScalar, //
    BinaryScalar //
>;

struct Scalar;

}  // namespace lsql::ir
