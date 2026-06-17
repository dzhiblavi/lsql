#pragma once

#include "tests/back/exec/MockRecord.h"

#include "back/exec/phys/Operation.h"
#include "back/exec/phys/Subscriber.h"

#include <catch2/catch_test_macros.hpp>
#include <queue>

namespace lsql::back::exec::phys {

struct OperationTest : Subscriber {
    static constexpr int64_t eof = -1;

    OperationTest() = default;

    ~OperationTest() {
        CAPTURE(expected_values.size());
        CHECK(expected_values.empty());
    }

    void setOperation(Arc<Operation> o) {
        op = o;
        op->output(this);
    }

    // Subscriber
    bool consume(const back::exec::Record* record) override {
        CAPTURE(record ? record->value(0).get<int64_t>() : eof);

        REQUIRE(!expected_values.empty());
        auto val = expected_values.front();
        expected_values.pop();

        if (val == eof) {
            CHECK(record == nullptr);
            return false;
        } else {
            REQUIRE(record != nullptr);
            CHECK(record->value(0).get<int64_t>() == val);
        }

        return !request_stop;
    }

    // Subscriber
    prof::ScopeHandleBase scopeHandle() const override { return {}; }

    bool pushImpl(auto op, int64_t value) {
        if (value == eof) {
            return op->push(nullptr);
        } else {
            MockRecord record({{0, Value(value)}});
            return op->push(&record);
        }
    }

    void push(auto op, int64_t value) { CHECK(pushImpl(op, value)); }
    void pushFinal(auto op, int64_t value) { CHECK_FALSE(pushImpl(op, value)); }

    template <std::convertible_to<int64_t>... Values>
    void expect(Values... values) {
        (expected_values.push(values), ...);
    }

    Arc<Operation> op;
    bool request_stop = false;
    std::queue<int> expected_values;
};

}  // namespace lsql::back::exec::phys
