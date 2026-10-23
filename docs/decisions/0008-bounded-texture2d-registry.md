# ADR 0008: Bounded in-memory Texture2D registry

- **Status:** Accepted as an initial runtime resource-ownership primitive
- **Date:** 2026-10-23
- **Scope:** CPU Texture2D ownership and lookup only; not a persistent asset database or GPU resource manager

## Context

Texture2D can own bounded RGBA8 pixels, but callers lack a shared, bounded mapping from stable resource IDs to engine-owned texture values. A first registry should establish ownership, duplicate handling and resource budgets without prematurely defining the serialized scene reference, importer, material ABI or GPU lifetime.

## Decision

1. Add render::Texture2DRegistry, indexed by the existing 128-bit core::StableId value. IDs are supplied by the caller; the registry does not generate them.
2. Registration copies a valid texture into registry ownership. Invalid IDs/data, duplicate IDs, allocation failures and budget violations return explicit status codes; failure does not change the registry or its aggregate pixel count.
3. Enforce hard engine ceilings of 4,096 resources and 16,777,216 aggregate pixels (64 MiB RGBA8), with per-instance limits that may be lower. Per-texture dimension and payload validation remains owned by Texture2D.
4. Unregistration releases the texture and its pixel budget. Lookups return non-owning const pointers, valid until the referenced entry is removed or the registry is cleared/destroyed. Inserting other entries does not invalidate references in the ordered map.
5. Keep the type dependency-free and single-owner-thread only. It owns CPU data and does not decode files, generate IDs, serialize scene references, synchronize threads or allocate GPU resources.

## Consequences

- A headless caller can register, look up and release small texture resources with deterministic budget accounting.
- The current registry does not connect to Renderable2D or SceneCodec; scene-persisted texture references require a separately documented and tested scene-format version/migration.
- Image decoding, content hashing, asset import metadata, path-confined file access, cache policy, materials, sampler state and renderer-device lifetime remain future work.

## References

- ADR 0006: Explicit Renderable2D data and scene format v2 migration.
- ADR 0007: Bounded CPU Texture2D data and reference-canvas sampling.
- [Architecture](../ARCHITECTURE.md)
- [Roadmap](../ROADMAP.md)
