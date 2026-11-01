# StormEngine Roadmap

Statuses are evidence-based. Planned means no implementation exists. Implemented means source code exists. Tested means an applicable automated test has passed. Target-architecture cross-compilation is recorded separately from physical-device verification. Verified requires the appropriate target-device or platform runtime validation.

| Phase | Scope | Current status |
|---|---|---|
| 0 | Repository audit and project bootstrap | Implemented (initial repository setup and main initialization recorded in Git history) |
| 1 | CMake, C++20 foundation, errors/results, logging contract, memory budget utility, minimal lifecycle, normalized platform lifecycle contract, bounded lifecycle event queue, generation-checked surface lifecycle contract, fixed-step timing utility, host tests | In progress; existing host tests pass with GCC, Clang, Apple Clang, and ASan/UBSan. Fixed-step scheduler implementation and its dedicated host tests are now added; the next CI run must validate them. Foundation/Core/Platform static libraries cross-compile for Android arm64-v8a and iOS device arm64. More core services remain planned. |
| 2 | Android/iOS platform adapters and native lifecycle | In progress; an Android NativeActivity entry point now maps lifecycle commands and ANativeWindow metadata into platform-neutral lifecycle/surface contracts. Android arm64 CI compiles and links the shared bridge with the NDK Native App Glue. iOS UIKit/Objective-C++ adapter, installable app targets, input integration, deployment, and device tests remain planned. |
| 3 | Game loop, event system, scene/entity/component and serialization systems | Planned |
| 4 | True 1D gameplay and physics model | Planned |
| 5 | 2D rendering, physics, animation and tools | Planned |
| 6 | In-game UI runtime | Planned |
| 7 | 3D rendering foundation and model pipeline | Planned |
| 8 | Mobile editor | Planned |
| 9 | Android and iOS export pipelines | Planned |
| 10 | Advanced audio, scripting, profiling, plugins and networking | Planned |
| 11 | Security, compatibility, stress and performance hardening | Planned |
| 12 | Release engineering | Planned |

The fixed-step scheduler is a reusable timing primitive only. It reports how many fixed updates a caller may execute and an interpolation alpha; it does not itself run the game loop or physics.

A phase must not be marked complete merely because files or directory names exist. Each capability needs implementation and validation evidence. Cross-compilation alone does not complete the mobile platform phase.
