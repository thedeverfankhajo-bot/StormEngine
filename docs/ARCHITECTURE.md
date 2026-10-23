# StormEngine architecture

**Status:** initial direction, not a frozen design. Decisions must be backed by prototypes, test results, and recorded trade-offs.

## Product shape

StormEngine is intended to be one engine with 1D, 2D, and 3D scene capabilities, rather than three separate codebases. Dimension-aware components and scene/project settings may constrain movement, rendering, and editor tools, but should not duplicate unrelated runtime infrastructure.

The Unreal Engine 5 comparison is a long-term product ambition, not a claim of present feature parity. Large-engine capabilities will be added only when their architecture, dependencies, performance, and maintenance costs are understood.

## Architectural boundaries

The planned dependency direction is from higher-level modules toward narrow lower-level interfaces. Lower-level code must not depend on the editor.

| Module | Responsibility | Current state |
| --- | --- | --- |
| Core | Versioning, status/error conventions, math, deterministic timing, lifecycle and low-level utilities | Version metadata, dimension-aware math, fixed-step clock, Status, RuntimeLifecycle and logging dispatch |
| Platform | Windowing, input, monotonic clocks, filesystems, threads and OS integration | Planned |
| Render | Render-device abstraction, resource lifetime, command submission and capabilities | Planned; no graphics API dependency yet |
| Scene | Worlds/scenes, entities, transforms, lifecycle and serialization | Planned |
| Assets | Importers, validation, metadata, cache and cooked asset packages | Planned |
| Physics | 2D/3D simulation interfaces, collision queries and fixed-step integration | Planned |
| Audio | Device abstraction, spatial audio and asset streaming | Planned |
| Scripting | Safe, versioned gameplay extension API | Design pending |
| Editor | Project/scene/asset tools, inspectors and build/export UI | Planned |
| Tools | Headless asset processing, validation, packaging and diagnostics | Planned |

## Core and language

- Use C++20 initially to keep the language baseline explicit and broadly supported by modern desktop toolchains and the Android NDK.
- Prefer RAII, value semantics, narrow interfaces, explicit ownership and standard library types. Avoid raw owning pointers.
- Keep public headers small and stable. Compile implementation details in library targets where practical.
- Keep compiler flags target-scoped. Never force optimization flags globally; build configurations should own optimization policy.
- Do not introduce templates, SIMD specializations, allocators, a custom ECS, or a custom container library without a measured reason.

## Errors and runtime lifecycle

The Status type provides a shared error vocabulary and an owned diagnostic message, so a message cannot dangle because its original temporary string disappeared. A failure cannot use ErrorCode::None; that misuse is normalized to Internal. Subsystems should return meaningful, stable error categories and include context without embedding sensitive user data.

RuntimeLifecycle validates transitions among Uninitialized, Initialized, Running, Paused, Stopping, Stopped, and Failed. Invalid transitions return InvalidState and leave the state unchanged. Resource setup/teardown remains the responsibility of the future platform/runtime owner; finish_stop() is called only once resources are released. A fatal fail() stores the first failure reason and moves to Failed. Stopped and Failed require explicit reset() before the runtime can be initialized again.

The initial state machine is single-owner-thread only and is not a synchronization primitive. Platform events, callbacks and shutdown requests must be marshalled to the owning runtime thread; thread-safe event delivery will be designed with the platform shell. A successful status from initialize() means the state transition was accepted, not that any OS/GPU resource has already been allocated.

## Logging and diagnostics

The core `Logger` applies a minimum severity filter and forwards records to a caller-provided sink. It performs no I/O, formatting, or process-global configuration. The default instance has no sink, so logging is silent until a host explicitly supplies one.

A log record carries non-owning string views. They are valid only during the synchronous sink call; a sink that retains a message must copy it. The caller owns the sink context and must keep it alive while the logger uses it. Sinks must not throw. The logger is not thread-safe, so concurrent producers must serialize access externally until a dedicated diagnostics queue is designed.

## Deterministic timing

The initial FixedStepClock accepts caller-supplied frame deltas rather than reading a platform clock. It bounds each accepted frame delta, caps the number of simulation steps requested per frame, reports discarded time explicitly, and exposes a fractional accumulator for render interpolation. Invalid (negative or non-finite) deltas are rejected without mutating state. Reconfiguration is validated and resets the accumulator so time from a previous simulation step does not leak into the new configuration.

This is a timing primitive, not a complete runtime loop. The platform layer will later provide monotonic time and lifecycle-aware pause/resume handling. Simulation determinism still depends on gameplay, physics, scheduling, and floating-point behavior; a fixed step alone does not guarantee deterministic replay or networking.

## Rendering strategy

Rendering must sit behind a Render Hardware Interface (RHI), separating engine-facing operations from specific APIs. Candidate backends include Vulkan and compatibility-oriented options where platform coverage warrants them. No backend is selected as a hard dependency in this bootstrap.

Before committing to a renderer, build a small proof of concept covering device selection, swapchain/surface behavior, resource lifetimes, shader compilation, mobile thermal/battery behavior, driver variation, and CI validation. A backend is not "Android supported" merely because it compiles for Android.

## Scenes, dimensions and simulation

- One scene model should express a 1D, 2D, or 3D project through explicit dimension settings.
- Transform conventions, coordinate handedness, units, angles, matrix layout, and serialization rules must be documented before renderer/physics integration.
- Simulation timing should be separate from rendering frequency. Fixed-step simulation and interpolation are available as a core primitive; integration into an actual runtime loop remains future work.
- The ECS versus scene-graph/entity API must be decided with representative workloads and editor requirements, not fashion alone.
- Scene and asset formats must be versioned, validated, and have migration policies before they become long-term user data.

## Assets and extensions

Treat project files and imported assets as untrusted input. Parsers must enforce size/count/depth limits, validate offsets and references, and fail safely on malformed data. Asset import should be separated from runtime loading where feasible. Do not execute scripts or native code merely because they appear inside an asset package.

Before adding an external dependency, record its purpose, supported platforms, license, transitive dependencies, update policy, and fallback/exit strategy. Plugin ABI and scripting sandboxing remain open design decisions.

## Android and desktop

- The native core must remain independent of Android UI and lifecycle details.
- Android-specific surface, activity lifecycle, input, files, storage permissions and packaging belong in platform integration code.
- Use the official Android NDK CMake toolchain for cross-compilation.
- Keep desktop and Android build options explicit and verify each target with a supported compiler/ABI matrix.
- Do not claim platform support until native builds, package integration, smoke tests and a documented minimum OS/device profile exist.

## Quality, security and commercial readiness

Each subsystem should have unit tests and an appropriate integration test. Performance claims require a reproducible benchmark, the tested hardware/software profile and a baseline. Regression thresholds should be added after collecting reliable measurements.

Before a public release, require at least: license and third-party notices; documented support matrix; reproducible build steps; automated tests; dependency and vulnerability review; asset-format compatibility policy; crash/diagnostic policy; release versioning; platform packaging/signing guidance; and a support/disclosure process.

## Decisions intentionally deferred

The following choices should be recorded as Architecture Decision Records before adoption: renderer backend and shader toolchain, windowing/input library, physics engine, audio backend, scripting language/runtime, editor UI framework, ECS/scene model, asset container, plugin ABI and project license.
