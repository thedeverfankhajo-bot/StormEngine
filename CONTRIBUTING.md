# Contributing to StormEngine

Thanks for helping build StormEngine.

## Before you start

StormEngine is an early-stage C++20 engine. APIs and architecture can change quickly. For larger changes, open an issue first so the design and scope can be discussed before substantial implementation work.

## Development requirements

- C++20 compiler.
- CMake 3.20 or newer.
- Git.
- For Android work: Android SDK/NDK and the project Gradle tooling used by CI.

## Build and test

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/storm_sandbox
```

When changing the OpenGL ES backend, also validate the Android build and the native smoke test when the change affects Android rendering or platform integration.

## Coding expectations

- Keep production code C++20 and portable unless a platform-specific implementation is required.
- Prefer small, focused changes with clear ownership and lifetime semantics.
- Preserve generation-safe entity/resource handles and existing invariants.
- Avoid unrelated formatting or refactoring in feature/fix commits.
- Use deterministic tests for new behavior and regressions.
- Check error paths, resource destruction, invalid handles, and boundary conditions.
- Keep public headers documented through clear names and concise comments where behavior is non-obvious.

## Rendering changes

For render-device, shader, mesh, buffer, texture, or backend changes:

- Keep backend-neutral types independent from a specific graphics API where possible.
- Validate resource handles before use.
- Consider resource lifetime and destruction order.
- Add or update tests for validation and lifecycle behavior.
- For GLES changes, consider shader compilation/linking, vertex layouts, draw parameters, and Android device compatibility.

## Pull requests

A good pull request should:

1. Explain the problem and the intended solution.
2. Keep the diff focused.
3. Include tests for behavior that can be tested on the host.
4. State whether Android/GLES behavior is affected.
5. Mention any API, ABI, build, or compatibility impact.
6. Report commands used to validate the change.

Maintainers may ask for changes before merging. CI must remain green unless a failing check is directly explained by the change and intentionally being addressed.

## Commit messages

Use short, imperative commit subjects, for example:

```text
Add shader resource lifecycle tests
Fix world transform cycle detection
Improve GLES shader validation
```

## Reporting security issues

Do not open a public issue for a suspected vulnerability. Follow [`SECURITY.md`](SECURITY.md) instead.

## License

The repository does not currently declare a project license. Do not assume that a contribution is licensed under a particular open-source license until the repository adds one.