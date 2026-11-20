#include "vulkan_clear_renderer.hpp"

#include "storm/rendering/presentation_policy.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

namespace Storm::Platform::Android {
namespace {

constexpr std::uint32_t kMaximumPhysicalDevices = 64U;
constexpr std::uint32_t kMaximumQueueFamilies = 64U;
constexpr std::uint32_t kMaximumDeviceExtensions = 1024U;
constexpr std::uint32_t kMaximumSurfaceFormats = 256U;
constexpr std::uint32_t kMaximumSwapchainImages = 8U;

[[nodiscard]] Error vk_error(const char* operation, VkResult result) {
    return Error::make(
        ErrorCode::IoFailure,
        std::string(operation) + " failed with VkResult " +
            std::to_string(static_cast<int>(result)));
}

[[nodiscard]] bool has_extension(
    VkPhysicalDevice device, const char* requested) {
    std::uint32_t count = 0U;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS ||
        count == 0U || count > kMaximumDeviceExtensions) {
        return false;
    }
    std::vector<VkExtensionProperties> extensions(count);
    const VkResult result =
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
    if (result != VK_SUCCESS || count > extensions.size()) {
        return false;
    }
    extensions.resize(count);
    return std::any_of(extensions.begin(), extensions.end(),
        [requested](const VkExtensionProperties& extension) {
            return std::strcmp(extension.extensionName, requested) == 0;
        });
}

} // namespace

VulkanClearRenderer::~VulkanClearRenderer() {
    shutdown();
}

bool VulkanClearRenderer::is_initialized() const noexcept {
    return device_ != VK_NULL_HANDLE && swapchain_ != VK_NULL_HANDLE;
}

Result<void> VulkanClearRenderer::initialize(ANativeWindow* window) {
    if (window == nullptr) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidArgument, "Vulkan renderer requires a valid ANativeWindow"));
    }
    if (device_ != VK_NULL_HANDLE || instance_ != VK_NULL_HANDLE) {
        return Result<void>::failure(Error::make(
            ErrorCode::InvalidState, "Vulkan renderer is already initialized"));
    }

    window_ = window;
    ANativeWindow_acquire(window_);
    try {
        auto result = create_instance_and_surface();
        if (result) result = select_physical_device();
        if (result) result = create_device();
        if (result) result = create_swapchain_resources();
        if (!result) shutdown();
        return result;
    } catch (const std::bad_alloc&) {
        shutdown();
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Vulkan initialization exceeded the available host memory"));
    } catch (const std::length_error&) {
        shutdown();
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Vulkan initialization exceeded a container capacity"));
    }
}

Result<void> VulkanClearRenderer::create_instance_and_surface() {
    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };
    VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    application.pApplicationName = "StormEngine";
    application.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    application.pEngineName = "StormEngine";
    application.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    application.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create.pApplicationInfo = &application;
    create.enabledExtensionCount = 2U;
    create.ppEnabledExtensionNames = extensions;
    const VkResult created = vkCreateInstance(&create, nullptr, &instance_);
    if (created != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateInstance", created));

    VkAndroidSurfaceCreateInfoKHR surface_info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    surface_info.window = window_;
    const VkResult surface_result =
        vkCreateAndroidSurfaceKHR(instance_, &surface_info, nullptr, &surface_);
    if (surface_result != VK_SUCCESS) {
        return Result<void>::failure(vk_error("vkCreateAndroidSurfaceKHR", surface_result));
    }
    return Result<void>::success();
}

