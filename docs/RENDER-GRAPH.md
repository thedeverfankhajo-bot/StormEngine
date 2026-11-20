# Backend-Independent Render Graph

## Purpose and current boundary

Storm::Rendering::RenderGraph is a platform-neutral scheduling and resource-lifetime compiler. It does not include Vulkan, Metal, Android, or Apple SDK headers and does not issue GPU commands.

The graph describes texture-like resources using width, height, layers, sample count, and a small format enum. Resource descriptions are bounded by configurable extent and estimated byte limits. Names, resource counts, pass counts, access declarations, and explicit dependencies are bounded as well.

## Passes and dependencies

Each pass has a unique name and declares at most one access to each resource. An access is either read or write. This initial graph version uses a single-writer model: one pass may write a resource, while all declared readers are ordered after that writer. An uninitialized transient resource cannot be read. Resources flagged as externally initialized may be read without an in-graph writer, which is intended for acquired swapchain/drawable inputs.

Explicit dependencies express ordering not represented by resource use. Compilation performs a stable topological sort using pass creation order as the tie-breaker. Cycles, duplicate dependencies, invalid handles, multiple writers, and uninitialized transient reads return typed errors.

The single-writer constraint is intentional. Multi-version resources, read/write attachments, load/store operations, subresources, queues, and synchronization stages require additional explicit semantics and are not inferred from this first version.

## Resource lifetime and alias planning

For every used resource, the compiled plan reports first and last pass positions in final execution order. Compatible transient resource descriptions may share an alias slot only when their inclusive lifetime intervals do not overlap. External and unused resources are excluded from transient slots.

This result is an allocation opportunity, not a GPU allocation. A concrete backend must translate the plan into API-specific allocations, memory compatibility checks, barriers/transitions, queue ownership, and lifetime-safe destruction. The current conservative descriptor equality is not a substitute for Vulkan memory requirements or Metal heap compatibility.

## Ownership and limits

The graph owns its metadata and returns a value snapshot from compilation. Graph handles are index values intended to be used with the graph that created them; applications must not mix handles from different graph instances. Graph mutation after compile does not mutate an already returned plan, but applications should compile again before executing a changed graph.

The graph is single-thread-affine. It bounds persistent declarations and catches allocation failures during construction/compilation. The compiler's temporary adjacency matrix is bounded by the supported maximum of 512 passes.

## Validation status

Eleven deterministic test cases are registered in stormengine_render_graph_tests: inferred ordering, external inputs, transient read-before-write rejection, multiple-writer rejection, explicit dependency ordering and cycle detection, duplicate/invalid access rejection, disjoint lifetime aliasing, overlapping lifetime non-aliasing, description limits, bounded capacities/names, and moved-from object handling.

No GPU backend, resource transition, render-pass execution, frame submission, or presentation is claimed by this module.
