# Support

StormEngine is an early-stage engine under active development. Before asking for help, please check the README and existing issues for related information.

## Bug reports

Use a GitHub issue when you have a reproducible bug. Include:

- What you expected to happen.
- What actually happened.
- Minimal reproduction steps.
- Relevant logs or compiler errors.
- Operating system and toolchain versions.
- Whether the problem occurs in host builds, Android builds, or both.
- The commit or branch being tested when relevant.

For rendering issues, include the backend and enough information to identify the failing path.

## Build problems

Start with a clean build directory when practical:

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTORM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

If the failure is Android-specific, include the Gradle/CMake configuration and the relevant build output. Never paste credentials, signing keys, access tokens, or other secrets.

## Feature requests

Use an issue to describe the problem or use case first. Explain why the capability belongs in the engine and identify important constraints such as portability, performance, API stability, and Android compatibility.

## Security issues

Do not use public issues for vulnerabilities. Follow [`SECURITY.md`](SECURITY.md) for private reporting guidance.