# StormEngine Roadmap

Statuses are evidence-based. Planned means no implementation exists. Implemented means source code exists. Tested means an applicable automated test has passed. Target-architecture cross-compilation is recorded separately from physical-device verification. Verified requires the appropriate target-device or platform runtime validation.

| Phase | Scope | Current status |
|---|---|---|
| 0 | Repository audit and project bootstrap | Implemented (initial repository setup and main initialization recorded in Git history) |
| 1 | CMake, C++20 foundation, errors/results, logging contract, memory budget utility, minimal lifecycle, normalized platform lifecycle contract, bounded lifecycle event queue, generation-checked surface lifecycle contract, host tests | In progress; host tests pass with GCC, Clang, Apple Clang, and ASan/UBSan. Foundation/Core/Platform static libraries also cross-compile for Android arm64-v8a and iOS device arm64. More core services remain planned. |
| 2 | Android/iOS platform adapters and native lifecycle | In progress; target-toolchain CI cross-builds for Android arm64-v8a and iOS device arm64 pass. Native Activity/JNI and UIKit/Objective-C++ lifecycle adapters, native surface integration, app targets, deployment, and device tests remain planned. |
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

A phase must not be marked complete merely because files or directory names exist. Each capability needs implementation and validation evidence. Cross-compilation alone does not complete the mobile platform phase.
