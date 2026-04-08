#!/bin/bash

set -euo pipefail

IMAGE="${OZO_PODMAN_POSTGRES_IMAGE:-docker.io/library/postgres:16}"
CONTAINER_NAME="${OZO_PODMAN_POSTGRES_CONTAINER:-ozo-pg-test}"
HOST="${OZO_PG_TEST_HOST:-127.0.0.1}"
PORT="${OZO_PG_TEST_PORT:-55432}"
DB="${POSTGRES_DB:-ozo_test_db}"
USER="${POSTGRES_USER:-ozo_test_user}"
PASSWORD="${POSTGRES_PASSWORD:-v4Xpkocl~5l6h219Ynk1lJbM61jIr!ca}"
BUILD_DIR="${OZO_PG_BUILD_DIR:-build-podman-pg}"
JOBS="${OZO_BUILD_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu)}"

cleanup() {
    podman rm -f "${CONTAINER_NAME}" >/dev/null 2>&1 || true
}

trap cleanup EXIT

cleanup

podman run -d \
    --name "${CONTAINER_NAME}" \
    -e POSTGRES_DB="${DB}" \
    -e POSTGRES_USER="${USER}" \
    -e POSTGRES_PASSWORD="${PASSWORD}" \
    -p "${HOST}:${PORT}:5432" \
    "${IMAGE}" >/dev/null

for _ in $(seq 1 60); do
    if podman exec "${CONTAINER_NAME}" pg_isready -U "${USER}" -d "${DB}" >/dev/null 2>&1; then
        break
    fi
    sleep 1
done

if ! podman exec "${CONTAINER_NAME}" pg_isready -U "${USER}" -d "${DB}" >/dev/null 2>&1; then
    podman logs "${CONTAINER_NAME}" >&2 || true
    echo "PostgreSQL in container ${CONTAINER_NAME} did not become ready in time." >&2
    exit 1
fi

CONNINFO="host=${HOST} port=${PORT} dbname=${DB} user=${USER} password=${PASSWORD}"

cmake -S . -B "${BUILD_DIR}" \
    -DOZO_BUILD_TESTS=ON \
    -DOZO_BUILD_PG_TESTS=ON \
    -DOZO_PG_TEST_CONNINFO="${CONNINFO}"

cmake --build "${BUILD_DIR}" -j"${JOBS}" --target ozo_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure -R ozo_tests
