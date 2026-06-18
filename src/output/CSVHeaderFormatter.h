#pragma once

#include "output/Consumer.h"
#include "output/Sink.h"

#include "core/schema/FieldBinding.h"

namespace lsql::output {

template <Sink S>
class CSVHeaderFormatter : public Consumer {
 public:
    CSVHeaderFormatter(S* sink, ConstFieldBindingPtr binding)
        : binding_(std::move(binding))
        , sink_(sink) {}

    void consume(Record& r) override {
        if (!header_written_) {
            writeHeader(r);
            header_written_ = true;
        }

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
    void writeHeader(const Record& r) {
        if (r.empty()) {
            sink_->push("");
            return;
        }

        std::stringstream ss;
        for (auto&& [id, _] : r) {
            ss << binding_->name(id) << ',';
        }

        auto str = ss.str();
        str.pop_back();
        sink_->push(str);
    }

    bool header_written_ = false;
    ConstFieldBindingPtr binding_;
    S* sink_;
};

}  // namespace lsql::output
