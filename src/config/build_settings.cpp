#include "config/build_settings.h"

#include <format>

namespace lsql::config {

std::string formatBuildSettings() {
    return std::format(
        "build_settings:\n"
        "  language.line_identifier: {}\n"
        "  diagnostics.stack_traces_enabled: {}\n"
        "  diagnostics.source_context_lines: {}",
        Language::LineIdentifier,
        Diagnostics::StackTracesEnabled ? "true" : "false",
        Diagnostics::SourceContextLines);
}

}  // namespace lsql::config
