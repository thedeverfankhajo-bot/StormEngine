#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_TYPE="${STORM_BUILD_TYPE:-Debug}"
BUILD_DIR="${STORM_BUILD_DIR:-$ROOT_DIR/build/termux-$(printf "%s" "$BUILD_TYPE" | tr "[:upper:]" "[:lower:]")}"
BUILD_TESTS="${STORM_BUILD_TESTS:-ON}"
BUILD_GLES="${STORM_BUILD_GLES:-OFF}"

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        printf "Missing command: %s\n" "$1" >&2
        printf "Install: pkg install clang cmake ninja make git\n" >&2
        exit 127
    }
}

need_cmd clang++
need_cmd cmake
need_cmd git

if command -v ninja >/dev/null 2>&1; then
    GENERATOR_ARGS=(-G Ninja)
else
    need_cmd make
    GENERATOR_ARGS=(-G "Unix Makefiles")
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" "${GENERATOR_ARGS[@]}" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DSTORM_BUILD_TESTS="$BUILD_TESTS" \
    -DSTORM_BUILD_GLES="$BUILD_GLES" \
    -DSTORM_ENABLE_WARNINGS=ON

cmake --build "$BUILD_DIR" --parallel "${STORM_BUILD_JOBS:-2}"

if [ "$BUILD_TESTS" = "ON" ]; then
    ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

if [ -x "$BUILD_DIR/storm_sandbox" ]; then
    "$BUILD_DIR/storm_sandbox"
fi
