// ============================================================================
// ImGee: Isolated Internal stb_image Wrapper (contrib/ImGee/imgee_stbi_internal.h)
// ----------------------------------------------------------------------------
// Author: Steven L. Goldberg (@slgoldberg) and the Rogers
// License: BSD 3-Clause (see LICENSE / ImgWindow license terms)
//
// Provides a 100% isolated, header-only PNG/JPEG/BMP/GIF/TGA image decoder for
// ImGee and ImgWindow::RegisterMemoryImage with ZERO global namespace or
// preprocessor collisions against host plugins that also include <stb_image.h>
// (in any include order, and regardless of version or linkage).
//
// Configuration Knobs (Optional):
//   - #define IMGEE_DISABLE_STB_IMAGE   : Disables ImGee's built-in stb_image
//   - #define IMGEE_EXTERNAL_STB_IMAGE  : Emits declarations only; add
//                                         contrib/ImGee/imgee_stb_image.cpp to
//                                         your CMake target to compile once.
// ============================================================================

#pragma once

#ifndef IMGEE_STBI_INTERNAL_H
#define IMGEE_STBI_INTERNAL_H

#if !defined(IMGEE_DISABLE_STB_IMAGE)

// 1. Pre-include all standard C & SIMD headers outside the private C++ namespace
//    so system headers are never expanded inside namespace ImGeeStbi.
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>

#if !defined(STBI_NO_SIMD) && (defined(__x86_64__) || defined(_M_X64) || (defined(__i386) && defined(__SSE2__)) || defined(_M_IX86))
    #if !(defined(__MINGW32__) && (defined(__i386) || defined(_M_IX86)) && !defined(STBI_MINGW_ENABLE_SSE2))
        #include <emmintrin.h>
        #if defined(_MSC_VER) && _MSC_VER >= 1400
            #include <intrin.h>
        #endif
    #endif
#endif

#if defined(STBI_NEON) && !defined(STBI_NO_SIMD)
    #include <arm_neon.h>
#endif

