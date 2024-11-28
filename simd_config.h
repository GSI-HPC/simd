/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                  Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_CONFIG_H_
#define PROTOTYPE_SIMD_CONFIG_H_

#include <bits/c++config.h>

#if SIMD_IN_STD
#define SIMD_NSPC std::datapar
#define SIMD_TOPLEVEL_NSPC std
#else
#define SIMD_NSPC cpp26::datapar
#define SIMD_TOPLEVEL_NSPC cpp26
#endif
namespace cpp26
{
  using namespace std;
}

#ifndef SIMD_IS_A_RANGE
// FIXME: not conforming to P1928
#define SIMD_IS_A_RANGE 1
#endif

#ifndef IFNDR_SIMD_PRECONDITIONS
// FIXME: not conforming to P1928
#define IFNDR_SIMD_PRECONDITIONS 1
#endif

#ifndef SIMD_DISABLED_HAS_API
// FIXME: not conforming to P1928
#define SIMD_DISABLED_HAS_API 1
#endif

#ifndef SIMD_HAS_SUBSCRIPT_GATHER
// cf. P2664
#define SIMD_HAS_SUBSCRIPT_GATHER 0
#endif

#ifndef SIMD_BROADCAST_CONDITIONAL_EXPLICIT
// FIXME: not conforming to P1928
#define SIMD_BROADCAST_CONDITIONAL_EXPLICIT 1
#endif

#ifndef SIMD_IMPLICIT_INTRINSICS_CONVERSION
// FIXME: not conforming to P1928
#define SIMD_IMPLICIT_INTRINSICS_CONVERSION 1
#endif

#if SIMD_IMPLICIT_INTRINSICS_CONVERSION
#define _GLIBCXX_SIMD_IMPLDEF_CONV_EXPLICIT
#else
#define _GLIBCXX_SIMD_IMPLDEF_CONV_EXPLICIT explicit
#endif

// x86 macros {

#ifdef __MMX__
#define _GLIBCXX_SIMD_HAVE_MMX 1ull
#else
#define _GLIBCXX_SIMD_HAVE_MMX 0ull
#endif

#if defined __SSE__ || defined __x86_64__
#define _GLIBCXX_SIMD_HAVE_SSE 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE 0ull
#endif

#if defined __SSE2__ || defined __x86_64__
#define _GLIBCXX_SIMD_HAVE_SSE2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE2 0ull
#endif

#ifdef __SSE3__
#define _GLIBCXX_SIMD_HAVE_SSE3 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE3 0ull
#endif

#ifdef __SSSE3__
#define _GLIBCXX_SIMD_HAVE_SSSE3 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSSE3 0ull
#endif

#ifdef __SSE4_1__
#define _GLIBCXX_SIMD_HAVE_SSE4_1 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE4_1 0ull
#endif

#ifdef __SSE4_2__
#define _GLIBCXX_SIMD_HAVE_SSE4_2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE4_2 0ull
#endif

#ifdef __XOP__
#define _GLIBCXX_SIMD_HAVE_XOP 1ull
#else
#define _GLIBCXX_SIMD_HAVE_XOP 0ull
#endif

#ifdef __AVX__
#define _GLIBCXX_SIMD_HAVE_AVX 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX 0ull
#endif

#ifdef __AVX2__
#define _GLIBCXX_SIMD_HAVE_AVX2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX2 0ull
#endif

#ifdef __BMI__
#define _GLIBCXX_SIMD_HAVE_BMI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_BMI 0ull
#endif

#ifdef __BMI2__
#define _GLIBCXX_SIMD_HAVE_BMI2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_BMI2 0ull
#endif

#ifdef __LZCNT__
#define _GLIBCXX_SIMD_HAVE_LZCNT 1ull
#else
#define _GLIBCXX_SIMD_HAVE_LZCNT 0ull
#endif

