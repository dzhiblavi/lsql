#pragma once

#include "output/Consumer.h"
#include "output/Sink.h"

#include "core/schema/FieldBinding.h"

namespace lsql::output {

template <Sink S>
class TSKVFormatter : public Consumer {
 public:
    TSKVFormatter(S* sink, ConstFieldBindingPtr binding)
        : binding_(std::move(binding))
        , sink_(sink) {}

    void consume(Record& r) override {
        std::stringstream ss;

        for (auto&& [id, value] : r) {
            ss << binding_->name(id) << '=' << to_string(std::move(value)) << '\t';
        }

        sink_->push(ss.str());
    }

    void done() override { sink_->done(); }

 private:
    ConstFieldBindingPtr binding_;
    S* sink_;
};

}  // namespace lsql::output
