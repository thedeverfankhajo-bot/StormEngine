# Testing and CI

## Current automated coverage

The current host CMake configuration registers twenty-three CTest executables. Registration is not evidence of a passing run: each compiler, sanitizer, and mobile target result must be taken from the specific [Foundation CI](https://github.com/thedeverfankhajo-bot/StormEngine/actions/workflows/foundation-ci.yml) run for the commit being evaluated. The dedicated Physics2D executable contains twenty-four named world cases.

- stormengine_foundation_tests (11 cases):
  - Success and failure paths for typed Result<T> values.
  - Memory-budget capacity, over-reservation, release validation, and concurrent reservations.
  - Injectable logging and structured record fields.
  - Engine initialization and shutdown lifecycle.
  - Platform lifecycle transitions, invalid transitions, and background launch.
  - Lifecycle event queue FIFO ordering, capacity/close behavior, and concurrent multi-producer delivery.
- stormengine_platform_surface_tests (7 cases):
  - Surface attach validation and monotonically increasing generations.
  - Resize validation and generation preservation.
  - Detach/recreation behavior and stale-callback rejection.
  - Invalid resize/detach operations when no surface is attached.
  - Logical point dimensions and display scale converted to rounded pixel extents.
  - Rejection of zero, negative, non-finite, or sub-pixel-empty dimensions and invalid scales.
  - Overflow rejection when a logical size cannot fit a 32-bit pixel extent.
- stormengine_android_lifecycle_tests (5 cases):
  - START/RESUME/FOCUS/PAUSE/STOP event normalization.
  - STOP from an active state with an explicit inactive transition.
  - Focus recovery when a separate RESUME event is absent.
  - Idempotent duplicate lifecycle events.
  - Terminal destroy behavior and invalid event rejection.
- stormengine_ios_lifecycle_tests (7 cases):
  - Foreground/background callback sequence.
  - Active callback recovery from background.
  - Background launch and repeated background delivery.
  - Idempotent scene callbacks.
  - Independent lifecycle state for two scenes.
  - Terminal app termination.
  - Unknown scene/application event rejection.
- stormengine_fixed_step_scheduler_tests (7 cases):
  - Invalid step, frame-delta limit, and catch-up budget configuration.
  - Partial frame accumulation and fixed-step emission.
  - Frame-delta clamping.
  - Catch-up budget enforcement while preserving the fractional remainder.
  - Negative-delta rejection without mutating scheduler state.
  - Explicit accumulator reset.
  - Duration-overflow reporting without mutating scheduler state.
- stormengine_game_loop_tests (9 cases):
  - Configuration validation.
  - Deterministic input/event/fixed-update/variable-update/render ordering.
  - Zero-duration frame behavior.
  - Frame-delta clamping and discarded-backlog reporting.
  - Fixed-update and variable-update failure propagation.
  - Early termination on input failure.
  - Negative-delta rejection before callbacks.
  - Timing reset across suspension.
- stormengine_event_dispatcher_tests (10 cases):
  - Subscription-order dispatch and exact event-type matching.
  - Successful dispatch with no listeners.
  - RAII disconnection and move-only subscription behavior.
  - Immediate skip after disconnecting a pending listener during dispatch.
  - New listeners taking effect on the next dispatch.
  - Clearing all listeners during an active dispatch.
  - Handler error propagation and early termination.
  - Rejection of an empty std::function handler.
  - Safe subscription destruction after the dispatcher is gone.
  - Idempotent explicit disconnection.
- stormengine_math_tests (18 cases):
  - Vector and matrix finite-value checks and identity matrix values.
  - Quaternion identity and normalization.
  - Zero-quaternion rejection.
  - Invalid axis-angle input rejection.
  - Right-handed axis-angle rotation.
  - Translation and scale matrix construction.
  - TRS scale-before-rotation behavior.
  - Parent-before-local matrix multiplication order.
  - Bounds-checked matrix access.
- stormengine_transform_system_tests (13 cases):
  - Identity defaults for entities without Transform3D.
  - Local Transform3D insertion, readback, and replacement.
  - Rejection of stale and foreign EntityHandles.
  - Rejection of non-finite transform values.
  - Local transform world-matrix calculation.
  - Parent/child translation composition.
  - Parent rotation and scale inheritance.
  - Identity behavior for grouping entities without transforms.
  - Child transform behavior after parent destruction/orphaning.
  - Iterative evaluation of a 512-entity deep hierarchy.
  - Invalid stored Transform3D diagnostics.
  - World-transform preservation across Scene moves.
- stormengine_scene_tests (18 cases):
  - Scene name validation and unique process identity.
  - Entity creation, name access, rename, and active entity counts.
  - Rejection of handles from another Scene.
  - Stale-handle invalidation and generation changes on entity-slot reuse.
  - Type-safe, scene-local component storage and const access.
  - Duplicate component and missing component error paths.
  - Component cleanup on removal and entity destruction.
  - Scene move ownership and moved-from invalidation.
  - Parent assignment and ordered direct-child lookup.
  - Reparent consistency and idempotent parent assignment.
  - Detach behavior and sibling order preservation.
  - Self-parent and ancestry-cycle rejection.
  - Foreign and stale parent-handle rejection.
  - Parent destruction orphaning direct children while preserving subtrees.
  - Child destruction unlinking and clean entity-slot reuse.
  - Hierarchy and handles preserved across Scene moves.
  - Deterministic live-entity iteration and stale-slot skipping.
  - Mutable/const typed component iteration, slot ordering, and empty-store behavior.
- stormengine_scene_serialization_tests (13 cases):
  - Full Scene round-trip of names, entities, hierarchy, sibling order, and registered components.
  - Built-in Transform1D and Transform3D codecs plus custom Position/Velocity codecs.
  - Canonical archive output independent of caller codec ordering.
  - Empty Scene round-trip without component codecs.
  - Rejection of component stores without an explicit codec.
  - Invalid magic, unsupported archive version, truncated input, and trailing-byte rejection.
  - Archive byte, entity-count, and string-size limit enforcement.
  - Unknown component ID and unsupported component schema rejection.
  - Invalid parent references and hierarchy cycle rejection.
  - Duplicate component record rejection.
  - Invalid Transform3D payload rejection.
  - Transactional BinaryWriter/BinaryReader string failure behavior.
- stormengine_mobile_frame_driver_tests (11 cases):
  - Invalid GameLoop configuration rejection.
  - Suppression of every game callback while lifecycle is inactive.
  - Suppression of callbacks while no surface is attached.
  - Zero-delta behavior for the first eligible frame.
  - Caller-supplied monotonic timestamp and fixed-step scheduling.
  - Detection of a background/resume sequence between frame calls.
  - Detection of surface detach/reattach through generation changes.
  - Rejection and recovery from a backward timestamp.
  - Non-replay of elapsed time after a callback failure.
  - Preservation of timing across a resize with the same surface generation.
  - Lifecycle revision changes only for non-idempotent transitions.

- stormengine_one_d_physics_tests (33 cases):
  - World configuration validation and unique world identities.
  - Body definition validation, including finite values, positive interval extent, valid restitution, and dynamic mass.
  - World-scoped generation-checked body handles, stale-handle rejection, and slot reuse.
  - Semi-implicit gravity integration and once-per-successful-step force consumption.
  - Immediate impulse application to dynamic-body velocity.
  - Kinematic integration and static-body immobility.
  - Inelastic and elastic one-dimensional interval collision response.
  - Exact-touch collision response when bodies are moving toward one another.
  - Trigger contact reporting without collision resolution.
  - Symmetric collision layer/mask filtering.
  - Body-count and contact-count limits with typed resource-exhaustion errors.
  - Invalid setter/force operations and transactional rejection of invalid steps.
  - Preservation of body state and accumulated force when the contact limit aborts a step.
  - Deterministic contact ordering by body-slot pair.
  - Begin/Stay/End transitions for persistent contacts, including trigger metadata.
  - End + Begin transitions when a destroyed body's slot is reused with a new generation.
  - Contact event ordering, move-preserved history, and transactional history on failed steps.
  - Closed-interval overlap queries with stable hit ordering, collision-layer filtering, and optional trigger exclusion.
  - Nearest-hit raycasts in both scalar directions, including deterministic equal-distance tie-breaking.
  - Raycast collision-layer/trigger filtering, origin-inside and boundary hits, and inclusive maximum-distance handling.
  - Rejection of invalid ray directions/ranges, numeric overflow, invalid stored collider bounds, and moved-from worlds.

These are host-side tests. The Android and iOS lifecycle cases test platform-neutral event policies; they do not invoke Android framework or UIKit callbacks. The GameLoop tests do not exercise an Android/iOS clock, native lifecycle gate, GPU, or physical device.

## Transform2D vertical-slice coverage

This change registers two additional CTest executables (11 named cases total):

- `stormengine_transform2d_tests` (8 cases): Matrix3 identity and bounds; translation, counter-clockwise rotation and T*R*S order; finite-value validation; Scene local/world transforms; parent rotation/scale and identity grouping; stale/foreign entity rejection; corrupt component rejection; hierarchy overflow; iterative deep-chain and Scene-move behavior; independent coexistence with Transform3D.
- `stormengine_transform2d_serialization_tests` (3 cases): Transform2D Scene round-trip; invalid component rejection during encoding; malformed numeric payload and unsupported schema rejection.

These are host tests. Android arm64-v8a and iOS device arm64 target builds compile the Math, Core, and Serialization implementation, but do not run host test executables or verify physical-device behavior. Validation: Foundation CI run [41603833151](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41603833151) for commit `8652eeb4d3d8cc75db75f8c081a40e1e52b7b1d0` completed all seven jobs successfully. All 17 CTest executables passed under GCC, Clang, Apple Clang, and Clang ASan/UBSan; clang-tidy passed; Android arm64 target libraries and the arm64 debug APK/native-library packaging checks passed; iOS device arm64 libraries and the unsigned app bundle built successfully. This is host/CI evidence only, not physical-device runtime verification.

## Local host workflow

With CMake 3.24+, Ninja, and a C++20 compiler installed:

    cmake --preset host-debug
    cmake --build --preset host-debug --parallel
    ctest --preset host-debug

A direct CMake configure/build/test flow is also supported and documented in the repository README.

## GitHub Actions

The Foundation workflow runs on pushes to main and on manual dispatch. The latest successful seven-job workflow run [41373342369](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41373342369) for current HEAD commit `8035d755d90d8995d7134b85da46b731aa664b68` completed all seven jobs successfully:

- Linux with GCC: configure, build, and run host CTest.
- Linux with Clang: configure, build, and run host CTest.
- Linux with Clang AddressSanitizer and UndefinedBehaviorSanitizer: build and run host CTest with leak detection enabled.
- Linux Clang Static Analysis: run `clang-tidy` on the production CMake libraries with enabled Clang Static Analyzer checks treated as errors.
- Android NDK: cross-compile Foundation, Math, Physics1D, Input, Core, Serialization (including explicit Transform1D and Transform3D SceneSerializer instantiations), Platform, and Runtime for arm64-v8a, then compile and link the NativeActivity shared library against the NDK-provided Native App Glue, Android, and Log libraries. The job installs the Android SDK/NDK packages, assembles an arm64-v8a debug APK with Gradle, and checks that the APK contains `libstorm_android_native_activity.so`. The Gradle application configuration currently declares `minSdk 21`; CI also compiles the native target with `ANDROID_PLATFORM=android-21`.
- iOS Device SDK: cross-compile Foundation, Math, Physics1D, Input, Core, Serialization (including explicit Transform1D and Transform3D SceneSerializer instantiations), Platform, and Runtime for arm64 using the iPhoneOS SDK and Xcode.
- macOS with Apple Clang: configure, build, and run host CTest.

The Android and iOS jobs disable host test executables because those binaries cannot be run as ordinary host programs from the target toolchains. The Android job additionally assembles a debug APK and verifies its native-library packaging; the iOS job builds an unsigned device-architecture `.app` bundle. Neither job installs the app, invokes Android or UIKit callbacks on a device, creates an Android AAB, signs an iOS app, creates an archive/IPA, or runs on physical hardware. `minSdk 21` is the current Gradle package setting, not evidence that runtime behavior has been validated on API 21 devices.

The iOS job uses CMake's Xcode generator and the iPhoneOS Device SDK. The latest successful run [41330980892](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41330980892) compiled `engine/serialization/src/scene_serializer.cpp`, explicitly instantiating the built-in Transform1D and Transform3D archive paths into `libstorm_serialization.a` for device arm64. The Android job built the same Serialization target for arm64-v8a and linked the NativeActivity shared library. These target builds do not create an iOS application archive or signed IPA, or an Android APK/AAB.

All fifteen CTest executables (176 named cases) passed in GCC, Clang, Clang ASan/UBSan, and Apple Clang host jobs in run [41373342369](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41373342369). The 1D PhysicsWorld suite accounts for thirty-three dedicated cases. The Scene archive suite validates the versioned binary format and codec contracts on the host; it does not exercise app-sandbox file access or a real mobile device. Physics1D tests validate the scalar simulation and contact rules on the host; target compilation does not validate runtime behavior, input/render integration, or device performance. The Runtime suite does not call Android or UIKit frame sources, and the iOS lifecycle suite does not invoke UIKit.

The sanitizer job runs the same host tests with leak detection enabled and UndefinedBehaviorSanitizer configured to halt on the first reported issue. Sanitizer coverage increases confidence in tested code paths but is not a substitute for a security audit, fuzzing, or device testing.

## Validation boundaries

- A green host CI run proves only that the tested Foundation/Core configuration built and its host tests passed on those runners.
- Android/iOS target builds prove that the selected native targets compile for the configured architecture and SDK/toolchain. CI also assembles an arm64-v8a Android debug APK and checks native-library packaging, and builds an unsigned iOS `.app` bundle. These checks do not prove installability, OS callback behavior, runtime behavior, graphics availability, or device compatibility.
- AddressSanitizer can detect classes of memory errors on exercised paths; UndefinedBehaviorSanitizer can detect supported forms of undefined behavior on exercised paths. Neither proves the absence of all memory-safety or correctness defects.
- The current workflow does not run ThreadSanitizer, dependency vulnerability scanning, fuzz tests, Android instrumentation tests, or iOS simulator/device tests. Clang static analysis is run separately by the `Linux / Clang static analysis` job.
- Android/iOS runtime compatibility requires installable native application targets, platform lifecycle integration, target-specific tests, and real-device evidence.
- Scene archive cross-compilation instantiates the built-in Transform1D and Transform3D codecs for each mobile toolchain. The thirteen archive behavior tests run on host CI only; no on-device serialization or platform file-system integration is claimed.

See [Android NativeActivity Lifecycle Bridge](ANDROID-NATIVE-ACTIVITY.md), [Core Game Loop](GAME-LOOP.md), [Mobile Runtime Frame Driver](MOBILE-RUNTIME.md), [Core Event System](EVENT-SYSTEM.md), and [Transform System](TRANSFORM-SYSTEM.md) for the exact implementation scope and limitations.


The iOS target build verifies that the Objective-C++ scene delegate and host application compile against the selected iPhoneOS SDK and produce an unsigned device-architecture `.app` bundle. CI does not instantiate the delegate, run UIKit lifecycle callbacks, sign the app, create an Xcode archive or IPA, install it, or test on an iPhone. The UIView surface adapter currently reports validated geometry metadata only; it does not create a Metal drawable or invoke the Runtime frame driver.


## Static analysis

The `Linux / Clang static analysis` workflow job installs `clang-tidy`,
configures the project with Clang, and builds the production CMake libraries
with `CMAKE_CXX_CLANG_TIDY` enabled and test executables disabled. The
repository-level `.clang-tidy` configuration enables Clang Static Analyzer
checks; findings from enabled checks are treated as errors. Diagnostics are
reported for StormEngine implementation sources and their engine/platform
headers. Host test executables are built and run in the separate compiler and
sanitizer jobs; they are intentionally not analyzed by this job because some
negative tests deliberately exercise moved-from objects and invalid enum
sentinels as part of the documented API contract.

The `clang-analyzer-core.uninitialized.Assign` check is explicitly excluded
for now: Clang 18.1.3 reports a false positive when copying the initialized
`Storm::Error` value out of the `std::variant` used by `Result<T>`.
`Error::code` has a default member initializer, and `Error::make`
initializes both aggregate fields. Revisit this exclusion when the analyzer is
upgraded or the Result representation changes.

This gate supplements, but does not replace, host unit tests, ASan/UBSan, code
review, fuzzing, dependency auditing, or physical-device validation. It does not
claim ThreadSanitizer coverage or prove the absence of defects outside the
analyzed paths.


## 1D Scene adapter coverage added in the current change

The new stormengine_one_d_scene_tests executable adds nine named cases for:
- Requiring a finite Transform1D component before body attachment.
- Treating the Scene scalar position as authoritative during attachment.
- Synchronizing successful fixed-step positions back to Scene.
- Explicit teleport, velocity, force, and impulse operations.
- Entity/body handle lookup and explicit detach.
- Cleanup after entity destruction or Transform1D removal.
- Rejection of foreign Scenes and stale entities.
- Preserving Scene/body state when the physics step fails.
- Preserving the Scene binding through Scene and adapter move operations.

The test target is registered in CTest, and the optional adapter is included
in Android arm64 and iOS device arm64 target-build jobs. All nine adapter cases
and both target builds passed in CI run [41340647156](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41340647156).


## Portable touch input coverage added in the current change

The `stormengine_touch_input_tests` executable contains seventeen named cases for:
- Configuration and buffer-capacity validation.
- Attached-surface and generation validation.
- Down/Move/Up lifecycle and accumulated per-frame deltas.
- Multi-touch and active-pointer limits.
- Duplicate Down and unknown transition rejection.
- Non-finite coordinate rejection without state mutation.
- Monotonic timestamp enforcement across pointers.
- Stale event rejection after surface recreation.
- Automatic cancellation on surface detach.
- Explicit cancellation and pointer-ID reuse.
- Per-frame event and delta reset behavior.
- Transactional event-buffer back-pressure.
- Transactional cancellation failure when capacity is insufficient.
- Invalid pointer-phase rejection.
- Safe rejection of mutations on a moved-from input instance.
- Multi-pointer callback batch processing with stable frame order and accumulated deltas.
- Transactional cleanup of partially applied batches after duplicate transitions.
- Active-pointer cancellation when a batch contains a backwards timestamp.

The target is added to the CMake host-test graph and Android/iOS native
cross-compilation jobs. The seventeen cases exercise the portable C++ contract
on host CI; they do not invoke Android MotionEvent or UIKit callbacks on a
physical device.


## Initial Physics2D vertical slice

The new `stormengine_two_d_physics_tests` executable registers sixteen named
cases covering configuration and body validation, world-scoped generation
handles, semi-implicit fixed-step integration, force and impulse behavior,
Static/Kinematic/Dynamic bodies, circle-circle, circle-box and axis-aligned
box-box contacts, normal and friction impulses, triggers, symmetric collision
filters, spatial-hash candidate reduction, cell/candidate/contact/body budgets,
transactional failure, coordinate bounds, and moved-from world behavior.

The new `Storm::Physics2D` target is registered as a separate CMake library.
CI run [41610785631](https://github.com/thedeverfankhajo-bot/StormEngine/actions/runs/41610785631) passed all sixteen Physics2D cases in the GCC,
Clang, Apple Clang, and ASan/UBSan host configurations. The Android arm64-v8a
and iOS device arm64 jobs built and verified its static library. Android debug
APK assembly and native-library packaging checks also passed; the iOS job
produced the repository's unsigned app bundle. These are host tests, target
builds, and package checks only; they do not verify a renderer, native input,
Scene integration, or physical-device runtime behavior.


## PhysicsScene2D Scene-adapter coverage

The new stormengine_two_d_scene_tests executable adds eleven named
integration cases for:

- Requiring a live unparented entity with a finite Transform2D.
- Treating Scene translation as authoritative on body attachment.
- Synchronizing fixed-step body positions while preserving rotation and scale.
- Teleport, velocity, force, and impulse operations.
- Entity/body handle lookup and explicit detach.
- Cleanup after entity destruction or Transform2D removal.
- Rejection of foreign Scenes and stale entities.
- Preserving Scene and body state after an invalid physics step.
- Rejecting parented entities before simulation state can advance.
- Preserving bindings through Scene and adapter move operations.
- Rejecting coordinate limits that cannot fit finite Transform2D values.

The adapter is a separate Storm::PhysicsScene2D CMake library; Core and the
standalone Physics2D world do not depend on it. The Android arm64-v8a and iOS
device arm64 CI jobs build and verify its static library. These checks remain
host integration tests and target cross-compilation, not physical-device
runtime verification. The adapter maps only local translation; it does not
synchronize parent transforms, apply rotation/scale to colliders, or provide
angular physics.


## TouchInput callback batch coverage

The portable touch tests add three cases for multi-pointer batch ingestion:
ordered Down/Move batches and per-pointer deltas; cancellation of partial state
when a later transition is duplicated; and cleanup when a batch timestamp
moves backwards. A failed batch discards its partial event list and retains
only cancellation events for pointers that had already become active.


## Native touch integration scope

The Android NativeActivity adapter consumes NDK MotionEvent Down,
PointerDown, Move, PointerUp, Up, and Cancel callbacks. Android input coordinates
are window-relative pixels and are converted to logical units using the active
SurfaceLifecycle density. The NDK event timestamp is forwarded in the
System.nanoTime nanosecond time base.

The iOS host view enables multiple touches and forwards
touchesBegan/Moved/Ended/Cancelled callbacks. Touch positions use view-local
UIKit points; one UIEvent timestamp is shared by the event's pointer batch and
converted from seconds since system startup to nanoseconds.

Both host adapters validate the active surface generation and cancel active
pointers on focus loss or surface detachment. They update a host-owned
TouchInput instance only; no Core game-loop or gameplay consumer is connected.
The CI cross-compilation jobs validate linkage and target compilation, not
physical-device callback behavior.
