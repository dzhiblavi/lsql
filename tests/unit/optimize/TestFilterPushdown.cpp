#include "optimize/filter_pushdown.h"
#include "tests/unit/optimize/IR.h"

#include "util/equal.h"

#include <catch2/catch_all.hpp>

#include <utility>
#include <vector>

namespace lsql::opt {

TEST_CASE("Filter is pushed through field projection into UNION ALL inputs") {
    auto input = test::query(
        test::filter(
            test::project(
                test::unionAll(test::file("a"), test::file("b")),
                test::projector(test::Output, test::field(test::Timestamp, ValueType::Boolean)),
                test::schema({test::Output})),
            test::field(test::Output, ValueType::Boolean)));
    auto expected = test::query(
        test::project(
            test::unionAll(
                test::filter(test::file("a"), test::field(test::Timestamp, ValueType::Boolean)),
                test::filter(test::file("b"), test::field(test::Timestamp, ValueType::Boolean))),
            test::projector(test::Output, test::field(test::Timestamp, ValueType::Boolean)),
            test::schema({test::Output})));

    Context ctx;
    ctx.nextPass();
    auto optimized = filterPushdown(std::move(input), ctx);

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

TEST_CASE("Filter conjunction is split across a mixed projection") {
    auto project = [](ir::Relation source) {
        std::vector<ir::Projector> projectors;
        projectors.push_back(
            test::projector(test::Output, test::field(test::Timestamp, ValueType::Boolean)));
        projectors.push_back(test::projector(test::Result, test::boolean(true)));
        return test::project(
            std::move(source), std::move(projectors), test::schema({test::Output, test::Result}));
    };

    std::vector<ir::Scalar> args;
    args.push_back(test::field(test::Output, ValueType::Boolean));
    args.push_back(test::field(test::Result, ValueType::Boolean));
    auto input = test::query(
        test::filter(
            project(test::file()),
            ir::Scalar{
                .node = ir::FnCallScalar{.function = func::And{}, .args = std::move(args)},
                .value_type = ValueType::Boolean,
            }));
    auto expected = test::query(
        test::filter(
            project(test::filter(test::file(), test::field(test::Timestamp, ValueType::Boolean))),
            test::field(test::Result, ValueType::Boolean)));

    Context ctx;
    ctx.nextPass();
    auto optimized = filterPushdown(std::move(input), ctx);

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

TEST_CASE("Filter is pushed into the left input of LEFT JOIN") {
    auto join = [](ir::Relation left) {
        auto schema = Schema::concat(left.schema, test::schema({test::Output}));
        return ir::Relation{
            .node =
                ir::LeftJoinRelation{
                    .left = box(std::move(left)),
                    .right = box(test::file("right", test::schema({test::Output}))),
                    .left_key = box(test::field(test::Timestamp, ValueType::Boolean)),
                    .right_key = box(test::field(test::Output, ValueType::Boolean)),
                },
            .schema = std::move(schema),
        };
    };
    auto input = test::query(
        test::filter(join(test::file()), test::field(test::Timestamp, ValueType::Boolean)));
    auto expected = test::query(
        join(test::filter(test::file(), test::field(test::Timestamp, ValueType::Boolean))));

    Context ctx;
    ctx.nextPass();
    auto optimized = filterPushdown(std::move(input), ctx);

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

TEST_CASE("Filter is pushed through sort") {
    auto input = test::query(
        test::filter(test::sort(test::file()), test::field(test::Timestamp, ValueType::Boolean)));
    auto expected = test::query(
        test::sort(test::filter(test::file(), test::field(test::Timestamp, ValueType::Boolean))));

    Context ctx;
    ctx.nextPass();
    auto optimized = filterPushdown(std::move(input), ctx);

    CHECK(ctx.changes());
    CHECK(util::equal(optimized.statements, expected.statements));
}

}  // namespace lsql::opt
