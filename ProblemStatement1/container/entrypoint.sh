#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR=${PROJECT_DIR:-/project}
KERNEL_REPO=${KERNEL_REPO:-https://github.com/FreeRTOS/FreeRTOS-Kernel.git}
KERNEL_BRANCH=${KERNEL_BRANCH:-main}
BUILD_DIR=${BUILD_DIR:-/workspace/build}
AUTOMATED=${RUN_AUTOMATED_TESTS:-0}

log() {
    printf '\n[container] %s\n' "$1"
}

if [ ! -d "${PROJECT_DIR}/ProblemStatement1" ]; then
    echo "ProblemStatement1 sources not found in \"${PROJECT_DIR}\"" >&2
    exit 1
fi

log "Cloning FreeRTOS kernel (${KERNEL_BRANCH})..."
rm -rf /workspace/FreeRTOS-Kernel
if ! git clone --depth 1 --branch "${KERNEL_BRANCH}" "${KERNEL_REPO}" /workspace/FreeRTOS-Kernel; then
    echo "Failed to clone FreeRTOS kernel from ${KERNEL_REPO}" >&2
    exit 1
fi

log "Preparing example sources..."
cp "${PROJECT_DIR}/ProblemStatement1/main.c" \
   /workspace/FreeRTOS-Kernel/examples/cmake_example/main.c
cp "${PROJECT_DIR}/ProblemStatement1/FreeRTOSConfig.h" \
   /workspace/FreeRTOS-Kernel/examples/template_configuration/FreeRTOSConfig.h

log "Configuring CMake project..."
cmake -B "${BUILD_DIR}" -S /workspace/FreeRTOS-Kernel/examples/cmake_example \
    -DFREERTOS_PORT=GCC_POSIX \
    -DCMAKE_C_FLAGS="-Wno-conversion $([ "${AUTOMATED}" != "0" ] && printf '%s' "-DRUN_AUTOMATED_TESTS")"

log "Building example..."
cmake --build "${BUILD_DIR}"

log "Running demo..."
"${BUILD_DIR}/example"