// 2. Snapshot all stb_image configuration & internal preprocessor macros so
//    any host <stb_image.h> included before or after this file is untouched.
#pragma push_macro("STBI_INCLUDE_STB_IMAGE_H")
#pragma push_macro("STB_IMAGE_IMPLEMENTATION")
#pragma push_macro("STB_IMAGE_STATIC")
#pragma push_macro("STBIDEF")
#pragma push_macro("STBI_VERSION")
#pragma push_macro("STBI_ONLY_JPEG")
#pragma push_macro("STBI_ONLY_PNG")
#pragma push_macro("STBI_ONLY_BMP")
#pragma push_macro("STBI_ONLY_TGA")
#pragma push_macro("STBI_ONLY_GIF")
#pragma push_macro("STBI_ONLY_PSD")
#pragma push_macro("STBI_ONLY_HDR")
#pragma push_macro("STBI_ONLY_PIC")
#pragma push_macro("STBI_ONLY_PNM")
#pragma push_macro("STBI_ONLY_ZLIB")
#pragma push_macro("STBI_NO_JPEG")
#pragma push_macro("STBI_NO_PNG")
#pragma push_macro("STBI_NO_BMP")
#pragma push_macro("STBI_NO_PSD")
#pragma push_macro("STBI_NO_TGA")
#pragma push_macro("STBI_NO_GIF")
#pragma push_macro("STBI_NO_HDR")
#pragma push_macro("STBI_NO_PIC")
#pragma push_macro("STBI_NO_PNM")
#pragma push_macro("STBI_NO_LINEAR")
#pragma push_macro("STBI_NO_STDIO")
#pragma push_macro("STBI_NO_ZLIB")
#pragma push_macro("STBI_SUPPORT_ZLIB")
#pragma push_macro("STBI_NO_SIMD")
#pragma push_macro("STBI_SSE2")
#pragma push_macro("STBI_NEON")
#pragma push_macro("STBI_MINGW_ENABLE_SSE2")
#pragma push_macro("STBI_NO_FAILURE_STRINGS")
#pragma push_macro("STBI_FAILURE_USERMSG")
#pragma push_macro("STBI_ASSERT")
#pragma push_macro("STBI_MALLOC")
#pragma push_macro("STBI_REALLOC")
#pragma push_macro("STBI_FREE")
#pragma push_macro("STBI_REALLOC_SIZED")
#pragma push_macro("STBI_NOTUSED")
#pragma push_macro("STBI_HAS_LROTL")
#pragma push_macro("STBI_SIMD_ALIGN")
#pragma push_macro("stbi_inline")
#pragma push_macro("stbi_lrot")
#pragma push_macro("STBI__X64_TARGET")
#pragma push_macro("STBI__X86_TARGET")
#pragma push_macro("stbi__err")
#pragma push_macro("stbi__errpf")
#pragma push_macro("stbi__errpuc")
#pragma push_macro("STBI__BYTECAST")
#pragma push_macro("STBI__COMBO")
#pragma push_macro("STBI__CASE")
#pragma push_macro("stbi__float2int")
#pragma push_macro("FAST_BITS")
#pragma push_macro("stbi__f2f")
#pragma push_macro("stbi__fsh")
#pragma push_macro("STBI__IDCT_1D")
#pragma push_macro("dct_const")
#pragma push_macro("dct_rot")
#pragma push_macro("dct_widen")
#pragma push_macro("dct_wadd")
#pragma push_macro("dct_wsub")
#pragma push_macro("dct_bfly32o")
#pragma push_macro("dct_interleave8")
#pragma push_macro("dct_interleave16")
#pragma push_macro("dct_pass")
#pragma push_macro("dct_long_mul")
#pragma push_macro("dct_long_mac")
#pragma push_macro("dct_trn16")
#pragma push_macro("dct_trn32")
#pragma push_macro("dct_trn64")
#pragma push_macro("dct_trn8_8")
#pragma push_macro("dct_trn8_16")
#pragma push_macro("dct_trn8_32")
#pragma push_macro("STBI__MARKER_none")
#pragma push_macro("STBI__RESTART")
#pragma push_macro("stbi__DNL")
#pragma push_macro("stbi__SOI")
#pragma push_macro("stbi__EOI")
#pragma push_macro("stbi__SOF")
#pragma push_macro("stbi__SOS")
#pragma push_macro("stbi__SOF_progressive")
#pragma push_macro("stbi__div4")
#pragma push_macro("stbi__div16")
#pragma push_macro("stbi__float2fixed")
#pragma push_macro("STBI__ZFAST_BITS")
#pragma push_macro("STBI__ZFAST_MASK")
#pragma push_macro("STBI__PNG_TYPE")
#pragma push_macro("STBI__HDR_BUFLEN")

#undef STBI_INCLUDE_STB_IMAGE_H
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_STATIC
#undef STBIDEF
#undef STBI_VERSION
#undef STBI_ONLY_JPEG
#undef STBI_ONLY_PNG
#undef STBI_ONLY_BMP
#undef STBI_ONLY_TGA
#undef STBI_ONLY_GIF
#undef STBI_ONLY_PSD
#undef STBI_ONLY_HDR
#undef STBI_ONLY_PIC
#undef STBI_ONLY_PNM
#undef STBI_ONLY_ZLIB
#undef STBI_NO_JPEG
#undef STBI_NO_PNG
#undef STBI_NO_BMP
#undef STBI_NO_PSD
#undef STBI_NO_TGA
#undef STBI_NO_GIF
#undef STBI_NO_HDR
#undef STBI_NO_PIC
#undef STBI_NO_PNM
#undef STBI_NO_LINEAR
#undef STBI_NO_STDIO
#undef STBI_NO_ZLIB
#undef STBI_SUPPORT_ZLIB
#undef STBI_NO_SIMD
#undef STBI_SSE2
#undef STBI_NEON
#undef STBI_MINGW_ENABLE_SSE2
#undef STBI_NO_FAILURE_STRINGS
#undef STBI_FAILURE_USERMSG
#undef STBI_ASSERT
#undef STBI_MALLOC
#undef STBI_REALLOC
#undef STBI_FREE
#undef STBI_REALLOC_SIZED
#undef STBI_NOTUSED
#undef STBI_HAS_LROTL
#undef STBI_SIMD_ALIGN
#undef stbi_inline
#undef stbi_lrot
#undef STBI__X64_TARGET
#undef STBI__X86_TARGET
#undef stbi__err
#undef stbi__errpf
#undef stbi__errpuc
#undef STBI__BYTECAST
#undef STBI__COMBO
#undef STBI__CASE
#undef stbi__float2int
#undef FAST_BITS
#undef stbi__f2f
#undef stbi__fsh
#undef STBI__IDCT_1D
#undef dct_const
#undef dct_rot
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_interleave8
#undef dct_interleave16
#undef dct_pass
#undef dct_long_mul
#undef dct_long_mac
#undef dct_trn16
#undef dct_trn32
#undef dct_trn64
#undef dct_trn8_8
#undef dct_trn8_16
#undef dct_trn8_32
#undef STBI__MARKER_none
#undef STBI__RESTART
#undef stbi__DNL
#undef stbi__SOI
#undef stbi__EOI
#undef stbi__SOF
#undef stbi__SOS
#undef stbi__SOF_progressive
#undef stbi__div4
#undef stbi__div16
#undef stbi__float2fixed
#undef STBI__ZFAST_BITS
#undef STBI__ZFAST_MASK
#undef STBI__PNG_TYPE
#undef STBI__HDR_BUFLEN

