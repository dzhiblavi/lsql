#include "tests/exec/op/MockOperation.h"
#include "tests/exec/op/OperationTest.h"

#include "exec/op/UnionAll.h"

#include <catch2/catch_all.hpp>

namespace lsql::exec {

struct UnionAllTest : OperationTest {
    UnionAllTest() { setOperation(unionAll(left, right)); }

    std::shared_ptr<MockOperation> left = std::make_shared<MockOperation>();
    std::shared_ptr<MockOperation> right = std::make_shared<MockOperation>();
};

TEST_CASE_METHOD(UnionAllTest, "BothStreamsEmpty") {
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

TEST_CASE_METHOD(UnionAllTest, "OneStreamOnlyOtherIdling") {
    expect(1);
    push(left, 1);

    expect(2);
    push(left, 2);

    expect(3);
    push(left, 3);

    expect(4);
    push(left, 4);

    pushFinal(left, eof);
    expect(eof);
    pushFinal(right, eof);
}

TEST_CASE_METHOD(UnionAllTest, "OneStreamOnlyOtherDoneImmediately") {
    pushFinal(right, eof);

    expect(1);
    push(left, 1);

    expect(2);
    push(left, 2);

    expect(3);
    push(left, 3);

    expect(4);
    push(left, 4);

    expect(eof);
    pushFinal(left, eof);
}

TEST_CASE_METHOD(UnionAllTest, "TwoNonEmptyStreams") {
    expect(1);
    push(left, 1);

    expect(2);
    push(right, 2);

    pushFinal(left, eof);

    expect(3);
    push(right, 3);

    expect(eof);
    pushFinal(right, eof);
}

TEST_CASE_METHOD(UnionAllTest, "StopRequestedBothActive") {
    expect(1);
    push(left, 1);

    expect(2);
    push(right, 2);

    request_stop = true;
    expect(3);
    pushFinal(left, 3);

    pushFinal(right, 4);  // 4 not emitted
}

TEST_CASE_METHOD(UnionAllTest, "StopRequestedOneActive") {
    expect(1);
    push(left, 1);

    expect(2);
    push(right, 2);

    pushFinal(right, eof);

    request_stop = true;
    expect(3);
    pushFinal(left, 3);
}

TEST_CASE_METHOD(UnionAllTest, "StopRequestedBothDone") {
    expect(1);
    push(left, 1);

    expect(2);
    push(right, 2);

    pushFinal(right, eof);

    request_stop = true;
    expect(eof);
    pushFinal(left, eof);
}

}  // namespace lsql::exec
