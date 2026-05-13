#include "tests/exec/MockRecord.h"
#include "tests/exec/op/MockOperation.h"

#include "exec/expr/IdentifierExpression.h"
#include "exec/op/MergeSorted.h"

#include <catch2/catch_all.hpp>

namespace lsql::exec {

struct MergeSortedTest : Subscriber {
    static constexpr int64_t eof = -1;

    MergeSortedTest() {
        op = mergeSorted(
            left,
            right,
            SortList{
                std::make_shared<IdentifierExpression>("test-value"),
            },
            false);

        CHECK(op->minPhase() == 0);
        op->subscribe(0, this);
    }

    ~MergeSortedTest() {
        CAPTURE(expected_values.size());
        CHECK(expected_values.empty());
    }

    bool consume(int /*phase*/, const exec::Record* record) override {
        CAPTURE(record ? record->value("test-value").get<int64_t>() : eof);

        REQUIRE(!expected_values.empty());
        auto val = expected_values.front();
        expected_values.pop();

        if (val == eof) {
            CHECK(record == nullptr);
            return false;
        } else {
            REQUIRE(record != nullptr);
            CHECK(record->value("test-value").get<int64_t>() == val);
        }

        return !request_stop;
    }

    void push(auto op, int64_t value) {
        if (value == eof) {
            op->push(0, nullptr);
        } else {
            MockRecord record({{"test-value", Value(value)}});
            op->push(0, &record);
        }
    }

    template <std::convertible_to<int64_t>... Values>
    void expect(Values... values) {
        (expected_values.push(values), ...);
    }

    std::shared_ptr<MockOperation> left = std::make_shared<MockOperation>();
    std::shared_ptr<MockOperation> right = std::make_shared<MockOperation>();
    OperationPtr op;

    bool request_stop = false;
    std::queue<int> expected_values;
};

TEST_CASE_METHOD(MergeSortedTest, "BothStreamsEmpty") {
    expect(eof);

    SECTION("left, right") {
        push(left, eof);
        push(right, eof);
    }
    SECTION("right, left") {
        push(right, eof);
        push(left, eof);
    }
}

TEST_CASE_METHOD(MergeSortedTest, "OneStreamPushingBuffering") {
    int index = GENERATE(0, 1);
    auto pusher = index == 0 ? left : right;
    auto sleeper = index == 0 ? right : left;

    push(pusher, 1);
    push(pusher, 2);
    push(pusher, 3);
    push(pusher, 4);
    push(pusher, eof);

    expect(1, 2, 3, 4, eof);
    push(sleeper, eof);
}

TEST_CASE_METHOD(MergeSortedTest, "OneStreamPushingStreaming") {
    int index = GENERATE(0, 1);
    auto pusher = index == 0 ? left : right;
    auto sleeper = index == 0 ? right : left;

    push(sleeper, eof);

    expect(1);
    push(pusher, 1);

    expect(2);
    push(pusher, 2);

    expect(3);
    push(pusher, 3);

    expect(4);
    push(pusher, 4);

    expect(eof);
    push(pusher, eof);
}

TEST_CASE_METHOD(MergeSortedTest, "InterleavedEqualKeys") {
    push(left, 1);
    expect(1);
    push(right, 1);

    expect(1);
    push(left, 2);

    expect(2);
    push(right, 2);

    expect(2);
    push(left, eof);
    expect(eof);
    push(right, eof);
}

TEST_CASE_METHOD(MergeSortedTest, "AlternatingInterleaved") {
    push(left, 1);

    expect(1);
    push(right, 2);

    expect(2);
    push(left, 3);

    expect(3);
    push(right, 4);

    expect(4);
    push(left, 5);

    expect(5);
    push(right, 6);

    expect(6);
    push(left, eof);

    expect(eof);
    push(right, eof);
}

TEST_CASE_METHOD(MergeSortedTest, "RightSmallerValues") {
    push(right, 1);
    expect(1);
    push(right, 2);

    expect(2);
    push(right, 3);

    expect(3);
    push(left, 4);

    expect(4);
    push(left, 5);

    expect(5);
    push(left, 6);

    expect(6);
    push(left, eof);
    expect(eof);
    push(right, eof);
}

TEST_CASE_METHOD(MergeSortedTest, "LeftSmallerValues") {
    push(left, 1);
    expect(1);
    push(left, 2);

    expect(2);
    push(left, 3);

    expect(3);
    push(right, 4);

    expect(4);
    push(right, 5);

    expect(5);
    push(right, 6);

    expect(6);
    push(left, eof);
    expect(eof);
    push(right, eof);
}

TEST_CASE_METHOD(MergeSortedTest, "OneStreamEmptyBeforeMerge") {
    push(left, eof);

    expect(1);
    push(right, 1);

    expect(2);
    push(right, 2);

    expect(3);
    push(right, 3);

    expect(eof);
    push(right, eof);
}

TEST_CASE_METHOD(MergeSortedTest, "UnevenBursts") {
    push(left, 1);
    push(left, 3);
    push(left, 5);
    push(left, 7);
    push(left, 9);

    expect(1, 2);
    push(right, 2);

    expect(3, 4);
    push(right, 4);

    expect(5, 6);
    push(right, 6);

    expect(7, 8);
    push(right, 8);

    expect(9);
    push(right, 10);

    expect(10);
    push(left, eof);

    expect(eof);
    push(right, eof);
}

}  // namespace lsql::exec
