# Scene file format

**Current format:** version 2 (version 1 is still readable)  
**Magic:** `STSC`  
**Encoding:** binary, little-endian integers and IEEE-754 binary32 floating-point values  
**Maximum file size:** 64 MiB

StormEngine's dependency-free `SceneCodec` persists scene dimensions, stable entity IDs, names, dimension-aware transforms and supported scene components. The format never writes native C++ structs, pointer values, padding or host-endian integers directly. Saving emits version 2; loading explicitly accepts version 1 and version 2. Unknown versions and unknown required component bits fail with `Unsupported`.

## Shared 12-byte header

The header is followed by exactly the declared number of entity records.

| Header field | Size | Meaning |
| --- | ---: | --- |
| Magic | 4 bytes | ASCII `STSC` |
| Version | 2 bytes | Unsigned integer; version 1 or 2 when loading, version 2 when saving |
| Dimension | 1 byte | `1`, `2`, or `3` |
| Reserved flags | 1 byte | Must be zero |
| Entity count | 4 bytes | Unsigned integer, little-endian |

## Version 1 entity record

Each version-1 entity record is stored in ascending stable-ID byte order and contains:

| Record field | Size | Meaning |
| --- | ---: | --- |
| Stable ID | 16 bytes | UUID-compatible ID bytes in canonical text/network order; Nil is invalid |
| Name length | 2 bytes | Unsigned byte count, little-endian |
| Name | 1–128 bytes | Name bytes; empty names and embedded NUL bytes are rejected |
| Transform | Dimension-specific | Finite binary32 values, ordered as below |

Version 1 has no component mask. On migration, a version-1 2D entity receives the default visible `Renderable2D` component so old scenes keep their previous reference-canvas placeholder appearance. Version-1 1D and 3D entities remain without this 2D-only component.

## Version 2 entity record

Each version-2 record keeps the version-1 ID, name and transform fields in the same order, then adds:

| Record field | Size | Meaning |
| --- | ---: | --- |
| Component mask | 4 bytes | Unsigned little-endian bit mask; unknown bits are rejected as unsupported |
| Renderable2D payload | 13 bytes, if present | Size X/Y (2 binary32 floats), RGBA tint (4 bytes), visibility (1 byte: only 0 or 1) |

Component-mask bit 0 indicates a `Renderable2D` payload. A zero mask means the entity has no renderable component and may be a logic-only entity. The component is valid only for 2D scenes. Its dimensions must be finite and strictly positive; the tint uses straight RGBA bytes, and visibility `0` suppresses drawing without removing the component. Texture references, material IDs, sort order and GPU resource handles are intentionally not part of this initial quad contract.

## Transform values

Transform values are ordered as follows:

- **1D:** position, scale (2 floats).
- **2D:** position X, position Y, rotation in radians, scale X, scale Y (5 floats).
- **3D:** position X/Y/Z, Euler rotation X/Y/Z in radians, scale X/Y/Z (9 floats).

When local transforms are converted to matrices, StormEngine maps 1D values onto X and 2D values onto the XY plane. 3D Euler rotations are applied X, then Y, then Z, following the column-vector composition `T * Rz * Ry * Rx * S`. See [ADR 0002](decisions/0002-coordinate-system-and-matrix-conventions.md).

The name field is a bounded byte string; neither version currently claims to validate UTF-8. A higher-level project/editor layer should establish the user-facing text encoding policy before relying on these bytes as localized text.

## Validation and failure behavior

Loaders reject truncated headers or records, wrong magic, unsupported versions, nonzero reserved flags, invalid or mismatched dimensions, impossible entity counts, entity counts above the destination capacity, invalid/Nil/unsorted IDs, invalid names, non-finite transform values, unknown component-mask bits, malformed Renderable2D payloads, trailing bytes, and files larger than 64 MiB.

Loading requires a pre-created empty destination `Scene` whose dimension matches the file. Parsed entities are staged separately and swapped into the destination only after the entire file validates. On failure, the destination remains empty and unchanged. Saving similarly constructs a temporary byte vector and replaces the caller's output only when encoding succeeds. Loading version 1 migrates to the current in-memory model; serializing that model writes version 2.

## API example

```cpp
#include <cstddef>
#include <vector>

#include "storm/scene/scene_codec.hpp"

std::vector<std::byte> file_bytes;
storm::scene::Scene scene{storm::scene::Dimension::Two};

const auto save_status = storm::scene::SceneCodec::serialize(scene, file_bytes);
// Persist file_bytes using the bounded SceneFile service.

storm::scene::Scene loaded{storm::scene::Dimension::Two};
const auto load_status = storm::scene::SceneCodec::deserialize(file_bytes, loaded);
```

The codec only converts bytes to and from the in-memory scene model. It does not open files, create project bundles, follow asset references, import assets or make scene data trusted executable code.

## File I/O service

Include `storm/scene/scene_file.hpp` when the application needs to persist these bytes to a local file:

```cpp
#include "storm/scene/scene_file.hpp"

storm::scene::Scene scene{storm::scene::Dimension::Two};
const auto save = storm::scene::SceneFile::save_atomic("level.stsc", scene);

storm::scene::Scene reopened{storm::scene::Dimension::Two};
const auto load = storm::scene::SceneFile::load("level.stsc", reopened);
```

The file service reads at most 64 MiB, rejects non-regular files and final-component symlinks/reparse points, and does not fill a non-empty destination. Saving writes and flushes a uniquely named temporary file in the destination directory before replacing the destination. On POSIX, it also flushes the parent directory after rename; if that final directory flush fails, the replacement may already be visible even though the call reports an I/O error. On Windows, the implementation uses `FlushFileBuffers` and `MoveFileExW` with replace/write-through flags.

This is bounded local-file I/O, **not** a project sandbox. A higher-level project service must canonicalize or otherwise constrain untrusted project paths to the authorized project root. Durable storage guarantees also depend on filesystem and device behavior.
