#include "storm/rendering/render_graph.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace {
#define EXPECT(condition) do { if (!(condition)) return std::string("Expectation failed: ") + #condition; } while (false)

using Storm::ErrorCode;
using Storm::Rendering::GraphResourceAccess;
using Storm::Rendering::GraphResourceAccessDecl;
using Storm::Rendering::GraphResourceDesc;
using Storm::Rendering::GraphTextureFormat;
using Storm::Rendering::RenderGraph;
using Storm::Rendering::RenderGraphConfig;

[[nodiscard]] std::optional<std::string> inferred_resource_order() {
    auto created = RenderGraph::create();
    EXPECT(created);
    auto graph = std::move(created).value();
    auto color = graph.create_resource("scene-color", GraphResourceDesc{});
    EXPECT(color);
    const std::array read_access{
        GraphResourceAccessDecl{color.value(), GraphResourceAccess::Read}};
    const std::array write_access{
        GraphResourceAccessDecl{color.value(), GraphResourceAccess::Write}};
    auto consumer = graph.add_pass("composite", read_access);
    auto producer = graph.add_pass("scene", write_access);
    EXPECT(consumer);
    EXPECT(producer);
    auto plan = graph.compile();
    EXPECT(plan);
    EXPECT(plan.value().execution_order().size() == 2U);
    EXPECT(plan.value().execution_order()[0] == producer.value());
    EXPECT(plan.value().execution_order()[1] == consumer.value());
    return {};
}

[[nodiscard]] std::optional<std::string> external_resource_read() {
    auto created = RenderGraph::create();
    EXPECT(created);
    auto graph = std::move(created).value();
    auto backbuffer = graph.create_resource("backbuffer", GraphResourceDesc{}, true);
    EXPECT(backbuffer);
    const std::array accesses{
        GraphResourceAccessDecl{backbuffer.value(), GraphResourceAccess::Read}};
    EXPECT(graph.add_pass("present", accesses));
    auto plan = graph.compile();
    EXPECT(plan);
    EXPECT(plan.value().resource_lifetimes().size() == 1U);
    EXPECT(plan.value().resource_lifetimes()[0].is_used());
    EXPECT(plan.value().resource_lifetimes()[0].alias_slot ==
        Storm::Rendering::GraphResourceHandle::invalid_index);
    EXPECT(plan.value().transient_slot_count() == 0U);
    return {};
}

[[nodiscard]] std::optional<std::string> transient_read_before_write_rejected() {
    auto created = RenderGraph::create();
    EXPECT(created);
    auto graph = std::move(created).value();
    auto color = graph.create_resource("transient-color", GraphResourceDesc{});
    EXPECT(color);
    const std::array accesses{
        GraphResourceAccessDecl{color.value(), GraphResourceAccess::Read}};
    EXPECT(graph.add_pass("sample", accesses));
    auto plan = graph.compile();
    EXPECT(!plan);
    EXPECT(plan.error().code == ErrorCode::InvalidState);
    return {};
}

[[nodiscard]] std::optional<std::string> multiple_writers_rejected() {
    auto created = RenderGraph::create();
    EXPECT(created);
    auto graph = std::move(created).value();
    auto color = graph.create_resource("color", GraphResourceDesc{});
    EXPECT(color);
    const std::array accesses{
        GraphResourceAccessDecl{color.value(), GraphResourceAccess::Write}};
    EXPECT(graph.add_pass("first-writer", accesses));
    EXPECT(graph.add_pass("second-writer", accesses));
    auto plan = graph.compile();
    EXPECT(!plan);
    EXPECT(plan.error().code == ErrorCode::InvalidState);
    return {};
}

