#pragma once

#include "storm/foundation/result.hpp"

#include <android/native_window.h>

#ifndef VK_USE_PLATFORM_ANDROID_KHR
#define VK_USE_PLATFORM_ANDROID_KHR
#endif
#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Storm::Platform::Android {

// A deliberately small Vulkan smoke renderer. It owns the Vulkan instance,
// device, Android surface, swapchain, and synchronization objects, and presents
// a real clear-only frame. It is not the engine's Sprite/3D renderer.
class VulkanClearRenderer final {
public:
    VulkanClearRenderer() = default;
    ~VulkanClearRenderer();

    VulkanClearRenderer(const VulkanClearRenderer&) = delete;
    VulkanClearRenderer& operator=(const VulkanClearRenderer&) = delete;
    VulkanClearRenderer(VulkanClearRenderer&&) = delete;
    VulkanClearRenderer& operator=(VulkanClearRenderer&&) = delete;

    [[nodiscard]] Result<void> initialize(ANativeWindow* window);
    [[nodiscard]] Result<void> render_frame();
    [[nodiscard]] Result<void> recreate_swapchain();
    void shutdown() noexcept;

    [[nodiscard]] bool is_initialized() const noexcept;

private:
    [[nodiscard]] Result<void> create_instance_and_surface();
    [[nodiscard]] Result<void> select_physical_device();
    [[nodiscard]] Result<void> create_device();
    [[nodiscard]] Result<void> create_swapchain_resources();
    void destroy_swapchain_resources() noexcept;

    ANativeWindow* window_{nullptr};
    VkInstance instance_{VK_NULL_HANDLE};
    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    std::uint32_t queue_family_index_{0U};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue queue_{VK_NULL_HANDLE};
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    VkFormat swapchain_format_{VK_FORMAT_UNDEFINED};
    VkExtent2D swapchain_extent_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
    std::vector<VkFramebuffer> framebuffers_;
    VkRenderPass render_pass_{VK_NULL_HANDLE};
    VkCommandPool command_pool_{VK_NULL_HANDLE};
    VkCommandBuffer command_buffer_{VK_NULL_HANDLE};
    VkSemaphore image_available_{VK_NULL_HANDLE};
    VkSemaphore render_finished_{VK_NULL_HANDLE};
    VkFence frame_fence_{VK_NULL_HANDLE};
};

} // namespace Storm::Platform::Android
