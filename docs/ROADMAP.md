# StormEngine Roadmap

Statuses are evidence-based. Planned means no implementation exists. Implemented means source code exists. Tested means an applicable automated test has passed. Target-architecture cross-compilation is recorded separately from physical-device verification. Verified requires the appropriate target-device or platform runtime validation.

| Phase | Scope | Current status |
|---|---|---|
| 0 | Repository audit and project bootstrap | Implemented (initial repository setup and main initialization recorded in Git history) |
| 1 | CMake, C++20 foundation, errors/results, logging contract, memory budget utility, minimal lifecycle, normalized platform lifecycle contract, bounded lifecycle event queue, generation-checked surface lifecycle contract, fixed-step timing utility, host tests | In progress; Foundation/Core/Platform host test executables pass with GCC, Clang, Apple Clang, and ASan/UBSan. The seven fixed-step scheduler tests pass in all four host configurations. Core/Platform libraries cross-compile for Android arm64-v8a and iOS device arm64. |
| 2 | Android/iOS platform adapters and native lifecycle | In progress; an Android NativeActivity entry point maps lifecycle commands and ANativeWindow metadata into platform-neutral lifecycle/surface contracts. A platform-neutral iOS scene-event adapter and an Objective-C++ UIWindowSceneDelegate bridge are implemented; seven host policy tests pass and the delegate source is pending iOS SDK CI compilation. Installable app targets, runtime consumer integration, input integration, deployment, and device tests remain planned. |
| 3 | Game loop, event system, scene/entity/component and serialization systems | In progress; a platform-neutral callback-driven GameLoop coordinates input, events, bounded fixed updates, variable update, and render. A synchronous typed EventDispatcher is now implemented with RAII subscriptions, deterministic mutation semantics, and Result-based handler failure propagation; its ten dedicated tests passed in the six-job CI run. Native clock/lifecycle integration, scenes, entities, components, serialization, and cross-thread game-event transport remain planned. |
| 4 | True 1D gameplay and physics model | Planned |
| 5 | 2D rendering, physics, animation and tools | Planned |
| 6 | In-game UI runtime | Planned |
| 7 | 3D rendering foundation and model pipeline | Planned |
| 8 | Mobile editor | Planned |
| 9 | Android and iOS export pipelines | Planned |
| 10 | Advanced audio, scripting, profiling, plugins and networking | Planned |
| 11 | Security, compatibility, stress and performance hardening | Planned |
| 12 | Release engineering | Planned |

The GameLoop is a frame coordinator only. It reports timing outcomes and calls application-supplied phases; it does not itself implement input devices, physics, animation, rendering, or platform lifecycle handling.

A phase must not be marked complete merely because files or directory names exist. Each capability needs implementation and validation evidence. Cross-compilation alone does not complete the mobile platform phase.
