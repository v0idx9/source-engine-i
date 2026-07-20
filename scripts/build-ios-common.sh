#!/usr/bin/env bash
# Shared iOS cross-compilation environment for dependency and engine builds.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${ROOT}/lib/darwin/aarch64"
DEPS="${ROOT}/build-ios-deps"
IOS_MIN_VERSION="${IOS_MIN_VERSION:-12.0}"

if [ "${IOS_SIMULATOR:-0}" = "1" ]; then
	IOS_SDK="iphonesimulator"
	IOS_CLANG_TARGET="arm64-apple-ios${IOS_MIN_VERSION}-simulator"
else
	IOS_SDK="iphoneos"
	IOS_CLANG_TARGET="arm64-apple-ios${IOS_MIN_VERSION}"
fi
IOS_AUTOTOOLS_HOST="aarch64-apple-darwin"

SDKROOT="$(xcrun --sdk "${IOS_SDK}" --show-sdk-path)"
export CC="clang --target=${IOS_CLANG_TARGET} -isysroot ${SDKROOT} -mios-version-min=${IOS_MIN_VERSION}"
export CXX="clang++ --target=${IOS_CLANG_TARGET} -isysroot ${SDKROOT} -mios-version-min=${IOS_MIN_VERSION}"
export AR="$(xcrun --sdk "${IOS_SDK}" --find ar)"
export RANLIB="$(xcrun --sdk "${IOS_SDK}" --find ranlib)"
export STRIP="$(xcrun --sdk "${IOS_SDK}" --find strip)"
# -std=gnu17: zlib and friends use K&R-style definitions that a C23 default
# (Xcode 26's clang) rejects outright.
export CFLAGS="-O2 -fPIC -DNDEBUG -std=gnu17"
export CXXFLAGS="-O2 -fPIC -DNDEBUG -std=c++11"
export LDFLAGS="-L${OUT}"
export PKG_CONFIG_PATH=""
export PKG_CONFIG_LIBDIR=""

mkdir -p "${OUT}" "${DEPS}"

copy_if_exists() {
	if [ -f "$1" ]; then
		cp "$1" "${OUT}/"
	fi
}
