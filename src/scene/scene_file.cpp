#include "storm/scene/scene_file.hpp"

#include "storm/core/stable_id_generator.hpp"
#include "storm/scene/scene_codec.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// Prevent windows.h's max/min macros from breaking standard-library calls.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace storm::scene {
namespace {

[[nodiscard]] core::Status invalid_path_status() {
    return core::Status::failure(
        core::ErrorCode::InvalidArgument, "scene file path is empty or has an invalid filename");
}

[[nodiscard]] bool is_valid_file_path(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }

    const auto& native = path.native();
    if (native.empty() ||
        native.find(std::filesystem::path::value_type{}) !=
            std::filesystem::path::string_type::npos) {
        return false;
    }

    const std::filesystem::path filename = path.filename();
    return !filename.empty() && filename != std::filesystem::path{"."} &&
           filename != std::filesystem::path{".."};
}

[[nodiscard]] core::Status resource_exhausted_status() {
    return core::Status::failure(core::ErrorCode::ResourceExhausted, std::string{});
}

#if defined(_WIN32)

[[nodiscard]] core::Status windows_io_status(const char* operation, DWORD error) {
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
        return core::Status::failure(core::ErrorCode::NotFound, operation);
    }
    return core::Status::failure(core::ErrorCode::Io, operation);
}

class ScopedHandle final {
public:
    explicit ScopedHandle(HANDLE handle) noexcept : handle_(handle) {}
    ~ScopedHandle() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            (void)::CloseHandle(handle_);
        }
    }

    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;

    [[nodiscard]] HANDLE get() const noexcept {
        return handle_;
    }

    [[nodiscard]] bool close_checked() noexcept {
        if (handle_ == INVALID_HANDLE_VALUE) {
            return true;
        }
        const HANDLE handle = handle_;
        handle_ = INVALID_HANDLE_VALUE;
        return ::CloseHandle(handle) != 0;
    }

private:
    HANDLE handle_;
};

class ScopedTempPath final {
public:
    explicit ScopedTempPath(const std::filesystem::path& path) noexcept : path_(path) {}
    ~ScopedTempPath() {
        if (active_) {
            (void)::DeleteFileW(path_.c_str());
        }
    }

    ScopedTempPath(const ScopedTempPath&) = delete;
    ScopedTempPath& operator=(const ScopedTempPath&) = delete;

    void release() noexcept {
        active_ = false;
    }

private:
    const std::filesystem::path& path_;
    bool active_{true};
};

[[nodiscard]] core::Status write_atomic_native(const std::filesystem::path& destination,
                                              std::span<const std::byte> bytes) {
    std::filesystem::path parent = destination.parent_path();
    if (parent.empty()) {
        parent = std::filesystem::path{"."};
    }

    for (unsigned int attempt = 0U; attempt < 8U; ++attempt) {
        core::StableId nonce;
        const core::Status nonce_status = core::StableIdGenerator::generate_v4(nonce);
        if (!nonce_status.ok()) {
            return nonce_status;
        }

        const std::string temp_name = ".storm-scene-" + nonce.to_string() + ".tmp";
        const std::filesystem::path temp_path = parent / std::filesystem::path{temp_name};
        const HANDLE raw_handle = ::CreateFileW(
            temp_path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
            FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_WRITE_THROUGH, nullptr);

        if (raw_handle == INVALID_HANDLE_VALUE) {
            const DWORD error = ::GetLastError();
            if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS) {
                continue;
            }
            return windows_io_status("could not create a temporary scene file", error);
        }

        ScopedTempPath temporary_path{temp_path};
        ScopedHandle file{raw_handle};
        std::size_t offset = 0U;
        while (offset < bytes.size()) {
            const std::size_t remaining = bytes.size() - offset;
            const DWORD requested = static_cast<DWORD>(
                std::min(remaining, static_cast<std::size_t>(std::numeric_limits<DWORD>::max())));
            DWORD written = 0U;
            if (::WriteFile(file.get(), bytes.data() + offset, requested, &written, nullptr) == 0 ||
                written == 0U) {
                return windows_io_status("could not write the temporary scene file",
                                         ::GetLastError());
            }
            offset += static_cast<std::size_t>(written);
        }

        if (::FlushFileBuffers(file.get()) == 0) {
            return windows_io_status("could not flush the temporary scene file", ::GetLastError());
        }
        if (!file.close_checked()) {
            return windows_io_status("could not close the temporary scene file", ::GetLastError());
        }

        if (::MoveFileExW(temp_path.c_str(), destination.c_str(),
                          MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
            return windows_io_status("could not atomically replace the scene file", ::GetLastError());
        }
        temporary_path.release();
        return core::Status::success();
    }

    return core::Status::failure(
        core::ErrorCode::ResourceExhausted, "could not reserve a unique temporary scene filename");
}

