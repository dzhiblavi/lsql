#include "ir/equal.h"
#include "optimize/optimize.h"
#include "tests/optimize/IR.h"

#include <catch2/catch_all.hpp>

#include <utility>
#include <vector>

namespace lsql::opt {

TEST_CASE("Coalesce with leading nulls and a constant is folded") {
    std::vector<ir::Scalar> args;
    args.push_back(test::null(ValueType::Integer));
    args.push_back(test::integer(7));
    args.push_back(test::field(test::Timestamp));

    auto input = test::query(
        test::project(
            test::file(),
            test::projector(test::Output, test::coalesce(std::move(args), ValueType::Integer)),
            test::fieldSet({test::Output})));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(
        test::project(
            test::file(),
            test::projector(test::Output, test::integer(7)),
            test::fieldSet({test::Output})));

    CHECK(ctx.changes());
    CHECK(ir::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
