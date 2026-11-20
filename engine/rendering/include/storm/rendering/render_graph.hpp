#pragma once

#include "storm/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Storm::Rendering {

enum class GraphTextureFormat : std::uint8_t {
    RGBA8Unorm,
    BGRA8Unorm,
    RGBA16Float,
    Depth32Float,
    Depth24Stencil8,
};

enum class GraphResourceAccess : std::uint8_t {
    Read,
    Write,
};

struct GraphResourceDesc final {
    std::uint32_t width{1U};
    std::uint32_t height{1U};
    std::uint32_t layers{1U};
    std::uint32_t samples{1U};
    GraphTextureFormat format{GraphTextureFormat::RGBA8Unorm};

    [[nodiscard]] friend bool operator==(
        const GraphResourceDesc&, const GraphResourceDesc&) noexcept = default;
};

struct GraphResourceHandle final {
    static constexpr std::uint32_t invalid_index = UINT32_MAX;
    std::uint32_t index{invalid_index};

    [[nodiscard]] bool is_valid() const noexcept {
        return index != invalid_index;
    }
    [[nodiscard]] friend bool operator==(
        const GraphResourceHandle&, const GraphResourceHandle&) noexcept = default;
};

struct GraphPassHandle final {
    static constexpr std::uint32_t invalid_index = UINT32_MAX;
    std::uint32_t index{invalid_index};

    [[nodiscard]] bool is_valid() const noexcept {
        return index != invalid_index;
    }
    [[nodiscard]] friend bool operator==(
        const GraphPassHandle&, const GraphPassHandle&) noexcept = default;
};

struct GraphResourceAccessDecl final {
    GraphResourceHandle resource{};
    GraphResourceAccess access{GraphResourceAccess::Read};
};

struct RenderGraphConfig final {
    std::size_t maximum_resources{128U};
    std::size_t maximum_passes{128U};
    std::size_t maximum_accesses{1024U};
    std::size_t maximum_dependencies{1024U};
    std::uint32_t maximum_extent{16384U};
    std::uint64_t maximum_resource_bytes{256ULL * 1024ULL * 1024ULL};
};

struct GraphResourceLifetime final {
    // Indices into the compiled execution order, not pass creation IDs.
    std::uint32_t first_pass_index{GraphResourceHandle::invalid_index};
    std::uint32_t last_pass_index{GraphResourceHandle::invalid_index};
    // Compatible transient resources with disjoint lifetimes may share a slot.
    // External and unused resources keep invalid_index.
    std::uint32_t alias_slot{GraphResourceHandle::invalid_index};

    [[nodiscard]] bool is_used() const noexcept {
        return first_pass_index != GraphResourceHandle::invalid_index;
    }
};

class RenderGraphPlan final {
public:
    [[nodiscard]] std::span<const GraphPassHandle> execution_order() const noexcept;
    [[nodiscard]] std::span<const GraphResourceLifetime> resource_lifetimes() const noexcept;
    [[nodiscard]] std::size_t transient_slot_count() const noexcept;

private:
    friend class RenderGraph;
    std::vector<GraphPassHandle> execution_order_;
    std::vector<GraphResourceLifetime> resource_lifetimes_;
    std::size_t transient_slot_count_{0U};
};

// Backend-independent render scheduling and transient-resource lifetime analysis.
// Handles are scoped to the RenderGraph instance that created them.
class RenderGraph final {
public:
    static constexpr std::size_t maximum_supported_resources = 1024U;
    static constexpr std::size_t maximum_supported_passes = 512U;
    static constexpr std::size_t maximum_supported_accesses = 8192U;
    static constexpr std::size_t maximum_supported_dependencies = 8192U;
    static constexpr std::size_t maximum_name_bytes = 63U;

    [[nodiscard]] static Result<RenderGraph> create(RenderGraphConfig config = {});

    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&& other) noexcept;
    RenderGraph& operator=(RenderGraph&& other) noexcept;

    [[nodiscard]] Result<GraphResourceHandle> create_resource(
        std::string_view name,
        GraphResourceDesc description,
        bool externally_initialized = false);

    [[nodiscard]] Result<GraphPassHandle> add_pass(
        std::string_view name,
        std::span<const GraphResourceAccessDecl> accesses = {});

    // The pass executes after its prerequisite. Resource read-after-write edges
    // are inferred during compile(); explicit edges express other ordering.
    [[nodiscard]] Result<void> add_dependency(
        GraphPassHandle pass,
        GraphPassHandle prerequisite);

    // The returned plan is a snapshot. Later graph mutations require a fresh
    // compile() before the plan is used for execution.
    [[nodiscard]] Result<RenderGraphPlan> compile() const;

    [[nodiscard]] std::size_t resource_count() const noexcept;
    [[nodiscard]] std::size_t pass_count() const noexcept;

private:
    struct ResourceRecord final {
        std::string name;
        GraphResourceDesc description{};
        bool externally_initialized{false};
    };

    struct PassRecord final {
        std::string name;
        std::size_t first_access{0U};
        std::size_t access_count{0U};
    };

    struct Dependency final {
        std::uint32_t prerequisite{0U};
        std::uint32_t pass{0U};
    };

    explicit RenderGraph(RenderGraphConfig config);
    [[nodiscard]] Result<void> ensure_initialized() const;
    [[nodiscard]] bool valid_name(std::string_view name) const noexcept;
    [[nodiscard]] bool valid_description(GraphResourceDesc description) const noexcept;

    RenderGraphConfig config_{};
    std::vector<ResourceRecord> resources_;
    std::vector<PassRecord> passes_;
    std::vector<GraphResourceAccessDecl> accesses_;
    std::vector<Dependency> dependencies_;
    bool initialized_{true};
};

} // namespace Storm::Rendering
