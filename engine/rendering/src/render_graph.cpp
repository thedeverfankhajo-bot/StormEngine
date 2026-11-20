#include "storm/rendering/render_graph.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace Storm::Rendering {
namespace {

[[nodiscard]] Error invalid_argument(const char* message) {
    return Error::make(ErrorCode::InvalidArgument, message);
}
[[nodiscard]] Error invalid_state(const char* message) {
    return Error::make(ErrorCode::InvalidState, message);
}
[[nodiscard]] Error exhausted(const char* message) {
    return Error::make(ErrorCode::ResourceExhausted, message);
}

[[nodiscard]] bool bytes_per_pixel(GraphTextureFormat format, std::uint64_t& bytes) noexcept {
    switch (format) {
    case GraphTextureFormat::RGBA8Unorm:
    case GraphTextureFormat::BGRA8Unorm:
    case GraphTextureFormat::Depth32Float:
    case GraphTextureFormat::Depth24Stencil8:
        bytes = 4U;
        return true;
    case GraphTextureFormat::RGBA16Float:
        bytes = 8U;
        return true;
    }
    return false;
}

[[nodiscard]] bool multiply_checked(
    std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& result) noexcept {
    if (rhs != 0U && lhs > std::numeric_limits<std::uint64_t>::max() / rhs) {
        return false;
    }
    result = lhs * rhs;
    return true;
}

} // namespace

std::span<const GraphPassHandle> RenderGraphPlan::execution_order() const noexcept {
    return execution_order_;
}

std::span<const GraphResourceLifetime> RenderGraphPlan::resource_lifetimes() const noexcept {
    return resource_lifetimes_;
}

std::size_t RenderGraphPlan::transient_slot_count() const noexcept {
    return transient_slot_count_;
}

RenderGraph::RenderGraph(RenderGraphConfig config) : config_(config) {
    resources_.reserve(config_.maximum_resources);
    passes_.reserve(config_.maximum_passes);
    accesses_.reserve(config_.maximum_accesses);
    dependencies_.reserve(config_.maximum_dependencies);
}

RenderGraph::RenderGraph(RenderGraph&& other) noexcept
    : config_(other.config_),
      resources_(std::move(other.resources_)),
      passes_(std::move(other.passes_)),
      accesses_(std::move(other.accesses_)),
      dependencies_(std::move(other.dependencies_)),
      initialized_(std::exchange(other.initialized_, false)) {}

RenderGraph& RenderGraph::operator=(RenderGraph&& other) noexcept {
    if (this != &other) {
        config_ = other.config_;
        resources_ = std::move(other.resources_);
        passes_ = std::move(other.passes_);
        accesses_ = std::move(other.accesses_);
        dependencies_ = std::move(other.dependencies_);
        initialized_ = std::exchange(other.initialized_, false);
    }
    return *this;
}

Result<RenderGraph> RenderGraph::create(RenderGraphConfig config) {
    if (config.maximum_resources == 0U ||
        config.maximum_resources > maximum_supported_resources ||
        config.maximum_passes == 0U ||
        config.maximum_passes > maximum_supported_passes ||
        config.maximum_accesses == 0U ||
        config.maximum_accesses > maximum_supported_accesses ||
        config.maximum_dependencies > maximum_supported_dependencies ||
        config.maximum_extent == 0U ||
        config.maximum_resource_bytes == 0U) {
        return Result<RenderGraph>::failure(invalid_argument(
            "RenderGraph limits are zero or exceed the supported bounds"));
    }

    try {
        return Result<RenderGraph>::success(RenderGraph{config});
    } catch (const std::bad_alloc&) {
        return Result<RenderGraph>::failure(exhausted(
            "Unable to reserve bounded RenderGraph storage"));
    } catch (const std::length_error&) {
        return Result<RenderGraph>::failure(exhausted(
            "RenderGraph limits exceed container capacity"));
    }
}

Result<void> RenderGraph::ensure_initialized() const {
    if (!initialized_) {
        return Result<void>::failure(invalid_state(
            "A moved-from RenderGraph cannot be used"));
    }
    return Result<void>::success();
}

bool RenderGraph::valid_name(std::string_view name) const noexcept {
    if (name.empty() || name.size() > maximum_name_bytes) {
        return false;
    }
    for (const char character : name) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte < 0x20U || byte == 0x7fU) {
            return false;
        }
    }
    return true;
}

