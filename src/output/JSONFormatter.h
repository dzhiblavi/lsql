#pragma once

#include "output/Consumer.h"
#include "output/Sink.h"

#include "core/schema/FieldBinding.h"

namespace lsql::output {

inline std::string escapeForJSON(std::string_view input) {
    std::ostringstream oss;

    for (char c : input) {
        switch (c) {
            case '"':
                oss << "\\\"";
                break;
            case '\\':
                oss << "\\\\";
                break;
            case '/':
                oss << "\\/";
                break;
            case '\b':
                oss << "\\b";
                break;
            case '\f':
                oss << "\\f";
                break;
            case '\n':
                oss << "\\n";
                break;
            case '\r':
                oss << "\\r";
                break;
            case '\t':
                oss << "\\t";
                break;
            default:
                // Control characters (0x00-0x1F) should be escaped as \uXXXX
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u00" << std::hex << std::uppercase
                        << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    oss << c;
                }
                break;
        }
    }

    return oss.str();
}

inline std::string toJSONStr(const Value& v) {
    return visit(
        util::Overloaded{
            [](null_t) -> std::string { return "null"; },
            [](bool x) -> std::string { return x ? "true" : "false"; },
            [](int64_t x) -> std::string { return std::to_string(x); },
            [](float x) -> std::string { return std::to_string(x); },
            [](std::string_view s) -> std::string {
                return std::format("\"{}\"", escapeForJSON(s));
            },
        },
        v.variant());
}

template <Sink S>
class JSONFormatter : public Consumer {
 public:
    JSONFormatter(S* sink, ConstFieldBindingPtr binding)
        : binding_(std::move(binding))
        , sink_(sink) {}

    void consume(Record& r) override {
        if (r.empty()) {
            sink_->push("{}");
            return;
        }

        std::stringstream ss;
        ss << '{';

        for (auto&& [id, value] : r) {
            ss << '"' << binding_->name(id) << '"';
            ss << ':' << toJSONStr(value) << ',';
        }

        ss.seekp(-1, std::ios_base::end);
        ss << '}';
        sink_->push(ss.str());
    }

    void done() override { sink_->done(); }

 private:
    ConstFieldBindingPtr binding_;
    S* sink_;
};

}  // namespace lsql::output
