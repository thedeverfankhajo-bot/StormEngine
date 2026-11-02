# Scene Serialization (version 1)

## Scope and status

The separate `Storm::Serialization` target converts a `Storm::Scene` to a bounded in-memory byte archive and constructs a new Scene from that archive.

The current archive format is version 1. This is a development API, not a stable persistence contract. It does not open files, access Android/iOS storage APIs, perform atomic file replacement, compress data, encrypt data, or calculate a file-integrity checksum.

Core does not depend on Serialization. The Serialization target depends on Core and Math, keeping archive code outside the gameplay Scene implementation.

## Component codec contract

A component is serializable only when the caller supplies a `ComponentCodec<T>` specialization with:

- `type_id`: a stable, project-owned identifier. Do not use `typeid(T).name()`, compiler-specific type names, or source file names.
- `schema_version`: a component payload schema version.
- `encode(const T&, BinaryWriter&)`: writes the complete component payload.
- `decode(schema_version, BinaryReader&)`: validates the supplied version and constructs a component value.

`Transform1D` has a built-in codec with ID `storm.math.transform1d` and schema version 1. Its scalar position is stored as an IEEE-754 64-bit value without narrowing.
`Transform3D` has a built-in codec with ID `storm.math.transform3d` and schema version 1. Other component types require an explicit specialization supplied by the game or engine module.

Codec implementations must validate their own payload values and consume the complete payload during decoding. They should not persist process-local pointers, addresses, runtime handles, or native object layout.

## API usage

~~~cpp
#include "storm/serialization/scene_serializer.hpp"

auto archive = Storm::Serialization::SceneSerializer::serialize<
    Game::Health, Storm::Math::Transform3D>(scene);
if (!archive) {
    // Handle archive.error().
}

auto restored = Storm::Serialization::SceneSerializer::deserialize<
    Game::Health, Storm::Math::Transform3D>(archive.value());
if (!restored) {
    // Handle restored.error().
}
~~~

The component type list is explicit at compile time. Before encoding, the serializer checks that every non-empty component store in the Scene has a supplied codec. If one is missing, serialization fails with `Unsupported`; component data is never silently omitted.

Loading creates a new Scene with a new process-local SceneId and new EntityHandles. Archive entity IDs are local references only and are not reused as runtime identities.

## Archive structure

Version 1 uses fixed-width little-endian integer fields and bit-preserved IEEE-754 32-bit floats and 64-bit doubles.

The archive contains:

1. Four-byte magic `STSC`.
2. Archive format version (`uint16`, currently 1).
3. Reserved flags (`uint16`, currently zero).
4. Scene name.
5. Ordered entity records with archive-local ID, name, parent ID, and sibling order.
6. Component blocks sorted by stable type ID.
7. For each component block: type ID, schema version, record count, then entity-local ID and bounded payload bytes for each record.

Entity records are emitted in ascending Scene slot order. Component records follow that same entity order. Sibling order is stored explicitly and restored, rather than inferred from entity creation order.

## Limits and malformed data

Default `SceneArchiveLimits`:

| Limit | Default |
|---|---:|
| Maximum archive bytes | 64 MiB |
| Maximum entities | 250,000 |
| Maximum component types | 256 |
| Maximum component records | 1,000,000 |
| Maximum string bytes | 1 MiB |
| Maximum component payload | 4 MiB |

The reader validates the archive magic, format version, reserved flags, field boundaries, configured sizes, contiguous entity IDs, parent references, sibling-order uniqueness/continuity, hierarchy cycles, component type ordering, component record references, duplicate components, codec schema versions, and complete payload consumption.

Unknown component IDs and unsupported versions fail explicitly. Malformed or over-limit input returns a typed error. Deserialization builds a temporary Scene and returns it only after the complete archive has been validated and reconstructed.

`BinaryReader::read_string` restores its input offset when a string read fails. `BinaryWriter::write_string` reserves the complete bounded field before writing its length and payload, avoiding a partially written string field after allocation or limit failure.

## Compatibility and migration

- Archive format version and component schema version are independent.
- A codec decides which component schema versions it can decode. Version 1 codecs currently reject other versions; no automatic migration registry exists.
- Stable component IDs are part of the project's data contract. Renaming a C++ class does not require changing its archive ID.
- Renaming a stable ID or changing field semantics requires a planned compatibility/migration decision.
- The archive format is not declared stable and may evolve before the first engine release.

## Validation evidence

Thirteen dedicated host test cases cover Scene round-trip, hierarchy and sibling order, built-in Transform1D/Transform3D and custom components, deterministic codec ordering, empty scenes, missing/unknown codecs, invalid magic/version, truncation/trailing input, limits, invalid parents/cycles, duplicate records, schema versions, invalid transform values, and transactional string/64-bit numeric operations.

The six-job GitHub Actions run [41322123527](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41322123527) passed GCC, Clang, Apple Clang, Clang ASan/UBSan, Android arm64-v8a, and iOS device arm64. Android and iOS explicitly instantiate the built-in Transform3D archive path at compile time. These are host-test and target-build results, not on-device serialization or filesystem integration tests.