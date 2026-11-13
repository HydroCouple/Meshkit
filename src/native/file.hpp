// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "meshkit/io.hpp"
#include "operation.hpp"
#include <array>
#include <atomic>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <random>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace meshkit::native_detail
{
/**
 * @brief Reject empty paths and paths containing an embedded NUL character.
 * @param[in] p Path to check.
 * @return True when p is non-empty and has no NUL; existence is not checked.
 */
inline bool validPath(const std::filesystem::path &p)
{
    return !p.empty() && p.native().find(std::filesystem::path::value_type{}) ==
                             std::filesystem::path::string_type::npos;
}
/** @brief 64-bit FNV-1a offset basis, the starting value of every running hash. */
constexpr std::uint64_t hashSeed = 14695981039346656037ull;
/**
 * @brief Fold bytes into a running 64-bit FNV-1a hash.
 * @param[in,out] hash Running hash, updated in place.
 * @param[in] p Borrowed bytes; may be null only when n is 0.
 * @param[in] n Number of bytes.
 */
inline void hashBytes(std::uint64_t &hash, const unsigned char *p, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i) {
        hash ^= p[i];
        hash *= 1099511628211ull;
    }
}
/**
 * @brief Exclusive temporary output file that commit() publishes atomically at a target path.
 *
 * Copy construction is deleted. Destroying an uncommitted transaction closes and removes the
 * temporary (errors ignored), so the target is never partially written.
 */
