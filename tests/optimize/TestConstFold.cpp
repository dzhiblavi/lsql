#include "ir/equal.h"
#include "optimize/optimize.h"
#include "tests/optimize/IR.h"

#include <catch2/catch_all.hpp>

#include <utility>

namespace lsql::opt {

TEST_CASE("Constant binary scalar is folded inside projection") {
    auto input = test::query(
        test::project(
            test::file(),
            test::projector(test::Output, test::add(test::integer(1), test::integer(2))),
            test::schema({test::Output})));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(
        test::project(
            test::file(),
            test::projector(test::Output, test::integer(3)),
            test::schema({test::Output})));

    CHECK(ctx.changes());
    CHECK(ir::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
