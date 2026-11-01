# StormEngine Roadmap

Statuses are evidence-based. Planned means no implementation exists. Implemented means source code exists. Tested means an applicable automated test has passed. Target-architecture cross-compilation is recorded separately from physical-device verification. Verified requires the appropriate target-device or platform runtime validation.

| Phase | Scope | Current status |
|---|---|---|
| 0 | Repository audit and project bootstrap | Implemented (initial repository setup and main initialization recorded in Git history) |
| 1 | CMake, C++20 foundation, errors/results, logging contract, memory budget utility, minimal lifecycle, normalized platform lifecycle contract, bounded lifecycle event queue, generation-checked surface lifecycle contract, fixed-step timing utility, host tests | In progress; Foundation/Core/Platform host test executables pass with GCC, Clang, Apple Clang, and ASan/UBSan. The seven fixed-step scheduler tests pass in all four host configurations. Core/Platform libraries cross-compile for Android arm64-v8a and iOS device arm64. |
| 2 | Android/iOS platform adapters and native lifecycle | In progress; Android NativeActivity and iOS scene lifecycle adapters are implemented and target-compiled. The new Runtime frame driver gates the Core loop on active lifecycle and surface readiness, but neither native adapter is wired to a native frame source or installable app target. Native input, frame pacing, deployment, and device tests remain planned. |
| 3 | Game loop, event system, scene/entity/component and serialization systems | In progress; Core GameLoop and typed EventDispatcher are implemented. The Runtime MobileFrameDriver adds lifecycle/surface gating, timestamp validation, and timing reset across lifecycle revisions and surface generations. Scene provides move-only ownership, generation-checked EntityHandles, scene-local typed components, validated hierarchy, deterministic entity/component iteration, and a separate version-1 bounded Scene archive with explicit stable-ID/schema-version component codecs. The archive has thirteen dedicated host tests, and its built-in Transform3D codec path is compiled for Android/iOS targets. The Math target and hierarchy-aware TransformSystem provide quaternion-based local TRS and iterative world-matrix composition. Reflection, multi-component queries, automatic schema migrations, transform caching/dirty propagation, filesystem persistence, activation/transitions, real native frame-source integration, and cross-thread game-event transport remain planned. |
| 4 | True 1D gameplay and physics model | In progress; the first scalar PhysicsWorld1D slice is implemented with bounded static/kinematic/dynamic interval bodies, force/impulse integration, triggers, filtered contacts, discrete one-axis resolution, filtered closed-interval overlap queries, and nearest-hit scalar raycasts. Twenty-seven dedicated host tests pass under GCC, Clang, Apple Clang, and Clang ASan/UBSan; the Physics1D library cross-compiles for Android arm64-v8a and iOS device arm64 in CI run [41329544664](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41329544664). Rendering, 1D camera/input/UI integration, continuous collision detection, persistent contact phases, and performance optimization remain planned. |
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