// 3. Configure lightweight raster-only UI decoder (PNG, JPEG, BMP, GIF, TGA)
#define STBI_NO_PSD
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_LINEAR
#define STBI_NO_FAILURE_STRINGS

#if !defined(IMGEE_EXTERNAL_STB_IMAGE)
    #define STB_IMAGE_STATIC
    #define STB_IMAGE_IMPLEMENTATION
#elif defined(IMGEE_STB_IMAGE_TU)
    #define STB_IMAGE_IMPLEMENTATION
#endif

// 4. Prefix all extern "C" STBIDEF symbols to prevent C-linkage collisions ([dcl.link]/6)
#define stbi_load_from_memory                       imgee_stbi_load_from_memory
#define stbi_load_from_callbacks                    imgee_stbi_load_from_callbacks
#define stbi_load_gif_from_memory                   imgee_stbi_load_gif_from_memory
#define stbi_load                                   imgee_stbi_load
#define stbi_load_from_file                         imgee_stbi_load_from_file
#define stbi_load_16_from_memory                    imgee_stbi_load_16_from_memory
#define stbi_load_16_from_callbacks                 imgee_stbi_load_16_from_callbacks
#define stbi_load_16                                imgee_stbi_load_16
#define stbi_load_from_file_16                      imgee_stbi_load_from_file_16
#define stbi_loadf_from_memory                      imgee_stbi_loadf_from_memory
#define stbi_loadf_from_callbacks                   imgee_stbi_loadf_from_callbacks
#define stbi_loadf                                  imgee_stbi_loadf
#define stbi_loadf_from_file                        imgee_stbi_loadf_from_file
#define stbi_hdr_to_ldr_gamma                       imgee_stbi_hdr_to_ldr_gamma
#define stbi_hdr_to_ldr_scale                       imgee_stbi_hdr_to_ldr_scale
#define stbi_ldr_to_hdr_gamma                       imgee_stbi_ldr_to_hdr_gamma
#define stbi_ldr_to_hdr_scale                       imgee_stbi_ldr_to_hdr_scale
#define stbi_is_hdr_from_callbacks                  imgee_stbi_is_hdr_from_callbacks
#define stbi_is_hdr_from_memory                     imgee_stbi_is_hdr_from_memory
#define stbi_is_hdr                                 imgee_stbi_is_hdr
#define stbi_is_hdr_from_file                       imgee_stbi_is_hdr_from_file
#define stbi_failure_reason                         imgee_stbi_failure_reason
#define stbi_image_free                             imgee_stbi_image_free
#define stbi_info_from_memory                       imgee_stbi_info_from_memory
#define stbi_info_from_callbacks                    imgee_stbi_info_from_callbacks
#define stbi_is_16_bit_from_memory                  imgee_stbi_is_16_bit_from_memory
#define stbi_is_16_bit_from_callbacks               imgee_stbi_is_16_bit_from_callbacks
#define stbi_info                                   imgee_stbi_info
#define stbi_info_from_file                         imgee_stbi_info_from_file
#define stbi_is_16_bit                              imgee_stbi_is_16_bit
#define stbi_is_16_bit_from_file                    imgee_stbi_is_16_bit_from_file
#define stbi_set_unpremultiply_on_load              imgee_stbi_set_unpremultiply_on_load
#define stbi_convert_iphone_png_to_rgb              imgee_stbi_convert_iphone_png_to_rgb
#define stbi_set_flip_vertically_on_load            imgee_stbi_set_flip_vertically_on_load
#define stbi_zlib_decode_malloc_guesssize           imgee_stbi_zlib_decode_malloc_guesssize
#define stbi_zlib_decode_malloc_guesssize_headerflag imgee_stbi_zlib_decode_malloc_guesssize_headerflag
#define stbi_zlib_decode_malloc                     imgee_stbi_zlib_decode_malloc
#define stbi_zlib_decode_buffer                     imgee_stbi_zlib_decode_buffer
#define stbi_zlib_decode_noheader_malloc            imgee_stbi_zlib_decode_noheader_malloc
#define stbi_zlib_decode_noheader_buffer            imgee_stbi_zlib_decode_noheader_buffer

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4100 4244 4456 4457 4701 4996)
#endif

