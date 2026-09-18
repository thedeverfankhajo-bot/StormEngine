#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_TYPE="${STORM_BUILD_TYPE:-Debug}"
BUILD_DIR="${STORM_BUILD_DIR:-$ROOT_DIR/build/termux-$(printf '%s' "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')}"
BUILD_TESTS="${STORM_BUILD_TESTS:-ON}"
JOBS="${STORM_BUILD_JOBS:-2}"
RUN_SANDBOX="${STORM_RUN_SANDBOX:-ON}"

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        printf 'Missing command: %s\n' "$1" >&2
        printf 'Install with: pkg install clang cmake ninja make git\n' >&2
        exit 127
    }
}

need_cmd clang++
need_cmd cmake
need_cmd git
need_cmd ctest

if command -v ninja >/dev/null 2>&1; then
    GENERATOR_ARGS=(-G Ninja)
else
    need_cmd make
    GENERATOR_ARGS=(-G "Unix Makefiles")
fi

case "$BUILD_TYPE" in
    Debug|Release|RelWithDebInfo|MinSizeRel) ;;
    *) printf 'Unsupported STORM_BUILD_TYPE: %s\n' "$BUILD_TYPE" >&2; exit 2 ;;
esac

case "$BUILD_TESTS" in ON|OFF) ;; *) printf 'STORM_BUILD_TESTS must be ON or OFF\n' >&2; exit 2 ;; esac
case "$RUN_SANDBOX" in ON|OFF) ;; *) printf 'STORM_RUN_SANDBOX must be ON or OFF\n' >&2; exit 2 ;; esac

if ! [[ "$JOBS" =~ ^[1-9][0-9]*$ ]]; then
    printf 'STORM_BUILD_JOBS must be a positive integer\n' >&2
    exit 2
fi

printf 'StormEngine Termux build\n'
printf '  root=%s\n  build=%s\n  type=%s\n  tests=%s\n  jobs=%s\n' \
    "$ROOT_DIR" "$BUILD_DIR" "$BUILD_TYPE" "$BUILD_TESTS" "$JOBS"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DSTORM_BUILD_TESTS="$BUILD_TESTS" \
    -DSTORM_BUILD_GLES=OFF \
    -DSTORM_ENABLE_WARNINGS=ON

cmake --build "$BUILD_DIR" --parallel "$JOBS"

if [ "$BUILD_TESTS" = "ON" ]; then
    ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

if [ "$RUN_SANDBOX" = "ON" ] && [ -x "$BUILD_DIR/storm_sandbox" ]; then
    "$BUILD_DIR/storm_sandbox"
fi
