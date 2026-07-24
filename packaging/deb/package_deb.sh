#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PACKAGE_NAME="${PACKAGE_NAME:-m5cardputerzero-cap-lora-1262-gps}"
PACKAGE_SUFFIX="${PACKAGE_SUFFIX:-m5stack1}"
DEB_ARCH="arm64"
MAINTAINER="${MAINTAINER:-m5stack <m5stack@m5stack.com>}"
PARALLEL="${PARALLEL:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/package}"
STAGE_DIR="${STAGE_DIR:-${ROOT_DIR}/build/deb-root}"
DIST_DIR="${DIST_DIR:-${ROOT_DIR}/dist}"
BIN_NAME="M5CardputerZero-Cap-LoRa-1262-GPS"
CMAKE_BIN="${CMAKE:-cmake}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
CAP_GPS_SYSROOT="${CAP_GPS_SYSROOT:-}"
CAP_GPS_FORCE_CROSS="${CAP_GPS_FORCE_CROSS:-0}"
READELF_BIN="${READELF:-readelf}"

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Required command not found: $1" >&2
        exit 1
    fi
}

read_cmake_cache_value() {
    local name="$1"
    local cache_file="${BUILD_DIR}/CMakeCache.txt"
    local line=""

    if [[ ! -f "${cache_file}" ]]; then
        echo "CMake cache not found: ${cache_file}" >&2
        return 1
    fi
    line="$(grep -E "^${name}(:[^=]*)?=" "${cache_file}" | tail -n 1 || true)"
    if [[ -z "${line}" ]]; then
        echo "CMake cache value not found: ${name}" >&2
        return 1
    fi
    printf "%s\n" "${line#*=}"
}

CMAKE_CONFIGURE_ARGS=(
    -S "${ROOT_DIR}"
    -B "${BUILD_DIR}"
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    -DCAP_GPS_BIN_NAME="${BIN_NAME}"
    -DCAP_GPS_USE_SDL=OFF
    -DCAP_GPS_OUTPUT_DIR="${BUILD_DIR}/dist"
)

host_arch="$(uname -m)"
if [[ "${CAP_GPS_FORCE_CROSS}" == "1" || ("${host_arch}" != "aarch64" && "${host_arch}" != "arm64") ]]; then
    if [[ -z "${CAP_GPS_SYSROOT}" ]]; then
        echo "Cross-packaging Cap-LoRa-1262-GPS requires CAP_GPS_SYSROOT with the CardputerZero BSP." >&2
        echo "Use packaging/docker/package_deb.sh, build natively on CardputerZero, or provide the target sysroot." >&2
        exit 1
    fi
    for compiler in aarch64-linux-gnu-gcc aarch64-linux-gnu-g++; do
        require_command "${compiler}"
    done
    READELF_BIN="${READELF:-aarch64-linux-gnu-readelf}"
    export PKG_CONFIG_SYSROOT_DIR="${CAP_GPS_SYSROOT}"
    export PKG_CONFIG_LIBDIR="${CAP_GPS_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:${CAP_GPS_SYSROOT}/usr/lib/pkgconfig:${CAP_GPS_SYSROOT}/usr/share/pkgconfig"
    export PKG_CONFIG_PATH=""
    CMAKE_CONFIGURE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/aarch64-linux-gnu.cmake")
    CMAKE_CONFIGURE_ARGS+=(-DCMAKE_SYSROOT="${CAP_GPS_SYSROOT}")
fi

for command in "${CMAKE_BIN}" "${READELF_BIN}" dpkg-deb; do
    require_command "${command}"
done

"${CMAKE_BIN}" "${CMAKE_CONFIGURE_ARGS[@]}"
if [[ "$(read_cmake_cache_value CAP_GPS_USE_SDL)" != "OFF" ]]; then
    echo "Invalid package build: CAP_GPS_USE_SDL must be OFF." >&2
    exit 1
fi
PACKAGE_VERSION="$(read_cmake_cache_value CMAKE_PROJECT_VERSION)"
"${CMAKE_BIN}" --build "${BUILD_DIR}" -j"${PARALLEL}"

