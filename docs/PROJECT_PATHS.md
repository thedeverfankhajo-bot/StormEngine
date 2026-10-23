# Project-relative logical paths

\`storm::project::ProjectPath\` is an owned, validated logical path for project resources. It is intentionally independent of \`std::filesystem::path\`: a logical project path is not yet an operating-system path and cannot open or modify files by itself.

## Version-1 path rules

- Paths are non-empty, relative, UTF-8 byte strings using \`/\` as their only separator.
- The full path is at most 1,024 bytes; a segment is at most 255 bytes; at most 64 segments are allowed.
- Empty segments, leading/trailing separators, \`.\`, and \`..\` segments are rejected.
- NUL, ASCII control bytes, backslashes, colons, Windows-invalid filename punctuation, and segment names ending in a dot or space are rejected.
- Windows reserved device names are rejected, including the common CON/PRN/AUX/NUL, COM1–COM9 and LPT1–LPT9 forms, their extensions, CONIN$/CONOUT$, and COM/LPT superscript 1–3 variants.
- Invalid, overlong, surrogate, truncated, and out-of-range UTF-8 sequences are rejected.
- Accepted spelling is preserved exactly. Paths are case-sensitive logical keys, and this type does not perform Unicode normalization or filesystem-specific case folding.
- Parsing failure leaves the caller's output value unchanged.

## Use

~~~cpp
#include "storm/project/project_path.hpp"

storm::project::ProjectPath asset_path;
const auto status =
    storm::project::ProjectPath::parse("assets/hero.png", asset_path);
if (!status.ok()) {
    // Report invalid project metadata to the caller.
}
~~~

## Security boundary

This type is a **syntax and naming policy**, not a filesystem sandbox. It does not resolve a root directory, check symlinks, prevent mount-point changes, or defeat time-of-check/time-of-use races. Do not use it as the only security check before opening user-controlled files.

A future project-root service must consume this logical value and perform root-confined I/O with platform-specific directory-handle/handle-relative operations. The current \`SceneFile\` service still accepts an ordinary filesystem path and intentionally remains separate until that service exists.
