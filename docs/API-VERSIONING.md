# API Versioning Policy

## Current status

StormEngine has no released or stable public API yet. The 0.1.0 value in CMake is an early project version, not a promise that public API compatibility guarantees are already in force.

The currently exposed C++ headers are development interfaces and may change while the foundation is being established. No package or binary compatibility promise is made at this stage.

The platform-neutral Storm::Platform::SurfaceLifecycle interface is a development API added during the foundation stage. It models one surface identity/generation stream; it does not own native handles, synchronize GPU work, or establish Android/iOS support. It is not a stable API guarantee.

The Storm::Platform::Android::apply_activity_event function and ActivityEvent enum are also development interfaces. Their transition policy is host-tested, but they are not a stable public API and do not prove Android framework runtime compatibility.

The `Storm::EventDispatcher`, its `Subscription` handle, and the `subscribe<Event>()` / `dispatch()` methods are development interfaces added to Core. They are not stable API guarantees. Dispatch is synchronous and single-thread-affine; handler errors stop dispatch, and callback exceptions propagate.

The `Storm::Scene`, `Storm::EntityHandle`, typed component storage, and entity hierarchy APIs are development interfaces. Scene handles use process-unique Scene IDs and per-slot generations; component references and child spans are non-owning. Component references are invalidated by component removal, entity destruction, or Scene destruction; child spans are invalidated by hierarchy mutation or Scene destruction. Hierarchy is single-thread-affine, rejects cycles, and orphans direct children when a parent is destroyed. These interfaces are not stable API guarantees and do not provide serialization, reflection, or an archetype ECS.

The `Storm::Serialization::BinaryWriter`, `BinaryReader`, and `SceneSerializer` are development interfaces. Scene archive format version 1 uses bounded little-endian fields, explicit stable component type IDs, and per-component schema versions. Transform3D has a built-in codec; other component types require an explicit `ComponentCodec<T>` specialization. Serialization covers only the component types supplied to the API; an unregistered component store fails rather than being silently omitted. Deserialization constructs a new Scene and validates limits, hierarchy, records, and complete codec payload consumption before returning it. The API returns in-memory bytes only and does not perform filesystem I/O or automatic schema migration. These interfaces are not stable API guarantees. See [Scene Serialization](SCENE-SERIALIZATION.md) and [ADR-0004](ADR-0004-VERSIONED-SCENE-ARCHIVE.md).

The `Storm::Math::Vector3`, `Quaternion`, `Matrix4`, and `Transform3D` types, together with `Storm::TransformSystem`, are development interfaces introduced in the initial 3D transform slice. Matrices are column-major and use column vectors; hierarchy composition is `parentWorld × local`. World matrices are evaluated on demand in O(depth) time and temporary memory. The API is not stable and does not provide caching, 2D planar constraints, scalar 1D transforms, or renderer integration. Transform3D persistence is provided by the separate Scene archive codec rather than by the math value type itself. See [Transform System](TRANSFORM-SYSTEM.md) and [ADR-0003](ADR-0003-TRANSFORM-CONVENTIONS.md).

The `Storm::Physics::OneD::PhysicsWorld1D`, `OverlapQuery1D`, `RaycastQuery1D`,
and `RaycastHit1D` interfaces are development APIs with no stability guarantee.
Overlap queries operate on closed scalar intervals and return handles in
body-slot order. Raycasts accept only a unit scalar direction of -1 or +1 and
return the nearest hit as an optional value; maximum distance is inclusive,
triggers and collision layers are filterable, and an origin inside or on an
interval reports a zero-distance hit. Equal-distance hits are resolved by
body-slot order. Both query operations are read-only and single-thread-affine.
They do not establish a complete 1D gameplay runtime or a verified Android/iOS
runtime feature.

The `ContactPhase1D` and `ContactEvent1D` types extend the same development
surface. Each successful `PhysicsWorld1D::step()` returns Begin/Stay/End
transitions ordered by body-slot pair. Pair identity includes both handle
generations; reuse of a slot emits End for the previous handle and Begin for the
new one. End payloads preserve the last reported contact geometry and may contain
stale historical handles. Failed steps do not advance contact history. These
interfaces are not stable API guarantees and are not yet bridged to the Core
EventDispatcher or Scene components. See [1D Game Development](1D-GAME-DEVELOPMENT.md).

## Intended policy before the first stable release
The `Storm::Input::TouchInput`, `PointerEvent`, `PointerSnapshot`, and `TouchInputConfig` types are experimental development interfaces with no compatibility guarantee. They provide bounded, single-thread-affine ingestion of multi-pointer Down/Move/Up/Cancel events, surface-generation validation, monotonic timestamp checks, and per-frame movement deltas. The implementation does not acquire native events or clocks and is not connected to Android MotionEvent or iOS UIKit callbacks; gesture recognition and action mapping are not included. See [Mobile Input Foundation](INPUT-SYSTEM.md).


- Use Semantic Versioning for published engine releases once a public compatibility baseline is declared.
- Document every public API change in the changelog and API reference.
- Mark experimental interfaces explicitly and keep them out of stable compatibility guarantees.
- Record breaking changes and provide migration notes for affected public APIs.
- Deprecate an established public API before removal whenever technically and safely practical.
- Keep platform-specific implementation details out of portable public headers. Android NDK types remain isolated in the Android platform target.

## Release checklist

Before declaring an API stable, the project must have a published API reference, compatibility matrix, automated API tests, documented platform support, and a migration policy. A version number alone does not make an API stable.
