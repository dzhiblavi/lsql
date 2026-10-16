#include "optimize/optimize.h"
#include "tests/unit/optimize/IR.h"

#include "util/equal.h"

#include <catch2/catch_all.hpp>

namespace lsql::opt {

TEST_CASE("Limit over sort is optimized to top-k") {
    auto input = test::query(test::limit(test::sort(test::file()), 10));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(test::topK(test::file(), 10));

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
