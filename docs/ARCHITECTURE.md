# StormEngine architecture

**Status:** initial direction, not a frozen design. Decisions must be backed by prototypes, test results, and recorded trade-offs.

## Product shape

StormEngine is intended to be one engine with 1D, 2D, and 3D scene capabilities, rather than three separate codebases. Dimension-aware components and scene/project settings may constrain movement, rendering, and editor tools, but should not duplicate unrelated runtime infrastructure.

The Unreal Engine 5 comparison is a long-term product ambition, not a claim of present feature parity. Large-engine capabilities will be added only when their architecture, dependencies, performance, and maintenance costs are understood.

## Architectural boundaries

The planned dependency direction is from higher-level modules toward narrow lower-level interfaces. Lower-level code must not depend on the editor.

| Module | Responsibility | Current state |
| --- | --- | --- |
| Core | Versioning, status/error conventions, math, deterministic timing, lifecycle and low-level utilities | Version metadata, dimension-aware math, fixed-step clock, RuntimeConfig, Status, RuntimeLifecycle, stable IDs and bounded diagnostics |
| Platform | Windowing, input, monotonic clocks, filesystems, threads and OS integration | Adapter-fed input state/event queue plus an optional SDL3 desktop bootstrap; full platform runtime still planned |
| Render | Render-device abstraction, resource lifetime, command submission and capabilities | Planned; no graphics API dependency yet |
| Scene | Worlds/scenes, entities, transforms, lifecycle and serialization | Initial in-memory Scene/Entity/Transform API; versioned serialization is not implemented |
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

## Platform-neutral digital input state

`storm/platform/input_state.hpp` provides a fixed-capacity, allocation-free state snapshot for 256 adapter-fed key slots and 8 pointer-button slots. Adapters update held state as native events arrive; `begin_frame()` clears edge flags while preserving held controls. Press and release edges are both retained if both transitions happen within one frame. `release_all()` publishes release edges for all held controls so a focus loss or device detach cannot leave a key stuck; `reset()` clears all state without publishing edges.

The primitive validates slot bounds, does not poll OS devices, and does not deliver events. `KeyCode` provides stable names for common physical keyboard keys; adapters translate native scan codes into those names. Text entry, IME and localized character events remain separate from physical key state. Like the initial runtime kernel, it is single-owner-thread only and provides no synchronization for concurrent event producers.

## Stable keyboard codes

`storm/platform/key_code.hpp` defines named physical-key codes for letters, digits, common navigation/control keys, F1–F12 and left/right modifiers. These names are engine-owned and independent of SDL numeric enums. Letter/digit names represent physical key positions, not translated text characters. Platform adapters map native key positions into this enum; text input, IME, dead keys and localized characters belong to a separate text-input service. `KeyCode::Count` is a sentinel and is rejected as a key/event value.

## Bounded platform event queue

`storm/platform/event_queue.hpp` defines engine-owned event values for quit requests, focus changes, digital key/button changes, pointer motion/wheel input and window resizing. `PlatformEventQueue<Capacity>` is a fixed-capacity FIFO; it does not allocate, overwrite already accepted events, and counts rejected malformed payloads separately from valid events rejected because the queue is full. Counters saturate instead of wrapping. Key slots, pointer-button slots and finite floating-point coordinates are validated before events enter the queue. Zero-sized surfaces are accepted to represent minimized windows.

The queue and `InputState` are single-owner-thread building blocks, not a cross-thread synchronization mechanism and not a native OS event pump. A future platform adapter must define key-code mapping and coordinate units, check push failures, and release held input on focus loss even if an overflow prevents the focus event from being queued. Window identifiers and multi-window routing remain future contracts.

## Errors and runtime lifecycle

The Status type provides a shared error vocabulary and an owned diagnostic message, so a message cannot dangle because its original temporary string disappeared. A failure cannot use ErrorCode::None; that misuse is normalized to Internal. Subsystems should return meaningful, stable error categories and include context without embedding sensitive user data.

