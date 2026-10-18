#include "optimize/optimize.h"
#include "tests/unit/optimize/IR.h"

#include "util/equal.h"

#include <catch2/catch_all.hpp>

#include <utility>

namespace lsql::opt {

TEST_CASE("Min over constant scalar is folded to constant aggregate") {
    auto input = test::query(
        test::aggregate(
            test::file(), test::min(test::Output, test::integer(5)), test::schema({test::Output})));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(
        test::aggregate(
            test::file(),
            test::constant(test::Output, Value(int64_t(5)), true),
            test::schema({test::Output})));

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

TEST_CASE("Count over non-null constant is folded to count-all") {
    std::vector<ir::Scalar> args;
    args.push_back(test::integer(0));

    auto input = test::query(
        test::aggregate(
            test::file(),
            ir::Aggregate{
                .node =
                    ir::FnCallAggregate{.function = func::CountNonNull{}, .args = std::move(args)},
                .output_field_id = test::Output,
                .value_type = ValueType::Integer,
            },
            test::schema({test::Output})));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(
        test::aggregate(
            test::file(),
            ir::Aggregate{
                .node = ir::FnCallAggregate{.function = func::CountAll{}, .args = {}},
                .output_field_id = test::Output,
                .value_type = ValueType::Integer,
            },
            test::schema({test::Output})));

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
