#pragma once

#include "exec/op/Operation.h"
#include "exec/op/Subscriber.h"
#include "tests/exec/MockRecord.h"

#include <catch2/catch_test_macros.hpp>
#include <queue>

namespace lsql::exec {

struct OperationTest : Subscriber {
    static constexpr int64_t eof = -1;

    ~OperationTest() {
        CAPTURE(expected_values.size());
        CHECK(expected_values.empty());
    }

    void setOperation(OperationPtr o) {
        CHECK(o->minPhase() == 0);
        op = o;
        op->subscribe(0, this, RequiredFields::withFields({"test-value"}));
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

    bool pushImpl(auto op, int64_t value) {
        if (value == eof) {
            return op->push(0, nullptr);
        } else {
            MockRecord record({{"test-value", Value(value)}});
            return op->push(0, &record);
        }
    }

    void push(auto op, int64_t value) { CHECK(pushImpl(op, value)); }
    void pushFinal(auto op, int64_t value) { CHECK_FALSE(pushImpl(op, value)); }

    template <std::convertible_to<int64_t>... Values>
    void expect(Values... values) {
        (expected_values.push(values), ...);
    }

    OperationPtr op;
    bool request_stop = false;
    std::queue<int> expected_values;
};

}  // namespace lsql::exec