#ifdef __SSE4A__
#define _GLIBCXX_SIMD_HAVE_SSE4A 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE4A 0ull
#endif

#ifdef __FMA__
#define _GLIBCXX_SIMD_HAVE_FMA 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FMA 0ull
#endif

#ifdef __FMA4__
#define _GLIBCXX_SIMD_HAVE_FMA4 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FMA4 0ull
#endif

#ifdef __F16C__
#define _GLIBCXX_SIMD_HAVE_F16C 1ull
#else
#define _GLIBCXX_SIMD_HAVE_F16C 0ull
#endif

#ifdef __AVXIFMA__
#define _GLIBCXX_SIMD_HAVE_AVXIFMA 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXIFMA 0ull
#endif

#ifdef __AVXNECONVERT__
#define _GLIBCXX_SIMD_HAVE_AVXNECONVERT 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXNECONVERT 0ull
#endif

#ifdef __AVXVNNI__
#define _GLIBCXX_SIMD_HAVE_AVXVNNI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXVNNI 0ull
#endif

#ifdef __AVXVNNIINT8__
#define _GLIBCXX_SIMD_HAVE_AVXVNNIINT8 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXVNNIINT8 0ull
#endif

#ifdef __AVXVNNIINT16__
#define _GLIBCXX_SIMD_HAVE_AVXVNNIINT16 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXVNNIINT16 0ull
#endif

#ifdef __POPCNT__
#define _GLIBCXX_SIMD_HAVE_POPCNT 1ull
#else
#define _GLIBCXX_SIMD_HAVE_POPCNT 0ull
#endif

#ifdef __AVX512F__
#define _GLIBCXX_SIMD_HAVE_AVX512F 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512F 0ull
#endif

#ifdef __AVX512DQ__
#define _GLIBCXX_SIMD_HAVE_AVX512DQ 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512DQ 0ull
#endif

#ifdef __AVX512VL__
#define _GLIBCXX_SIMD_HAVE_AVX512VL 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VL 0ull
#endif

#ifdef __AVX512BW__
#define _GLIBCXX_SIMD_HAVE_AVX512BW 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512BW 0ull
#endif

#ifdef __AVX512BITALG__
#define _GLIBCXX_SIMD_HAVE_AVX512BITALG 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512BITALG 0ull
#endif

#ifdef __AVX512VBMI2__
#define _GLIBCXX_SIMD_HAVE_AVX512VBMI2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VBMI2 0ull
#endif

#ifdef __AVX512VBMI__
#define _GLIBCXX_SIMD_HAVE_AVX512VBMI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VBMI 0ull
#endif

#ifdef __AVX512IFMA__
#define _GLIBCXX_SIMD_HAVE_AVX512IFMA 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512IFMA 0ull
#endif

#ifdef __AVX512CD__
#define _GLIBCXX_SIMD_HAVE_AVX512CD 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512CD 0ull
#endif

#ifdef __AVX512VNNI__
#define _GLIBCXX_SIMD_HAVE_AVX512VNNI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VNNI 0ull
#endif

#ifdef __AVX512VPOPCNTDQ__
#define _GLIBCXX_SIMD_HAVE_AVX512VPOPCNTDQ 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VPOPCNTDQ 0ull
#endif

#ifdef __AVX512VP2INTERSECT__
#define _GLIBCXX_SIMD_HAVE_AVX512VP2INTERSECT 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VP2INTERSECT 0ull
#endif

#ifdef __AVX512FP16__
#define _GLIBCXX_SIMD_HAVE_AVX512FP16 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512FP16 0ull
#endif