namespace ImGeeStbi {
#include "../stb_image.h"

    inline unsigned char* LoadFromMemory(const void* data, size_t len, int* outW, int* outH) {
        int comp = 0;
        return imgee_stbi_load_from_memory(static_cast<const stbi_uc*>(data), static_cast<int>(len), outW, outH, &comp, 4);
    }

    inline unsigned char* LoadFromFile(const char* filename, int* outW, int* outH) {
        int comp = 0;
        return imgee_stbi_load(filename, outW, outH, &comp, 4);
    }

    inline void ImageFree(void* pixels) {
        imgee_stbi_image_free(pixels);
    }
} // namespace ImGeeStbi

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

// 5. Clean up symbol-prefixing macros and restore exact prior preprocessor state
#undef stbi_load_from_memory
#undef stbi_load_from_callbacks
#undef stbi_load_gif_from_memory
#undef stbi_load
#undef stbi_load_from_file
#undef stbi_load_16_from_memory
#undef stbi_load_16_from_callbacks
#undef stbi_load_16
#undef stbi_load_from_file_16
#undef stbi_loadf_from_memory
#undef stbi_loadf_from_callbacks
#undef stbi_loadf
#undef stbi_loadf_from_file
#undef stbi_hdr_to_ldr_gamma
#undef stbi_hdr_to_ldr_scale
#undef stbi_ldr_to_hdr_gamma
#undef stbi_ldr_to_hdr_scale
#undef stbi_is_hdr_from_callbacks
#undef stbi_is_hdr_from_memory
#undef stbi_is_hdr
#undef stbi_is_hdr_from_file
#undef stbi_failure_reason
#undef stbi_image_free
#undef stbi_info_from_memory
#undef stbi_info_from_callbacks
#undef stbi_is_16_bit_from_memory
#undef stbi_is_16_bit_from_callbacks
#undef stbi_info
#undef stbi_info_from_file
#undef stbi_is_16_bit
#undef stbi_is_16_bit_from_file
#undef stbi_set_unpremultiply_on_load
#undef stbi_convert_iphone_png_to_rgb
#undef stbi_set_flip_vertically_on_load
#undef stbi_zlib_decode_malloc_guesssize
#undef stbi_zlib_decode_malloc_guesssize_headerflag
#undef stbi_zlib_decode_malloc
#undef stbi_zlib_decode_buffer
#undef stbi_zlib_decode_noheader_malloc
#undef stbi_zlib_decode_noheader_buffer

