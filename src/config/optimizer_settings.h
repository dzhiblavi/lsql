#pragma once

#include "config/dynamic_settings.h"

#include "core/exceptions.h"

namespace lsql::config {

enum class OptimizerSetting {
    Passes,
    CoalesceCostOverhead,
    LowerCostOverhead,
    SplitPartCostOverhead,
    SubstrCostOverhead,
    ParseTimestampCostOverhead,
    JSONValueCostOverhead,
    UnaryOpCostOverhead,
    BinaryOpCostOverhead,
    CastToStringCostOverhead,
    ParseStringCostOverhead,
    RegexCostOverhead,
    ProjectionCostOverhead,
    CachedScalarCostOverhead,
};

struct OptimizerSettings : DynamicSettings<OptimizerSetting, OptimizerSettings> {
    void setDefaults() {
        using enum OptimizerSetting;

        set(Passes, 5);
        set(CoalesceCostOverhead, 1);
        set(LowerCostOverhead, 1);
        set(SplitPartCostOverhead, 1);
        set(SubstrCostOverhead, 1);
        set(ParseTimestampCostOverhead, 10);
        set(JSONValueCostOverhead, 10);
        set(UnaryOpCostOverhead, 1);
        set(BinaryOpCostOverhead, 1);
        set(CastToStringCostOverhead, 2);
        set(ParseStringCostOverhead, 10);
        set(RegexCostOverhead, 4);
        set(ProjectionCostOverhead, 10);
        set(CachedScalarCostOverhead, 1);
    }

    void validate(OptimizerSetting setting, int64_t value) {
        require(value >= 0, "value for {} cannot be negative", magic_enum::enum_name(setting));
    }
};

OptimizerSettings& optimizer();

}  // namespace lsql::config