bool RenderGraph::valid_description(GraphResourceDesc description) const noexcept {
    if (description.width == 0U || description.height == 0U ||
        description.layers == 0U || description.width > config_.maximum_extent ||
        description.height > config_.maximum_extent ||
        description.layers > config_.maximum_extent) {
        return false;
    }
    if (description.samples != 1U && description.samples != 2U &&
        description.samples != 4U && description.samples != 8U) {
        return false;
    }

    std::uint64_t texel_bytes = 0U;
    if (!bytes_per_pixel(description.format, texel_bytes)) {
        return false;
    }

    std::uint64_t total = description.width;
    if (!multiply_checked(total, description.height, total) ||
        !multiply_checked(total, description.layers, total) ||
        !multiply_checked(total, description.samples, total) ||
        !multiply_checked(total, texel_bytes, total)) {
        return false;
    }
    return total <= config_.maximum_resource_bytes;
}

Result<GraphResourceHandle> RenderGraph::create_resource(
    std::string_view name,
    GraphResourceDesc description,
    bool externally_initialized) {
    const auto initialized = ensure_initialized();
    if (!initialized) {
        return Result<GraphResourceHandle>::failure(initialized.error());
    }
    if (!valid_name(name) || !valid_description(description)) {
        return Result<GraphResourceHandle>::failure(invalid_argument(
            "RenderGraph resource name or texture description is invalid"));
    }
    if (resources_.size() >= config_.maximum_resources) {
        return Result<GraphResourceHandle>::failure(exhausted(
            "RenderGraph resource capacity is exhausted"));
    }
    if (resources_.size() >= static_cast<std::size_t>(GraphResourceHandle::invalid_index)) {
        return Result<GraphResourceHandle>::failure(exhausted(
            "RenderGraph resource handle space is exhausted"));
    }
    if (std::any_of(resources_.begin(), resources_.end(),
        [name](const ResourceRecord& resource) { return resource.name == name; })) {
        return Result<GraphResourceHandle>::failure(invalid_argument(
            "RenderGraph resource names must be unique"));
    }

    try {
        std::string owned_name{name};
        const auto index = static_cast<std::uint32_t>(resources_.size());
        resources_.push_back(ResourceRecord{
            .name = std::move(owned_name),
            .description = description,
            .externally_initialized = externally_initialized});
        return Result<GraphResourceHandle>::success(GraphResourceHandle{index});
    } catch (const std::bad_alloc&) {
        return Result<GraphResourceHandle>::failure(exhausted(
            "Unable to allocate RenderGraph resource metadata"));
    } catch (const std::length_error&) {
        return Result<GraphResourceHandle>::failure(exhausted(
            "RenderGraph resource metadata exceeds container limits"));
    }
}

Result<GraphPassHandle> RenderGraph::add_pass(
    std::string_view name,
    std::span<const GraphResourceAccessDecl> accesses) {
    const auto initialized = ensure_initialized();
    if (!initialized) {
        return Result<GraphPassHandle>::failure(initialized.error());
    }
    if (!valid_name(name)) {
        return Result<GraphPassHandle>::failure(invalid_argument(
            "RenderGraph pass name is empty, too long, or contains control characters"));
    }
    if (passes_.size() >= config_.maximum_passes ||
        passes_.size() >= static_cast<std::size_t>(GraphPassHandle::invalid_index)) {
        return Result<GraphPassHandle>::failure(exhausted(
            "RenderGraph pass capacity is exhausted"));
    }
    if (accesses.size() > config_.maximum_accesses - accesses_.size()) {
        return Result<GraphPassHandle>::failure(exhausted(
            "RenderGraph access declaration capacity is exhausted"));
    }
    if (std::any_of(passes_.begin(), passes_.end(),
        [name](const PassRecord& pass) { return pass.name == name; })) {
        return Result<GraphPassHandle>::failure(invalid_argument(
            "RenderGraph pass names must be unique"));
    }

    for (std::size_t i = 0U; i < accesses.size(); ++i) {
        const GraphResourceAccessDecl& access = accesses[i];
        if (!access.resource.is_valid() ||
            static_cast<std::size_t>(access.resource.index) >= resources_.size()) {
            return Result<GraphPassHandle>::failure(invalid_argument(
                "RenderGraph pass references an invalid resource handle"));
        }
        if (access.access != GraphResourceAccess::Read &&
            access.access != GraphResourceAccess::Write) {
            return Result<GraphPassHandle>::failure(invalid_argument(
                "RenderGraph pass contains an unknown resource access mode"));
        }
        for (std::size_t previous = 0U; previous < i; ++previous) {
            if (accesses[previous].resource == access.resource) {
                return Result<GraphPassHandle>::failure(invalid_argument(
                    "A RenderGraph pass may declare each resource only once"));
            }
        }
    }

    try {
        std::string owned_name{name};
        const auto index = static_cast<std::uint32_t>(passes_.size());
        const std::size_t first_access = accesses_.size();
        passes_.push_back(PassRecord{
            .name = std::move(owned_name),
            .first_access = first_access,
            .access_count = accesses.size()});
        accesses_.insert(accesses_.end(), accesses.begin(), accesses.end());
        return Result<GraphPassHandle>::success(GraphPassHandle{index});
    } catch (const std::bad_alloc&) {
        return Result<GraphPassHandle>::failure(exhausted(
            "Unable to allocate RenderGraph pass metadata"));
    } catch (const std::length_error&) {
        return Result<GraphPassHandle>::failure(exhausted(
            "RenderGraph pass metadata exceeds container limits"));
    }
}

