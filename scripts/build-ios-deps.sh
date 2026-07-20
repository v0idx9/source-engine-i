#!/usr/bin/env bash
# Cross-compile third-party static libraries required for the iOS engine build.
set -euo pipefail

source "$(dirname "$0")/build-ios-common.sh"

echo "Building iOS dependencies into ${OUT}"

CMAKE_IOS=(
	-G Ninja
	-DCMAKE_SYSTEM_NAME=iOS
	-DCMAKE_OSX_ARCHITECTURES=arm64
	-DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_MIN_VERSION}"
	-DCMAKE_OSX_SYSROOT="${SDKROOT}"
	-DCMAKE_BUILD_TYPE=Release
	-DCMAKE_POLICY_VERSION_MINIMUM=3.5
)

need_libs=(libz.a libbz2.a libpng.a libjpeg.a libfreetype2.a libcurl.a libSDL2.a)
# sdlmgr.cpp talks to EGL directly, so a cached deps dir without ANGLE is not complete.
if [ "${BUILD_ANGLE:-0}" = "1" ]; then
	need_libs+=(libEGL.dylib libGLESv2.dylib)
fi
all_present=true
for lib in "${need_libs[@]}"; do
	if [ ! -f "${OUT}/${lib}" ]; then
		all_present=false
		break
	fi
done
if [ "${all_present}" = true ]; then
	echo "All iOS deps already present in ${OUT}, skipping rebuild."
	exit 0
fi

build_zlib() {
	echo "==> zlib"
	local dir="${DEPS}/zlib"
	mkdir -p "${dir}"
	cd "${ROOT}/thirdparty/zlib"
	CHOST="${IOS_AUTOTOOLS_HOST}" ./configure --static --prefix="${dir}"
	make -j"$(sysctl -n hw.ncpu)" clean
	make -j"$(sysctl -n hw.ncpu)"
	make install
	copy_if_exists "${dir}/lib/libz.a"
}

