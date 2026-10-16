#include "optimize/optimize.h"
#include "tests/unit/optimize/IR.h"

#include "util/equal.h"

#include <catch2/catch_all.hpp>

#include <utility>

namespace lsql::opt {

TEST_CASE("Nested projections are collapsed") {
    auto input = test::query(
        test::project(
            test::project(
                test::file(),
                test::projector(test::Output, test::field(test::Timestamp)),
                test::schema({test::Output})),
            test::projector(test::Result, test::field(test::Output)),
            test::schema({test::Result})));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(
        test::project(
            test::file(),
            test::projector(test::Result, test::field(test::Timestamp)),
            test::schema({test::Result})));

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
