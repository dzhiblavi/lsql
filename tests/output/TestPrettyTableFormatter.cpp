#include "output/PrettyTableFormatter.h"

#include <catch2/catch_all.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace lsql::output {

namespace {

struct StringSink {
    void push(std::string_view value) { lines.emplace_back(value); }
    void done() { done_called = true; }

    std::vector<std::string> lines;
    bool done_called = false;
};

}  // namespace

TEST_CASE("Pretty table escapes controls and aligns UTF-8 values") {
    auto binding = std::make_shared<FieldBinding>();
    auto title = binding->add("title", ValueType::String);
    auto count = binding->add("count", ValueType::Integer);

    StringSink sink;
    PrettyTableFormatter formatter(&sink, binding);

    Record first = {{title, std::string("Hello\nworld")}, {count, int64_t(42)}};
    formatter.consume(first);

    Record second = {{title, std::string("Привет")}, {count, vnull}};
    formatter.consume(second);
    formatter.done();

    CHECK(
        sink.lines ==
        std::vector<std::string>{
            "title         count",
            "------------  -----",
            "Hello\\nworld  42   ",
            "Привет        null ",
        });
    CHECK(sink.done_called);
}

}  // namespace lsql::output
