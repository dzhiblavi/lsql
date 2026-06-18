#include "back/storage/CommandStream.h"

#include "config/build_settings.h"
#include "core/exceptions.h"

#include <array>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <format>
#include <mutex>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <utility>

namespace lsql::back::storage {

namespace {

class Fd {
 public:
    Fd() = default;
    explicit Fd(int fd) : fd_(fd) {}

    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;

    Fd(Fd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    ~Fd() { close(); }

    int get() const { return fd_; }

    int release() { return std::exchange(fd_, -1); }

    void close() {
        if (fd_ < 0) {
            return;
        }

        std::ignore = ::close(fd_);
        fd_ = -1;
    }

 private:
    int fd_ = -1;
};

std::array<Fd, 2> makePipe() {
    int pipe_fds[2];
    require(::pipe(pipe_fds) == 0, "pipe failed: {}", std::strerror(errno));
    return {Fd(pipe_fds[0]), Fd(pipe_fds[1])};
}

[[noreturn]] void childExec(const std::string& command, Fd stdout_write, Fd stderr_write) {
    if (::dup2(stdout_write.get(), STDOUT_FILENO) < 0) {
        _exit(127);
    }
    if (::dup2(stderr_write.get(), STDERR_FILENO) < 0) {
        _exit(127);
    }

    stdout_write.close();
    stderr_write.close();

    execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
    _exit(127);
}

class CommandStream : public Stream {
 public:
    explicit CommandStream(std::string command) : command_(std::move(command)) { start(); }

    ~CommandStream() override {
        closeStdout();
        if (!finished_) {
            terminate();
        }
        joinStderr();
    }

    size_t read(size_t max_count, std::span<char> dest) override {
        if (finished_) {
            return 0;
        }

        max_count = std::min(max_count, dest.size());
        if (max_count == 0) {
            return 0;
        }

        while (true) {
            auto n = ::read(stdout_fd_.get(), dest.data(), max_count);
            if (n > 0) {
                return static_cast<size_t>(n);
            }

            if (n == 0) {
                closeStdout();
                wait();
                return 0;
            }

            if (errno != EINTR) {
                const auto error = std::string(std::strerror(errno));
                closeStdout();
                terminate();
                throwError("failed to read stream command output: {}", error);
            }
        }
    }

 private:
    void start() {
        auto stdout_pipe = makePipe();
        auto stderr_pipe = makePipe();

        pid_ = ::fork();
        require(pid_ >= 0, "fork failed: {}", std::strerror(errno));

        if (pid_ == 0) {
            stdout_pipe[0].close();
            stderr_pipe[0].close();
            childExec(command_, std::move(stdout_pipe[1]), std::move(stderr_pipe[1]));
        }

        stdout_pipe[1].close();
        stderr_pipe[1].close();

        stdout_fd_ = std::move(stdout_pipe[0]);
        stderr_fd_ = std::move(stderr_pipe[0]);
        stderr_thread_ = std::thread([this] { drainStderr(); });
    }

    void drainStderr() {
        std::array<char, config::Storage::CommandStderrBufferSize> buf{};
        while (true) {
            auto n = ::read(stderr_fd_.get(), buf.data(), buf.size());
            if (n > 0) {
                appendStderr(std::string_view(buf.data(), static_cast<size_t>(n)));
                continue;
            }

            if (n == 0) {
                stderr_fd_.close();
                return;
            }

            if (errno != EINTR) {
                return;
            }
        }
    }

    void appendStderr(std::string_view data) {
        std::lock_guard lock(stderr_mutex_);
        stderr_.append(data);
        if (stderr_.size() > config::Storage::CommandStderrTailSize) {
            stderr_.erase(0, stderr_.size() - config::Storage::CommandStderrTailSize);
        }
    }

    std::string stderrTail() {
        std::lock_guard lock(stderr_mutex_);
        return stderr_;
    }

    void closeStdout() { stdout_fd_.close(); }

    void joinStderr() {
        if (stderr_thread_.joinable()) {
            stderr_thread_.join();
        }
        stderr_fd_.close();
    }

    void wait() {
        int status = 0;
        while (::waitpid(pid_, &status, 0) < 0) {
            if (errno == EINTR) {
                continue;
            }
            throwError("waitpid failed for stream command: {}", std::strerror(errno));
        }

        finished_ = true;
        joinStderr();

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return;
        }

        if (WIFEXITED(status)) {
            throwError(
                "stream command exited with code {}: {}",
                WEXITSTATUS(status),
                stderrTail());
        }

        if (WIFSIGNALED(status)) {
            throwError(
                "stream command terminated by signal {}: {}", WTERMSIG(status), stderrTail());
        }

        throwError("stream command failed: {}", stderrTail());
    }

    void terminate() {
        if (pid_ <= 0 || finished_) {
            return;
        }

        std::ignore = ::kill(pid_, SIGTERM);

        int status = 0;
        while (::waitpid(pid_, &status, 0) < 0 && errno == EINTR) {
        }
        finished_ = true;
    }

    std::string command_;
    pid_t pid_ = -1;
    bool finished_ = false;

    Fd stdout_fd_;
    Fd stderr_fd_;
    std::thread stderr_thread_;

    std::mutex stderr_mutex_;
    std::string stderr_;
};

}  // namespace

Box<Stream> CommandStreamSource::stream() const {
    return box<CommandStream>(command_);
}

std::string CommandStreamSource::describe() const {
    return std::format("stream command '{}'", command_);
}

}  // namespace lsql::back::storage
