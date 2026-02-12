/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef FN
#error "must define FN"
#endif

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

namespace
{
using flt16_2 = __vec_builtin_type<_Float16, 2>;
using flt16_4 = __vec_builtin_type<_Float16, 4>;
using flt16_8 = __vec_builtin_type<_Float16, 8>;
using flt16_16 = __vec_builtin_type<_Float16, 16>;
using flt16_32 = __vec_builtin_type<_Float16, 32>;

using flt32_2 = __vec_builtin_type<float, 2>;
using flt32_4 = __vec_builtin_type<float, 4>;
using flt32_8 = __vec_builtin_type<float, 8>;
using flt32_16 = __vec_builtin_type<float, 16>;

using flt64_2 = __vec_builtin_type<double, 2>;
using flt64_4 = __vec_builtin_type<double, 4>;
using flt64_8 = __vec_builtin_type<double, 8>;
}

#ifdef __AVX512FP16__
template flt16_2 CONCAT(__fast_, FN)(flt16_2, flt16_2, flt16_2);
template flt16_4 CONCAT(__fast_, FN)(flt16_4, flt16_4, flt16_4);
template flt16_8 CONCAT(__fast_, FN)(flt16_8, flt16_8, flt16_8);
template flt16_16 CONCAT(__fast_, FN)(flt16_16, flt16_16, flt16_16);
template flt16_32 CONCAT(__fast_, FN)(flt16_32, flt16_32, flt16_32);

template flt16_2 CONCAT(__, FN)(flt16_2, flt16_2, flt16_2);
template flt16_4 CONCAT(__, FN)(flt16_4, flt16_4, flt16_4);
template flt16_8 CONCAT(__, FN)(flt16_8, flt16_8, flt16_8);
template flt16_16 CONCAT(__, FN)(flt16_16, flt16_16, flt16_16);
template flt16_32 CONCAT(__, FN)(flt16_32, flt16_32, flt16_32);

template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_2)
CONCAT(__fast_2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_2, flt16_2, flt16_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_4)
CONCAT(__fast_2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_4, flt16_4, flt16_4);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_8)
CONCAT(__fast_2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_8, flt16_8, flt16_8);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_16)
CONCAT(__fast_2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_16, flt16_16, flt16_16);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_32)
CONCAT(__fast_2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_32, flt16_32, flt16_32);

template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_2)
CONCAT(__2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_2, flt16_2, flt16_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_4)
CONCAT(__2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_4, flt16_4, flt16_4);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_8)
CONCAT(__2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_8, flt16_8, flt16_8);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_16)
CONCAT(__2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_16, flt16_16, flt16_16);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt16_32, flt16_32)
CONCAT(__2x_, FN)(flt16_32, flt16_32, flt16_32, flt16_32, flt16_32, flt16_32);
#endif

template flt32_2 CONCAT(__fast_, FN)(flt32_2, flt32_2, flt32_2);
template flt32_4 CONCAT(__fast_, FN)(flt32_4, flt32_4, flt32_4);
template flt64_2 CONCAT(__fast_, FN)(flt64_2, flt64_2, flt64_2);

template flt32_2 CONCAT(__, FN)(flt32_2, flt32_2, flt32_2);
template flt32_4 CONCAT(__, FN)(flt32_4, flt32_4, flt32_4);
template flt64_2 CONCAT(__, FN)(flt64_2, flt64_2, flt64_2);

#ifdef __AVX__
template flt32_8 CONCAT(__fast_, FN)(flt32_8, flt32_8, flt32_8);
template flt64_4 CONCAT(__fast_, FN)(flt64_4, flt64_4, flt64_4);

template flt32_8 CONCAT(__, FN)(flt32_8, flt32_8, flt32_8);
template flt64_4 CONCAT(__, FN)(flt64_4, flt64_4, flt64_4);
#endif

#ifdef __AVX512F__
template flt32_16 CONCAT(__fast_, FN)(flt32_16, flt32_16, flt32_16);
template flt64_8  CONCAT(__fast_, FN)(flt64_8, flt64_8, flt64_8);

template flt32_16 CONCAT(__, FN)(flt32_16, flt32_16, flt32_16);
template flt64_8  CONCAT(__, FN)(flt64_8, flt64_8, flt64_8);
#endif

