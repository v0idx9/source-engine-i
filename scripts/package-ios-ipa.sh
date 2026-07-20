#!/usr/bin/env bash
# Package iOS engine build artifacts into a signed .ipa app bundle.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

APP_NAME="${IOS_APP_NAME:-Portal}"
BUNDLE_ID="${IOS_BUNDLE_ID:-com.sourceengine.portal}"
BUNDLE_VERSION="${IOS_BUNDLE_VERSION:-1.0.0}"
BUILD_NUMBER="${IOS_BUILD_NUMBER:-1}"
MIN_OS="${IOS_MIN_VERSION:-12.0}"
SIGN_IDENTITY="${IOS_SIGN_IDENTITY:--}"

LAUNCHER="${ROOT}/build/launcher_main/hl2_launcher"
if [ ! -f "${LAUNCHER}" ]; then
	echo "error: ${LAUNCHER} not found; run scripts/build-ios-aarch64.sh first" >&2
	exit 1
fi

STAGING="${ROOT}/build-ios-ipa"
APP_DIR="${STAGING}/Payload/${APP_NAME}.app"
rm -rf "${STAGING}"
mkdir -p "${APP_DIR}"

echo "==> Staging ${APP_NAME}.app"
cp "${LAUNCHER}" "${APP_DIR}/hl2_launcher"
chmod +x "${APP_DIR}/hl2_launcher"

while IFS= read -r dylib; do
	cp "${dylib}" "${APP_DIR}/$(basename "${dylib}")"
done < <(find "${ROOT}/build" -name '*.dylib' -type f | sort -u)

# Prebuilt runtime dylibs: SDL2 plus ANGLE's EGL/GLESv2, which the iOS renderer
# resolves at load time and so must ship inside the .app.
for runtime_lib in libSDL2.dylib libEGL.dylib libGLESv2.dylib; do
	if [ -f "${ROOT}/lib/darwin/aarch64/${runtime_lib}" ]; then
		cp "${ROOT}/lib/darwin/aarch64/${runtime_lib}" "${APP_DIR}/${runtime_lib}"
	fi
done

cp "${ROOT}/ios/Info.plist" "${APP_DIR}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier ${BUNDLE_ID}" "${APP_DIR}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString ${BUNDLE_VERSION}" "${APP_DIR}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion ${BUILD_NUMBER}" "${APP_DIR}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :MinimumOSVersion ${MIN_OS}" "${APP_DIR}/Info.plist"

printf 'APPL????' > "${APP_DIR}/PkgInfo"

fix_install_names() {
	local target="$1"
	local base
	base="$(basename "${target}")"

	if [[ "${base}" == *.dylib ]]; then
		install_name_tool -id "@executable_path/${base}" "${target}" 2>/dev/null || true
	fi

	local dep
	while IFS= read -r dep; do
		[[ -z "${dep}" ]] && continue
		case "${dep}" in
			@executable_path/*|/usr/lib/*|/System/*|/Library/*)
				continue
				;;
		esac
		local depbase="${dep##*/}"
		if [[ -f "${APP_DIR}/${depbase}" ]]; then
			install_name_tool -change "${dep}" "@executable_path/${depbase}" "${target}" 2>/dev/null || true
		elif [[ "${depbase}" == libSDL2*.dylib ]] && [[ -f "${APP_DIR}/libSDL2.dylib" ]]; then
			install_name_tool -change "${dep}" "@executable_path/libSDL2.dylib" "${target}" 2>/dev/null || true
		fi
	done < <(otool -L "${target}" 2>/dev/null | tail -n +2 | awk '{print $1}')
}

echo "==> Fixing dylib install names"
for target in "${APP_DIR}/hl2_launcher" "${APP_DIR}"/*.dylib; do
	[[ -e "${target}" ]] || continue
	fix_install_names "${target}"
done

ENTITLEMENTS="${ROOT}/ios/entitlements.plist"
if [ -n "${IOS_CODESIGN_ENTITLEMENTS:-}" ] && [ -f "${IOS_CODESIGN_ENTITLEMENTS}" ]; then
	ENTITLEMENTS="${IOS_CODESIGN_ENTITLEMENTS}"
fi

echo "==> Code signing with identity: ${SIGN_IDENTITY}"
/usr/bin/codesign --force --sign "${SIGN_IDENTITY}" \
	--entitlements "${ENTITLEMENTS}" \
	--timestamp=none \
	"${APP_DIR}/hl2_launcher"
for dylib in "${APP_DIR}"/*.dylib; do
	/usr/bin/codesign --force --sign "${SIGN_IDENTITY}" --timestamp=none "${dylib}"
done
/usr/bin/codesign --force --sign "${SIGN_IDENTITY}" \
	--entitlements "${ENTITLEMENTS}" \
	--deep \
	--timestamp=none \
	"${APP_DIR}"

IPA_PATH="${ROOT}/build/${APP_NAME}-ios-aarch64.ipa"
rm -f "${IPA_PATH}"
(
	cd "${STAGING}"
	zip -qr "${IPA_PATH}" Payload
)

echo "==> Created ${IPA_PATH}"
echo "    Bundle ID: ${BUNDLE_ID}"
echo "    Install game files into the app Documents folder on device (VALVE_GAME_PATH)."