class Transaction
{
    std::filesystem::path target_, temp_;
    bool overwrite_, durable_;
    std::FILE *file_ = nullptr;

public:
    /**
     * @brief Validate the target and create its exclusive temporary.
     *
     * The temporary is `.meshkit-<random>.tmp` in the target's directory (or the current
     * directory when the target has none), created exclusively (owner read/write only on
     * POSIX); up to 32 random names are tried while candidates already exist.
     * @param[in] target Destination file path; copied.
     * @param[in] options Supplies overwrite and durable, copied for commit().
     * @throws Stop `path` for an empty or NUL-containing path or a file name that is empty,
     * `.` or `..`; `open_output` when no temporary can be created (both status Failed).
     */
    Transaction(const std::filesystem::path &target, const IOOptions &options)
        : target_(target), overwrite_(options.overwrite), durable_(options.durable)
    {
        if (!validPath(target) || target.filename().empty() || target.filename() == "." ||
            target.filename() == "..")
            fail("path", "Invalid output path.", RunStatus::Failed);
        auto parent = target.parent_path();
        if (parent.empty())
            parent = ".";
        static std::atomic<std::uint64_t> sequence{0};
        std::random_device random;
        for (int attempt = 0; attempt < 32; ++attempt) {
            const auto nonce = (std::uint64_t(random()) << 32) ^ random() ^ ++sequence;
            temp_ = parent / (".meshkit-" + std::to_string(nonce) + ".tmp");
#ifdef _WIN32
            file_ = _wfopen(temp_.c_str(), L"wbx");
#else
            // Restrict access from creation, even with a permissive process umask.
            const int fd = ::open(temp_.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
            if (fd >= 0) {
                file_ = ::fdopen(fd, "wb");
                if (!file_) {
                    const int error = errno;
                    ::close(fd);
                    ::unlink(temp_.c_str());
                    errno = error;
                    break;
                }
            }
#endif
            if (file_)
                return;
            if (errno != EEXIST)
                break;
        }
        fail("open_output",
             "Cannot create an exclusive output temporary: " + std::string(std::strerror(errno)),
             RunStatus::Failed);
    }
    /** @brief Close and remove the temporary unless commit() published it; errors ignored. */
    ~Transaction()
    {
        if (file_)
            std::fclose(file_);
        if (!temp_.empty()) {
            std::error_code error;
            std::filesystem::remove(temp_, error);
        }
    }
    Transaction(const Transaction &) = delete;
    /** @brief The open temporary, owned by this object (do not close it); null after commit(). */
    std::FILE *file() const { return file_; }
    /**
     * @brief Flush, optionally sync, close and publish the temporary at the target; call once.
     *
     * With overwrite the temporary replaces the target (rename(); Windows MoveFileExW with
     * MOVEFILE_REPLACE_EXISTING). Without it POSIX hard-links the temporary to the target, which
     * fails if the target exists, then unlinks the temporary; Windows moves without replacing.
     * With durable the contents are synced (fsync/_commit; MOVEFILE_WRITE_THROUGH) first.
     * @throws Stop `write`, `sync`, `close` or `publish` (status Failed); the destructor then
     * removes the temporary and any existing target is left as it was.
     */
    void commit()
    {
        if (std::fflush(file_) != 0)
            fail("write", "Cannot flush output.", RunStatus::Failed);
        if (durable_) {
#ifdef _WIN32
            if (_commit(_fileno(file_)) != 0)
                fail("sync", "Cannot synchronize output.", RunStatus::Failed);
#else
            if (::fsync(fileno(file_)) != 0)
                fail("sync", "Cannot synchronize output.", RunStatus::Failed);
#endif
        }
        const int closed = std::fclose(file_);
        file_ = nullptr;
        if (closed != 0)
            fail("close", "Cannot close output.", RunStatus::Failed);
#ifdef _WIN32
        const DWORD flags =
            (overwrite_ ? MOVEFILE_REPLACE_EXISTING : 0) | (durable_ ? MOVEFILE_WRITE_THROUGH : 0);
        if (!MoveFileExW(temp_.c_str(), target_.c_str(), flags))
            fail("publish", "Cannot atomically publish output (destination may already exist).",
                 RunStatus::Failed);
#else
        if (overwrite_) {
            if (::rename(temp_.c_str(), target_.c_str()) != 0)
                fail("publish",
                     "Cannot atomically replace output: " + std::string(std::strerror(errno)),
                     RunStatus::Failed);
        } else {
            if (::link(temp_.c_str(), target_.c_str()) != 0)
                fail("publish",
                     "Cannot publish output without replacing an existing file: " +
                         std::string(std::strerror(errno)),
                     RunStatus::Failed);
            ::unlink(temp_.c_str());
        }
#endif
        temp_.clear();
    }
};
/**
 * @brief Buffered little-endian binary writer with a running FNV-1a hash and byte count.
 *
 * Uses a 64 KiB buffer. The destructor does not flush: call flush() before committing, or
 * buffered bytes are lost.
 */
class Output
{
    std::FILE *file_;
    Work &work_;
    std::array<unsigned char, 65536> buffer_{};
    std::size_t used_ = 0;

public:
    /// Bytes accepted by raw(), hashed or not.
    std::uint64_t bytes = 0, hash = hashSeed;
    /** @var hash
     *  @brief FNV-1a hash of the bytes written with hashed true, starting at hashSeed. */
    /**
     * @brief Bind the destination and work budget.
     * @param[in] file Borrowed open stream; not closed by this object.
     * @param[in] work Borrowed budget polled for cancellation on each flush; must outlive this.
     */
    Output(std::FILE *file, Work &work) : file_(file), work_(work) {}
    /**
     * @brief Poll cancellation, then write out the buffered bytes.
     * @throws Stop `cancelled`, or `write` (status Failed) on a short write.
     */
    void flush()
    {
        work_.check();
        if (used_ && std::fwrite(buffer_.data(), 1, used_, file_) != used_)
            fail("write", "Output write failed.", RunStatus::Failed);
        used_ = 0;
    }
    /**
     * @brief Append bytes, flushing whenever the buffer fills.
     * @param[in] data Borrowed bytes; may be null only when n is 0.
     * @param[in] n Number of bytes.
     * @param[in] hashed Whether the bytes enter #hash.
     * @throws Stop from flush().
     */
    void raw(const void *data, std::size_t n, bool hashed = true)
    {
        auto p = static_cast<const unsigned char *>(data);
        if (hashed)
            hashBytes(hash, p, n);
        bytes += n;
        while (n) {
            const auto count = std::min(n, buffer_.size() - used_);
            std::memcpy(buffer_.data() + used_, p, count);
            used_ += count;
            p += count;
            n -= count;
            if (used_ == buffer_.size())
                flush();
        }
    }
    /**
     * @brief Append the low bytes of an unsigned value, least significant first.
     * @param[in] value Value to write; bits above width bytes are dropped.
     * @param[in] width Byte count, 0 to 8 (unchecked).
     * @param[in] hashed Whether the bytes enter #hash.
     */
    void number(std::uint64_t value, int width, bool hashed = true)
    {
        unsigned char data[8];
        for (int i = 0; i < width; ++i) {
            data[i] = value & 255;
            value >>= 8;
        }
        raw(data, width, hashed);
    }
    /**
     * @brief Append a double's 8-byte bit pattern, little-endian and hashed.
     * @param[in] value Value to write, NaN payloads and signed zero included.
     */
    void real(double value)
    {
        std::uint64_t bits;
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&bits, &value, 8);
        number(bits, 8);
    }
    /**
     * @brief Append a counted string: its uint64 byte length, then its bytes, both hashed.
     * @param[in] s Bytes to write; UTF-8 is not checked here.
     */
    void text(const std::string &s)
    {
        number(s.size(), 8);
        raw(s.data(), s.size());
    }
};
/**
 * @brief Buffered little-endian binary reader with a running FNV-1a hash and byte count.
 *
 * Reads the stream in 64 KiB blocks, so it may consume bytes beyond those decoded.
 */
