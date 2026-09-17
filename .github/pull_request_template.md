## Summary

<!-- What does this change do? -->

## Motivation

<!-- What problem does this solve, and why is this approach appropriate? -->

## Technical changes

<!-- List the important implementation changes. -->

## Validation

- [ ] `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON`
- [ ] `cmake --build build --parallel`
- [ ] `ctest --test-dir build --output-on-failure`
- [ ] `./build/storm_sandbox`
- [ ] Android build/smoke test checked when applicable

## Android / GLES impact

<!-- Describe Android, OpenGL ES, Gradle, NDK, or platform impact. Write "None" if not applicable. -->

## API / compatibility impact

<!-- Public API, ABI, serialization, resource compatibility, or build-system changes. -->

## Security considerations

<!-- Mention input validation, resource lifetime, shader/asset handling, secrets, or other security impact. Write "None" if not applicable. -->

## Checklist

- [ ] The change is focused and avoids unrelated refactoring.
- [ ] Tests were added or updated where behavior changed.
- [ ] Existing tests pass locally.
- [ ] No secrets or credentials are included.
- [ ] Documentation was updated when needed.
- [ ] Breaking changes are clearly described above.