#ifdef __AVX512BF16__
#define _GLIBCXX_SIMD_HAVE_AVX512BF16 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512BF16 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_SSE
#define _GLIBCXX_SIMD_HAVE_SSE_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_SSE2
#define _GLIBCXX_SIMD_HAVE_FULL_SSE_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FULL_SSE_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_AVX
#define _GLIBCXX_SIMD_HAVE_AVX_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_AVX2
#define _GLIBCXX_SIMD_HAVE_FULL_AVX_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FULL_AVX_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_AVX512F
#define _GLIBCXX_SIMD_HAVE_AVX512_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_AVX512BW
#define _GLIBCXX_SIMD_HAVE_FULL_AVX512_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FULL_AVX512_ABI 0ull
#endif

#if defined __x86_64__ && !_GLIBCXX_SIMD_HAVE_SSE2
#error "Use of SSE2 is required on AMD64"
#endif

//}

// ARM macros {
#if defined __ARM_NEON
#define _GLIBCXX_SIMD_HAVE_NEON 1
#else
#define _GLIBCXX_SIMD_HAVE_NEON 0
#endif
#if defined __ARM_NEON && (__ARM_ARCH >= 8 || defined __aarch64__)
#define _GLIBCXX_SIMD_HAVE_NEON_A32 1
#else
#define _GLIBCXX_SIMD_HAVE_NEON_A32 0
#endif
#if defined __ARM_NEON && defined __aarch64__
#define _GLIBCXX_SIMD_HAVE_NEON_A64 1
#else
#define _GLIBCXX_SIMD_HAVE_NEON_A64 0
#endif
#if (__ARM_FEATURE_SVE_BITS > 0 && __ARM_FEATURE_SVE_VECTOR_OPERATORS==1)
#define _GLIBCXX_SIMD_HAVE_SVE 1
#else
#define _GLIBCXX_SIMD_HAVE_SVE 0
#endif
// }

#ifndef _GLIBCXX_SIMD_INTRINSIC
#ifdef _GLIBCXX_SIMD_NO_ALWAYS_INLINE
#define _GLIBCXX_SIMD_INTRINSIC inline
#define _GLIBCXX_SIMD_ALWAYS_INLINE
#else
#define _GLIBCXX_SIMD_INTRINSIC [[__gnu__::__always_inline__, __gnu__::__artificial__]] inline
#define _GLIBCXX_SIMD_ALWAYS_INLINE [[__gnu__::__always_inline__]]
#endif
#endif

#ifndef _GLIBCXX_SIMD_LIST_BINARY
#define _GLIBCXX_SIMD_LIST_BINARY(__macro) __macro(|) __macro(&) __macro(^)
#define _GLIBCXX_SIMD_LIST_SHIFTS(__macro) __macro(<<) __macro(>>)
#define _GLIBCXX_SIMD_LIST_ARITHMETICS(__macro)                                \
  __macro(+) __macro(-) __macro(*) __macro(/) __macro(%)

#define _GLIBCXX_SIMD_ALL_BINARY(__macro)                                      \
  _GLIBCXX_SIMD_LIST_BINARY(__macro) static_assert(true)
#define _GLIBCXX_SIMD_ALL_SHIFTS(__macro)                                      \
  _GLIBCXX_SIMD_LIST_SHIFTS(__macro) static_assert(true)
#define _GLIBCXX_SIMD_ALL_ARITHMETICS(__macro)                                 \
  _GLIBCXX_SIMD_LIST_ARITHMETICS(__macro) static_assert(true)
#endif

#if defined __GXX_CONDITIONAL_IS_OVERLOADABLE__ and SIMD_CONDITIONAL_OPERATOR
#define simd_select_impl operator?:
#endif

#define _GLIBCXX_SIMD_INT_PACK(N, pack, code)                                                      \
  [&]<int... pack> [[__gnu__::__always_inline__]] (std::integer_sequence<int, pack...>)            \
    code (std::make_integer_sequence<int, N>())

#if __cpp_deleted_function >= 202403L
#define _GLIBCXX_DELETE_MSG(msg) delete(msg)
#else
#define _GLIBCXX_DELETE_MSG(msg) delete
#endif


#endif  // PROTOTYPE_SIMD_CONFIG_H_
