# StormEngine Roadmap

Statuses are evidence-based. \`Planned\` means not implemented. \`Implemented\` means code exists. \`Tested\` means an applicable automated test has passed. \`Verified\` requires the appropriate target-device or platform validation.

| Phase | Scope | Current status |
|---|---|---|
| 0 | Repository audit and project bootstrap | Implemented (empty repository confirmed and main initialized) |
| 1 | CMake, C++20 foundation, errors/results, logging contract, memory budget utility, minimal lifecycle, host tests | Implemented; build and host tests passed locally |
| 2 | Android/iOS platform adapters and native lifecycle | Planned |
| 3 | Game loop, events, scene/entity/component and serialization systems | Planned |
| 4 | True 1D gameplay and physics model | Planned |
| 5 | 2D rendering, physics, animation and tools | Planned |
| 6 | In-game UI runtime | Planned |
| 7 | 3D rendering foundation and model pipeline | Planned |
| 8 | Mobile editor | Planned |
| 9 | Android and iOS export pipelines | Planned |
| 10 | Advanced audio, scripting, profiling, plugins and networking | Planned |
| 11 | Security, compatibility, stress and performance hardening | Planned |
| 12 | Release engineering | Planned |

A phase must not be marked complete merely because files or directory names exist. Each capability needs implementation and validation evidence.
