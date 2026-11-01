# StormEngine Architecture (initial direction)

## Current implementation boundary

The current implementation contains:
- `foundation`: typed errors/results, logging contracts, and memory-budget accounting.
- `core`: a minimal engine lifecycle model, fixed-step time accumulator, and callback-driven frame coordinator.
- `platform`: platform-neutral lifecycle and surface contracts, plus Android NativeActivity lifecycle normalization.
- Host CTest executables covering the implemented contracts.

The Core GameLoop is not yet connected to a native clock, Android event pump, iOS application, renderer, physics implementation, scene system, editor, or export tooling.

## Dependency direction

The intended dependency direction is inward-to-outward:

1. **Foundation** — value types, diagnostics contracts, low-level utilities, and platform-independent primitives.
2. **Core** — engine lifecycle, fixed-step scheduling, frame orchestration, events, scenes, entities, and resource orchestration. Core may depend on Foundation, but not on Editor, Platform SDK headers, or a concrete graphics API.
3. **Platform adapters** — Android and iOS lifecycle, input, surfaces, audio focus, storage, and system services.
4. **Rendering backends** — backend-specific GPU work behind renderer-facing contracts.
5. **Runtime systems** — physics, animation, audio, UI runtime, asset pipeline, serialization, and optional networking/scripting.
6. **Mobile editor** — a mobile application that edits project data through stable engine/editor services. Editor UI must not become a dependency of the game runtime.
7. **Export tooling** — Android and iOS packaging/signing workflows with platform-specific validation.

No dependency on a specific editor framework or GPU API is introduced into Foundation or Core. The current `engine/platform` module depends only on Foundation and defines a normalized lifecycle state contract, a bounded event queue for forwarding native lifecycle callbacks, and a generation-checked surface lifecycle contract. It does not include Android or Apple SDK headers in its public API.

## Core frame orchestration

`Storm::GameLoop` owns a `FixedStepScheduler` and accepts an `IGameLoopCallbacks` implementation supplied by the application/runtime layer. A successful tick invokes input, event processing, zero-or-more fixed updates, one variable update, and rendering in that order.

The loop receives elapsed time from its caller and does not create threads or read clocks. Platform lifecycle gating remains outside Core: the native runtime calls the loop only while foreground/active and resets timing across suspension so background duration is not simulated. Callback failures abort the current frame and are returned with phase context; the caller owns runtime failure recovery.

See [Core Game Loop](GAME-LOOP.md) for the exact callback and failure contract.

## Platform boundaries

The current `engine/platform` module depends only on Foundation and exposes normalized lifecycle, bounded event-queue, surface identity, and Android lifecycle event contracts. Native SDK types are kept in platform integration code. See [Engine Lifecycle](ENGINE-LIFECYCLE.md) and [Android NativeActivity bridge](ANDROID-NATIVE-ACTIVITY.md).

## Rendering direction

Rendering will use a backend interface rather than exposing API-specific handles to gameplay code. Android and iOS have different native graphics environments; backend selection and fallback behavior must be based on device capability checks and documented minimum requirements. A backend is not considered implemented until it builds for its target, renders a test scene, and has validation evidence from physical devices.

## Dimensional models

1D, 2D, and 3D are distinct spatial/gameplay models. A 1D world uses scalar coordinates and one-axis physics. A 2D world uses planar coordinates and 2D collision/rendering systems. A 3D world uses vector transforms and 3D spatial systems. A 1D scene may use a 2D presentation layer without converting its gameplay model to 2D.

## Architectural decision records

Major choices (editor framework, rendering APIs, physics library, asset formats, scripting runtime, and export strategy) require an ADR recording context, alternatives, evidence, security/licensing impact, mobile constraints, and consequences. No such stack choice is locked by this bootstrap commit.
