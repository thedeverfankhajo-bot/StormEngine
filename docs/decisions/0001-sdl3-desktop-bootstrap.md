# ADR 0001: SDL3 for the initial desktop platform bootstrap

- **Status:** Accepted for the opt-in desktop prototype only
- **Date:** 2026-10-23
- **Upstream:** [libsdl-org/SDL](https://github.com/libsdl-org/SDL)
- **Selected release:** SDL 3.4.18, tag `release-3.4.18`
- **Tag commit:** `829a65d769d935c4852f8159e964312c0957260a`
- **Archive SHA-256:** `9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3`

## Context

StormEngine has an OS-independent bounded input state and FIFO event contract, but no native window or visible executable. The next vertical slice needs a portable way to open a desktop window, receive keyboard/mouse events and present a frame without exposing platform types in core headers.

## Decision

Use the official SDL3 3.4.18 source archive for an **optional desktop bootstrap demo**. CMake FetchContent downloads the release archive only when `STORM_BUILD_SDL3_DEMO=ON`; its SHA-256 is checked. SDL is built as a static dependency with its tests, examples and install actions disabled. The ordinary core build remains dependency-free by default, while desktop CI explicitly enables this option to compile the integration across Linux, macOS and Windows.

SDL is distributed under its permissive zlib license. The full upstream notice and provenance are recorded in [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## Scope and non-goals

- This selects SDL3 only for the first desktop window/event prototype; it is not a permanent engine-wide platform ABI.
- The sample maps a limited subset of SDL physical scancodes to provisional input slots. StormEngine's bounded CPU `SoftwareCanvas2D` rasterizes the moving rectangle; SDL updates a streaming texture and presents the framebuffer.
- The sample renderer is **not** the production Render Hardware Interface (RHI), a sprite/texture asset pipeline or a selected 3D renderer.
- This does not produce an Android APK. SDL Activity/JNI integration, app packaging and real-device lifecycle tests remain separate work.
- The normal core library, headers, installed package and Android core cross-build do not require SDL3 while the option is off.

## Why SDL3

SDL documents CMake support for Windows, macOS, Linux and Android and publishes a permissive zlib license. This provides one maintained OS-adapter candidate for the first desktop slice, rather than implementing separate native window loops immediately. The release is pinned by a source archive checksum and not by a moving branch.

## Trade-offs and follow-up

- Opt-in FetchContent requires network access the first time the demo is configured, unless the archive is already cached.
- SDL's supported features and transitive/system dependencies vary by platform; successful compilation is not proof that the demo runs correctly on every machine.
- The key-slot mapping in the prototype is provisional and must be replaced or formalized before becoming a stable public engine input API.
- Before making SDL a default or integrating it with Android app packaging, review upstream release notes, licensing/attribution, security advisories, build time and per-platform runtime behavior.
- Keep rendering behind a future StormEngine RHI and compare actual desktop/mobile device, surface, shader and lifetime requirements before selecting the production 2D/3D backend.

## References

- [SDL 3.4.18 release and source archive checksum](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18)
- [SDL CMake build guidance](https://wiki.libsdl.org/SDL3/README-cmake)
- [SDL Android build and packaging guidance](https://wiki.libsdl.org/SDL3/README-android)
- [SDL upstream license](https://github.com/libsdl-org/SDL/blob/release-3.4.18/LICENSE.txt)