RuntimeLifecycle validates transitions among Uninitialized, Initialized, Running, Paused, Stopping, Stopped, and Failed. Invalid transitions return InvalidState and leave the state unchanged. Resource setup/teardown remains the responsibility of the future platform/runtime owner; finish_stop() is called only once resources are released. A fatal fail() stores the first failure reason and moves to Failed. Stopped and Failed require explicit reset() before the runtime can be initialized again.

The initial state machine is single-owner-thread only and is not a synchronization primitive. Platform events, callbacks and shutdown requests must be marshalled to the owning runtime thread; thread-safe event delivery will be designed with the platform shell. A successful status from initialize() means the state transition was accepted, not that any OS/GPU resource has already been allocated.

## Runtime configuration

`RuntimeConfig` groups the fixed-step simulation settings and minimum log severity used by the initial runtime kernel. Call `validate()` before initializing services; invalid timing bounds or an unknown log level return `InvalidArgument`. Validation does not allocate resources, start the runtime, load files, or mutate the configuration. Persistent project settings and config-file parsing/migration are separate future layers.

## Logging and diagnostics

The core `Logger` applies a minimum severity filter and forwards records to a caller-provided sink. It performs no I/O, formatting, or process-global configuration. The default instance has no sink, so logging is silent until a host explicitly supplies one.

A log record carries non-owning string views. They are valid only during the synchronous sink call; a sink that retains a message must copy it. The caller owns the sink context and must keep it alive while the logger uses it. Sinks must not throw. The logger is not thread-safe, so concurrent producers must serialize access externally until a dedicated diagnostics queue is designed.

## Bounded diagnostics buffer

`DiagnosticBuffer<Capacity, MaxCategoryBytes, MaxMessageBytes>` stores a fixed number of copied records in a ring. The capacity and byte limits are compile-time parameters; appending a message does not allocate memory. Once full, it overwrites the oldest record and increments a saturating dropped-record counter. Per-severity and total counters include accepted records even after their entries are evicted, until `clear()` resets the buffer.

Records with an invalid severity or `Off` are rejected. Long category/message fields are truncated by bytes and flagged in the returned view. Because the data is byte-oriented, callers that need valid UTF-8 must ensure truncation does not split a multi-byte sequence or use a formatting layer that does. Views returned from `at()` are non-owning and become logically invalid if their slot is overwritten, the buffer is cleared, or the buffer is destroyed. The buffer is not thread-safe; callers must serialize access.

The buffer can be attached to the existing logger using `DiagnosticBuffer<...>::capture_sink`. It is a bounded in-memory diagnostics primitive, not persistent crash reporting or a multi-threaded queue.

## Stable identifiers

`StableId` stores 128 identifier bits in byte order matching the canonical UUID text representation and supports the RFC 9562 hex-and-dash format. Parsing accepts upper- or lowercase hexadecimal but rejects malformed separators and the all-zero Nil UUID, which this API reserves to mean "no ID". The default-constructed value is invalid.

