# API Versioning Policy

## Current status

StormEngine has no released or stable public API yet. The 0.1.0 value in CMake is an early project version, not a promise that public API compatibility guarantees are already in force.

The currently exposed C++ headers are development interfaces and may change while the foundation is being established. No package or binary compatibility promise is made at this stage.

The platform-neutral `Storm::Platform::SurfaceLifecycle` interface is a development API added during the foundation stage. It models one surface identity/generation stream; it does not own native handles, synchronize GPU work, or establish Android/iOS support. It is not a stable API guarantee.

## Intended policy before the first stable release

- Use Semantic Versioning for published engine releases once a public compatibility baseline is declared.
- Document every public API change in the changelog and API reference.
- Mark experimental interfaces explicitly and keep them out of stable compatibility guarantees.
- Record breaking changes and provide migration notes for affected public APIs.
- Deprecate an established public API before removal whenever technically and safely practical.
- Keep platform-specific implementation details out of portable public headers.

## Release checklist

Before declaring an API stable, the project must have a published API reference, compatibility matrix, automated API tests, documented platform support, and a migration policy. A version number alone does not make an API stable.