Result<void> RenderGraph::add_dependency(
    GraphPassHandle pass,
    GraphPassHandle prerequisite) {
    const auto initialized = ensure_initialized();
    if (!initialized) {
        return initialized;
    }
    if (!pass.is_valid() || !prerequisite.is_valid() ||
        static_cast<std::size_t>(pass.index) >= passes_.size() ||
        static_cast<std::size_t>(prerequisite.index) >= passes_.size()) {
        return Result<void>::failure(invalid_argument(
            "RenderGraph dependency references an invalid pass handle"));
    }
    if (pass == prerequisite) {
        return Result<void>::failure(invalid_argument(
            "A RenderGraph pass cannot depend on itself"));
    }
    if (dependencies_.size() >= config_.maximum_dependencies) {
        return Result<void>::failure(exhausted(
            "RenderGraph dependency capacity is exhausted"));
    }
    if (std::any_of(dependencies_.begin(), dependencies_.end(),
        [pass, prerequisite](const Dependency& dependency) {
            return dependency.pass == pass.index &&
                   dependency.prerequisite == prerequisite.index;
        })) {
        return Result<void>::failure(invalid_argument(
            "The RenderGraph dependency is already declared"));
    }

    dependencies_.push_back(Dependency{
        .prerequisite = prerequisite.index,
        .pass = pass.index});
    return Result<void>::success();
}

