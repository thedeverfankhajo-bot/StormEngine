#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_TYPE="${STORM_BUILD_TYPE:-Debug}"
BUILD_DIR="${STORM_BUILD_DIR:-$ROOT_DIR/build/termux-$(printf '%s' "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')}"
BUILD_TESTS="${STORM_BUILD_TESTS:-ON}"
BUILD_GLES="${STORM_BUILD_GLES:-OFF}"
BUILD_SANITIZERS="${STORM_ENABLE_SANITIZERS:-OFF}"
JOBS="${STORM_BUILD_JOBS:-2}"
RUN_SANDBOX="${STORM_RUN_SANDBOX:-ON}"
CC="${CC:-clang}"
CXX="${CXX:-clang++}"
GENERATOR="${STORM_BUILD_GENERATOR:-Ninja}"

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        printf 'Missing command: %s\n' "$1" >&2
        printf 'Install with: pkg install clang cmake ninja make git\n' >&2
        exit 127
    }
}

need_cmd "$CC"
need_cmd "$CXX"
need_cmd cmake
need_cmd git

if [ "$GENERATOR" = "Ninja" ]; then
    need_cmd ninja
    GENERATOR_ARGS=(-G Ninja)
elif [ "$GENERATOR" = "Unix Makefiles" ]; then
    need_cmd make
    GENERATOR_ARGS=(-G "Unix Makefiles")
else
    printf 'Unsupported STORM_BUILD_GENERATOR: %s\n' "$GENERATOR" >&2
    exit 2
fi

CMAKE_VERSION="$(cmake --version | sed -n '1s/[^0-9]*//p')"
if [ -z "$CMAKE_VERSION" ] || [ "$(printf '%s\n3.20.0\n' "$CMAKE_VERSION" | sort -V | head -n1)" != "3.20.0" ]; then
    printf 'CMake 3.20 or newer is required (found %s)\n' "${CMAKE_VERSION:-unknown}" >&2
    exit 2
fi

case "$BUILD_TYPE" in
    Debug|Release|RelWithDebInfo|MinSizeRel) ;;
    *) printf 'Unsupported STORM_BUILD_TYPE: %s\n' "$BUILD_TYPE" >&2; exit 2 ;;
esac
case "$BUILD_TESTS" in ON|OFF) ;; *) printf 'STORM_BUILD_TESTS must be ON or OFF\n' >&2; exit 2 ;; esac
case "$BUILD_GLES" in ON|OFF) ;; *) printf 'STORM_BUILD_GLES must be ON or OFF\n' >&2; exit 2 ;; esac
if [ "$BUILD_GLES" = "ON" ]; then
    printf '%s\n' 'STORM_BUILD_GLES=ON is Android/NDK-only and cannot be built by the native Termux toolchain.' >&2
    printf '%s\n' 'Use an Android Gradle/NDK build for GLES, or leave STORM_BUILD_GLES=OFF for Termux.' >&2
    exit 2
fi
case "$BUILD_SANITIZERS" in ON|OFF) ;; *) printf 'STORM_ENABLE_SANITIZERS must be ON or OFF\n' >&2; exit 2 ;; esac
case "$JOBS" in ''|*[!0-9]*|0) printf 'STORM_BUILD_JOBS must be a positive integer\n' >&2; exit 2 ;; esac
case "$RUN_SANDBOX" in ON|OFF) ;; *) printf 'STORM_RUN_SANDBOX must be ON or OFF\n' >&2; exit 2 ;; esac

printf '%s\n' 'StormEngine Termux build'
printf '  root=%s\n  build=%s\n  type=%s\n  tests=%s\n  GLES=%s\n  sanitizers=%s\n  jobs=%s\n  compiler=%s\n  sandbox=%s\n' \
    "$ROOT_DIR" "$BUILD_DIR" "$BUILD_TYPE" "$BUILD_TESTS" "$BUILD_GLES" "$BUILD_SANITIZERS" "$JOBS" "$CXX" "$RUN_SANDBOX"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DSTORM_BUILD_TESTS="$BUILD_TESTS" \
    -DSTORM_BUILD_GLES="$BUILD_GLES" \
    -DSTORM_ENABLE_WARNINGS=ON \
    -DSTORM_ENABLE_SANITIZERS="$BUILD_SANITIZERS"

cmake --build "$BUILD_DIR" --parallel "$JOBS"

if [ "$BUILD_TESTS" = "ON" ]; then
    ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

if [ "$RUN_SANDBOX" = "ON" ] && [ -x "$BUILD_DIR/storm_sandbox" ]; then
    "$BUILD_DIR/storm_sandbox"
fi
