# Contributing to StormEngine

## Scope

StormEngine is an active C++20 engine project. Keep changes focused, testable, portable, and explicit about ownership and lifetime.

For a substantial new subsystem or public API, open or update an issue before implementation when practical.

## Requirements

- C++20 compiler.
- CMake 3.20 or newer.
- Git.
- Android SDK/NDK and the repository Gradle tooling for Android work.
- Termux users: use the repository Termux script rather than assuming an Android SDK exists in `$HOME`.

## Termux validation

On an Android phone running Termux, install `clang`, `cmake`, `ninja` (or `make`), and `git`, then run `./scripts/termux-build.sh`. This validates the native portable engine; it does not replace Android APK validation.

## Host validation

Run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/storm_sandbox
```

For GLES/Android changes, validate the Android workflow or an equivalent Android build. Do not claim device-level behavior unless it was tested on a device/emulator. Record the device family, Android API level, ABI, GPU/renderer, and commit when reporting mobile validation.\n\nFor Termux, run both Debug and Release native builds before claiming the portable development workflow is healthy.

## Code

- Use C++20 and keep public APIs portable unless a backend/platform boundary requires otherwise.
- Preserve resource and entity lifetime invariants.
- Validate external and untrusted input at subsystem boundaries.
- Check finite values, integer overflow, bounds, invalid handles, and destruction order where relevant.
- Avoid hidden ownership and unnecessary global state.
- Keep backend-neutral code independent of OpenGL ES types.
- Add deterministic regression tests for new behavior and bug fixes.
- Do not mix unrelated formatting or refactoring with feature changes.
- Use clear names and comments only where behavior is not obvious from the code.

## Rendering

Rendering changes must consider:

- resource lifetime and generation safety;
- shader compilation/linking and program lifetime;
- vertex/index bounds and integer overflow;
- render-state transitions;
- texture and sampler binding;
- Android/EGL context and surface lifetime;
- backend-independent API behavior.

## Pull requests

A pull request should:

1. Describe the problem.
2. Describe the implementation and important invariants.
3. Include tests or explain why a test is not practical.
4. State Linux/Android/GLES impact.
5. State API, ABI, build, and compatibility impact.
6. List validation commands and CI results.

Do not merge with unexplained failing required checks.

## Commits

Use short imperative subjects, for example:

```text
Add shader resource lifecycle tests
Fix world transform cycle detection
Cache GLES uniform locations
```

## Security

Do not disclose vulnerabilities in public issues. Follow [SECURITY.md](SECURITY.md).

## License

By contributing, you agree that your contribution is provided under the repository's MIT License, subject to applicable law and the repository's contribution terms.