Result<RenderGraphPlan> RenderGraph::compile() const {
    const auto initialized = ensure_initialized();
    if (!initialized) {
        return Result<RenderGraphPlan>::failure(initialized.error());
    }

    try {
        const std::size_t pass_count = passes_.size();
        const std::size_t resource_count = resources_.size();
        const std::uint32_t invalid = GraphResourceHandle::invalid_index;

        std::vector<std::uint32_t> writers(resource_count, invalid);
        for (std::size_t pass_index = 0U; pass_index < pass_count; ++pass_index) {
            const PassRecord& pass = passes_[pass_index];
            for (std::size_t offset = 0U; offset < pass.access_count; ++offset) {
                const GraphResourceAccessDecl& access =
                    accesses_[pass.first_access + offset];
                if (access.access == GraphResourceAccess::Write) {
                    std::uint32_t& writer = writers[access.resource.index];
                    if (writer != invalid) {
                        return Result<RenderGraphPlan>::failure(invalid_state(
                            "A graph resource has multiple writers; use distinct resource versions"));
                    }
                    writer = static_cast<std::uint32_t>(pass_index);
                }
            }
        }

        const std::size_t matrix_size = pass_count * pass_count;
        std::vector<std::uint8_t> edges(matrix_size, 0U);
        std::vector<std::uint32_t> indegree(pass_count, 0U);
        const auto add_edge = [&edges, pass_count, &indegree](
            std::uint32_t from, std::uint32_t to) {
            const std::size_t edge_index =
                static_cast<std::size_t>(from) * pass_count +
                static_cast<std::size_t>(to);
            if (edges[edge_index] == 0U) {
                edges[edge_index] = 1U;
                ++indegree[to];
            }
        };

        for (std::size_t resource_index = 0U;
             resource_index < resource_count; ++resource_index) {
            const std::uint32_t writer = writers[resource_index];
            for (std::size_t pass_index = 0U; pass_index < pass_count; ++pass_index) {
                const PassRecord& pass = passes_[pass_index];
                for (std::size_t offset = 0U; offset < pass.access_count; ++offset) {
                    const GraphResourceAccessDecl& access =
                        accesses_[pass.first_access + offset];
                    if (access.resource.index != resource_index ||
                        access.access != GraphResourceAccess::Read) {
                        continue;
                    }
                    if (writer == invalid) {
                        if (!resources_[resource_index].externally_initialized) {
                            return Result<RenderGraphPlan>::failure(invalid_state(
                                "A transient graph resource is read before it is written"));
                        }
                    } else if (writer != pass_index) {
                        add_edge(writer, static_cast<std::uint32_t>(pass_index));
                    }
                }
            }
        }

        for (const Dependency& dependency : dependencies_) {
            add_edge(dependency.prerequisite, dependency.pass);
        }

        RenderGraphPlan plan;
        plan.execution_order_.reserve(pass_count);
        std::vector<std::uint8_t> emitted(pass_count, 0U);
        for (std::size_t iteration = 0U; iteration < pass_count; ++iteration) {
            std::size_t next = pass_count;
            for (std::size_t candidate = 0U; candidate < pass_count; ++candidate) {
                if (emitted[candidate] == 0U && indegree[candidate] == 0U) {
                    next = candidate;
                    break;
                }
            }
            if (next == pass_count) {
                return Result<RenderGraphPlan>::failure(invalid_state(
                    "RenderGraph dependencies contain a cycle"));
            }

            emitted[next] = 1U;
            plan.execution_order_.push_back(GraphPassHandle{
                static_cast<std::uint32_t>(next)});
            for (std::size_t dependent = 0U; dependent < pass_count; ++dependent) {
                const std::size_t edge_index = next * pass_count + dependent;
                if (edges[edge_index] != 0U) {
                    --indegree[dependent];
                }
            }
        }

        plan.resource_lifetimes_.resize(resource_count);
        std::vector<std::uint32_t> execution_positions(pass_count, invalid);
        for (std::size_t position = 0U;
             position < plan.execution_order_.size(); ++position) {
            execution_positions[plan.execution_order_[position].index] =
                static_cast<std::uint32_t>(position);
        }

        for (std::size_t pass_index = 0U; pass_index < pass_count; ++pass_index) {
            const std::uint32_t position = execution_positions[pass_index];
            const PassRecord& pass = passes_[pass_index];
            for (std::size_t offset = 0U; offset < pass.access_count; ++offset) {
                const std::uint32_t resource_index =
                    accesses_[pass.first_access + offset].resource.index;
                GraphResourceLifetime& lifetime =
                    plan.resource_lifetimes_[resource_index];
                if (!lifetime.is_used()) {
                    lifetime.first_pass_index = position;
                    lifetime.last_pass_index = position;
                } else {
                    lifetime.first_pass_index =
                        std::min(lifetime.first_pass_index, position);
                    lifetime.last_pass_index =
                        std::max(lifetime.last_pass_index, position);
                }
            }
        }

        struct AliasSlotState final {
            GraphResourceDesc description{};
            std::uint32_t last_use{0U};
        };
        std::vector<AliasSlotState> alias_slots;
        alias_slots.reserve(resource_count);

        for (std::size_t resource_index = 0U;
             resource_index < resource_count; ++resource_index) {
            const ResourceRecord& resource = resources_[resource_index];
            GraphResourceLifetime& lifetime = plan.resource_lifetimes_[resource_index];
            if (resource.externally_initialized || !lifetime.is_used()) {
                continue;
            }

            std::uint32_t selected_slot = invalid;
            for (std::size_t slot_index = 0U;
                 slot_index < alias_slots.size(); ++slot_index) {
                AliasSlotState& slot = alias_slots[slot_index];
                if (slot.description == resource.description &&
                    slot.last_use < lifetime.first_pass_index) {
                    selected_slot = static_cast<std::uint32_t>(slot_index);
                    slot.last_use = lifetime.last_pass_index;
                    break;
                }
            }

            if (selected_slot == invalid) {
                if (alias_slots.size() >= static_cast<std::size_t>(invalid)) {
                    return Result<RenderGraphPlan>::failure(exhausted(
                        "RenderGraph transient slot handle space is exhausted"));
                }
                selected_slot = static_cast<std::uint32_t>(alias_slots.size());
                alias_slots.push_back(AliasSlotState{
                    .description = resource.description,
                    .last_use = lifetime.last_pass_index});
            }
            lifetime.alias_slot = selected_slot;
        }

        plan.transient_slot_count_ = alias_slots.size();
        return Result<RenderGraphPlan>::success(std::move(plan));
    } catch (const std::bad_alloc&) {
        return Result<RenderGraphPlan>::failure(exhausted(
            "Unable to allocate RenderGraph compilation workspace"));
    } catch (const std::length_error&) {
        return Result<RenderGraphPlan>::failure(exhausted(
            "RenderGraph compilation workspace exceeds container limits"));
    }
}

std::size_t RenderGraph::resource_count() const noexcept {
    return initialized_ ? resources_.size() : 0U;
}

std::size_t RenderGraph::pass_count() const noexcept {
    return initialized_ ? passes_.size() : 0U;
}

} // namespace Storm::Rendering
