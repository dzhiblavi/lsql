#include "tests/exec/op/MockOperation.h"
#include "tests/exec/op/OperationTest.h"

#include "exec/expr/IdentifierExpression.h"
#include "exec/op/MergeSorted.h"

#include <catch2/catch_all.hpp>

namespace lsql::exec {

struct MergeSortedTest : OperationTest {
    MergeSortedTest() {
        setOperation(mergeSorted(
            left, right, SortList{std::make_shared<IdentifierExpression>(0)}, false, binding));
    }

    std::shared_ptr<MockOperation> left = std::make_shared<MockOperation>(binding);
    std::shared_ptr<MockOperation> right = std::make_shared<MockOperation>(binding);
};

TEST_CASE_METHOD(MergeSortedTest, "BothStreamsEmpty") {
    expect(eof);

    SECTION("left, right") {
        pushFinal(left, eof);
        pushFinal(right, eof);
    }
    SECTION("right, left") {
        pushFinal(right, eof);
        pushFinal(left, eof);
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
    pushFinal(pusher, eof);

    expect(1, 2, 3, 4, eof);
    pushFinal(sleeper, eof);
}

TEST_CASE_METHOD(MergeSortedTest, "OneStreamPushingStreaming") {
    int index = GENERATE(0, 1);
    auto pusher = index == 0 ? left : right;
    auto sleeper = index == 0 ? right : left;

    pushFinal(sleeper, eof);

    expect(1);
    push(pusher, 1);

    expect(2);
    push(pusher, 2);

    expect(3);
    push(pusher, 3);

    expect(4);
    push(pusher, 4);

    expect(eof);
    pushFinal(pusher, eof);
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
    pushFinal(left, eof);
    expect(eof);
    pushFinal(right, eof);
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
    pushFinal(left, eof);

    expect(eof);
    pushFinal(right, eof);
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
    pushFinal(left, eof);
    expect(eof);
    pushFinal(right, eof);
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
    pushFinal(left, eof);
    expect(eof);
    pushFinal(right, eof);
}

TEST_CASE_METHOD(MergeSortedTest, "OneStreamEmptyBeforeMerge") {
    pushFinal(left, eof);

    expect(1);
    push(right, 1);

    expect(2);
    push(right, 2);

    expect(3);
    push(right, 3);

    expect(eof);
    pushFinal(right, eof);
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
    pushFinal(left, eof);

    expect(eof);
    pushFinal(right, eof);
}

}  // namespace lsql::exec