[[nodiscard]] core::Status read_file_bounded(const std::filesystem::path& source,
                                             std::vector<std::byte>& output) {
    const HANDLE raw_handle = ::CreateFileW(
        source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (raw_handle == INVALID_HANDLE_VALUE) {
        return windows_io_status("could not open the scene file", ::GetLastError());
    }
    ScopedHandle file{raw_handle};

    BY_HANDLE_FILE_INFORMATION information{};
    if (::GetFileInformationByHandle(file.get(), &information) == 0) {
        return windows_io_status("could not inspect the scene file", ::GetLastError());
    }
    if ((information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U ||
        (information.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U ||
        ::GetFileType(file.get()) != FILE_TYPE_DISK) {
        return core::Status::failure(
            core::ErrorCode::InvalidArgument, "scene path must refer to a regular file, not a reparse point");
    }

    LARGE_INTEGER file_size{};
    if (::GetFileSizeEx(file.get(), &file_size) == 0 || file_size.QuadPart < 0) {
        return windows_io_status("could not determine the scene file size", ::GetLastError());
    }
    if (static_cast<unsigned long long>(file_size.QuadPart) >
        static_cast<unsigned long long>(SceneCodec::maximum_serialized_bytes)) {
        return core::Status::failure(
            core::ErrorCode::ResourceExhausted, "scene file exceeds the configured size limit");
    }

    try {
        std::vector<std::byte> encoded(static_cast<std::size_t>(file_size.QuadPart));
        std::size_t offset = 0U;
        while (offset < encoded.size()) {
            const DWORD requested = static_cast<DWORD>(
                std::min(encoded.size() - offset,
                         static_cast<std::size_t>(std::numeric_limits<DWORD>::max())));
            DWORD received = 0U;
            if (::ReadFile(file.get(), encoded.data() + offset, requested, &received, nullptr) == 0) {
                return windows_io_status("could not read the scene file", ::GetLastError());
            }
            if (received == 0U) {
                return core::Status::failure(
                    core::ErrorCode::Io, "scene file changed or ended while being read");
            }
            offset += static_cast<std::size_t>(received);
        }

        std::byte extra{};
        DWORD received_extra = 0U;
        if (::ReadFile(file.get(), &extra, 1U, &received_extra, nullptr) == 0) {
            return windows_io_status("could not verify the end of the scene file", ::GetLastError());
        }
        if (received_extra != 0U) {
            return core::Status::failure(
                core::ErrorCode::Io, "scene file changed while being read");
        }

        output.swap(encoded);
    } catch (const std::bad_alloc&) {
        return resource_exhausted_status();
    }

    return core::Status::success();
}

#else

class ScopedFd final {
public:
    explicit ScopedFd(int descriptor = -1) noexcept : descriptor_(descriptor) {}
    ~ScopedFd() {
        if (descriptor_ >= 0) {
            (void)::close(descriptor_);
        }
    }

    ScopedFd(const ScopedFd&) = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;

    [[nodiscard]] int get() const noexcept {
        return descriptor_;
    }

    [[nodiscard]] bool close_checked() noexcept {
        if (descriptor_ < 0) {
            return true;
        }
        const int descriptor = descriptor_;
        descriptor_ = -1;
        return ::close(descriptor) == 0;
    }

private:
    int descriptor_;
};

class ScopedTempAt final {
public:
    ScopedTempAt(int directory, const std::string& name) noexcept
        : directory_(directory), name_(name) {}
    ~ScopedTempAt() {
        if (active_) {
            (void)::unlinkat(directory_, name_.c_str(), 0);
        }
    }

    ScopedTempAt(const ScopedTempAt&) = delete;
    ScopedTempAt& operator=(const ScopedTempAt&) = delete;

    void release() noexcept {
        active_ = false;
    }

private:
    int directory_;
    const std::string& name_;
    bool active_{true};
};

[[nodiscard]] core::Status posix_io_status(const char* operation, int error) {
    if (error == ENOENT) {
        return core::Status::failure(core::ErrorCode::NotFound, operation);
    }
    return core::Status::failure(core::ErrorCode::Io, operation);
}

[[nodiscard]] bool sync_descriptor(int descriptor) noexcept {
    while (::fsync(descriptor) != 0) {
        if (errno != EINTR) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] core::Status write_atomic_native(const std::filesystem::path& destination,
                                              std::span<const std::byte> bytes) {
    std::filesystem::path parent = destination.parent_path();
    if (parent.empty()) {
        parent = std::filesystem::path{"."};
    }

    const int raw_directory = ::open(parent.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY);
    if (raw_directory < 0) {
        return posix_io_status("could not open the scene directory", errno);
    }
    ScopedFd directory{raw_directory};
    const std::string target_name = destination.filename().native();

    for (unsigned int attempt = 0U; attempt < 8U; ++attempt) {
        core::StableId nonce;
        const core::Status nonce_status = core::StableIdGenerator::generate_v4(nonce);
        if (!nonce_status.ok()) {
            return nonce_status;
        }

        const std::string temp_name = ".storm-scene-" + nonce.to_string() + ".tmp";
        const int raw_file = ::openat(
            directory.get(), temp_name.c_str(),
            O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, S_IRUSR | S_IWUSR);

        if (raw_file < 0) {
            const int error = errno;
            if (error == EEXIST) {
                continue;
            }
            return posix_io_status("could not create a temporary scene file", error);
        }

        ScopedTempAt temporary_file{directory.get(), temp_name};
        ScopedFd file{raw_file};
        std::size_t offset = 0U;
        while (offset < bytes.size()) {
            const ssize_t written = ::write(file.get(), bytes.data() + offset, bytes.size() - offset);
            if (written > 0) {
                offset += static_cast<std::size_t>(written);
                continue;
            }
            if (written < 0 && errno == EINTR) {
                continue;
            }
            return posix_io_status("could not write the temporary scene file", errno);
        }

        if (!sync_descriptor(file.get())) {
            return posix_io_status("could not flush the temporary scene file", errno);
        }
        if (!file.close_checked()) {
            return posix_io_status("could not close the temporary scene file", errno);
        }

        if (::renameat(directory.get(), temp_name.c_str(), directory.get(), target_name.c_str()) != 0) {
            return posix_io_status("could not atomically replace the scene file", errno);
        }
        temporary_file.release();

        // The rename has already committed if this directory flush fails; report uncertain
        // crash durability rather than pretending it succeeded fully.
        if (!sync_descriptor(directory.get())) {
            return posix_io_status("scene file was replaced but its directory could not be flushed",
                                   errno);
        }
        return core::Status::success();
    }

    return core::Status::failure(
        core::ErrorCode::ResourceExhausted, "could not reserve a unique temporary scene filename");
}

[[nodiscard]] core::Status read_file_bounded(const std::filesystem::path& source,
                                             std::vector<std::byte>& output) {
    const int raw_file = ::open(source.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (raw_file < 0) {
        return posix_io_status("could not open the scene file", errno);
    }
    ScopedFd file{raw_file};

    struct stat information {};
    if (::fstat(file.get(), &information) != 0) {
        return posix_io_status("could not inspect the scene file", errno);
    }
    if (!S_ISREG(information.st_mode) || information.st_size < 0) {
        return core::Status::failure(
            core::ErrorCode::InvalidArgument, "scene path must refer to a regular file");
    }
    if (static_cast<std::uintmax_t>(information.st_size) >
        static_cast<std::uintmax_t>(SceneCodec::maximum_serialized_bytes)) {
        return core::Status::failure(
            core::ErrorCode::ResourceExhausted, "scene file exceeds the configured size limit");
    }

    try {
        std::vector<std::byte> encoded(static_cast<std::size_t>(information.st_size));
        std::size_t offset = 0U;
        while (offset < encoded.size()) {
            const ssize_t received =
                ::read(file.get(), encoded.data() + offset, encoded.size() - offset);
            if (received > 0) {
                offset += static_cast<std::size_t>(received);
                continue;
            }
            if (received < 0 && errno == EINTR) {
                continue;
            }
            return posix_io_status("scene file changed or ended while being read",
                                   received < 0 ? errno : EIO);
        }

        std::byte extra{};
        ssize_t received_extra = 0;
        do {
            received_extra = ::read(file.get(), &extra, 1U);
        } while (received_extra < 0 && errno == EINTR);
        if (received_extra < 0) {
            return posix_io_status("could not verify the end of the scene file", errno);
        }
        if (received_extra != 0) {
            return core::Status::failure(
                core::ErrorCode::Io, "scene file changed while being read");
        }

        output.swap(encoded);
    } catch (const std::bad_alloc&) {
        return resource_exhausted_status();
    }

    return core::Status::success();
}

#endif

} // namespace

core::Status SceneFile::save_atomic(const std::filesystem::path& destination,
                                    const Scene& scene) {
    try {
        if (!is_valid_file_path(destination)) {
            return invalid_path_status();
        }

        std::vector<std::byte> encoded;
        const core::Status encode_status = SceneCodec::serialize(scene, encoded);
        if (!encode_status.ok()) {
            return encode_status;
        }
        return write_atomic_native(destination, encoded);
    } catch (const std::bad_alloc&) {
        return resource_exhausted_status();
    } catch (const std::filesystem::filesystem_error&) {
        return core::Status::failure(core::ErrorCode::Io, "filesystem path operation failed");
    }
}

core::Status SceneFile::load(const std::filesystem::path& source, Scene& destination) {
    try {
        if (!is_valid_file_path(source)) {
            return invalid_path_status();
        }
        const core::Status scene_status = destination.validate();
        if (!scene_status.ok()) {
            return scene_status;
        }
        if (destination.entity_count() != 0U) {
            return core::Status::failure(
                core::ErrorCode::InvalidState, "destination scene must be empty");
        }

        std::vector<std::byte> encoded;
        const core::Status read_status = read_file_bounded(source, encoded);
        if (!read_status.ok()) {
            return read_status;
        }
        return SceneCodec::deserialize(encoded, destination);
    } catch (const std::bad_alloc&) {
        return resource_exhausted_status();
    } catch (const std::filesystem::filesystem_error&) {
        return core::Status::failure(core::ErrorCode::Io, "filesystem path operation failed");
    }
}

} // namespace storm::scene
