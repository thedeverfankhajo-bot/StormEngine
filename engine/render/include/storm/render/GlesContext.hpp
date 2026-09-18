#pragma once

namespace storm::render {

class GlesContext final {
public:
    GlesContext() = default;
    ~GlesContext();

    GlesContext(const GlesContext&) = delete;
    GlesContext& operator=(const GlesContext&) = delete;

    bool initializePbuffer(int width = 1, int height = 1) noexcept;
    bool initializeWindow(void* nativeWindow) noexcept;
    void shutdown() noexcept;
    bool makeCurrent() noexcept;
    bool swap() noexcept;

    [[nodiscard]] bool valid() const noexcept { return valid_; }

private:
    bool valid_{false};
    void* display_{nullptr};
    void* surface_{nullptr};
    void* context_{nullptr};
};

} // namespace storm::render
