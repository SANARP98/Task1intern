#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd -- "${SCRIPT_DIR}/../.." && pwd)
IMAGE_NAME=${IMAGE_NAME:-freertos-posix-sim}
RUN_AUTOMATED_TESTS=${RUN_AUTOMATED_TESTS:-0}

if ! command -v docker >/dev/null 2>&1; then
    echo "docker is required to build and run the container" >&2
    exit 127
fi

echo "[host] Building container image '${IMAGE_NAME}'..."
docker build -t "${IMAGE_NAME}" "${SCRIPT_DIR}"

echo "[host] Running FreeRTOS demo inside container..."
docker run --rm \
    -e RUN_AUTOMATED_TESTS="${RUN_AUTOMATED_TESTS}" \
    -v "${PROJECT_ROOT}:/project:ro" \
    "${IMAGE_NAME}"
