#include "ir/equal.h"
#include "optimize/optimize.h"
#include "tests/optimize/IR.h"

#include <catch2/catch_all.hpp>

#include <utility>

namespace lsql::opt {

TEST_CASE("Projection over empty source is pruned to empty relation") {
    auto fields = test::schema({test::Output});
    auto input = test::query(
        test::project(
            test::empty(Schema::withField(test::Timestamp)),
            test::projector(test::Output, test::field(test::Timestamp)),
            fields));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(test::empty(fields));

    CHECK(ctx.changes());
    CHECK(ir::equal(optimized.statements, expected.statements));
}

TEST_CASE("Zero limit is pruned to empty relation") {
    auto fields = Schema::withField(test::Timestamp);
    auto input = test::query(test::limit(test::file(), 0));

    Context ctx;
    auto optimized = optimize(std::move(input), ctx);

    auto expected = test::query(test::empty(fields));

    CHECK(ctx.changes());
    CHECK(ir::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
