/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_X86_DETAIL_H_
#define PROTOTYPE_X86_DETAIL_H_

#include "simd_meta.h"
#include "vec_detail.h"

#include <cstdint>

#if _GLIBCXX_SIMD_HAVE_SSE

#pragma GCC push_options
// ensure GCC knows about the __builtin_ia32_* calls
#pragma GCC target("sse2", "sse3", "ssse3", "sse4.1", "sse4.2", "avx", "avx2", "bmi", "bmi2")
#pragma GCC pop_options

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"
namespace std::__detail
{
  struct _ArchFlags
  {
    uint64_t _M_flags = (_GLIBCXX_SIMD_HAVE_MMX << 0)
                          | (_GLIBCXX_SIMD_HAVE_SSE << 1)
                          | (_GLIBCXX_SIMD_HAVE_SSE2 << 2)
                          | (_GLIBCXX_SIMD_HAVE_SSE3 << 3)
                          | (_GLIBCXX_SIMD_HAVE_SSSE3 << 4)
                          | (_GLIBCXX_SIMD_HAVE_SSE4_1 << 5)
                          | (_GLIBCXX_SIMD_HAVE_SSE4_2 << 6)
                          | (_GLIBCXX_SIMD_HAVE_XOP << 7)
                          | (_GLIBCXX_SIMD_HAVE_AVX << 8)
                          | (_GLIBCXX_SIMD_HAVE_AVX2 << 9)
                          | (_GLIBCXX_SIMD_HAVE_BMI << 10)
                          | (_GLIBCXX_SIMD_HAVE_BMI2 << 11)
                          | (_GLIBCXX_SIMD_HAVE_LZCNT << 12)
                          | (_GLIBCXX_SIMD_HAVE_SSE4A << 13)
                          | (_GLIBCXX_SIMD_HAVE_FMA << 14)
                          | (_GLIBCXX_SIMD_HAVE_FMA4 << 15)
                          | (_GLIBCXX_SIMD_HAVE_F16C << 16)
                          | (_GLIBCXX_SIMD_HAVE_POPCNT << 17)
                          | (_GLIBCXX_SIMD_HAVE_AVX512F << 18)
                          | (_GLIBCXX_SIMD_HAVE_AVX512DQ << 19)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VL << 20)
                          | (_GLIBCXX_SIMD_HAVE_AVX512BW << 21)
                          | (_GLIBCXX_SIMD_HAVE_AVX512BITALG << 22)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VBMI << 23)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VBMI2 << 24)
                          | (_GLIBCXX_SIMD_HAVE_AVX512IFMA << 25)
                          | (_GLIBCXX_SIMD_HAVE_AVX512CD << 26)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VNNI << 27)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VPOPCNTDQ << 28)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VP2INTERSECT << 29)
                          | (_GLIBCXX_SIMD_HAVE_AVX512FP16 << 30)
                          | (_GLIBCXX_SIMD_HAVE_AVXIFMA << 31)
                          | (_GLIBCXX_SIMD_HAVE_AVXNECONVERT << 32)
                          | (_GLIBCXX_SIMD_HAVE_AVXVNNI << 33)
                          | (_GLIBCXX_SIMD_HAVE_AVXVNNIINT8 << 34)
                          | (_GLIBCXX_SIMD_HAVE_AVXVNNIINT16 << 35);

    constexpr bool
    _M_test(int __bit) const
    { return ((_M_flags >> __bit) & 1) == 1; }

    constexpr bool
    _M_have_mmx() const
    { return _M_test(0); }

    constexpr bool
    _M_have_sse() const
    { return _M_test(1); }

    constexpr bool
    _M_have_sse2() const
    { return _M_test(2); }

    constexpr bool
    _M_have_sse3() const
    { return _M_test(3); }

    constexpr bool
    _M_have_ssse3() const
    { return _M_test(4); }

    constexpr bool
    _M_have_sse4_1() const
    { return _M_test(5); }

    constexpr bool
    _M_have_sse4_2() const
    { return _M_test(6); }

    constexpr bool
    _M_have_xop() const
    { return _M_test(7); }

    constexpr bool
    _M_have_avx() const
    { return _M_test(8); }

    constexpr bool
    _M_have_avx2() const
    { return _M_test(9); }

    constexpr bool
    _M_have_bmi() const
    { return _M_test(10); }

    constexpr bool
    _M_have_bmi2() const
    { return _M_test(11); }

    constexpr bool
    _M_have_lzcnt() const
    { return _M_test(12); }

    constexpr bool
    _M_have_sse4a() const
    { return _M_test(13); }

    constexpr bool
    _M_have_fma() const
    { return _M_test(14); }

    constexpr bool
    _M_have_fma4() const
    { return _M_test(15); }

    constexpr bool
    _M_have_f16c() const
    { return _M_test(16); }

    constexpr bool
    _M_have_popcnt() const
    { return _M_test(17); }

    constexpr bool
    _M_have_avx512f() const
    { return _M_test(18); }

    constexpr bool
    _M_have_avx512dq() const
    { return _M_test(19); }

    constexpr bool
    _M_have_avx512vl() const
    { return _M_test(20); }

    constexpr bool
    _M_have_avx512bw() const
    { return _M_test(21); }

    constexpr bool
    _M_have_avx512bitalg() const
    { return _M_test(22); }

    constexpr bool
    _M_have_avx512vbmi() const
    { return _M_test(23); }

    constexpr bool
    _M_have_avx512vbmi2() const
    { return _M_test(24); }

    constexpr bool
    _M_have_avx512ifma() const
    { return _M_test(25); }

    constexpr bool
    _M_have_avx512cd() const
    { return _M_test(26); }

    constexpr bool
    _M_have_avx512vnni() const
    { return _M_test(27); }

    constexpr bool
    _M_have_avx512vpopcntdq() const
    { return _M_test(28); }

    constexpr bool
    _M_have_avx512vp2intersect() const
    { return _M_test(29); }

    constexpr bool
    _M_have_avx512fp16() const
    { return _M_test(30); }
  };

  template <__vectorizable _Tp>
    struct __x86_builtin_int;

  template <__vectorizable _Tp>
    using __x86_builtin_int_t = typename __x86_builtin_int<_Tp>::type;

  template <__vectorizable _Tp>
    requires(sizeof(_Tp) == 1)
    struct __x86_builtin_int<_Tp>
    { using type = char; };

  template <__vectorizable _Tp>
    requires(sizeof(_Tp) == 2)
    struct __x86_builtin_int<_Tp>
    { using type = short; };

  template <__vectorizable _Tp>
    requires(sizeof(_Tp) == 4)
    struct __x86_builtin_int<_Tp>
    { using type = int; };

  template <__vectorizable _Tp>
    requires(sizeof(_Tp) == 8)
    struct __x86_builtin_int<_Tp>
    { using type = long long; };

  template <__vectorizable _Tp>
    struct __x86_builtin_fp;

  template <__vectorizable _Tp>
    using __x86_builtin_fp_t = typename __x86_builtin_fp<_Tp>::type;