This value type does not generate IDs by itself. `StableIdGenerator::generate_v4` now generates UUIDv4 values using Windows CNG, Apple's Security randomization service, or the POSIX `/dev/urandom` kernel source. It checks each API's result, sets UUIDv4 version/variant bits as specified by [RFC 9562](https://www.rfc-editor.org/rfc/rfc9562.html), and leaves the output unchanged if entropy retrieval fails; it never falls back to a non-cryptographic PRNG. The in-memory `StableIdRegistry` detects duplicates and missing entries using an ordered, bytewise registry with logarithmic lookup; it rejects invalid IDs and reports duplicate registration as `AlreadyExists`. It is not thread-safe. Persistence, synchronization, and scene/asset ownership remain responsibilities of future layers.

## Deterministic timing

The initial FixedStepClock accepts caller-supplied frame deltas rather than reading a platform clock. It bounds each accepted frame delta, caps the number of simulation steps requested per frame, reports discarded time explicitly, and exposes a fractional accumulator for render interpolation. Invalid (negative or non-finite) deltas are rejected without mutating state. Reconfiguration is validated and resets the accumulator so time from a previous simulation step does not leak into the new configuration.

This is a timing primitive, not a complete runtime loop. The platform layer will later provide monotonic time and lifecycle-aware pause/resume handling. Simulation determinism still depends on gameplay, physics, scheduling, and floating-point behavior; a fixed step alone does not guarantee deterministic replay or networking.

## Desktop platform bootstrap

The optional \`STORM_BUILD_SDL3_DEMO\` target uses SDL3 3.4.18 for a desktop window, keyboard/mouse event pumping and a minimal interactive 2D sample. The adapter maps supported SDL physical scancodes to the engine-owned `KeyCode` enum, so gameplay code does not depend on SDL numeric scancode values. It routes events through \`PlatformEventQueue\`, consumes focus/resize/input events on one owner thread, releases held inputs after focus loss or queue overflow, and advances movement through the existing fixed-step clock.

This dependency is opt-in for ordinary developers and pinned by an official source-archive SHA-256; desktop CI opts in to build it on Linux, macOS and Windows. The SDL renderer is used only to prove window/event/frame integration. The engine-facing render-device abstraction, GPU-resource lifetime model, texture/sprite asset pipeline and 3D RHI remain unimplemented. Android application glue, APK packaging and device lifecycle tests are separate work; building the SDL source alone would not establish Android app support.

## Rendering strategy

Production rendering must sit behind a Render Hardware Interface (RHI), separating engine-facing operations from specific APIs. Candidate backends include Vulkan and compatibility-oriented options where platform coverage warrants them. The SDL renderer used by the desktop demo is not that RHI and is not a production-backend selection.

Before committing to a renderer, build a small proof of concept covering device selection, swapchain/surface behavior, resource lifetimes, shader compilation, mobile thermal/battery behavior, driver variation, and CI validation. A backend is not "Android supported" merely because it compiles for Android.

## Scenes, dimensions and simulation

- One scene model should express a 1D, 2D, or 3D project through explicit dimension settings.
- Transform conventions, coordinate handedness, units, angles, matrix layout, and serialization rules must be documented before renderer/physics integration.
- Simulation timing should be separate from rendering frequency. Fixed-step simulation and interpolation are available as a core primitive; integration into an actual runtime loop remains future work.
- The ECS versus scene-graph/entity API must be decided with representative workloads and editor requirements, not fashion alone.
- Scene and asset formats must be versioned, validated, and have migration policies before they become long-term user data.

## Scene, entities and transforms

The initial `Scene` owns its `Entity` objects in an ordered map keyed by secure UUIDv4 `StableId` values. Creation is synchronous, reports failures using the shared `Status` vocabulary, and changes the caller's output ID only when insertion succeeds. Scene entity count and entity-name length are bounded; empty names and embedded NUL bytes are rejected. Map lookup is logarithmic, and pointers returned by `find_entity()` are non-owning and must not outlive the scene or the referenced entity.

A scene has exactly one validated `Dimension`. Each entity starts with the matching `Transform1D`, `Transform2D`, or `Transform3D`; transform updates reject non-finite components and cross-dimension values without changing the old transform. Entity names can be changed after validation, while empty, oversized, or embedded-NUL names are rejected without mutating the old name. `visit_entities()` traverses the owned map in stable-ID byte order through a non-allocating callback, without exposing the storage container; callbacks must not mutate the same scene during traversal. This is a small ownership/API foundation, not yet a world hierarchy, component system, or renderer contract. The dependency-free version-1 SceneCodec provides bounded binary save/load for these records; the format is documented in [Scene format](SCENE_FORMAT.md), while multi-version migration remains future work. Scenes are single-owner-thread only. Transform angles are in radians, while axis order, handedness and renderer/physics conventions remain open decisions to document before those systems depend on them.

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
