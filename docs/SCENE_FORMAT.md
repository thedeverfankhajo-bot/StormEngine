# Scene file format

**Current format:** version 1  
**Magic:** `STSC`  
**Encoding:** binary, little-endian integers and IEEE-754 binary32 floating-point values  
**Maximum file size:** 64 MiB

StormEngine's version-1 `SceneCodec` persists the current in-memory `Scene`, `Entity`, and dimension-aware `Transform` records without a third-party serialization dependency. The format is deliberately explicit: it never writes native C++ structs, pointer values, padding, or host-endian integers directly.

## File layout

The 12-byte header is followed by exactly the declared number of entity records.

| Header field | Size | Meaning |
| --- | ---: | --- |
| Magic | 4 bytes | ASCII `STSC` |
| Version | 2 bytes | Unsigned integer; currently `1` |
| Dimension | 1 byte | `1`, `2`, or `3` |
| Reserved flags | 1 byte | Must be zero in version 1 |
| Entity count | 4 bytes | Unsigned integer; little-endian |

Each entity record is stored in ascending stable-ID byte order:

| Record field | Size | Meaning |
| --- | ---: | --- |
| Stable ID | 16 bytes | UUID-compatible ID bytes in canonical text/network order; Nil is invalid |
| Name length | 2 bytes | Unsigned byte count, little-endian |
| Name | 1–128 bytes | Name bytes; empty names and embedded NUL bytes are rejected |
| Transform | Dimension-specific | Finite binary32 values, in the order below |

Transform values are ordered as follows:

- **1D:** position, scale (2 floats).
- **2D:** position X, position Y, rotation in radians, scale X, scale Y (5 floats).
- **3D:** position X/Y/Z, Euler rotation X/Y/Z in radians, scale X/Y/Z (9 floats).

When local transforms are converted to matrices, StormEngine maps 1D values onto X and 2D values onto the XY plane. 3D Euler rotations are applied X, then Y, then Z, following the column-vector composition `T * Rz * Ry * Rx * S`. See [ADR 0002](decisions/0002-coordinate-system-and-matrix-conventions.md). This defines matrix interpretation without changing the version-1 serialized field order.

The name field is a bounded byte string; version 1 does not claim to validate UTF-8. A higher-level project/editor layer should establish the user-facing text encoding policy before relying on these bytes as localized text.

## Validation and failure behavior

Loaders reject truncated headers or records, wrong magic, unknown versions, nonzero reserved flags, invalid or mismatched dimensions, impossible entity counts, entity counts above the destination capacity, invalid/Nil/unsorted IDs, invalid names, non-finite transform values, trailing bytes, and files larger than 64 MiB.

Loading requires a pre-created empty destination `Scene` whose dimension matches the file. Parsed entities are staged separately and swapped into the destination only after the entire file validates. On failure, the destination remains empty and unchanged. Saving similarly constructs a temporary byte vector and replaces the caller's output only when encoding succeeds.

Version 1 is implemented. **Migration between versions is not yet implemented**; a version the current codec does not understand returns an explicit `Unsupported` status rather than being guessed or silently reinterpreted.

## API example

```cpp
#include <cstddef>
#include <vector>

#include "storm/scene/scene_codec.hpp"

std::vector<std::byte> file_bytes;
storm::scene::Scene scene{storm::scene::Dimension::Two};

const auto save_status = storm::scene::SceneCodec::serialize(scene, file_bytes);
// Persist file_bytes using a future bounded filesystem/project service.

storm::scene::Scene loaded{storm::scene::Dimension::Two};
const auto load_status = storm::scene::SceneCodec::deserialize(file_bytes, loaded);
```

The codec only converts bytes to and from the in-memory scene model. It does not open files, create project bundles, follow references, import assets, or make scene data trusted executable code.


## File I/O service

Include `storm/scene/scene_file.hpp` when the application needs to persist these bytes to a local file:

~~~cpp
#include "storm/scene/scene_file.hpp"

storm::scene::Scene scene{storm::scene::Dimension::Two};
const auto save = storm::scene::SceneFile::save_atomic("level.stsc", scene);

storm::scene::Scene reopened{storm::scene::Dimension::Two};
const auto load = storm::scene::SceneFile::load("level.stsc", reopened);
~~~

The file service reads at most 64 MiB, rejects non-regular files and final-component symlinks/reparse points, and does not fill a non-empty destination. Saving writes and flushes a uniquely named temporary file in the destination directory before replacing the destination. On POSIX, it also flushes the parent directory after rename; if that final directory flush fails, the replacement may already be visible even though the call reports an I/O error. On Windows, the implementation uses `FlushFileBuffers` and `MoveFileExW` with replace/write-through flags.

This is bounded local-file I/O, **not** a project sandbox. A higher-level project service must canonicalize or otherwise constrain untrusted project paths to the authorized project root. Durable storage guarantees also depend on filesystem and device behavior.
