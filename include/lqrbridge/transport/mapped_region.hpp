#pragma once
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <system_error>
#include <utility>
            
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace lqr {
    // RAII map of the DDR L2 cache window into this process.
    // FPGA sends state cache alligned over ACP coherent in L2,]
    // so we map those lines once off the hotpath.
    // Error loudly if we can't create the mapping!
    class MappedRegion {
        int fd_ = -1;
        void* base_ = nullptr;
        std::size_t len_ = 0;

        // Adopt the acquired handles.
        // Called only by our map
        MappedRegion(int fd, void* base, std::size_t len) noexcept :
            fd_(fd), base_(base), len_(len) {}

        void reset() noexcept {
            if (base_ && base_ != MAP_FAILED) {
                ::munmap(base_, len_);
            }

            if (fd_ >= 0) {
                ::close(fd_);
            }

            //
            base_ = nullptr;
            fd_ = -1;
            len_ = 0;
        }

    public:
        // Rule of 5
        MappedRegion() noexcept = default; // build null, fill with mapping

        ~MappedRegion() {
            reset(); // destroy mapping
        }

        // There's no way to copy the memory map => forbidden
        MappedRegion(const MappedRegion&) = delete; // copy ctor
        MappedRegion& operator=(const MappedRegion&) = delete; // copy assign

        MappedRegion(MappedRegion&& o) noexcept : // move ctor
            fd_(std::exchange(o.fd_, -1)), // set old val and return it
            base_(std::exchange(o.base_, nullptr)),
            len_(std::exchange(o.len_, 0)) {}

        MappedRegion& operator=(MappedRegion&& o) noexcept { // move copy
            if (this != &o) {
                reset();
                fd_ = std::exchange(o.fd_, -1);
                base_ = std::exchange(o.base_, nullptr);
                len_ = std::exchange(o.len_, 0);
            }
            return *this;
        }

        // Mapping factory
        [[nodiscard]] static std::expected<MappedRegion, std::error_code>
        map(std::uintptr_t phys, std::size_t len) noexcept {
            const int fd = ::open("/dev/mem", O_RDONLY | O_CLOEXEC);
            if (fd < 0) {
                return std::unexpected(
                    std::error_code(errno, std::generic_category())
                );
            }
            void* const base = ::mmap(
                nullptr,
                len,
                PROT_READ,
                MAP_SHARED,
                fd,
                static_cast<off_t>(phys)
            );

            if (base == MAP_FAILED) {
                const int e = errno;
                ::close(fd);
                return std::unexpected(
                    std::error_code(e, std::generic_category())
                );
            }
            return MappedRegion(fd, base, len);
        }

        // FPGA mutates this => always re-read, never try to cache!
        [[nodiscard]] const volatile std::uint32_t* words() const noexcept {
            return static_cast<const volatile std::uint32_t*>(base_);
        }
        [[nodiscard]] std::size_t size() const noexcept {
            return len_;
        }
    };
}