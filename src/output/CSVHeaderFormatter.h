#pragma once

#include "output/Consumer.h"
#include "output/Sink.h"

namespace lsql::output {

template <Sink S>
class CSVHeaderFormatter : public Consumer {
 public:
    CSVHeaderFormatter(S sink, ConstFieldBindingPtr binding)
        : binding_(std::move(binding))
        , sink_(std::move(sink)) {}

    void consume(Record& r) override {
        if (!header_written_) {
            writeHeader(r);
            header_written_ = true;
        }

        std::stringstream ss;

        for (auto&& [id, value] : r) {
            ss << to_string(std::move(value)) << ',';
        }

        ss.seekp(-1, std::ios_base::end);
        sink_.push(ss.str());
    }

    void done() override { sink_.done(); }

 private:
    void writeHeader(const Record& r) {
        std::stringstream ss;

        for (auto&& [id, _] : r) {
            ss << binding_->name(id) << ',';
        }

        ss.seekp(-1, std::ios_base::end);
        sink_.push(ss.str());
    }

    bool header_written_ = false;
    ConstFieldBindingPtr binding_;
    [[no_unique_address]] S sink_;
};

}  // namespace lsql::output
