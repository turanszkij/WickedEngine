#!/usr/bin/env bash
set -e

CURRENT_DIR="$(dirname $0)"

# brew install open-image-denoise
# brew install lld


cmake -B"${CURRENT_DIR}/build" -GNinja "$CURRENT_DIR" \
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
-DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
-DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \

# example: --target Editor --target WickedEngine_ext_shaders
cmake --build "${CURRENT_DIR}/build" "$@"
