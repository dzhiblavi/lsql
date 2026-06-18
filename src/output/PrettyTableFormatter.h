#pragma once

#include "output/Consumer.h"
#include "output/Sink.h"

#include "core/schema/FieldBinding.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace lsql::output {

template <Sink S>
class PrettyTableFormatter : public Consumer {
 public:
    PrettyTableFormatter(S* sink, ConstFieldBindingPtr binding)
        : binding_(std::move(binding))
        , sink_(sink) {}

    void consume(Record& r) override {
        if (!header_read_) {
            readHeader(r);
            header_read_ = true;
        }

        std::vector<std::string> row;
        row.reserve(r.size());
        for (auto&& [_, value] : r) {
            row.push_back(to_string(std::move(value)));
        }

        rows_.push_back(std::move(row));
    }

    void done() override {
        auto widths = columnWidths();
        if (!header_.empty()) {
            pushRow(header_, widths);
            pushSeparator(widths);
        }

        for (const auto& row : rows_) {
            pushRow(row, widths);
        }

        sink_->done();
    }

 private:
    void readHeader(const Record& r) {
        header_.reserve(r.size());
        for (auto&& [id, _] : r) {
            header_.emplace_back(binding_->name(id));
        }
    }

    std::vector<size_t> columnWidths() const {
        std::vector<size_t> widths(header_.size(), 0);
        for (size_t i = 0; i < header_.size(); ++i) {
            widths[i] = header_[i].size();
        }

        for (const auto& row : rows_) {
            if (row.size() > widths.size()) {
                widths.resize(row.size(), 0);
            }
            for (size_t i = 0; i < row.size(); ++i) {
                widths[i] = std::max(widths[i], row[i].size());
            }
        }

        return widths;
    }

    void pushRow(const std::vector<std::string>& row, const std::vector<size_t>& widths) {
        std::stringstream ss;
        for (size_t i = 0; i < widths.size(); ++i) {
            if (i > 0) {
                ss << "  ";
            }

            if (i < row.size()) {
                ss << row[i];
                ss << std::string(widths[i] - row[i].size(), ' ');
            } else {
                ss << std::string(widths[i], ' ');
            }
        }

        sink_->push(ss.str());
    }

    void pushSeparator(const std::vector<size_t>& widths) {
        std::stringstream ss;
        for (size_t i = 0; i < widths.size(); ++i) {
            if (i > 0) {
                ss << "  ";
            }
            ss << std::string(widths[i], '-');
        }

        sink_->push(ss.str());
    }

    bool header_read_ = false;
    ConstFieldBindingPtr binding_;
    S* sink_;
    std::vector<std::string> header_;
    std::vector<std::vector<std::string>> rows_;
};

}  // namespace lsql::output