#pragma pop_macro("STBI__HDR_BUFLEN")
#pragma pop_macro("STBI__PNG_TYPE")
#pragma pop_macro("STBI__ZFAST_MASK")
#pragma pop_macro("STBI__ZFAST_BITS")
#pragma pop_macro("stbi__float2fixed")
#pragma pop_macro("stbi__div16")
#pragma pop_macro("stbi__div4")
#pragma pop_macro("stbi__SOF_progressive")
#pragma pop_macro("stbi__SOS")
#pragma pop_macro("stbi__SOF")
#pragma pop_macro("stbi__EOI")
#pragma pop_macro("stbi__SOI")
#pragma pop_macro("stbi__DNL")
#pragma pop_macro("STBI__RESTART")
#pragma pop_macro("STBI__MARKER_none")
#pragma pop_macro("dct_trn8_32")
#pragma pop_macro("dct_trn8_16")
#pragma pop_macro("dct_trn8_8")
#pragma pop_macro("dct_trn64")
#pragma pop_macro("dct_trn32")
#pragma pop_macro("dct_trn16")
#pragma pop_macro("dct_long_mac")
#pragma pop_macro("dct_long_mul")
#pragma pop_macro("dct_pass")
#pragma pop_macro("dct_interleave16")
#pragma pop_macro("dct_interleave8")
#pragma pop_macro("dct_bfly32o")
#pragma pop_macro("dct_wsub")
#pragma pop_macro("dct_wadd")
#pragma pop_macro("dct_widen")
#pragma pop_macro("dct_rot")
#pragma pop_macro("dct_const")
#pragma pop_macro("STBI__IDCT_1D")
#pragma pop_macro("stbi__fsh")
#pragma pop_macro("stbi__f2f")
#pragma pop_macro("FAST_BITS")
#pragma pop_macro("stbi__float2int")
#pragma pop_macro("STBI__CASE")
#pragma pop_macro("STBI__COMBO")
#pragma pop_macro("STBI__BYTECAST")
#pragma pop_macro("stbi__errpuc")
#pragma pop_macro("stbi__errpf")
#pragma pop_macro("stbi__err")
#pragma pop_macro("STBI__X86_TARGET")
#pragma pop_macro("STBI__X64_TARGET")
#pragma pop_macro("stbi_lrot")
#pragma pop_macro("stbi_inline")
#pragma pop_macro("STBI_SIMD_ALIGN")
#pragma pop_macro("STBI_HAS_LROTL")
#pragma pop_macro("STBI_NOTUSED")
#pragma pop_macro("STBI_REALLOC_SIZED")
#pragma pop_macro("STBI_FREE")
#pragma pop_macro("STBI_REALLOC")
#pragma pop_macro("STBI_MALLOC")
#pragma pop_macro("STBI_ASSERT")
#pragma pop_macro("STBI_FAILURE_USERMSG")
#pragma pop_macro("STBI_NO_FAILURE_STRINGS")
#pragma pop_macro("STBI_MINGW_ENABLE_SSE2")
#pragma pop_macro("STBI_NEON")
#pragma pop_macro("STBI_SSE2")
#pragma pop_macro("STBI_NO_SIMD")
#pragma pop_macro("STBI_SUPPORT_ZLIB")
#pragma pop_macro("STBI_NO_ZLIB")
#pragma pop_macro("STBI_NO_STDIO")
#pragma pop_macro("STBI_NO_LINEAR")
#pragma pop_macro("STBI_NO_PNM")
#pragma pop_macro("STBI_NO_PIC")
#pragma pop_macro("STBI_NO_HDR")
#pragma pop_macro("STBI_NO_GIF")
#pragma pop_macro("STBI_NO_TGA")
#pragma pop_macro("STBI_NO_PSD")
#pragma pop_macro("STBI_NO_BMP")
#pragma pop_macro("STBI_NO_PNG")
#pragma pop_macro("STBI_NO_JPEG")
#pragma pop_macro("STBI_ONLY_ZLIB")
#pragma pop_macro("STBI_ONLY_PNM")
#pragma pop_macro("STBI_ONLY_PIC")
#pragma pop_macro("STBI_ONLY_HDR")
#pragma pop_macro("STBI_ONLY_PSD")
#pragma pop_macro("STBI_ONLY_GIF")
#pragma pop_macro("STBI_ONLY_TGA")
#pragma pop_macro("STBI_ONLY_BMP")
#pragma pop_macro("STBI_ONLY_PNG")
#pragma pop_macro("STBI_ONLY_JPEG")
#pragma pop_macro("STBI_VERSION")
#pragma pop_macro("STBIDEF")
#pragma pop_macro("STB_IMAGE_STATIC")
#pragma pop_macro("STB_IMAGE_IMPLEMENTATION")
#pragma pop_macro("STBI_INCLUDE_STB_IMAGE_H")

#define IMGEE_HAS_STBI_INTERNAL 1

#endif // !IMGEE_DISABLE_STB_IMAGE

#endif // IMGEE_STBI_INTERNAL_H
