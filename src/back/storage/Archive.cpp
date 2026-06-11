#include "back/storage/Archive.h"

#include "util/require.h"

#include <zlib.h>

namespace lsql::back::storage {

namespace {

class ZlibStream {
 public:
    ZlibStream() { init(); }
    ~ZlibStream() { std::ignore = inflateEnd(&impl_); }

    z_stream& native() { return impl_; }

    void reset() {
        auto ret = inflateEnd(&impl_);
        require(ret == Z_OK, "zlib: inflateEnd call failed: {}", ret);
        init();
    }

 private:
    void init() {
        impl_ = {};
        int ret = inflateInit2(&impl_, 32 + MAX_WBITS);  // auto zlib/gzip
        require(ret == Z_OK, "zlib: inflateInit2 call failed");
    }

    z_stream impl_{};
};

class NativeArchiveStream : public Stream {
 public:
    explicit NativeArchiveStream(Arc<File> file) : file_(std::move(file)) {
        verify(file_ != nullptr);
    }

    void reset() {
        compressed_eof_ = false;
        decompressed_eof_ = false;
        compressed_offset_ = 0;
        stream_.reset();
    }

    size_t read(size_t max_count, std::span<char> dest) override {
        if (decompressed_eof_) {
            return 0;
        }

        max_count = std::min(max_count, dest.size());
        if (max_count == 0) {
            return 0;
        }

        auto& strm = stream_.native();
        strm.next_out = reinterpret_cast<Bytef*>(dest.data());
        strm.avail_out = static_cast<uInt>(max_count);

        while (strm.avail_out > 0 && !decompressed_eof_) {
            if (strm.avail_in == 0 && !compressed_eof_) {
                const size_t remaining = file_->size() - compressed_offset_;
                const size_t to_read = std::min(CompressedChunkSize, remaining);

                if (to_read == 0) {
                    compressed_eof_ = true;
                } else {
                    size_t n =
                        file_->read(compressed_offset_, std::span<char>(inbuf_.data(), to_read));

                    if (n == 0) {
                        compressed_eof_ = true;
                    } else {
                        compressed_offset_ += n;
                        strm.next_in = reinterpret_cast<Bytef*>(inbuf_.data());
                        strm.avail_in = static_cast<uInt>(n);
                    }
                }
            }

            int ret = inflate(&strm, compressed_eof_ ? Z_FINISH : Z_NO_FLUSH);

            if (ret == Z_STREAM_END) {
                decompressed_eof_ = true;
                break;
            }

            if (ret == Z_BUF_ERROR) {
                require(!compressed_eof_, "truncated compressed stream");
                continue;
            }

            require(ret == Z_OK, "inflate failed: {}", ret);
            require(strm.avail_in > 0 || !compressed_eof_, "truncated compressed stream");
        }

        return max_count - strm.avail_out;
    }

 private:
    static constexpr size_t CompressedChunkSize = 64 * 1024;

    Arc<File> file_;
    std::array<char, CompressedChunkSize> inbuf_{};

    // state
    bool compressed_eof_ = false;
    bool decompressed_eof_ = false;
    size_t compressed_offset_ = 0;
    ZlibStream stream_;
};

}  // namespace

Box<Stream> NativeArchive::stream() const {
    return box<NativeArchiveStream>(file_);
}

}  // namespace lsql::back::storage