[[nodiscard]] std::optional<std::string> explicit_dependencies_and_cycles() {
    auto created = RenderGraph::create();
    EXPECT(created);
    auto graph = std::move(created).value();
    auto final = graph.add_pass("final", {});
    auto prepare = graph.add_pass("prepare", {});
    EXPECT(final);
    EXPECT(prepare);
    EXPECT(graph.add_dependency(final.value(), prepare.value()));
    auto plan = graph.compile();
    EXPECT(plan);
    EXPECT(plan.value().execution_order()[0] == prepare.value());

    auto cyclic_created = RenderGraph::create();
    EXPECT(cyclic_created);
    auto cyclic = std::move(cyclic_created).value();
    auto a = cyclic.add_pass("a", {});
    auto b = cyclic.add_pass("b", {});
    EXPECT(a);
    EXPECT(b);
    EXPECT(cyclic.add_dependency(a.value(), b.value()));
    EXPECT(cyclic.add_dependency(b.value(), a.value()));
    auto cyclic_plan = cyclic.compile();
    EXPECT(!cyclic_plan);
    EXPECT(cyclic_plan.error().code == ErrorCode::InvalidState);
    return {};
}

[[nodiscard]] std::optional<std::string> duplicate_and_invalid_accesses_rejected() {
    auto created = RenderGraph::create();
    EXPECT(created);
    auto graph = std::move(created).value();
    auto color = graph.create_resource("color", GraphResourceDesc{});
    EXPECT(color);
    const std::array duplicate{
        GraphResourceAccessDecl{color.value(), GraphResourceAccess::Read},
        GraphResourceAccessDecl{color.value(), GraphResourceAccess::Write}};
    EXPECT(!graph.add_pass("invalid-duplicate", duplicate));
    const std::array invalid{
        GraphResourceAccessDecl{
            color.value(), static_cast<GraphResourceAccess>(0xffU)}};
    EXPECT(!graph.add_pass("invalid-mode", invalid));
    const std::array stale{
        GraphResourceAccessDecl{
            Storm::Rendering::GraphResourceHandle{99U}, GraphResourceAccess::Read}};
    EXPECT(!graph.add_pass("stale-resource", stale));
    return {};
}

[[nodiscard]] std::optional<std::string> transient_aliasing_for_disjoint_lifetimes() {
    auto created = RenderGraph::create();
    EXPECT(created);
    auto graph = std::move(created).value();
    const GraphResourceDesc description{
        .width = 64U, .height = 64U, .layers = 1U, .samples = 1U,
        .format = GraphTextureFormat::RGBA8Unorm};
    auto first = graph.create_resource("first", description);
    auto second = graph.create_resource("second", description);
    EXPECT(first);
    EXPECT(second);
    const std::array write_first{
        GraphResourceAccessDecl{first.value(), GraphResourceAccess::Write}};
    const std::array read_first{
        GraphResourceAccessDecl{first.value(), GraphResourceAccess::Read}};
    const std::array write_second{
        GraphResourceAccessDecl{second.value(), GraphResourceAccess::Write}};
    EXPECT(graph.add_pass("first-write", write_first));
    EXPECT(graph.add_pass("first-read", read_first));
    EXPECT(graph.add_pass("second-write", write_second));
    auto plan = graph.compile();
    EXPECT(plan);
    EXPECT(plan.value().transient_slot_count() == 1U);
    EXPECT(plan.value().resource_lifetimes()[0].first_pass_index == 0U);
    EXPECT(plan.value().resource_lifetimes()[0].last_pass_index == 1U);
    EXPECT(plan.value().resource_lifetimes()[1].first_pass_index == 2U);
    EXPECT(plan.value().resource_lifetimes()[0].alias_slot ==
        plan.value().resource_lifetimes()[1].alias_slot);
    return {};
}

[[nodiscard]] std::optional<std::string> overlapping_lifetimes_do_not_alias() {
    auto created = RenderGraph::create();
    EXPECT(created);
    auto graph = std::move(created).value();
    auto first = graph.create_resource("first", GraphResourceDesc{});
    auto second = graph.create_resource("second", GraphResourceDesc{});
    EXPECT(first);
    EXPECT(second);
    const std::array write_first{
        GraphResourceAccessDecl{first.value(), GraphResourceAccess::Write}};
    const std::array write_second{
        GraphResourceAccessDecl{second.value(), GraphResourceAccess::Write}};
    const std::array read_both{
        GraphResourceAccessDecl{first.value(), GraphResourceAccess::Read},
        GraphResourceAccessDecl{second.value(), GraphResourceAccess::Read}};
    EXPECT(graph.add_pass("write-first", write_first));
    EXPECT(graph.add_pass("write-second", write_second));
    EXPECT(graph.add_pass("read-both", read_both));
    auto plan = graph.compile();
    EXPECT(plan);
    EXPECT(plan.value().transient_slot_count() == 2U);
    EXPECT(plan.value().resource_lifetimes()[0].alias_slot !=
        plan.value().resource_lifetimes()[1].alias_slot);
    return {};
}

