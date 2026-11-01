# StormEngine Architecture (initial direction)

## Current implementation boundary

The current implementation contains only `foundation` and a minimal `core` lifecycle model. It has no native platform entry point, game loop, renderer, scene system, physics implementation, editor, or export tooling.

## Dependency direction

The intended dependency direction is inward-to-outward:

1. **Foundation** — value types, diagnostics contracts, low-level utilities, and platform-independent primitives.
2. **Core** — engine lifecycle, scheduling, events, scenes, entities, and resource orchestration. Core may depend on Foundation, but not on Editor or a concrete graphics API.
3. **Platform adapters** — Android and iOS lifecycle, input, surfaces, audio focus, storage, and system services.
4. **Rendering backends** — backend-specific GPU work behind renderer-facing contracts.
5. **Runtime systems** — physics, animation, audio, UI runtime, asset pipeline, serialization, and optional networking/scripting.
6. **Mobile editor** — a mobile application that edits project data through stable engine/editor services. Editor UI must not become a dependency of the game runtime.
7. **Export tooling** — Android and iOS packaging/signing workflows with platform-specific validation.

No dependency on a specific editor framework or GPU API is introduced into Foundation.

## Rendering direction

Rendering will use a backend interface rather than exposing API-specific handles to gameplay code. Android and iOS have different native graphics environments; backend selection and fallback behavior must be based on device capability checks and documented minimum requirements. A backend is not considered implemented until it builds for its target, renders a test scene, and has validation evidence from physical devices.

## Dimensional models

1D, 2D, and 3D are distinct spatial/gameplay models. A 1D world uses scalar coordinates and one-axis physics. A 2D world uses planar coordinates and 2D collision/rendering systems. A 3D world uses vector transforms and 3D spatial systems. A 1D scene may use a 2D presentation layer without converting its gameplay model to 2D.

## Architectural decision records

Major choices (editor framework, rendering APIs, physics library, asset formats, scripting runtime, and export strategy) require an ADR recording context, alternatives, evidence, security/licensing impact, mobile constraints, and consequences. No such stack choice is locked by this bootstrap commit.