#ifdef __AVX512F__
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_16, flt32_2)
CONCAT(__fast_2x_, FN)(flt32_16, flt32_16, flt32_16, flt32_2, flt32_2, flt32_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_16, flt32_4)
CONCAT(__fast_2x_, FN)(flt32_16, flt32_16, flt32_16, flt32_4, flt32_4, flt32_4);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_16, flt32_8)
CONCAT(__fast_2x_, FN)(flt32_16, flt32_16, flt32_16, flt32_8, flt32_8, flt32_8);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_16, flt32_16)
CONCAT(__fast_2x_, FN)(flt32_16, flt32_16, flt32_16, flt32_16, flt32_16, flt32_16);

template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_8, flt64_2)
CONCAT(__fast_2x_, FN)(flt64_8, flt64_8, flt64_8, flt64_2, flt64_2, flt64_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_8, flt64_4)
CONCAT(__fast_2x_, FN)(flt64_8, flt64_8, flt64_8, flt64_4, flt64_4, flt64_4);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_8, flt64_8)
CONCAT(__fast_2x_, FN)(flt64_8, flt64_8, flt64_8, flt64_8, flt64_8, flt64_8);

template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_16, flt32_2)
CONCAT(__2x_, FN)(flt32_16, flt32_16, flt32_16, flt32_2, flt32_2, flt32_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_16, flt32_4)
CONCAT(__2x_, FN)(flt32_16, flt32_16, flt32_16, flt32_4, flt32_4, flt32_4);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_16, flt32_8)
CONCAT(__2x_, FN)(flt32_16, flt32_16, flt32_16, flt32_8, flt32_8, flt32_8);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_16, flt32_16)
CONCAT(__2x_, FN)(flt32_16, flt32_16, flt32_16, flt32_16, flt32_16, flt32_16);

template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_8, flt64_2)
CONCAT(__2x_, FN)(flt64_8, flt64_8, flt64_8, flt64_2, flt64_2, flt64_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_8, flt64_4)
CONCAT(__2x_, FN)(flt64_8, flt64_8, flt64_8, flt64_4, flt64_4, flt64_4);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_8, flt64_8)
CONCAT(__2x_, FN)(flt64_8, flt64_8, flt64_8, flt64_8, flt64_8, flt64_8);
#elif defined __AVX__
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_8, flt32_2)
CONCAT(__fast_2x_, FN)(flt32_8, flt32_8, flt32_8, flt32_2, flt32_2, flt32_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_8, flt32_4)
CONCAT(__fast_2x_, FN)(flt32_8, flt32_8, flt32_8, flt32_4, flt32_4, flt32_4);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_8, flt32_8)
CONCAT(__fast_2x_, FN)(flt32_8, flt32_8, flt32_8, flt32_8, flt32_8, flt32_8);

template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_4, flt64_2)
CONCAT(__fast_2x_, FN)(flt64_4, flt64_4, flt64_4, flt64_2, flt64_2, flt64_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_4, flt64_4)
CONCAT(__fast_2x_, FN)(flt64_4, flt64_4, flt64_4, flt64_4, flt64_4, flt64_4);

template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_8, flt32_2)
CONCAT(__2x_, FN)(flt32_8, flt32_8, flt32_8, flt32_2, flt32_2, flt32_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_8, flt32_4)
CONCAT(__2x_, FN)(flt32_8, flt32_8, flt32_8, flt32_4, flt32_4, flt32_4);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_8, flt32_8)
CONCAT(__2x_, FN)(flt32_8, flt32_8, flt32_8, flt32_8, flt32_8, flt32_8);

template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_4, flt64_2)
CONCAT(__2x_, FN)(flt64_4, flt64_4, flt64_4, flt64_2, flt64_2, flt64_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt64_4, flt64_4)
CONCAT(__2x_, FN)(flt64_4, flt64_4, flt64_4, flt64_4, flt64_4, flt64_4);
#else
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_4, flt32_2)
CONCAT(__fast_2x_, FN)(flt32_4, flt32_4, flt32_4, flt32_2, flt32_2, flt32_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_4, flt32_4)
CONCAT(__fast_2x_, FN)(flt32_4, flt32_4, flt32_4, flt32_4, flt32_4, flt32_4);

template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_4, flt32_2)
CONCAT(__2x_, FN)(flt32_4, flt32_4, flt32_4, flt32_2, flt32_2, flt32_2);
template _GLIBCXX_SIMD_MATH_RET_TYPE(flt32_4, flt32_4)
CONCAT(__2x_, FN)(flt32_4, flt32_4, flt32_4, flt32_4, flt32_4, flt32_4);
#endif
