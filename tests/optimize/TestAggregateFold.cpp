#include "ir/equal.h"
#include "optimize/optimize.h"
#include "tests/optimize/IR.h"

#include <catch2/catch_all.hpp>

#include <utility>

namespace lsql::opt {

TEST_CASE("Min over constant scalar is folded to constant aggregate") {
    auto input = test::query(
        test::aggregate(
            test::file(),
            test::min(test::Output, test::integer(5)),
            test::fieldSet({test::Output})));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(
        test::aggregate(
            test::file(),
            test::constant(test::Output, Value(int64_t(5)), true),
            test::fieldSet({test::Output})));

    CHECK(ctx.changes());
    CHECK(ir::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
