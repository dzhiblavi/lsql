#include "optimize/filter_pushdown.h"
#include "tests/unit/optimize/IR.h"

#include "util/equal.h"

#include <catch2/catch_all.hpp>

#include <utility>

namespace lsql::opt {

TEST_CASE("Filter is pushed into UNION ALL inputs") {
    auto input = test::query(
        test::filter(
            test::unionAll(test::file("a"), test::file("b")),
            test::field(test::Timestamp, ValueType::Boolean)));
    auto expected = test::query(
        test::unionAll(
            test::filter(test::file("a"), test::field(test::Timestamp, ValueType::Boolean)),
            test::filter(test::file("b"), test::field(test::Timestamp, ValueType::Boolean))));

    Context ctx;
    ctx.nextPass();
    auto optimized = filterPushdown(std::move(input), ctx);

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