build_bzip2() {
	echo "==> bzip2"
	local dir="${DEPS}/bzip2"
	mkdir -p "${dir}"
	cd "${dir}"
	${CC} ${CFLAGS} -c \
		"${ROOT}/utils/bzip2/blocksort.c" \
		"${ROOT}/utils/bzip2/bzlib.c" \
		"${ROOT}/utils/bzip2/compress.c" \
		"${ROOT}/utils/bzip2/crctable.c" \
		"${ROOT}/utils/bzip2/decompress.c" \
		"${ROOT}/utils/bzip2/huffman.c" \
		"${ROOT}/utils/bzip2/randtable.c"
	${AR} rcs "${OUT}/libbz2.a" ./*.o
	rm -f ./*.o
}

build_libpng() {
	echo "==> libpng"
	local dir="${DEPS}/libpng"
	local zlib_inc="${DEPS}/zlib/include"
	mkdir -p "${dir}"
	cd "${ROOT}/thirdparty/libpng"
	./configure --host="${IOS_AUTOTOOLS_HOST}" --enable-static --disable-shared --prefix="${dir}" \
		CC="${CC}" AR="${AR}" RANLIB="${RANLIB}" \
		CPPFLAGS="-I${zlib_inc}" CFLAGS="${CFLAGS} -I${zlib_inc}" \
		LDFLAGS="${LDFLAGS} -L${OUT}" LIBS="-lz"
	make -j"$(sysctl -n hw.ncpu)" clean
	make -j"$(sysctl -n hw.ncpu)"
	make install
	copy_if_exists "${dir}/lib/libpng16.a"
	if [ -f "${dir}/lib/libpng16.a" ] && [ ! -f "${OUT}/libpng.a" ]; then
		cp "${dir}/lib/libpng16.a" "${OUT}/libpng.a"
	fi
}

build_libjpeg() {
	echo "==> libjpeg"
	local dir="${DEPS}/libjpeg"
	mkdir -p "${dir}"
	cd "${ROOT}/thirdparty/libjpeg"
	./configure --host="${IOS_AUTOTOOLS_HOST}" --enable-static --disable-shared --prefix="${dir}" \
		CC="${CC}" AR="${AR}" RANLIB="${RANLIB}" CFLAGS="${CFLAGS}"
	make -j"$(sysctl -n hw.ncpu)" clean
	make -j"$(sysctl -n hw.ncpu)"
	make install
	copy_if_exists "${dir}/lib/libjpeg.a"
}

build_freetype() {
	echo "==> freetype"
	local dir="${DEPS}/freetype"
	mkdir -p "${dir}"
	cd "${ROOT}/thirdparty/freetype"
	cmake -S . -B "${dir}/build" "${CMAKE_IOS[@]}" \
		-DBUILD_SHARED_LIBS=OFF \
		-DFT_DISABLE_HARFBUZZ=ON \
		-DFT_DISABLE_BROTLI=ON \
		-DCMAKE_INSTALL_PREFIX="${dir}"
	cmake --build "${dir}/build" --target install
	if [ -f "${dir}/lib/libfreetype.a" ]; then
		cp "${dir}/lib/libfreetype.a" "${OUT}/libfreetype2.a"
	fi
}

build_curl() {
	echo "==> curl"
	local dir="${DEPS}/curl"
	mkdir -p "${dir}"
	cd "${ROOT}/thirdparty/curl"
	# curl 7.79 enables Secure Transport only when CMAKE_SYSTEM_NAME=Darwin.
	cmake -S . -B "${dir}/build" -G Ninja \
		-DCMAKE_SYSTEM_NAME=Darwin \
		-DCMAKE_OSX_ARCHITECTURES=arm64 \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_MIN_VERSION}" \
		-DCMAKE_OSX_SYSROOT="${SDKROOT}" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
		-DCMAKE_CROSSCOMPILING=TRUE \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_CURL_EXE=OFF \
		-DCURL_STATICLIB=ON \
		-DCMAKE_USE_SECTRANSP=ON \
		-DCMAKE_USE_OPENSSL=OFF \
		-DCURL_CA_BUNDLE=none \
		-DCURL_DISABLE_LDAP=ON \
		-DCURL_DISABLE_LDAPS=ON \
		-DCURL_DISABLE_SSH=ON \
		-DCMAKE_USE_LIBSSH2=OFF \
		-DHTTP_ONLY=ON \
		-DHAVE_POLL_FINE_EXITCODE=0 \
		-DCMAKE_INSTALL_PREFIX="${dir}"
	cmake --build "${dir}/build" --target install
	copy_if_exists "${dir}/lib/libcurl.a"
}

build_sdl2() {
	echo "==> SDL2"
	local dir="${DEPS}/sdl2"
	mkdir -p "${dir}"
	local sdl_src="${ROOT}/thirdparty/SDL-src"
	if [ ! -d "${sdl_src}" ]; then
		sdl_src="${ROOT}/thirdparty/SDL"
	fi
	cd "${sdl_src}"
	cmake -S . -B "${dir}/build" "${CMAKE_IOS[@]}" \
		-DSDL_STATIC=OFF \
		-DSDL_SHARED=ON \
		-DSDL_TEST=OFF \
		-DSDL_TESTS=OFF \
		-DSDL_RENDER=OFF \
		-DCMAKE_INSTALL_PREFIX="${dir}"
	if [ -f "${dir}/build/build.ninja" ]; then
		sed -i '' 's/-Werror=declaration-after-statement/-Wno-error=declaration-after-statement/g' "${dir}/build/build.ninja"
	fi
	cmake --build "${dir}/build" --target install
	copy_if_exists "${dir}/lib/libSDL2.dylib"
	rm -f "${OUT}/libSDL2.a"
}

build_angle() {
	if [ "${BUILD_ANGLE:-0}" != "1" ]; then
		echo "==> skipping ANGLE (BUILD_ANGLE=${BUILD_ANGLE:-0})"
		return 0
	fi

	echo "==> ANGLE"
	local angle_dir="${DEPS}/angle"
	local angle_src="${angle_dir}/src"
	local angle_build="${angle_dir}/build"
	mkdir -p "${angle_src}" "${angle_build}"

	# ANGLE is a GN project (.gn/BUILD.gn/DEPS, no CMakeLists.txt), so it has to be
	# built with depot_tools rather than cmake.
	local depot_tools="${angle_dir}/depot_tools"
	if [ ! -d "${depot_tools}/.git" ]; then
		git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git "${depot_tools}"
	fi
	export PATH="${depot_tools}:${PATH}"
	# Let depot_tools self-bootstrap: it writes python3_bin_reldir.txt on first
	# run, which gclient's siso hook requires. Pinning DEPOT_TOOLS_UPDATE=0 here
	# skips that and the sync fails at configure_siso.py.
	gclient --version >/dev/null 2>&1 || true

	if [ ! -d "${angle_src}/.git" ]; then
		git clone https://chromium.googlesource.com/angle/angle.git "${angle_src}"
	fi

	cd "${angle_src}"
	python3 scripts/bootstrap.py
	gclient sync -D --force --no-history

	# ios_enable_code_signing=false: we sign the .app ourselves at packaging time.
	# target_environment is mandatory for target_os="ios" (build/config/apple/mobile_config.gni).
	gn gen "${angle_build}" --args="target_os=\"ios\" target_cpu=\"arm64\" target_environment=\"device\" ios_deployment_target=\"${IOS_MIN_VERSION}\" is_debug=false ios_enable_code_signing=false use_siso=false angle_enable_metal=true angle_enable_vulkan=false angle_enable_gl=false angle_enable_null=false angle_enable_swiftshader=false angle_build_tests=false"

	autoninja -C "${angle_build}" libEGL libGLESv2

	copy_if_exists "${angle_build}/libEGL.dylib"
	copy_if_exists "${angle_build}/libGLESv2.dylib"

	mkdir -p "${ROOT}/thirdparty/angle/include"
	rsync -a "${angle_src}/include/" "${ROOT}/thirdparty/angle/include/"
}

build_zlib
build_bzip2
build_libpng
build_libjpeg
build_freetype
build_curl
build_sdl2
# Not optional when enabled: the iOS renderer needs EGL to link.
build_angle

echo "Installed iOS libraries:"
ls -la "${OUT}"
