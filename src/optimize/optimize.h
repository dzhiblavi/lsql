#pragma once

#include "ir/Statement.h"

#include "util/StrBuilder.h"

namespace lsql::opt {

class Context {
 public:
    Context() = default;

    bool changes() const { return changes_; }

    void nextPass() {
        ++pass_;
        changes_ = false;
        pass_notes_.emplace_back();
    }

    Context& setChanges() {
        changes_ = true;
        return *this;
    }

    template <typename... Args>
    Context& note(std::format_string<const Args&...> fmt, const Args&... args) {
        pass_notes_.back().item(fmt, args...);
        return *this;
    }

    std::string report() const {
        util::StrBuilder report("Optimizer Report");
        for (size_t i = 0; i < pass_notes_.size(); ++i) {
            report.item(util::StrBuilder("pass {}", i).block(pass_notes_[i]));
        }
        return report.render();
    }

 private:
    std::vector<util::StrBuilder> pass_notes_;
    int pass_ = 0;
    bool changes_ = false;
};

// Single pass
ir::Program optimize(ir::Program program, Context& ctx);

}  // namespace lsql::opt
