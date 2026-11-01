# ADR-0008: Native Mobile Frame Sources

- Status: Accepted; implementation pending
- Date: 2026-11-01
- Scope: Android and iOS frame scheduling and timestamp delivery to Runtime

## Context

StormEngine has a platform-neutral `GameLoop` and a `MobileFrameDriver` that accepts caller-supplied monotonic nanosecond timestamps. The Android NativeActivity host currently blocks in `ALooper_pollOnce(-1)`; the iOS host owns UIKit lifecycle and surface metadata but has no frame scheduler. Neither host is connected to `MobileFrameDriver`.

The Android application declares `minSdk 21` and currently packages only `arm64-v8a`. Raising the minimum platform level simply to use a newer frame callback API would be an unnecessary compatibility regression. The iOS deployment target is 13.0.

## Decision

### Android

1. Keep `minSdk 21`.
2. On API 24+, use NDK `AChoreographer` on the NativeActivity app thread. The NDK contract requires `AChoreographer_getInstance()` to be called on a thread with an associated native looper; the app thread already owns the `ALooper` used by Native App Glue.
3. Resolve API-24+ NDK symbols at runtime rather than creating an unconditional API-24 dynamic-link dependency in a binary whose minimum API is 21.
4. Prefer `AChoreographer_postFrameCallback64` on API 29+. On API 24–28, use the legacy callback only for the currently supported 64-bit Android ABI. The legacy callback's C `long` timestamp is 64-bit on arm64. A future 32-bit ABI must not reuse this path without a tested timestamp-wrap reconstruction strategy.
5. On API 21–23, retain the native event pump and compute a monotonic deadline for the next nominal frame. Pass the remaining time as the timeout to `ALooper_pollOnce`, then read `CLOCK_MONOTONIC` when the deadline expires. The initial fallback target is configurable with a nominal 60 Hz period. This fallback is timer-paced, not VSync, and must be labeled as such in diagnostics.
6. Native Choreographer callbacks are one-shot. Re-arm only when lifecycle is `ForegroundActive` and a validated surface is attached. A pending callback after deactivation must observe the inactive state and must not re-arm.

### iOS

1. Use `CADisplayLink` on the existing iOS 13+ UIKit host and register it with the main run loop in common modes.
2. Keep lifecycle transitions, surface metadata updates, the display-link callback, and the initial `MobileFrameDriver` consumer on the main thread. The current Platform contracts and driver are single-thread-affine; no unsynchronized cross-thread calls are permitted.
3. Use Core Animation's monotonic media-time domain for timestamp delivery. Convert the selected display-link timestamp to signed nanoseconds with finite/range checks before calling `tick_at_monotonic_nanoseconds()`. Keep one timestamp convention for the full active period.
4. Invalidate the display link when the scene is no longer active or the view surface is detached; recreate/resume it only after both eligibility conditions return.
5. Do not hardcode a universal 60 FPS. The operating system controls actual callback cadence. If an explicit preferred frame-rate range is added later, gate the iOS 15+ API and treat it as a request, not a guarantee; do not build new code around deprecated `preferredFramesPerSecond`.

### Shared runtime contract

- The native frame source owns scheduling only; it does not own GameLoop, gameplay callbacks, rendering, or GPU resources.
- `MobileFrameDriver` remains the single consumer that checks lifecycle and surface eligibility, resets timing across environment changes, and forwards elapsed time into Core.
- The first integration remains thread-affine on the native event thread. A future dedicated engine/render thread requires a separately designed bounded handoff, explicit ownership, shutdown, and back-pressure policy.
- The native source must stop producing periodic callbacks while backgrounded or surface-less. This is both a correctness and battery policy.

## Consequences

Positive:
- The product retains Android API 21 as its declared minimum.
- Newer Android versions can use platform display synchronization without forcing a higher global minimum.
- Older Android versions retain a bounded fallback without falsely claiming VSync.
- iOS uses the system display scheduler rather than a fixed sleep loop.
- The clock boundary remains platform-neutral and testable through `MobileFrameDriver`.

Limitations:
- This ADR does not implement a frame source or connect either native host to `MobileFrameDriver`.
- The API 21–23 fallback cannot promise display-synchronized pacing.
- Current Android support is arm64-v8a only; no 32-bit callback timestamp handling is approved.
- The main-thread iOS approach is the initial integration boundary, not a promise that expensive game workloads will remain on the UI thread forever.
- No physical-device frame-pacing, thermal, battery, or latency measurements exist.

## Validation required before marking the feature implemented

- Unit-test timestamp conversion, deadline rollover, inactive-state stop/restart, surface detach/reattach, and monotonic regression.
- Android API 21–23: verify the timed looper fallback and lifecycle wake behavior on emulator/device.
- Android API 24–28: verify callback scheduling on the app looper and the arm64 legacy callback timestamp.
- Android API 29+: verify the 64-bit callback path.
- iOS 13+: verify display-link creation, common-mode callback delivery, stop/restart on scene transitions, and timestamp conversion.
- Record device model, OS version, display capability, requested/observed cadence, frame-time distribution, and background behavior for device tests.
- Keep cross-compilation and APK/app packaging evidence separate from physical-device runtime evidence.

## References

- [Android NDK Choreographer](https://developer.android.com/ndk/reference/group/choreographer)
- [Android Java Choreographer.FrameCallback](https://developer.android.com/reference/android/view/Choreographer.FrameCallback)
- [Apple CADisplayLink](https://developer.apple.com/documentation/quartzcore/cadisplaylink)
- [Apple CADisplayLink preferredFrameRateRange](https://developer.apple.com/documentation/quartzcore/cadisplaylink/preferredframeraterange)
- [Apple CACurrentMediaTime](https://developer.apple.com/documentation/quartzcore/cacurrentmediatime)
