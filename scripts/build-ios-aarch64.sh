#!/usr/bin/env bash
# Build the Source Engine for iOS (arm64 device by default).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${ROOT}"

git submodule update --init --recursive

# iOS SDK has no <malloc.h>; patch IVP sources after submodule checkout.
if [ -d "${ROOT}/ivp" ]; then
	python3 - "${ROOT}/ivp" <<'PY'
import sys
from pathlib import Path

root = Path(sys.argv[1])

ivu_types = root / "ivp_utility" / "ivu_types.hxx"
if ivu_types.is_file():
    text = ivu_types.read_text()
    old = "#   ifdef OSX\n#       include <malloc/malloc.h>\n#   else\n#       include <malloc.h>"
    new = "#   if defined(__APPLE__)\n#       include <malloc/malloc.h>\n#   else\n#       include <malloc.h>"
    if old in text and new not in text:
        ivu_types.write_text(text.replace(old, new, 1))

for path in root.rglob("*"):
    if not path.is_file() or path.suffix not in {".cxx", ".cpp", ".c", ".h", ".hxx", ".hpp"}:
        continue
    text = path.read_text(errors="ignore")
    if "#include <malloc.h>" not in text:
        continue
    path.write_text(text.replace("#include <malloc.h>", "#include <malloc/malloc.h>"))
PY
fi

if [ ! -d "${ROOT}/lib/darwin/aarch64" ] || [ -z "$(ls -A "${ROOT}/lib/darwin/aarch64" 2>/dev/null || true)" ]; then
	./scripts/build-ios-deps.sh
fi

CONFIGURE_ARGS=(
	-T release
	--ios
	--togles
	--disable-warns
	--build-games=portal
	--skip-sdl2-sanity-check
)

if [ -d "${ROOT}/lib/darwin/aarch64/libEGL.framework" ]; then
	CONFIGURE_ARGS+=(--angle)
else
	echo "WARNING: libEGL.framework not found in lib/darwin/aarch64; building with native OpenGLES." >&2
fi

# iOS uses static libSDL2.a and SDL-src headers via scripts/waifulib/sdl2.py.

if [ "${IOS_SIMULATOR:-0}" = "1" ]; then
	CONFIGURE_ARGS+=(--simulator)
fi

./waf configure "${CONFIGURE_ARGS[@]}" "$@"
./waf build -v

echo
echo "iOS build finished. Engine libraries are under ${ROOT}/build/"
