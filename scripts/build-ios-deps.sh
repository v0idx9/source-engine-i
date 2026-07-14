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

	if [ ! -d "${angle_src}/.git" ]; then
		git clone --depth 1 --branch main https://github.com/google/angle.git "${angle_src}"
	fi

	cmake -S "${angle_src}" -B "${angle_build}" "${CMAKE_IOS[@]}" \
		-DANGLE_ENABLE_METAL=ON \
		-DANGLE_ENABLE_OPENGL=OFF \
		-DANGLE_ENABLE_VULKAN=OFF \
		-DANGLE_ENABLE_NULL=OFF \
		-DANGLE_ENABLE_SWIFTSHADER=OFF \
		-DANGLE_ENABLE_TRACE=OFF \
		-DANGLE_ENABLE_TESTS=OFF \
		-DANGLE_ENABLE_EGL=ON \
		-DANGLE_ENABLE_GLES2=ON \
		-DANGLE_ENABLE_GLES3=ON

	cmake --build "${angle_build}" --target EGL GLESv2

	local angle_out="${angle_build}/lib"
	copy_if_exists "${angle_out}/libEGL.dylib"
	copy_if_exists "${angle_out}/libEGL.a"
	copy_if_exists "${angle_out}/libGLESv2.dylib"
	copy_if_exists "${angle_out}/libGLESv2.a"

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
build_angle || {
	echo "WARNING: ANGLE build failed; engine can still be built without --angle." >&2
}

echo "Installed iOS libraries:"
ls -la "${OUT}"
