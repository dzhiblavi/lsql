#pragma once

#include "output/Consumer.h"
#include "output/Sink.h"

#include "core/schema/FieldBinding.h"
#include "core/schema/Schema.h"

namespace lsql::output {

template <Sink S>
class CSVHeaderFormatter : public Consumer {
 public:
    CSVHeaderFormatter(S* sink, ConstFieldBindingPtr binding, const Schema& schema)
        : binding_(std::move(binding))
        , sink_(sink) {
        writeHeader(schema);
    }

    void consume(Record& r) override {
        if (r.empty()) {
            sink_->push("");
            return;
        }

        std::stringstream ss;
        for (auto&& [id, value] : r) {
            ss << to_string(std::move(value)) << ',';
        }

        auto str = ss.str();
        str.pop_back();
        sink_->push(str);
    }

    void done() override { sink_->done(); }

 private:
    void writeHeader(const Schema& schema) {
        if (schema.columns() == 0) {
            sink_->push("");
            return;
        }

        std::stringstream ss;
        for (auto id : schema.fieldIds()) {
            ss << binding_->name(id) << ',';
        }

        auto str = ss.str();
        str.pop_back();
        sink_->push(str);
    }

    ConstFieldBindingPtr binding_;
    S* sink_;
};

}  // namespace lsql::output
