# ADR-0003: Transform Math and Hierarchy Composition

- Status: Accepted for the initial Transform3D slice
- Date: 2026-11-01

## Context

StormEngine has a generation-safe Scene hierarchy, but no local/world transform representation. A renderer and future 2D/3D systems need a stable convention for composing local transforms through the entity hierarchy. The convention must be platform-neutral and testable without a graphics API.

## Decision

- Place reusable mathematical value types in the independent `Storm::Math` target.
- Represent 3D local transforms as translation, quaternion rotation, and scale (TRS).
- Use right-handed rotations and quaternions ordered x, y, z, w.
- Store 4x4 matrices column-major and apply them to column vectors.
- Build local matrices as Translation × Rotation × Scale.
- Compose world matrices as parentWorld × local.
- Keep Transform3D an optional, typed Scene component. An entity without the component is treated as an identity transform.
- Evaluate the ancestry iteratively on demand. Do not add a world-transform cache until benchmarks justify it and invalidation semantics are specified.
- Keep 1D gameplay spatial state independent and scalar; Transform3D does not define the 1D model.

## Alternatives considered

- Row-major / row-vector conventions: valid alternatives, but mixing them with common mobile graphics interfaces and later matrix consumers would create conversion risk. One explicit convention is preferable.
- Euler-angle storage: rejected as the primary rotation representation because quaternion composition avoids gimbal-lock behavior in general 3D rotation.
- Mandatory transform on every entity: rejected for now. Optional components keep grouping-only entities lightweight and allow non-spatial entities.
- Cached world matrices with dirty propagation: deferred. Hierarchy changes, component replacement, and move/destruction semantics need a measured cache invalidation contract.
- One vector transform for 1D, 2D, and 3D: rejected because 1D must preserve scalar gameplay semantics.

## Consequences

- Core can compute world matrices without depending on Vulkan, Metal, or an editor.
- World matrix queries cost O(hierarchy depth) time and temporary memory.
- Deep hierarchies do not recurse through the C++ call stack.
- Renderers must consume the documented matrix convention or convert explicitly at their backend boundary.
- Transform serialization, 2D planar constraints, 1D scalar transforms, cameras, physics integration, and device-level rendering remain future work.