#ifdef __STDCPP_FLOAT16_T__
  template <__vectorizable _Tp>
    requires(sizeof(_Tp) == 2)
    struct __x86_builtin_fp<_Tp>
    { using type = std::float16_t; };
#endif

  template <__vectorizable _Tp>
    requires(sizeof(_Tp) == 4)
    struct __x86_builtin_fp<_Tp>
    { using type = float; };

  template <__vectorizable _Tp>
    requires(sizeof(_Tp) == 8)
    struct __x86_builtin_fp<_Tp>
    { using type = double; };

  template <typename _TV>
    using __x86_intrin_t = __vec_builtin_type_bytes<
                             typename conditional_t<is_floating_point_v<__value_type_of<_TV>>,
                                                    __x86_builtin_fp<__value_type_of<_TV>>,
                                                    type_identity<long long>>::type,
                             sizeof(_TV) <= 16 ? 16z : sizeof(_TV)>;

  /**
   * Return __x with suitable type for Intel intrinsics. If __x is smaller than a full XMM register,
   * then a zero-padded 16-Byte object will be returned.
   */
  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr __x86_intrin_t<_TV>
    __to_x86_intrin(_TV __x)
    {
      static_assert(sizeof(_TV) <= 64);
      using _RV = __x86_intrin_t<_TV>;
      if constexpr (sizeof(_TV) < 16)
        {
          using _Up = __make_signed_int_t<_TV>;
          __vec_builtin_type_bytes<_Up, 16> __tmp = {__builtin_bit_cast(_Up, __x)};
          return reinterpret_cast<_RV>(__tmp);
        }
      else if constexpr (is_same_v<_TV, _RV>)
        return __x;
      else
        return reinterpret_cast<_RV>(__x);
    }

  _GLIBCXX_SIMD_INTRINSIC int
  __movmsk(__vec_builtin_sizeof<8, 16> auto __x) noexcept
  { return __builtin_ia32_movmskpd(reinterpret_cast<__v2double>(__x)); }

  _GLIBCXX_SIMD_INTRINSIC int
  __movmsk(__vec_builtin_sizeof<8, 32> auto __x) noexcept
  { return __builtin_ia32_movmskpd256(reinterpret_cast<__v4double>(__x)); }

  _GLIBCXX_SIMD_INTRINSIC int
  __movmsk(__vec_builtin_sizeof<4, 8> auto __x) noexcept
  {
#if defined  __x86_64__ and defined __BMI2__
    return __builtin_ia32_pext_di(__builtin_bit_cast(unsigned long long, __x),
                                  0x80000000'80000000ULL);
#else
    using _Float2 [[gnu::vector_size(8)]] = float;
    const _Float2 __tmp = __builtin_bit_cast(_Float2, __x);
    return __builtin_ia32_movmskps(__builtin_shufflevector(__tmp, _Float2(), 0, 1, 2, 3));
#endif
  }

  _GLIBCXX_SIMD_INTRINSIC int
  __movmsk(__vec_builtin_sizeof<4, 16> auto __x) noexcept
  { return __builtin_ia32_movmskps(reinterpret_cast<__v4float>(__x)); }

  _GLIBCXX_SIMD_INTRINSIC int
  __movmsk(__vec_builtin_sizeof<4, 32> auto __x) noexcept
  { return __builtin_ia32_movmskps256(reinterpret_cast<__v8float>(__x)); }

  template <__vec_builtin _TV, auto _Flags = _ArchFlags()>
    requires (sizeof(__value_type_of<_TV>) <= 2)
    _GLIBCXX_SIMD_ALWAYS_INLINE inline int
    __movmsk(_TV __x) noexcept
    {
      static_assert(__width_of<decltype(__x)> > 1);
      if constexpr (sizeof(__x) == 32)
        return __builtin_ia32_pmovmskb256(reinterpret_cast<__v32char>(__x));
      else if constexpr (sizeof(__x) == 16)
        return __builtin_ia32_pmovmskb128(reinterpret_cast<__v16char>(__x));
      else if constexpr (sizeof(__x) == 8)
        {
          using _Int2 [[gnu::vector_size(8)]] = int;
          const _Int2 __tmp = __builtin_bit_cast(_Int2, __x);
          return __builtin_ia32_pmovmskb128(
                   reinterpret_cast<__v16char>(
                     __builtin_shufflevector(__tmp, _Int2(), 0, 1, 2, 3)));
        }
      else if constexpr (sizeof(__x) == 4)
        {
          if constexpr (_Flags._M_have_bmi2())
            return __builtin_ia32_pext_si(__builtin_bit_cast(unsigned int, __x), 0x80808080u);
          using _Int1 [[gnu::vector_size(4)]] = int;
          const _Int1 __tmp = __builtin_bit_cast(_Int1, __x);
          return __builtin_ia32_pmovmskb128(
                   reinterpret_cast<__v16char>(
                     __builtin_shufflevector(__tmp, _Int1(), 0, 1, 1, 1)));
        }
      else if constexpr (sizeof(__x) == 2)
        {
          auto __bits = __builtin_bit_cast(unsigned short, __x);
          if constexpr (_Flags._M_have_bmi2())
            return __builtin_ia32_pext_si(__bits, 0x00008080u);
          else
            return ((__bits >> 7) & 1) | ((__bits & 0x8000) >> 14);
        }
      else
        static_assert(false);
    }

  // calling the andnot builtins inhibits some optimizations, whereas GCC seems to be perfectly able
  // to choose andn instructions by itself without any help
#if 0 // not defined __clang__
  // overload __vec_andnot from detail.h
  template <__vec_builtin _TV>
    requires (sizeof(_TV) >= 16)
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    __vec_andnot(_TV __a, _TV __b)
    {
      using _Tp = __value_type_of<_TV>;
      using _UV = __vec_builtin_type<__make_unsigned_int_t<_Tp>, __width_of<_TV>>;
      if (__builtin_is_constant_evaluated()
            or (__builtin_constant_p(__a) and __builtin_constant_p(__b)))
        return reinterpret_cast<_TV>(~reinterpret_cast<_UV>(__a) & reinterpret_cast<_UV>(__b));
      else
        return reinterpret_cast<_TV>([&] [[__gnu__::__always_inline__]] {
                 if constexpr (sizeof(_TV) == 16 and is_same_v<_Tp, float>)
                   return __builtin_ia32_andnps(__a, __b);
                 else if constexpr (sizeof(_TV) == 16 and is_same_v<_Tp, double>)
                   return __builtin_ia32_andnpd(__a, __b);
                 else if constexpr (sizeof(_TV) == 16 and is_integral_v<_Tp>)
                   return __builtin_ia32_pandn128(reinterpret_cast<__v2llong>(__a),
                                                  reinterpret_cast<__v2llong>(__b));
                 else if constexpr (sizeof(_TV) == 32 and is_same_v<_Tp, float>)
                   return __builtin_ia32_andnps256(__a, __b);
                 else if constexpr (sizeof(_TV) == 32 and is_same_v<_Tp, double>)
                   return __builtin_ia32_andnpd256(__a, __b);
                 else if constexpr (sizeof(_TV) == 32 and is_integral_v<_Tp> and __have_avx2)
                   return __builtin_ia32_andnotsi256(reinterpret_cast<__v4llong>(__a),
                                                     reinterpret_cast<__v4llong>(__b));
                 else if constexpr (sizeof(_TV) == 32 and is_integral_v<_Tp>)
                   return __builtin_ia32_andnpd256(reinterpret_cast<__v4double>(__a),
                                                   reinterpret_cast<__v4double>(__b));
                 else if constexpr (sizeof(_TV) == 64 and is_same_v<_Tp, float> and __have_avx512dq)
                   return __builtin_ia32_andnps512_mask(__a, __b, __v16float{}, -1);
                 else if constexpr (sizeof(_TV) == 64 and is_same_v<_Tp, double> and __have_avx512dq)
                   return __builtin_ia32_andnpd512_mask(__a, __b, __v8double{}, -1);
                 else if constexpr (sizeof(_TV) == 64)
                   return __builtin_ia32_pandnd512_mask(
                            reinterpret_cast<__v16int>(__a), reinterpret_cast<__v16int>(__b),
                            __v16int{}, -1);
               }());
    }
#endif // not __clang__

  _GLIBCXX_SIMD_INTRINSIC __v2double
  __hadd_insn(__v2double __x, __v2double __y)
  { return __builtin_ia32_haddpd(__x, __y); }

  _GLIBCXX_SIMD_INTRINSIC __v4double
  __hadd_insn(__v4double __x, __v4double __y)
  { return __builtin_ia32_haddpd256(__x, __y); }

  _GLIBCXX_SIMD_INTRINSIC __v4float
  __hadd_insn(__v4float __x, __v4float __y)
  { return __builtin_ia32_haddps(__x, __y); }

  _GLIBCXX_SIMD_INTRINSIC __v8float
  __hadd_insn(__v8float __x, __v8float __y)
  {
    __v4double __sum = reinterpret_cast<__v4double>(__builtin_ia32_haddps256(__x, __y));
    return reinterpret_cast<__v8float>(__builtin_shufflevector(__sum, __sum, 0, 2, 1, 3));
  }
}
#pragma GCC diagnostic pop

#endif  // _GLIBCXX_SIMD_HAVE_SSE
#endif  // PROTOTYPE_X86_DETAIL_H_
