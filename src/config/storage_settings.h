#pragma once

#include "config/dynamic_settings.h"

#include "core/exceptions.h"

namespace lsql::config {

enum class StorageSetting {
    TimestampSearchBufferSize,
    ArchiveInputChunkSize,
    StreamReadChunkSize,
    PageSizeMultiplier,
    CommandStderrBufferSize,
    CommandStderrTailSize,
};

struct StorageSettings : DynamicSettings<StorageSetting, StorageSettings> {
    void setDefaults() {
        using enum StorageSetting;

        set(TimestampSearchBufferSize, 8 * 1024);
        set(ArchiveInputChunkSize, 64 * 1024);
        set(StreamReadChunkSize, 64 * 1024);
        set(PageSizeMultiplier, 4096);
        set(CommandStderrBufferSize, 4 * 1024);
        set(CommandStderrTailSize, 16 * 1024);
    }

    void validate(StorageSetting setting, int64_t value) {
        require(value > 0, "value for {} must be positive", magic_enum::enum_name(setting));
    }
};

StorageSettings& storage();

}  // namespace lsql::config
