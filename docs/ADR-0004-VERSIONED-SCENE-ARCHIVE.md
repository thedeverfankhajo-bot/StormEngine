# ADR-0004: Bounded, Versioned Scene Archive

- Status: Accepted for the initial implementation
- Date: 2026-11-01
- Scope: Scene data serialization
- API status: Development only; no stable compatibility promise

## Context

The Core Scene owns generation-checked entities, a parent/child hierarchy, and scene-local typed component stores. The project needs a persistence boundary for later project-save and mobile-editor workflows without coupling Core to a file format, editor framework, or third-party parser.

The component registry uses C++ runtime type identity internally. That identity is not a suitable persistent identifier, and raw object-memory serialization would be unsafe and ABI-dependent.

## Decision

Introduce a separate `Storm::Serialization` target that depends on Core and Math; Core and Foundation do not depend on the archive module.

The initial archive is a bounded little-endian binary format with:

- A magic header and explicit archive format version.
- Explicit archive-local entity IDs.
- Parent references and sibling order stored separately from runtime handles.
- Stable project-owned component type IDs.
- Per-component schema versions.
- Explicit compile-time `ComponentCodec<T>` specializations.
- Configurable limits for total bytes, entities, component types, records, strings, and component payloads.
- Strict rejection of unknown component IDs, duplicate data, malformed references, invalid hierarchy, unsupported versions, and over-limit input.

The first built-in component codec is `Storm::Math::Transform3D`. Game-specific components provide their own codecs. The serializer verifies that all non-empty component stores in a Scene are covered by the explicitly supplied codec list.

Deserialization constructs a temporary Scene and returns it only after all entities, hierarchy links, and component payloads have been validated.

## Alternatives considered

### Raw memory/object serialization

Rejected. C++ object layout, padding, pointers, standard-library containers, and compiler ABI details are not portable persistent data formats.

### Runtime `typeid`/RTTI names as persistent IDs

Rejected. Type names may vary by compiler, build mode, namespace refactoring, or implementation. They are not stable project data identifiers.

### Unbounded generic JSON dependency

Deferred. No JSON dependency is required for this first archive slice. A later project manifest may use a structured text format if the editor and project UX justify it, but component payloads still require explicit schemas and bounds.

### Global runtime reflection registry

Deferred. It would add dynamic registration, lifetime, collision, plugin-ordering, and migration complexity before the editor requires runtime-discoverable component metadata. Explicit codecs keep the first serialization boundary auditable and compile-time checked.

## Consequences

- Serialization remains outside Core; Core can be built and tested without the archive module.
- Component data is never silently omitted: an unregistered component storage causes serialization to fail.
- Component codecs own payload validation and version interpretation.
- The archive is deterministic for a fixed Scene state and codec set: entity slot order, stable type-ID order, component slot order, and sibling order are explicit.
- Loading creates new runtime handles and a new SceneId; archive IDs are local to the archive.
- The implementation currently handles in-memory bytes only. File I/O, atomic file replacement, checksums, compression, automatic migrations, reflection, and on-device storage tests remain future work.

## Validation

Thirteen dedicated host tests pass under GCC, Clang, Apple Clang, and Clang ASan/UBSan. The built-in Transform3D serializer path is explicitly instantiated and cross-compiled for Android arm64-v8a and iOS device arm64 in the six-job CI run [41322123527](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41322123527).

This evidence does not establish physical-device runtime behavior or compatibility with future archive versions.