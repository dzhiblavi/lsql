#include "ir/equal.h"
#include "optimize/optimize.h"
#include "tests/optimize/IR.h"

#include <catch2/catch_all.hpp>

#include <utility>

namespace lsql::opt {

TEST_CASE("Constant true filter is removed") {
    auto input = test::query(test::filter(test::file(), test::boolean(true)));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(test::file());

    CHECK(ctx.changes());
    CHECK(ir::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
