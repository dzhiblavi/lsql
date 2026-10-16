#pragma once

#include "output/Consumer.h"
#include "output/Sink.h"

#include "core/schema/FieldBinding.h"
#include "core/schema/Schema.h"

#include <string>
#include <string_view>
#include <vector>

namespace lsql::output {

inline void appendJSONEscaped(std::string& output, std::string_view input) {
    for (char c : input) {
        switch (c) {
            case '"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            case '/':
                output += "\\/";
                break;
            case '\b':
                output += "\\b";
                break;
            case '\f':
                output += "\\f";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    constexpr std::string_view hex = "0123456789ABCDEF";
                    auto byte = static_cast<unsigned char>(c);
                    output += "\\u00";
                    output += hex[byte >> 4];
                    output += hex[byte & 0x0F];
                } else {
                    output += c;
                }
                break;
        }
    }
}

inline void appendJSONValue(std::string& output, const Value& value) {
    visit(
        util::Overloaded{
            [&](null_t) { output += "null"; },
            [&](bool x) { output += x ? "true" : "false"; },
            [&](int64_t x) { output += std::to_string(x); },
            [&](float x) { output += std::to_string(x); },
            [&](std::string_view s) {
                output += '"';
                appendJSONEscaped(output, s);
                output += '"';
            },
        },
        value.variant());
}

template <Sink S>
class JSONFormatter : public Consumer {
 public:
    JSONFormatter(S* sink, ConstFieldBindingPtr binding, const Schema& schema) : sink_(sink) {
        field_prefixes_.reserve(schema.columns());
        for (auto id : schema.fieldIds()) {
            auto& prefix = field_prefixes_.emplace_back("\"");
            appendJSONEscaped(prefix, binding->name(id));
            prefix += "\":";
        }
    }

    void consume(Record& r) override {
        verify_dbg(r.size() == field_prefixes_.size());
        buffer_.clear();
        buffer_ += '{';

        for (size_t i = 0; i < r.size(); ++i) {
            if (i != 0) {
                buffer_ += ',';
            }
            buffer_ += field_prefixes_[i];
            appendJSONValue(buffer_, r[i].second);
        }

        buffer_ += '}';
        sink_->push(buffer_);
    }

    void done() override { sink_->done(); }

 private:
    S* sink_;
    std::vector<std::string> field_prefixes_;
    std::string buffer_;
};

}  // namespace lsql::output