class Input
{
    std::istream &stream_;
    Work &work_;
    std::array<unsigned char, 65536> buffer_{};
    std::size_t at_ = 0, end_ = 0;

public:
    /// Bytes consumed through byte(), hashed or not.
    std::uint64_t bytes = 0, hash = hashSeed;
    /** @var hash
     *  @brief FNV-1a hash of the bytes read with hashed true, starting at hashSeed. */
    /**
     * @brief Bind the source and work budget.
     * @param[in] stream Borrowed binary stream; must outlive this object.
     * @param[in] work Borrowed budget polled for cancellation before each block read; must
     * outlive this object.
     */
    Input(std::istream &stream, Work &work) : stream_(stream), work_(work) {}
    /**
     * @brief Read one byte, refilling the buffer when it is empty.
     * @param[out] value Receives the byte; unchanged at end of stream.
     * @param[in] hashed Whether the byte enters #hash.
     * @return False at end of stream.
     * @throws Stop `cancelled`, or `read` (status Failed) when the stream reports a read error.
     */
    bool byte(unsigned char &value, bool hashed = true)
    {
        if (at_ == end_) {
            work_.check();
            stream_.read(reinterpret_cast<char *>(buffer_.data()), buffer_.size());
            end_ = std::size_t(stream_.gcount());
            at_ = 0;
            if (!end_) {
                if (stream_.bad())
                    fail("read", "Input read failed.", RunStatus::Failed);
                return false;
            }
        }
        value = buffer_[at_++];
        ++bytes;
        if (hashed)
            hashBytes(hash, &value, 1);
        return true;
    }
    /**
     * @brief Read exactly n bytes.
     * @param[out] target Borrowed buffer of at least n bytes.
     * @param[in] n Number of bytes.
     * @param[in] hashed Whether the bytes enter #hash.
     * @throws Stop `truncated` (status Failed) when the stream ends first, or from byte().
     */
    void raw(void *target, std::size_t n, bool hashed = true)
    {
        auto p = static_cast<unsigned char *>(target);
        for (std::size_t i = 0; i < n; ++i)
            if (!byte(p[i], hashed))
                fail("truncated", "Truncated input.", RunStatus::Failed);
    }
    /**
     * @brief Read a little-endian unsigned value of width bytes.
     * @param[in] width Byte count, 0 to 8 (unchecked).
     * @param[in] hashed Whether the bytes enter #hash.
     * @return The zero-extended value.
     * @throws Stop from raw().
     */
    std::uint64_t number(int width, bool hashed = true)
    {
        unsigned char data[8];
        raw(data, width, hashed);
        std::uint64_t value = 0;
        for (int i = 0; i < width; ++i)
            value |= std::uint64_t(data[i]) << (8 * i);
        return value;
    }
    /**
     * @brief Read 8 hashed bytes as a little-endian double bit pattern.
     * @return The double, which may be NaN or infinite; not validated here.
     * @throws Stop from raw().
     */
    double real()
    {
        const auto bits = number(8);
        double value;
        std::memcpy(&value, &bits, 8);
        return value;
    }
    /**
     * @brief Read a counted string (uint64 byte length, then bytes), charging a byte budget.
     * @param[in,out] remaining Metadata bytes still allowed; reduced by the string length.
     * @return The bytes; UTF-8 is not checked here.
     * @throws Stop `metadata_limit` (status ResourceLimit) when the length exceeds remaining,
     * or from raw().
     */
    std::string text(std::uint64_t &remaining)
    {
        auto n = number(8);
        if (n > remaining)
            fail("metadata_limit", "Metadata size exceeds the limit.", RunStatus::ResourceLimit);
        remaining -= n;
        std::string result(std::size_t(n), '\0');
        raw(result.data(), result.size());
        return result;
    }
};
/**
 * @brief Append one external id to a sparse id array that stays empty while ids are 1, 2, ...
 *
 * The first id that differs from index + 1 materializes the implicit ids 1..index; every later
 * call then appends.
 * @param[in,out] values Id array; empty means sequential one-based ids.
 * @param[in] id External id of record index.
 * @param[in] index Zero-based record number; calls must arrive in order 0, 1, 2, ...
 */
