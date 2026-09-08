#pragma once

#include "output/Consumer.h"
#include "output/Sink.h"

#include "core/schema/FieldBinding.h"

#include <algorithm>
#include <format>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace lsql::output {

namespace detail {

inline std::string escapeForPrettyTable(std::string_view input) {
    std::string result;
    result.reserve(input.size());

    constexpr std::string_view HexDigits = "0123456789abcdef";
    for (unsigned char c : input) {
        switch (c) {
            case '\b':
                result += "\\b";
                break;
            case '\f':
                result += "\\f";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                if (c < 0x20 || c == 0x7f) {
                    result += "\\x";
                    result.push_back(HexDigits[c >> 4]);
                    result.push_back(HexDigits[c & 0x0f]);
                } else {
                    result.push_back(static_cast<char>(c));
                }
                break;
        }
    }

    return result;
}

inline size_t prettyTableDisplayWidth(std::string_view value) {
    try {
        const size_t padded_size = std::formatted_size("{:<{}}", value, value.size());
        return value.size() - (padded_size - value.size());
    } catch (const std::format_error&) {
        return value.size();
    }
}

inline std::string prettyTablePad(std::string_view value, size_t width) {
    try {
        return std::format("{:<{}}", value, width);
    } catch (const std::format_error&) {
        std::string result(value);
        result.append(width - std::min(width, value.size()), ' ');
        return result;
    }
}

}  // namespace detail

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
            row.push_back(detail::escapeForPrettyTable(to_string(std::move(value))));
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
            header_.push_back(detail::escapeForPrettyTable(binding_->name(id)));
        }
    }

    std::vector<size_t> columnWidths() const {
        std::vector<size_t> widths(header_.size(), 0);
        for (size_t i = 0; i < header_.size(); ++i) {
            widths[i] = detail::prettyTableDisplayWidth(header_[i]);
        }

        for (const auto& row : rows_) {
            if (row.size() > widths.size()) {
                widths.resize(row.size(), 0);
            }
            for (size_t i = 0; i < row.size(); ++i) {
                widths[i] = std::max(widths[i], detail::prettyTableDisplayWidth(row[i]));
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
                ss << detail::prettyTablePad(row[i], widths[i]);
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