[[nodiscard]] std::optional<std::string> resource_description_limits() {
    auto created = RenderGraph::create(RenderGraphConfig{
        .maximum_resources = 4U,
        .maximum_passes = 4U,
        .maximum_accesses = 8U,
        .maximum_dependencies = 8U,
        .maximum_extent = 128U,
        .maximum_resource_bytes = 1024U});
    EXPECT(created);
    auto graph = std::move(created).value();
    EXPECT(!graph.create_resource("zero", GraphResourceDesc{.width = 0U}));
    EXPECT(!graph.create_resource("too-large", GraphResourceDesc{.width = 129U}));
    EXPECT(!graph.create_resource("too-many-bytes",
        GraphResourceDesc{.width = 32U, .height = 32U, .format = GraphTextureFormat::RGBA16Float}));
    EXPECT(!graph.create_resource("unsupported-samples",
        GraphResourceDesc{.samples = 3U}));
    EXPECT(!graph.create_resource("invalid-format",
        GraphResourceDesc{.format = static_cast<GraphTextureFormat>(0xffU)}));
    return {};
}

[[nodiscard]] std::optional<std::string> bounded_capacity_and_names() {
    auto created = RenderGraph::create(RenderGraphConfig{
        .maximum_resources = 1U,
        .maximum_passes = 1U,
        .maximum_accesses = 1U,
        .maximum_dependencies = 0U});
    EXPECT(created);
    auto graph = std::move(created).value();
    EXPECT(!graph.create_resource("", GraphResourceDesc{}));
    EXPECT(!graph.create_resource(std::string(64U, 'x'), GraphResourceDesc{}));
    EXPECT(graph.create_resource("only", GraphResourceDesc{}));
    EXPECT(!graph.create_resource("overflow", GraphResourceDesc{}));
    EXPECT(graph.add_pass("only-pass", {}));
    EXPECT(!graph.add_pass("overflow-pass", {}));
    return {};
}

[[nodiscard]] std::optional<std::string> move_invalidates_source() {
    auto created = RenderGraph::create();
    EXPECT(created);
    auto graph = std::move(created).value();
    auto moved = std::move(graph);
    EXPECT(moved.resource_count() == 0U);
    EXPECT(!graph.add_pass("invalid-source", {}));
    return {};
}

struct TestCase final {
    const char* name;
    std::optional<std::string> (*run)();
};

} // namespace

int main() {
    constexpr std::array tests{
        TestCase{"inferred_resource_order", inferred_resource_order},
        TestCase{"external_resource_read", external_resource_read},
        TestCase{"transient_read_before_write_rejected", transient_read_before_write_rejected},
        TestCase{"multiple_writers_rejected", multiple_writers_rejected},
        TestCase{"explicit_dependencies_and_cycles", explicit_dependencies_and_cycles},
        TestCase{"duplicate_and_invalid_accesses_rejected", duplicate_and_invalid_accesses_rejected},
        TestCase{"transient_aliasing_for_disjoint_lifetimes", transient_aliasing_for_disjoint_lifetimes},
        TestCase{"overlapping_lifetimes_do_not_alias", overlapping_lifetimes_do_not_alias},
        TestCase{"resource_description_limits", resource_description_limits},
        TestCase{"bounded_capacity_and_names", bounded_capacity_and_names},
        TestCase{"move_invalidates_source", move_invalidates_source},
    };
    std::size_t failures = 0U;
    for (const TestCase& test : tests) {
        const auto result = test.run();
        if (result.has_value()) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << *result << '\n';
        } else {
            std::cout << "[PASS] " << test.name << '\n';
        }
    }
    std::cout << "Executed " << tests.size() << " tests; failures: " << failures << '\n';
    return failures == 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
