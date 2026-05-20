#include "tests/exec/op/MockOperation.h"

#include "exec/op/Limit.h"
#include "tests/exec/op/OperationTest.h"

#include <catch2/catch_all.hpp>

namespace lsql::exec {

struct LimitTest : OperationTest {
    void set(int lim) { setOperation(limit(src, lim, binding)); }

    std::shared_ptr<MockOperation> src = std::make_shared<MockOperation>();
};

TEST_CASE_METHOD(LimitTest, "EmptyStream") {
    SECTION("Non-zero limit") {
        set(10);
    }
    SECTION("Zero limit") {
        set(0);
    }

    expect(eof);
    pushFinal(src, eof);
}

TEST_CASE_METHOD(LimitTest, "StreamEndedBeforeLimit") {
    set(10);

    expect(1, 2, 3, 4, 5, eof);
    push(src, 1);
    push(src, 2);
    push(src, 3);
    push(src, 4);
    push(src, 5);
    pushFinal(src, eof);
}

TEST_CASE_METHOD(LimitTest, "LimitExhaustedBeforeStreamEnded") {
    set(3);

    expect(1, 2, 3, eof);
    push(src, 1);
    push(src, 2);
    pushFinal(src, 3);
}

TEST_CASE_METHOD(LimitTest, "StopRequested") {
    set(10);

    expect(1, 2);
    push(src, 1);
    push(src, 2);

    request_stop = true;
    expect(3);  // no eof
    pushFinal(src, 3);
}

TEST_CASE_METHOD(LimitTest, "EOFOnLimitExhaustion") {
    set(2);

    expect(1);
    push(src, 1);
    expect(eof);
    pushFinal(src, eof);
}

}  // namespace lsql::exec