inline void appendId(std::vector<std::int64_t> &values, std::int64_t id, std::size_t index)
{
    if (values.empty() && id == std::int64_t(index + 1))
        return;
    if (values.empty()) {
        values.reserve(index + 1);
        for (std::size_t i = 0; i < index; ++i)
            values.push_back(i + 1);
    }
    values.push_back(id);
}
/**
 * @brief Append one marker to a sparse marker array that stays empty while markers are 0.
 *
 * The first nonzero marker materializes index zeros; every later call then appends.
 * @param[in,out] values Marker array; empty means all zero.
 * @param[in] marker Marker of record index.
 * @param[in] index Zero-based record number; calls must arrive in order 0, 1, 2, ...
 */
inline void appendMarker(std::vector<std::int32_t> &values, std::int32_t marker, std::size_t index)
{
    if (!marker && values.empty())
        return;
    if (values.empty())
        values.resize(index, 0);
    values.push_back(marker);
}
/**
 * @brief External id of a record from a possibly empty sparse id array.
 * @param[in] ids Id array; empty means sequential one-based ids.
 * @param[in] index Zero-based record number; must be in range when ids is non-empty.
 * @return index + 1 when ids is empty, otherwise ids[index].
 */
inline std::int64_t externalId(const std::vector<std::int64_t> &ids, std::size_t index)
{
    return ids.empty() ? std::int64_t(index + 1) : ids[index];
}
/**
 * @brief Reinterpret a 64-bit pattern as a two's-complement signed value.
 * @param[in] bits Bit pattern as read from a file.
 * @return The signed value with the same bits.
 */
inline std::int64_t signed64(std::uint64_t bits)
{
    std::int64_t value;
    std::memcpy(&value, &bits, 8);
    return value;
}
/**
 * @brief Reinterpret the low 32 bits of a pattern as a two's-complement signed value.
 * @param[in] bits Bit pattern as read from a file; higher bits are ignored.
 * @return The signed 32-bit value with the same low bits.
 */
inline std::int32_t signed32(std::uint64_t bits)
{
    std::uint32_t raw = std::uint32_t(bits);
    std::int32_t value;
    std::memcpy(&value, &raw, 4);
    return value;
}
/**
 * @brief Copy a caught Stop into a report.
 * @param[in,out] r Report whose status, code and message are overwritten.
 * @param[in] e Caught failure.
 */
inline void setError(IOReport &r, const Stop &e)
{
    r.status = e.status;
    r.code = e.code;
    r.message = e.message;
}
/**
 * @brief Record an allocation failure: status ResourceLimit, code `allocation`.
 * @param[in,out] r Report whose status, code and message are overwritten.
 */
inline void setException(IOReport &r, const std::bad_alloc &)
{
    r.status = RunStatus::ResourceLimit;
    r.code = "allocation";
    r.message = "Insufficient memory.";
}
/**
 * @brief Record any other standard exception: status Failed, code `exception`.
 * @param[in,out] r Report whose status, code and message are overwritten.
 * @param[in] e Caught exception; its what() text becomes the message.
 */
inline void setException(IOReport &r, const std::exception &e)
{
    r.status = RunStatus::Failed;
    r.code = "exception";
    r.message = e.what();
}
/**
 * @brief Run validateMesh() with the options' limits and cancellation poll.
 * @param[in] mesh Borrowed mesh, unchanged.
 * @param[in] o Supplies limits and cancelled.
 * @throws Stop carrying the validation report's code, message and status when it is not
 * complete.
 */
inline void validated(const Mesh &mesh, const IOOptions &o)
{
    const auto r = validateMesh(mesh, o.limits, o.cancelled);
    if (!r.complete())
        fail(r.code, r.message, r.status);
}
}
