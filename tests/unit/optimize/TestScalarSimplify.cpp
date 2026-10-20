#include "optimize/optimize.h"
#include "optimize/scalar_simplify.h"
#include "tests/unit/optimize/IR.h"

#include "util/equal.h"

#include <catch2/catch_all.hpp>

#include <utility>
#include <vector>

namespace lsql::opt {

static ir::Scalar booleanCall(func::Function function, auto... arguments) {
    std::vector<ir::Scalar> args;
    (args.push_back(std::move(arguments)), ...);

    return {
        .node = ir::FnCallScalar{.function = std::move(function), .args = std::move(args)},
        .value_type = ValueType::Boolean,
    };
}

static void checkScalarSimplification(ir::Scalar input, ir::Scalar expected) {
    auto project = [](ir::Scalar scalar) {
        return test::query(
            test::project(
                test::file(),
                test::projector(test::Output, std::move(scalar)),
                test::schema({test::Output})));
    };

    Context ctx;
    ctx.nextPass();
    auto optimized = scalarSimplify(project(std::move(input)), ctx);
    auto expected_program = project(std::move(expected));

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected_program.statements));
}

TEST_CASE("Conjunction with false folds to false") {
    auto expression = booleanCall(
        func::And{}, test::field(test::Timestamp, ValueType::Boolean), test::boolean(false));

    checkScalarSimplification(std::move(expression), test::boolean(false));
}

TEST_CASE("Disjunction with false folds to its other operand") {
    auto expression = booleanCall(
        func::Or{}, test::boolean(false), test::field(test::Timestamp, ValueType::Boolean));

    checkScalarSimplification(
        std::move(expression), test::field(test::Timestamp, ValueType::Boolean));
}

TEST_CASE("Double boolean negation is removed") {
    auto expression = booleanCall(
        func::BooleanNegate{},
        booleanCall(func::BooleanNegate{}, test::field(test::Timestamp, ValueType::Boolean)));

    checkScalarSimplification(
        std::move(expression), test::field(test::Timestamp, ValueType::Boolean));
}

TEST_CASE("Coalesce with leading nulls and a constant is folded") {
    std::vector<ir::Scalar> args;
    args.push_back(test::null(ValueType::Integer));
    args.push_back(test::integer(7));
    args.push_back(test::field(test::Timestamp));

    auto input = test::query(
        test::project(
            test::file(),
            test::projector(test::Output, test::coalesce(std::move(args), ValueType::Integer)),
            test::schema({test::Output})));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(
        test::project(
            test::file(),
            test::projector(test::Output, test::integer(7)),
            test::schema({test::Output})));

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

TEST_CASE("Cast to the argument type is removed") {
    auto cast = [](ir::Scalar arg) {
        std::vector<ir::Scalar> args;
        args.push_back(std::move(arg));
        return ir::Scalar{
            .node =
                ir::FnCallScalar{
                    .function = func::Cast{.cast_to = ValueType::Integer},
                    .args = std::move(args),
                },
            .value_type = ValueType::Integer,
        };
    };

    auto input = test::query(
        test::project(
            test::file(),
            test::projector(test::Output, cast(cast(test::field(test::Timestamp)))),
            test::schema({test::Output})));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(
        test::project(
            test::file(),
            test::projector(test::Output, cast(test::field(test::Timestamp))),
            test::schema({test::Output})));

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
