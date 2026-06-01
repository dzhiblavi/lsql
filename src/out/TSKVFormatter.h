#pragma once

#include "out/Consumer.h"
#include "out/Sink.h"

namespace lsql::out {

template <Sink S>
class TSKVFormatter : public Consumer {
 public:
    TSKVFormatter(S sink, ConstFieldBindingPtr binding)
        : binding_(std::move(binding))
        , sink_(std::move(sink)) {}

    void consume(Record& r) override {
        std::stringstream ss;

        for (auto&& [id, value] : r) {
            ss << binding_->name(id) << '=' << to_string(std::move(value)) << '\t';
        }

        sink_.push(ss.str());
    }

    void done() override { sink_.done(); }

 private:
    ConstFieldBindingPtr binding_;
    [[no_unique_address]] S sink_;
};

}  // namespace lsql::out
