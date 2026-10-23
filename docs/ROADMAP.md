# Roadmap

This roadmap is milestone-based, not a promise of release dates. A milestone is complete when its acceptance criteria are demonstrated in code and tests.

## M0 — Repository foundation

- Establish the C++20/CMake project, portable core library and test harness.
- Add baseline CI for Linux, macOS and Windows.
- Write architecture, build, security and contribution guidance.
- Capture the explicit rule that every change begins with a repository-wide review.

**Acceptance:** a clean configure/build/test succeeds on all three CI operating systems; no third-party runtime dependency is required.

## M1 — Deterministic runtime kernel

- [x] Define an initial fixed-step clock with frame-delta limits, a bounded per-frame step budget, interpolation alpha, and explicit dropped-time reporting.
- [x] Add a shared status/error convention and a validated runtime lifecycle state machine.
- [x] Add dependency-free core log-level filtering and synchronous dispatch to a caller-provided sink.
- [x] Add a 128-bit stable identifier value type with canonical UUID parsing/formatting and malformed/Nil ID rejection.
- [x] Add a validated in-memory runtime configuration for fixed-step timing and log severity.
- [x] Add an in-memory stable-ID registry that rejects duplicate and invalid IDs.
- [x] Add OS-backed UUIDv4 generation with checked native secure-random APIs and no weak fallback.
- [x] Add a fixed-capacity diagnostics ring with byte-bounded messages, severity counters, truncation flags and overwrite accounting.
- Establish resource ownership conventions and runtime integration.
- [x] Add a Linux AddressSanitizer/UndefinedBehaviorSanitizer CI job and instrument the core tests.
- Continue expanding boundary/malformed-input tests and integrate diagnostics with platform/runtime ownership.

**Acceptance for the implemented primitives:** deterministic tests cover fixed-step timing boundaries, invalid input, lifecycle transitions, failure propagation and explicit recovery. The remaining runtime-kernel work is still in progress.

## M2 — Platform shell and first visible 2D slice

- [x] Add a platform-neutral, fixed-capacity digital input-state primitive with per-frame edge tracking, bounds checks and focus-loss release behavior.
- [x] Add a fixed-capacity platform-event FIFO with payload validation, explicit overflow/rejection counts and input-state integration tests.
- [x] Define stable engine-owned physical `KeyCode` names and translate SDL desktop scancodes through an explicit adapter mapping.
- Prototype an opt-in SDL3 desktop window/input loop and visible 2D rectangle; CI compile-tests the example, but a manual interactive GUI smoke test is still required.
- Choose production window/input/platform dependencies after license and maintenance review; the prototype choice is not a frozen engine ABI.
- Add a desktop window, input events and a minimal render-device interface.
- Render a simple 2D scene with a camera, transform and texture/sprite.
- Add a documented example project and an automated smoke test where practical.

**Acceptance:** an executable starts, presents a frame, handles close/input events and reports unsupported platform features clearly.

## M3 — Asset and scene pipeline

- [x] Define a dimension-aware in-memory Scene/Entity/Transform contract with stable IDs, validated transform/name updates, and deterministic entity traversal.
- [x] Add a version-1 binary scene codec with stable-ID preservation, deterministic record order, bounded input/output, strict malformed-data validation, and transactional loading.
- [x] Add native bounded scene-file loading and same-directory atomic replacement with flush/error handling; the project-root sandbox and crash-recovery journal remain future work.
- [x] Define a portable UTF-8 `ProjectPath` value with lexical traversal/portable-name rejection and explicit failure atomicity; it is not yet an OS-level sandbox.
- Define the multi-version scene migration policy and migration tests before introducing a newer format version.
- Build import, validation, metadata, cache and cooked-asset steps.
- Add content hashes, import diagnostics and malformed-input tests.
- [x] Record the initial world-coordinate and matrix conventions; add tested 4×4 affine matrix operations. The project settings contract remains future work.

**Acceptance:** a sample scene can be imported, validated, saved, reopened and built headlessly with stable identifiers.

## M4 — 3D rendering and simulation

- Evaluate renderer backends with a vertical-slice prototype.
- Add meshes, materials, camera, lighting and shader compilation.
- Define physics integration and fixed-step behavior based on measured needs.
- Add render and simulation benchmarks before optimization work.

**Acceptance:** a tested 3D sample on the declared hardware matrix, with documented API/resource lifetimes and measurable performance baselines.

## M5 — Android native runtime

- Cross-compile core and selected runtime modules using the Android NDK.
- Integrate native lifecycle, surface, input, files and package build.
- Test arm64-v8a first, then decide additional ABIs based on support goals.
- Run device smoke tests, thermal/performance checks and lifecycle interruption tests.

**Acceptance:** a packaged test application launches on real devices, survives pause/resume and surface recreation, and documents API/device requirements.

## M6 — Editor and production workflow

- Select an editor UI framework based on portability, accessibility, licensing and maintenance.
- Build project browser, scene hierarchy, inspector, asset import feedback and viewport.
- Add undo/redo, crash-safe saving and build/export orchestration.
- Keep editor-only dependencies out of exported runtime builds.

**Acceptance:** a developer can create, edit, save, reopen and package a small sample without manually editing engine internals.

## M7 — Scripting, audio and broader production tools

- Select a scripting/runtime strategy with explicit trust and sandbox boundaries.
- Add audio, profiling, debug drawing, asset diagnostics and crash reporting policy.
- Add plugin boundaries only after defining ABI/versioning and trust constraints.

**Acceptance:** representative sample games exercise the workflows; performance and failure modes are documented.

## M8 — Commercial release readiness

- Decide and publish the project license and third-party notice process.
- Add versioned releases, supported-platform matrix, release checksums and provenance.
- Establish upgrade/migration policy for projects and asset formats.
- Document packaging, platform signing, privacy/data handling, support and vulnerability disclosure.
- Benchmark against declared workloads rather than unsupported competitor claims.

**Acceptance:** a clean machine can follow the documented process to build, test, package and verify a sample project for every platform advertised as supported.