Result<void> VulkanClearRenderer::select_physical_device() {
    std::uint32_t count = 0U;
    VkResult result = vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkEnumeratePhysicalDevices", result));
    if (count == 0U) return Result<void>::failure(Error::make(
        ErrorCode::Unsupported, "No Vulkan physical device is available"));
    if (count > kMaximumPhysicalDevices) return Result<void>::failure(Error::make(
        ErrorCode::ResourceExhausted,
        "Vulkan driver reported an unreasonable physical-device count"));

    std::vector<VkPhysicalDevice> devices(count);
    result = vkEnumeratePhysicalDevices(instance_, &count, devices.data());
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkEnumeratePhysicalDevices", result));
    if (count > devices.size()) return Result<void>::failure(Error::make(
        ErrorCode::ResourceExhausted,
        "Vulkan physical-device count changed beyond the allocated bound"));
    devices.resize(count);

    for (VkPhysicalDevice candidate : devices) {
        if (!has_extension(candidate, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;
        std::uint32_t family_count = 0U;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &family_count, nullptr);
        if (family_count == 0U || family_count > kMaximumQueueFamilies) continue;
        std::vector<VkQueueFamilyProperties> families(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &family_count, families.data());
        if (family_count > families.size()) continue;
        for (std::uint32_t index = 0U; index < family_count; ++index) {
            VkBool32 supported = VK_FALSE;
            const VkResult support_result = vkGetPhysicalDeviceSurfaceSupportKHR(
                candidate, index, surface_, &supported);
            if (support_result == VK_SUCCESS &&
                supported == VK_TRUE &&
                (families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
                physical_device_ = candidate;
                queue_family_index_ = index;
                return Result<void>::success();
            }
        }
    }
    return Result<void>::failure(Error::make(
        ErrorCode::Unsupported, "No Vulkan graphics queue supports Android presentation"));
}

Result<void> VulkanClearRenderer::create_device() {
    constexpr float priority = 1.0F;
    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = queue_family_index_;
    queue_info.queueCount = 1U;
    queue_info.pQueuePriorities = &priority;

    const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo create{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    create.queueCreateInfoCount = 1U;
    create.pQueueCreateInfos = &queue_info;
    create.enabledExtensionCount = 1U;
    create.ppEnabledExtensionNames = extensions;
    const VkResult result = vkCreateDevice(physical_device_, &create, nullptr, &device_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateDevice", result));
    vkGetDeviceQueue(device_, queue_family_index_, 0U, &queue_);
    if (queue_ == VK_NULL_HANDLE) return Result<void>::failure(Error::make(
        ErrorCode::Internal, "Vulkan returned a null presentation queue"));
    return Result<void>::success();
}

Result<void> VulkanClearRenderer::create_swapchain_resources() {
    VkSurfaceCapabilitiesKHR capabilities{};
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physical_device_, surface_, &capabilities);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", result));

    std::uint32_t format_count = 0U;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, nullptr);
    if (result != VK_SUCCESS || format_count == 0U) return Result<void>::failure(
        result == VK_SUCCESS
            ? Error::make(ErrorCode::Unsupported, "Android Vulkan surface has no formats")
            : vk_error("vkGetPhysicalDeviceSurfaceFormatsKHR", result));
    if (format_count > kMaximumSurfaceFormats) return Result<void>::failure(Error::make(
        ErrorCode::ResourceExhausted,
        "Vulkan driver reported an unreasonable surface-format count"));
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, formats.data());
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkGetPhysicalDeviceSurfaceFormatsKHR", result));
    if (format_count > formats.size()) return Result<void>::failure(Error::make(
        ErrorCode::ResourceExhausted,
        "Vulkan surface-format count changed beyond the allocated bound"));
    formats.resize(format_count);

    if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0U) {
        return Result<void>::failure(Error::make(
            ErrorCode::Unsupported,
            "Android Vulkan surface images cannot be used as color attachments"));
    }

    VkSurfaceFormatKHR selected{};
    if (formats.size() == 1U && formats.front().format == VK_FORMAT_UNDEFINED) {
        selected = VkSurfaceFormatKHR{
            VK_FORMAT_R8G8B8A8_UNORM, formats.front().colorSpace};
    } else {
        constexpr std::array<VkFormat, 2U> preferred_formats{
            VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM};
        for (const VkFormat preferred : preferred_formats) {
            const auto match = std::find_if(formats.begin(), formats.end(),
                [preferred](const VkSurfaceFormatKHR& format) {
                    return format.format == preferred;
                });
            if (match != formats.end()) {
                selected = *match;
                break;
            }
        }
    }
    if (selected.format == VK_FORMAT_UNDEFINED) {
        return Result<void>::failure(Error::make(
            ErrorCode::Unsupported,
            "Android Vulkan surface exposes no supported 8-bit RGBA/BGRA format"));
    }

    const std::int32_t window_width = ANativeWindow_getWidth(window_);
    const std::int32_t window_height = ANativeWindow_getHeight(window_);
    if (window_width <= 0 || window_height <= 0) return Result<void>::failure(Error::make(
        ErrorCode::InvalidState, "Android window has an invalid drawable extent"));
    const auto selected_extent = Storm::Rendering::resolve_presentation_extent(
        Storm::Rendering::PresentationExtent{
            static_cast<std::uint32_t>(window_width),
            static_cast<std::uint32_t>(window_height)},
        Storm::Rendering::PresentationExtent{
            capabilities.currentExtent.width, capabilities.currentExtent.height},
        Storm::Rendering::PresentationExtent{
            capabilities.minImageExtent.width, capabilities.minImageExtent.height},
        Storm::Rendering::PresentationExtent{
            capabilities.maxImageExtent.width, capabilities.maxImageExtent.height});
    if (!selected_extent) {
        return Result<void>::failure(selected_extent.error());
    }
    const VkExtent2D extent{
        selected_extent.value().width, selected_extent.value().height};

    const auto selected_image_count = Storm::Rendering::choose_presentation_image_count(
        capabilities.minImageCount, capabilities.maxImageCount, kMaximumSwapchainImages);
    if (!selected_image_count) {
        return Result<void>::failure(selected_image_count.error());
    }
    std::uint32_t image_count = selected_image_count.value();

    VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    constexpr std::array<VkCompositeAlphaFlagBitsKHR, 4> composite_options{
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };
    bool composite_alpha_selected = false;
    for (const auto option : composite_options) {
        if ((capabilities.supportedCompositeAlpha & option) != 0U) {
            composite = option;
            composite_alpha_selected = true;
            break;
        }
    }
    if (!composite_alpha_selected) {
        return Result<void>::failure(Error::make(
            ErrorCode::Unsupported,
            "Android Vulkan surface exposes no supported composite-alpha mode"));
    }

    VkSwapchainCreateInfoKHR swapchain_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_info.surface = surface_;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = selected.format;
    swapchain_info.imageColorSpace = selected.colorSpace;
    swapchain_info.imageExtent = extent;
    swapchain_info.imageArrayLayers = 1U;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.preTransform = capabilities.currentTransform;
    swapchain_info.compositeAlpha = composite;
    swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_info.clipped = VK_TRUE;
    result = vkCreateSwapchainKHR(device_, &swapchain_info, nullptr, &swapchain_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateSwapchainKHR", result));
    swapchain_format_ = selected.format;
    swapchain_extent_ = extent;

    result = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
    if (result != VK_SUCCESS || image_count == 0U) return Result<void>::failure(
        result == VK_SUCCESS ? Error::make(ErrorCode::Internal, "Vulkan swapchain returned no images")
                             : vk_error("vkGetSwapchainImagesKHR", result));
    if (image_count > kMaximumSwapchainImages) return Result<void>::failure(Error::make(
        ErrorCode::ResourceExhausted,
        "Vulkan swapchain image count exceeds the configured mobile bound"));
    images_.resize(image_count);
    result = vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, images_.data());
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkGetSwapchainImagesKHR", result));
    if (image_count == 0U || image_count > images_.size()) return Result<void>::failure(Error::make(
        ErrorCode::ResourceExhausted,
        "Vulkan swapchain image count changed beyond the allocated bound"));
    images_.resize(image_count);

    image_views_.reserve(images_.size());
    framebuffers_.reserve(images_.size());
    for (VkImage image : images_) {
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = swapchain_format_;
        view.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                           VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U};
        result = vkCreateImageView(device_, &view, nullptr, &image_views_.emplace_back());
        if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateImageView", result));
    }

    VkAttachmentDescription attachment{};
    attachment.format = swapchain_format_;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference reference{0U, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1U;
    subpass.pColorAttachments = &reference;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0U;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass.attachmentCount = 1U;
    pass.pAttachments = &attachment;
    pass.subpassCount = 1U;
    pass.pSubpasses = &subpass;
    pass.dependencyCount = 1U;
    pass.pDependencies = &dependency;
    result = vkCreateRenderPass(device_, &pass, nullptr, &render_pass_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateRenderPass", result));

    for (VkImageView image_view : image_views_) {
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer.renderPass = render_pass_;
        framebuffer.attachmentCount = 1U;
        framebuffer.pAttachments = &image_view;
        framebuffer.width = extent.width;
        framebuffer.height = extent.height;
        framebuffer.layers = 1U;
        result = vkCreateFramebuffer(device_, &framebuffer, nullptr, &framebuffers_.emplace_back());
        if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateFramebuffer", result));
    }

    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = queue_family_index_;
    result = vkCreateCommandPool(device_, &pool, nullptr, &command_pool_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateCommandPool", result));
    VkCommandBufferAllocateInfo command{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command.commandPool = command_pool_;
    command.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command.commandBufferCount = 1U;
    result = vkAllocateCommandBuffers(device_, &command, &command_buffer_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkAllocateCommandBuffers", result));

    VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    result = vkCreateSemaphore(device_, &semaphore, nullptr, &image_available_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateSemaphore(image)", result));
    result = vkCreateSemaphore(device_, &semaphore, nullptr, &render_finished_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateSemaphore(render)", result));
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    result = vkCreateFence(device_, &fence, nullptr, &frame_fence_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkCreateFence", result));
    return Result<void>::success();
}

Result<void> VulkanClearRenderer::render_frame() {
    if (!is_initialized()) return Result<void>::failure(Error::make(
        ErrorCode::InvalidState, "Vulkan clear renderer is not initialized"));
    VkResult result = vkWaitForFences(device_, 1U, &frame_fence_, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkWaitForFences", result));
    std::uint32_t image_index = 0U;
    result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_, VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) return recreate_swapchain();
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) return Result<void>::failure(vk_error("vkAcquireNextImageKHR", result));
    const bool acquired_suboptimal = result == VK_SUBOPTIMAL_KHR;
    if (static_cast<std::size_t>(image_index) >= framebuffers_.size()) {
        return Result<void>::failure(Error::make(
            ErrorCode::Internal,
            "Vulkan acquired an image index outside the swapchain framebuffer range"));
    }

    result = vkResetFences(device_, 1U, &frame_fence_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkResetFences", result));
    result = vkResetCommandBuffer(command_buffer_, 0U);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkResetCommandBuffer", result));

    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(command_buffer_, &begin);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkBeginCommandBuffer", result));
    VkClearValue clear{};
    clear.color = {{0.035F, 0.055F, 0.09F, 1.0F}};
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = render_pass_;
    pass.framebuffer = framebuffers_[image_index];
    pass.renderArea.extent = swapchain_extent_;
    pass.clearValueCount = 1U;
    pass.pClearValues = &clear;
    vkCmdBeginRenderPass(command_buffer_, &pass, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(command_buffer_);
    result = vkEndCommandBuffer(command_buffer_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkEndCommandBuffer", result));

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1U;
    submit.pWaitSemaphores = &image_available_;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1U;
    submit.pCommandBuffers = &command_buffer_;
    submit.signalSemaphoreCount = 1U;
    submit.pSignalSemaphores = &render_finished_;
    result = vkQueueSubmit(queue_, 1U, &submit, frame_fence_);
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkQueueSubmit", result));

    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1U;
    present.pWaitSemaphores = &render_finished_;
    present.swapchainCount = 1U;
    present.pSwapchains = &swapchain_;
    present.pImageIndices = &image_index;
    result = vkQueuePresentKHR(queue_, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        acquired_suboptimal) {
        return recreate_swapchain();
    }
    if (result != VK_SUCCESS) return Result<void>::failure(vk_error("vkQueuePresentKHR", result));
    return Result<void>::success();
}

void VulkanClearRenderer::destroy_swapchain_resources() noexcept {
    if (device_ == VK_NULL_HANDLE) return;
    static_cast<void>(vkDeviceWaitIdle(device_));
    if (frame_fence_ != VK_NULL_HANDLE) vkDestroyFence(device_, frame_fence_, nullptr);
    if (render_finished_ != VK_NULL_HANDLE) vkDestroySemaphore(device_, render_finished_, nullptr);
    if (image_available_ != VK_NULL_HANDLE) vkDestroySemaphore(device_, image_available_, nullptr);
    if (command_pool_ != VK_NULL_HANDLE) vkDestroyCommandPool(device_, command_pool_, nullptr);
    for (VkFramebuffer framebuffer : framebuffers_) vkDestroyFramebuffer(device_, framebuffer, nullptr);
    if (render_pass_ != VK_NULL_HANDLE) vkDestroyRenderPass(device_, render_pass_, nullptr);
    for (VkImageView view : image_views_) vkDestroyImageView(device_, view, nullptr);
    if (swapchain_ != VK_NULL_HANDLE) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    frame_fence_ = VK_NULL_HANDLE;
    render_finished_ = VK_NULL_HANDLE;
    image_available_ = VK_NULL_HANDLE;
    command_pool_ = VK_NULL_HANDLE;
    command_buffer_ = VK_NULL_HANDLE;
    framebuffers_.clear();
    image_views_.clear();
    images_.clear();
    render_pass_ = VK_NULL_HANDLE;
    swapchain_ = VK_NULL_HANDLE;
    swapchain_format_ = VK_FORMAT_UNDEFINED;
    swapchain_extent_ = {};
}

Result<void> VulkanClearRenderer::recreate_swapchain() {
    if (device_ == VK_NULL_HANDLE || surface_ == VK_NULL_HANDLE || window_ == nullptr) {
        return Result<void>::failure(Error::make(ErrorCode::InvalidState, "Cannot recreate an inactive Vulkan swapchain"));
    }
    const std::int32_t width = ANativeWindow_getWidth(window_);
    const std::int32_t height = ANativeWindow_getHeight(window_);
    if (width <= 0 || height <= 0) {
        return Result<void>::failure(Error::make(ErrorCode::InvalidState, "Cannot recreate Vulkan swapchain for an empty Android window"));
    }
    destroy_swapchain_resources();
    try {
        auto recreated = create_swapchain_resources();
        if (!recreated) {
            // Swapchain construction is incremental; roll back every handle
            // created before the failing Vulkan operation.
            destroy_swapchain_resources();
        }
        return recreated;
    } catch (const std::bad_alloc&) {
        destroy_swapchain_resources();
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Vulkan swapchain recreation exceeded the available host memory"));
    } catch (const std::length_error&) {
        destroy_swapchain_resources();
        return Result<void>::failure(Error::make(
            ErrorCode::ResourceExhausted,
            "Vulkan swapchain recreation exceeded a container capacity"));
    }
}

void VulkanClearRenderer::shutdown() noexcept {
    if (device_ != VK_NULL_HANDLE) {
        destroy_swapchain_resources();
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
        queue_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE && surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    physical_device_ = VK_NULL_HANDLE;
    if (window_ != nullptr) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
}

} // namespace Storm::Platform::Android
