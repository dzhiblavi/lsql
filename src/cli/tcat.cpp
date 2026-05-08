#include "data/PagedFile.h"
#include "exec/SearchTimestamp.h"

#include <tclap/CmdLine.h>

#include <span>
#include <unistd.h>  // write, STDOUT_FILENO

namespace lsql {

void printFileRange(const data::File& file, size_t from, size_t to) {
    static constexpr size_t BufSize = 1ull << 14;
    std::array<char, BufSize> buf;  // NOLINT

    while (from < to) {
        size_t need = std::min(to - from, BufSize);
        size_t size = file.read(from, std::span<char>(buf.data(), need));
        if (size == 0) {
            return;
        }

        ::write(STDOUT_FILENO, buf.data(), size);
        from += size;
    }
}

TCLAP::ValueArg<int> interval_arg{
    "i",
    "interval",
    "log interval in seconds",
    false,
    5,
    "integer",
};

TCLAP::ValueArg<std::string> timestamp_arg{
    "s",
    "since",
    "left time interval boundary",
    true,
    "",
    "tskv time format",
};

TCLAP::UnlabeledValueArg<std::string> file_arg{
    "path",
    "path to the log file",
    true,
    "",
    "filesystem path",
};

void parseArgs(std::span<const char*> argv) {
    TCLAP::CmdLine cmd{"tcat", ' ', "0.0.1"};
    cmd.add(&interval_arg);
    cmd.add(&timestamp_arg);
    cmd.add(&file_arg);
    cmd.setExceptionHandling(false);

    try {
        cmd.parse(static_cast<int>(argv.size()), argv.data());
    } catch (const TCLAP::ArgException& e) {
        throw std::runtime_error(std::format("error for argument '{}': {}", e.argId(), e.error()));
    } catch (const TCLAP::ExitException& e) {
        return;
    }
}

void main(std::span<const char*> argv) {
    parseArgs(argv);
    auto format = TimeFormat::SQL;
    auto ts = timestampFromString(timestamp_arg.getValue(), format);
    auto interval = interval_arg.getValue();
    auto file = data::NativePagedFile::open(file_arg.getValue());

    size_t from = exec::lowerBoundLine(*file, ts, format);
    if (from == std::string::npos) {
        return;
    }

    size_t to = exec::upperBoundLine(*file, ts + interval, format);
    printFileRange(*file, from, to);
}

}  // namespace lsql

int main(int argc, const char** argv) {
    try {
        lsql::main(std::span<const char*>(argv, argc));
        return 0;
    } catch (const std::runtime_error& e) {
        std::println("error: {}", e.what());
        return 1;
    } catch (...) {
        std::println("unknown error");
        return 2;
    }
}
