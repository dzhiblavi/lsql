#include "ir/equal.h"
#include "optimize/optimize.h"
#include "optimize/relation_simplify.h"
#include "tests/unit/optimize/IR.h"

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

TEST_CASE("Nested UNION ALL is flattened in one pass preserving order and duplicates") {
    auto input = test::query(
        test::unionAll(
            test::unionAll(test::file("a"), test::unionAll(test::file("b"), test::file("a"))),
            test::file("c"),
            test::unionAll(test::file("d"), test::file("e"))));
    auto expected = test::query(
        test::unionAll(
            test::file("a"),
            test::file("b"),
            test::file("a"),
            test::file("c"),
            test::file("d"),
            test::file("e")));

    Context ctx;
    ctx.nextPass();
    auto optimized = relationSimplify(std::move(input), ctx);

    CHECK(ctx.changes());
    CHECK(ir::equal(optimized.statements, expected.statements));

    ctx.nextPass();
    optimized = relationSimplify(std::move(optimized), ctx);

    CHECK_FALSE(ctx.changes());
    CHECK(ir::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