EXECUTABLE="${BUILD_DIR}/dist/${BIN_NAME}"
DESKTOP_TEMPLATE="${SCRIPT_DIR}/cap-lora-1262-gps.desktop.in"
SUDOERS_FILE="${SCRIPT_DIR}/m5cardputerzero-cap-lora-1262-gps.sudoers"
ICON_FILE="${SCRIPT_DIR}/images/cap-lora-1262-gps.png"
LICENSE_FILE="${ROOT_DIR}/LICENSE"
THIRD_PARTY_NOTICES_FILE="${ROOT_DIR}/THIRD_PARTY_NOTICES.md"
for path in "${EXECUTABLE}" "${DESKTOP_TEMPLATE}" "${SUDOERS_FILE}" "${ICON_FILE}" "${LICENSE_FILE}" \
    "${THIRD_PARTY_NOTICES_FILE}"; do
    if [[ ! -f "${path}" ]]; then
        echo "Required file not found: ${path}" >&2
        exit 1
    fi
done

if command -v visudo >/dev/null 2>&1; then
    visudo -c -f "${SUDOERS_FILE}"
fi

machine="$(${READELF_BIN} -h "${EXECUTABLE}" | awk -F: '/Machine:/ { sub(/^[[:space:]]+/, "", $2); print $2; exit }')"
if [[ "${machine}" != "AArch64" ]]; then
    echo "Invalid package executable architecture: expected AArch64, got ${machine:-unknown}." >&2
    exit 1
fi

dynamic_section="$(${READELF_BIN} -d "${EXECUTABLE}")"
if [[ "${dynamic_section}" == *"libSDL2"* ]]; then
    echo "Invalid package executable: SDL must not be linked in a device build." >&2
    exit 1
fi

rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}/DEBIAN" "${STAGE_DIR}/etc/sudoers.d" "${STAGE_DIR}/usr/share/APPLaunch/bin" \
    "${STAGE_DIR}/usr/share/APPLaunch/applications" \
    "${STAGE_DIR}/usr/share/APPLaunch/share/images" \
    "${STAGE_DIR}/usr/share/doc/${PACKAGE_NAME}" "${DIST_DIR}"
install -m 755 "${EXECUTABLE}" "${DIST_DIR}/${BIN_NAME}"
install -m 755 "${EXECUTABLE}" "${STAGE_DIR}/usr/share/APPLaunch/bin/${BIN_NAME}"
install -m 644 "${DESKTOP_TEMPLATE}" \
    "${STAGE_DIR}/usr/share/APPLaunch/applications/cap-lora-1262-gps.desktop"
install -m 440 "${SUDOERS_FILE}" \
    "${STAGE_DIR}/etc/sudoers.d/m5cardputerzero-cap-lora-1262-gps"
install -m 644 "${ICON_FILE}" \
    "${STAGE_DIR}/usr/share/APPLaunch/share/images/cap-lora-1262-gps.png"
install -m 644 "${LICENSE_FILE}" "${STAGE_DIR}/usr/share/doc/${PACKAGE_NAME}/LICENSE"
install -m 644 "${THIRD_PARTY_NOTICES_FILE}" \
    "${STAGE_DIR}/usr/share/doc/${PACKAGE_NAME}/THIRD_PARTY_NOTICES.md"

INSTALLED_SIZE="$(du -sk "${STAGE_DIR}/usr" | awk '{print $1}')"
cat >"${STAGE_DIR}/DEBIAN/control" <<EOF
Package: ${PACKAGE_NAME}
Version: ${PACKAGE_VERSION}
Section: utils
Priority: optional
Architecture: ${DEB_ARCH}
Maintainer: ${MAINTAINER}
Depends: libc6, libstdc++6, libgcc-s1, sudo
Installed-Size: ${INSTALLED_SIZE}
Description: Cap LoRa-1262 GPS application for M5CardputerZero APPLaunch
 GNSS position and receiver diagnostics for the Cap LoRa-1262 accessory.
EOF

DEB_PATH="${DIST_DIR}/${PACKAGE_NAME}_${PACKAGE_VERSION}_${PACKAGE_SUFFIX}_${DEB_ARCH}.deb"
dpkg-deb --build --root-owner-group "${STAGE_DIR}" "${DEB_PATH}"
if [[ "$(dpkg-deb -f "${DEB_PATH}" Architecture)" != "${DEB_ARCH}" ]]; then
    echo "Generated package has an invalid architecture field." >&2
    exit 1
fi
echo "Generated Debian package: ${DEB_PATH}"
