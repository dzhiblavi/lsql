#include "config/optimizer_settings.h"
#include "config/storage_settings.h"

namespace lsql::config {

namespace {

OptimizerSettings optimizer_{};
StorageSettings storage_{};

}  // namespace

OptimizerSettings& optimizer() {
    return optimizer_;
}

StorageSettings& storage() {
    return storage_;
}

}  // namespace lsql::config
