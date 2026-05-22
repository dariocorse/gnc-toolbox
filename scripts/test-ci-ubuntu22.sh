#!/usr/bin/env bash
set -euo pipefail

IMAGE="${CI_DOCKER_IMAGE:-ubuntu:22.04}"
BUILD_DIR="${CI_DOCKER_BUILD_DIR:-/tmp/gnc-toolbox-ci}"
PACKAGE_DIR="${CI_DOCKER_PACKAGE_DIR:-${BUILD_DIR}/package}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DOCKER_TTY_ARGS=(-i)
if [ -t 1 ]; then
  DOCKER_TTY_ARGS+=(-t)
fi

docker run --rm "${DOCKER_TTY_ARGS[@]}" \
  -e CI_BUILD_DIR="${BUILD_DIR}" \
  -e CI_PACKAGE_DIR="${PACKAGE_DIR}" \
  -v "${REPO_ROOT}:/work" \
  -w /work \
  "${IMAGE}" \
  bash -lc '
    set -euo pipefail

    export DEBIAN_FRONTEND=noninteractive
    echo "Installing Ubuntu 22.04 CI dependencies..."
    apt-get update -qq
    apt-get install -y -qq \
      build-essential \
      cmake \
      gnucash \
      gnucash-common \
      libcsv-dev \
      libglib2.0-dev

    uid=$(stat -c %u /work)
    gid=$(stat -c %g /work)

    if ! getent group "${gid}" >/dev/null; then
      groupadd -g "${gid}" ci
    fi

    if ! getent passwd "${uid}" >/dev/null; then
      useradd -m -u "${uid}" -g "${gid}" ci
    fi

    user_name=$(getent passwd "${uid}" | cut -d: -f1)

    echo "Running CI build, tests, and package as ${user_name}..."
    su "${user_name}" -c "make BUILD_DIR=\"${CI_BUILD_DIR}\" test && make BUILD_DIR=\"${CI_BUILD_DIR}\" PACKAGE_DIR=\"${CI_PACKAGE_DIR}\" package"
  '
