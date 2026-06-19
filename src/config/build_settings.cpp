#include "config/build_settings.h"

#include <format>

namespace lsql::config {

std::string formatBuildSettings() {
    return std::format(
        "build_settings:\n"
        "  language.line_identifier: {}\n"
        "  storage.timestamp_search_buffer_size: {}\n"
        "  storage.archive_input_chunk_size: {}\n"
        "  storage.stream_read_chunk_size: {}\n"
        "  storage.page_size_multiplier: {}\n"
        "  storage.command_stderr_buffer_size: {}\n"
        "  storage.command_stderr_tail_size: {}\n"
        "  optimizer.default_passes: {}\n"
        "  optimizer.fn_call_cost_overhead: {}\n"
        "  optimizer.coalesce_cost_overhead: {}\n"
        "  optimizer.unary_op_cost_overhead: {}\n"
        "  optimizer.binary_op_cost_overhead: {}\n"
        "  optimizer.cast_to_string_cost_overhead: {}\n"
        "  optimizer.parse_string_cost_overhead: {}\n"
        "  optimizer.regex_cost_overhead: {}\n"
        "  optimizer.projection_cost_overhead: {}\n"
        "  diagnostics.stack_traces_enabled: {}\n"
        "  diagnostics.source_context_lines: {}",
        Language::LineIdentifier,
        Storage::TimestampSearchBufferSize,
        Storage::ArchiveInputChunkSize,
        Storage::StreamReadChunkSize,
        Storage::PageSizeMultiplier,
        Storage::CommandStderrBufferSize,
        Storage::CommandStderrTailSize,
        Optimizer::DefaultPasses,
        Optimizer::FnCallCostOverhead,
        Optimizer::CoalesceCostOverhead,
        Optimizer::UnaryOpCostOverhead,
        Optimizer::BinaryOpCostOverhead,
        Optimizer::CastToStringCostOverhead,
        Optimizer::ParseStringCostOverhead,
        Optimizer::RegexCostOverhead,
        Optimizer::ProjectionCostOverhead,
        Diagnostics::StackTracesEnabled ? "true" : "false",
        Diagnostics::SourceContextLines);
}

}  // namespace lsql::config
