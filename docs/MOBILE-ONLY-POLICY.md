# Mobile-only product policy

## Supported product targets

- Android phones and tablets.
- iOS phones and tablets.
- The StormEngine editor is also mobile-only.

## Explicitly out of scope as product targets

Windows, Linux, macOS, web browsers, consoles, smart TVs, and other non-mobile runtime/editor targets are not supported product destinations.

## Permitted engineering environments

Desktop operating systems may be used for source editing, host-side unit tests, CI orchestration, Android/iOS cross-compilation, signing workflows, profiling, and debugging. These engineering uses do not create a supported desktop runtime or editor.

## Build-system guardrails

- Product packaging targets must be explicitly limited to Android and iOS.
- Host-side tests are allowed as engineering tools and must be labeled as such.
- Do not add desktop application, desktop editor, or browser export targets.
- Platform-specific code belongs in \`platforms/android\` or \`platforms/ios\` and must not leak into portable Foundation APIs.

## Compatibility evidence

A platform capability is not \`Verified\` until the relevant build and runtime test is recorded for a named device/model, OS version, and relevant feature configuration. Simulator-only evidence must be labeled separately from physical-device evidence.
