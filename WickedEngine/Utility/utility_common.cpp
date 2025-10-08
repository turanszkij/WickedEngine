// This file should contain implementations of header-only utility libraries
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // _CRT_SECURE_NO_WARNINGS

// prevent linking problems when using ASAN
extern "C" {

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define MINIMP4_IMPLEMENTATION
#include "minimp4.h"
#undef RETURN_ERROR

#include "mikktspace.c"

#include "zstd/zstd.c"

}
