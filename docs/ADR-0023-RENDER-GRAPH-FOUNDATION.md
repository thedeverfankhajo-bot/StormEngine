# ADR-0023: Backend-Independent Render Graph Foundation

- Status: Accepted
- Date: 2026-11-20
- Scope: First backend-neutral render scheduling and transient lifetime planning slice

## Context

StormEngine already has deterministic CPU-side 2D sprite submission and platform-neutral camera math, but no GPU device, swapchain/drawable, shader pipeline, command buffer, or frame presentation. A backend-independent scheduling contract is needed before a concrete mobile backend can be safely integrated.

The graph must fit the existing dependency direction: rendering may depend on Foundation but Foundation/Core must not depend on a GPU API or mobile SDK. It must also remain bounded and testable on a host toolchain.

## Decision

Add RenderGraph to the existing Rendering2D library. The graph owns bounded resource and pass metadata, supports explicit dependencies and inferred write-to-read ordering, and compiles to a deterministic stable topological order. Compilation rejects dependency cycles, reads from transient resources without a writer, multiple writers for a resource, and invalid declarations.

Each resource has a checked texture-like description and configurable maximum extent/estimated byte size. The compiled plan reports resource lifetimes and assigns compatible transient resources to a shared slot only when their inclusive lifetimes are disjoint. External resources are excluded from aliasing and can be read without an in-graph writer.

This version deliberately uses one writer per resource and read-only or write-only pass declarations. It does not yet model attachment load/store, subresources, read-modify-write, multiple queues, API-specific memory requirements, synchronization barriers, or pass callbacks. Those semantics will be introduced only with a concrete backend and executable GPU test slice.

## Consequences

The scheduler and lifetime analysis can be validated independently of Vulkan/Metal. The compiled plan gives future backends a deterministic pass order and conservative aliasing hints without leaking native handles into engine-facing types.

The graph does not allocate GPU memory, create textures, perform transitions, execute commands, or present frames. No Android/iOS GPU rendering capability is considered implemented until an actual backend renders a test scene and is validated on target runtimes/devices.
