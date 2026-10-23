#include "storm/core/stable_id_generator.hpp"

#include <cstddef>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#elif defined(__APPLE__)
#include <Security/SecRandom.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace storm::core {
namespace {

[[nodiscard]] Status fill_secure_random_bytes(StableId::Bytes& bytes) {
#if defined(_WIN32)
    const NTSTATUS result =
        ::BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()),
                          BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (result < 0) {
        return Status::failure(ErrorCode::Io, "Windows secure random generation failed");
    }
    return Status::success();
#elif defined(__APPLE__)
    const OSStatus result =
        ::SecRandomCopyBytes(kSecRandomDefault, bytes.size(), bytes.data());
    if (result != errSecSuccess) {
        return Status::failure(ErrorCode::Io, "Apple secure random generation failed");
    }
    return Status::success();
#else
    int descriptor = -1;
    do {
        descriptor = ::open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    } while (descriptor < 0 && errno == EINTR);

    if (descriptor < 0) {
        return Status::failure(ErrorCode::Io, "system secure random source could not be opened");
    }

    class ScopedFile final {
    public:
        explicit ScopedFile(int descriptor) noexcept : descriptor_(descriptor) {}
        ~ScopedFile() {
            if (descriptor_ >= 0) {
                (void)::close(descriptor_);
            }
        }

        ScopedFile(const ScopedFile&) = delete;
        ScopedFile& operator=(const ScopedFile&) = delete;

        [[nodiscard]] int get() const noexcept {
            return descriptor_;
        }

    private:
        int descriptor_;
    } source{descriptor};

    std::size_t offset = 0U;
    while (offset < bytes.size()) {
        const ssize_t count =
            ::read(source.get(), bytes.data() + offset, bytes.size() - offset);
        if (count > 0) {
            offset += static_cast<std::size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        return Status::failure(ErrorCode::Io, "system secure random source could not be read");
    }

    return Status::success();
#endif
}

} // namespace

Status StableIdGenerator::generate_v4(StableId& output) {
    StableId::Bytes bytes{};
    const Status random_status = fill_secure_random_bytes(bytes);
    if (!random_status.ok()) {
        return random_status;
    }

    // RFC 9562: version 4 in octet 6, RFC variant (10xxxxxx) in octet 8.
    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0FU) | 0x40U);
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3FU) | 0x80U);

    const auto generated = StableId::from_bytes(bytes);
    if (!generated.has_value()) {
        return Status::failure(ErrorCode::Internal, "secure UUID generation produced a Nil ID");
    }

    output = *generated;
    return Status::success();
}

} // namespace storm::core
