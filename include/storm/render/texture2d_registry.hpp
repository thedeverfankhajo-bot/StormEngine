#pragma once

#include "storm/core/stable_id.hpp"
#include "storm/core/status.hpp"
#include "storm/render/texture2d.hpp"

#include <cstddef>
#include <map>
#include <new>
#include <string>
#include <utility>

namespace storm::render {

// Owns a bounded set of CPU Texture2D values indexed by caller-supplied stable IDs.
// This in-memory registry is single-owner-thread only; it is not a persistent asset database.
class Texture2DRegistry final {
public:
    static constexpr std::size_t maximum_assets = 4'096U;
    static constexpr std::size_t maximum_pixels = Texture2D::maximum_pixels;

    // Limits may be reduced for project/runtime budgets. Values above engine hard limits are clamped.
    explicit Texture2DRegistry(
        std::size_t asset_limit = maximum_assets,
        std::size_t pixel_limit = maximum_pixels) noexcept
        : asset_limit_(asset_limit < maximum_assets ? asset_limit : maximum_assets),
          pixel_limit_(pixel_limit < maximum_pixels ? pixel_limit : maximum_pixels) {}

    // Copies the texture into engine ownership. Failed registration leaves the registry unchanged.
    [[nodiscard]] core::Status register_texture(const core::StableId& id,
                                                const Texture2D& texture) {
        if (!id.is_valid()) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument, "texture asset ID is invalid");
        }
        if (!is_valid_texture(texture)) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument, "texture asset data is empty or malformed");
        }
        if (textures_.find(id) != textures_.end()) {
            return core::Status::failure(
                core::ErrorCode::AlreadyExists, "texture asset ID is already registered");
        }
        if (textures_.size() >= asset_limit_) {
            return exhausted_status("texture asset count exceeds its configured limit");
        }

        const std::size_t pixels = texture.pixel_count();
        if (pixels > pixel_limit_ - total_pixels_) {
            return exhausted_status("texture assets exceed the configured pixel budget");
        }

        try {
            const auto inserted = textures_.emplace(id, texture);
            if (!inserted.second) {
                return core::Status::failure(
                    core::ErrorCode::AlreadyExists, "texture asset ID is already registered");
            }
        } catch (const std::bad_alloc&) {
            return exhausted_status({});
        }

        total_pixels_ += pixels;
        return core::Status::success();
    }

    [[nodiscard]] core::Status unregister_texture(const core::StableId& id) {
        if (!id.is_valid()) {
            return core::Status::failure(
                core::ErrorCode::InvalidArgument, "texture asset ID is invalid");
        }

        const auto found = textures_.find(id);
        if (found == textures_.end()) {
            return core::Status::failure(
                core::ErrorCode::NotFound, "texture asset ID is not registered");
        }

        total_pixels_ -= found->second.pixel_count();
        textures_.erase(found);
        return core::Status::success();
    }

    // Returned pointers are non-owning and remain valid until that asset is removed or the
    // registry is cleared/destroyed. Inserting a different asset does not invalidate map entries.
    [[nodiscard]] const Texture2D* find(const core::StableId& id) const noexcept {
        if (!id.is_valid()) {
            return nullptr;
        }
        const auto found = textures_.find(id);
        return found == textures_.end() ? nullptr : &found->second;
    }

    [[nodiscard]] bool contains(const core::StableId& id) const noexcept {
        return find(id) != nullptr;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return textures_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return textures_.empty();
    }

    [[nodiscard]] std::size_t total_pixels() const noexcept {
        return total_pixels_;
    }

    [[nodiscard]] std::size_t asset_limit() const noexcept {
        return asset_limit_;
    }

    [[nodiscard]] std::size_t pixel_limit() const noexcept {
        return pixel_limit_;
    }

    void clear() noexcept {
        textures_.clear();
        total_pixels_ = 0U;
    }

private:
    [[nodiscard]] static bool is_valid_texture(const Texture2D& texture) noexcept {
        if (texture.empty() || texture.width() == 0U || texture.height() == 0U ||
            texture.width() > Texture2D::maximum_dimension ||
            texture.height() > Texture2D::maximum_dimension ||
            texture.pixel_count() > Texture2D::maximum_pixels) {
            return false;
        }

        const std::size_t expected =
            static_cast<std::size_t>(texture.width()) *
            static_cast<std::size_t>(texture.height());
        return texture.pixel_count() == expected &&
               texture.pixels().size() == expected;
    }

    [[nodiscard]] static core::Status exhausted_status(std::string message) {
        return core::Status::failure(core::ErrorCode::ResourceExhausted, std::move(message));
    }

    std::size_t asset_limit_{};
    std::size_t pixel_limit_{};
    std::size_t total_pixels_{};
    std::map<core::StableId, Texture2D, core::StableIdLess> textures_{};
};

} // namespace storm::render
