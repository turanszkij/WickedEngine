// This file should contain implementations of header-only utility libraries
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // _CRT_SECURE_NO_WARNINGS

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define TINYDDSLOADER_IMPLEMENTATION
#include "tinyddsloader.h"





// Basis Universal library sources are compiled below for simplicity:

//#define BASISU_FORCE_DEVEL_MESSAGES 1
#define BASISU_NO_ITERATOR_DEBUG_LEVEL
#include "basis_universal/transcoder/basisu_transcoder.cpp"

#undef _CRT_SECURE_NO_WARNINGS
#include "basis_universal/encoder/jpgd.cpp"
#include "basis_universal/encoder/lodepng.cpp"
#include "basis_universal/encoder/apg_bmp.c"
#include "basis_universal/encoder/basisu_astc_decomp.cpp"
#include "basis_universal/encoder/basisu_backend.cpp"
#include "basis_universal/encoder/basisu_basis_file.cpp"
#include "basis_universal/encoder/basisu_bc7enc.cpp"
#include "basis_universal/encoder/basisu_comp.cpp"
#include "basis_universal/encoder/basisu_enc.cpp"
#include "basis_universal/encoder/basisu_etc.cpp"
#include "basis_universal/encoder/basisu_frontend.cpp"
#include "basis_universal/encoder/basisu_global_selector_palette_helpers.cpp"
#include "basis_universal/encoder/basisu_gpu_texture.cpp"
#include "basis_universal/encoder/basisu_kernels_sse.cpp"
#include "basis_universal/encoder/basisu_pvrtc1_4.cpp"
#include "basis_universal/encoder/basisu_resampler.cpp"
#include "basis_universal/encoder/basisu_resample_filters.cpp"
#include "basis_universal/encoder/basisu_ssim.cpp"
#include "basis_universal/encoder/basisu_uastc_enc.cpp"

#undef CLAMP
//#include "basis_universal/zstd/zstddeclib.c"
#include "basis_universal/zstd/zstd.c"

basist::etc1_global_selector_codebook g_basis_global_codebook(basist::g_global_selector_cb_size, basist::g_global_selector_cb);


