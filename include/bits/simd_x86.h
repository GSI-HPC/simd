/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef INCLUDE_BITS_SIMD_X86_H_
#define INCLUDE_BITS_SIMD_X86_H_

#include "vec_ops.h"

#if not _GLIBCXX_SIMD_HAVE_SSE
#error "wrong include for this target"
#endif

#pragma GCC push_options
// ensure GCC knows about the __builtin_ia32_* calls
#pragma GCC target("avx2,bmi,bmi2,avx512vl,avx512bw,avx512dq,avx10.2")
#pragma GCC pop_options

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

namespace std::simd
{
  static constexpr size_t __x86_max_general_register_size
#ifdef __x86_64__
    = 8;
#else
    = 4;
#endif

  [[__gnu__::__always_inline__]]
  inline int
  __x86_movmsk(__vec_builtin_type_bytes<__integer_from<8>, 16> __x)
  { return __builtin_ia32_movmskpd(__vec_bit_cast<double>(__x)); }

  [[__gnu__::__always_inline__]]
  inline int
  __x86_movmsk(__vec_builtin_type_bytes<__integer_from<8>, 32> __x)
  {
    return __builtin_ia32_movmskpd256(__vec_bit_cast<double>(__x));
  }

  template <_ArchFlags _Flags = {}>
    [[__gnu__::__always_inline__]]
    inline int
    __x86_movmsk(__vec_builtin_type_bytes<__integer_from<4>, 8> __x)
    {
#if __has_builtin(__builtin_ia32_pext_di)
      if constexpr (_Flags._M_have_bmi2())
        return __builtin_ia32_pext_di(__builtin_bit_cast(unsigned long long, __x),
                                      0x80000000'80000000ULL);
#else
      return __x86_movmsk(__vec_zero_pad_to_16(__x));
#endif
    }

  [[__gnu__::__always_inline__]]
  inline int
  __x86_movmsk(__vec_builtin_type_bytes<__integer_from<4>, 16> __x)
  { return __builtin_ia32_movmskps(__vec_bit_cast<float>(__x)); }

  [[__gnu__::__always_inline__]]
  inline int
  __x86_movmsk(__vec_builtin_type_bytes<__integer_from<4>, 32> __x)
  { return __builtin_ia32_movmskps256(__vec_bit_cast<float>(__x)); }

  template <__vec_builtin _TV, auto _Flags = _ArchFlags()>
    requires (sizeof(__vec_value_type<_TV>) <= 2)
    [[__gnu__::__always_inline__]]
    inline int
    __x86_movmsk(_TV __x)
    {
      static_assert(__width_of<_TV> > 1);
      if constexpr (sizeof(__x) == 32)
        return __builtin_ia32_pmovmskb256(__vec_bit_cast<char>(__x));
      else if constexpr (sizeof(__x) == 16)
        return __builtin_ia32_pmovmskb128(__vec_bit_cast<char>(__x));
      else if constexpr (sizeof(__x) == 8)
        {
#if __has_builtin(__builtin_ia32_pext_di)
          if constexpr (_Flags._M_have_bmi2())
            return __builtin_ia32_pext_di(__builtin_bit_cast(unsigned long long, __x),
                                          0x8080'8080'8080'8080ULL);
#endif
          return __x86_movmsk(__vec_zero_pad_to_16(__x));
        }
      else if constexpr (sizeof(__x) == 4)
        {
#if __has_builtin(__builtin_ia32_pext_si)
          if constexpr (_Flags._M_have_bmi2())
            return __builtin_ia32_pext_si(__builtin_bit_cast(unsigned int, __x), 0x80808080u);
#endif
          return __x86_movmsk(__vec_zero_pad_to_16(__x));
        }
      else if constexpr (sizeof(__x) == 2)
        {
          auto __bits = __builtin_bit_cast(unsigned short, __x);
#if __has_builtin(__builtin_ia32_pext_si)
          if constexpr (_Flags._M_have_bmi2())
            return __builtin_ia32_pext_si(__bits, 0x00008080u);
#endif
          return ((__bits >> 7) & 1) | ((__bits & 0x8000) >> 14);
        }
      else
        static_assert(false);
    }

  template <__vec_builtin _TV, _ArchFlags _Flags = {}>
    [[__gnu__::__always_inline__]]
    inline bool
    __x86_vec_is_zero(_TV __a)
    {
      using _Tp = __vec_value_type<_TV>;
      static_assert(is_integral_v<_Tp>);
      if constexpr (sizeof(_TV) <= __x86_max_general_register_size)
        return __builtin_bit_cast(__integer_from<sizeof(_TV)>, __a) == 0;
      else if constexpr (_Flags._M_have_avx())
        {
          if constexpr (sizeof(_TV) == 32)
            return __builtin_ia32_ptestz256(__vec_bit_cast<long long>(__a),
                                            __vec_bit_cast<long long>(__a));
          else if constexpr (sizeof(_TV) == 16)
            return __builtin_ia32_ptestz128(__vec_bit_cast<long long>(__a),
                                            __vec_bit_cast<long long>(__a));
          else if constexpr (sizeof(_TV) < 16)
            return __x86_vec_is_zero(__vec_zero_pad_to_16(__a));
          else
            static_assert(false);
        }
      else if constexpr (_Flags._M_have_sse4_1())
        {
          if constexpr (sizeof(_TV) == 16)
            return __builtin_ia32_ptestz128(__vec_bit_cast<long long>(__a),
                                            __vec_bit_cast<long long>(__a));
          else if constexpr (sizeof(_TV) < 16)
            return __x86_vec_is_zero(__vec_zero_pad_to_16(__a));
          else
            static_assert(false);
        }
      else
        return __x86_movmsk(__a) == 0;
    }

  template <__vec_builtin _TV, _ArchFlags _Flags = {}>
    [[__gnu__::__always_inline__]]
    inline int
    __x86_vec_testz(_TV __a, _TV __b)
    {
      static_assert(sizeof(_TV) == 16 or sizeof(_TV) == 32);
      static_assert(_Flags._M_have_sse4_1());
      if constexpr (sizeof(_TV) == 32)
        return __builtin_ia32_ptestz256(__vec_bit_cast<long long>(__a),
                                        __vec_bit_cast<long long>(__b));
      else
        return __builtin_ia32_ptestz128(__vec_bit_cast<long long>(__a),
                                        __vec_bit_cast<long long>(__b));
    }

  template <__vec_builtin _TV, _ArchFlags _Flags = {}>
    [[__gnu__::__always_inline__]]
    inline int
    __x86_vec_testc(_TV __a, _TV __b)
    {
      static_assert(sizeof(_TV) == 16 or sizeof(_TV) == 32);
      static_assert(_Flags._M_have_sse4_1());
      if constexpr (sizeof(_TV) == 32)
        return __builtin_ia32_ptestc256(__vec_bit_cast<long long>(__a),
                                        __vec_bit_cast<long long>(__b));
      else
        return __builtin_ia32_ptestc128(__vec_bit_cast<long long>(__a),
                                        __vec_bit_cast<long long>(__b));
    }

  template <int _Np, __vec_builtin _TV, _ArchFlags _Flags = {}>
    [[__gnu__::__always_inline__]]
    inline bool
    __x86_vecmask_all(_TV __k)
    {
      using _Tp = __vec_value_type<_TV>;
      static_assert(is_integral_v<_Tp> and is_signed_v<_Tp>);
      constexpr int __width = __width_of<_TV>;
      static_assert(sizeof(__k) <= 32);
      if constexpr (_Np == __width)
        {
          if constexpr (sizeof(__k) <= __x86_max_general_register_size)
            {
              using _Ip = __integer_from<sizeof(__k)>;
              return __builtin_bit_cast(_Ip, __k) == ~_Ip();
            }
          else if constexpr (not _Flags._M_have_sse4_1())
            {
              constexpr int __valid_bits = (1 << (sizeof(_Tp) == 2 ? _Np * 2 : _Np)) - 1;
              return __x86_movmsk(__k) == __valid_bits;
            }
          else if constexpr (sizeof(__k) < 16)
            return __x86_vecmask_all<_Np>(__vec_zero_pad_to_16(__k));
          else
            return 0 != __x86_vec_testc(__k, ~_TV());
        }
      else if constexpr (sizeof(__k) <= __x86_max_general_register_size)
        {
          using _Ip = __integer_from<sizeof(__k)>;
          constexpr _Ip __valid_bits = (_Ip(1) << (_Np * sizeof(_Tp) * __CHAR_BIT__)) - 1;
          return (__builtin_bit_cast(_Ip, __k) & __valid_bits) == __valid_bits;
        }
      else if constexpr (not _Flags._M_have_sse4_1())
        {
          constexpr int __valid_bits = (1 << (sizeof(_Tp) == 2 ? _Np * 2 : _Np)) - 1;
          return (__x86_movmsk(__k) & __valid_bits) == __valid_bits;
        }
      else if constexpr (sizeof(__k) < 16)
        return __x86_vecmask_all<_Np>(__vec_zero_pad_to_16(__k));
      else
        return 0 != __x86_vec_testc(__k, _S_vec_implicit_mask<_Np, _TV>);
    }

  template <int _Np, __vec_builtin _TV, _ArchFlags _Flags = {}>
    [[__gnu__::__always_inline__]]
    inline bool
    __x86_vecmask_any(_TV __k)
    {
      using _Tp = __vec_value_type<_TV>;
      static_assert(is_integral_v<_Tp> and is_signed_v<_Tp>);
      constexpr int __width = __width_of<_TV>;
      static_assert(sizeof(__k) <= 32);
      if constexpr (_Np == __width)
        return not __x86_vec_is_zero(__k);
      else if constexpr (sizeof(__k) <= __x86_max_general_register_size)
        {
          using _Ip = __integer_from<sizeof(__k)>;
          constexpr _Ip __valid_bits = (_Ip(1) << (_Np * sizeof(_Tp) * __CHAR_BIT__)) - 1;
          return (__builtin_bit_cast(_Ip, __k) & __valid_bits) != _Ip();
        }
      else if constexpr (not _Flags._M_have_sse4_1())
        {
          constexpr int __valid_bits = (1 << (sizeof(_Tp) == 2 ? _Np * 2 : _Np)) - 1;
          return (__x86_movmsk(__k) & __valid_bits) != 0;
        }
      else if constexpr (sizeof(__k) < 16)
        return __x86_vecmask_any<_Np>(__vec_zero_pad_to_16(__k));
      else
        return 0 == __x86_vec_testz(__k, _S_vec_implicit_mask<_Np, _TV>);
    }

  template <int _Np, __vec_builtin _TV, _ArchFlags _Flags = {}>
    [[__gnu__::__always_inline__]]
    inline bool
    __x86_vecmask_none(_TV __k)
    {
      using _Tp = __vec_value_type<_TV>;
      static_assert(is_integral_v<_Tp> and is_signed_v<_Tp>);
      constexpr int __width = __width_of<_TV>;
      static_assert(sizeof(__k) <= 32);
      if constexpr (_Np == __width)
        return __x86_vec_is_zero(__k);
      else if constexpr (sizeof(__k) <= __x86_max_general_register_size)
        {
          using _Ip = __integer_from<sizeof(__k)>;
          constexpr _Ip __valid_bits = (_Ip(1) << (_Np * sizeof(_Tp) * __CHAR_BIT__)) - 1;
          return (__builtin_bit_cast(_Ip, __k) & __valid_bits) == _Ip();
        }
      else if constexpr (not _Flags._M_have_sse4_1())
        {
          constexpr int __valid_bits = (1 << (sizeof(_Tp) == 2 ? _Np * 2 : _Np)) - 1;
          return (__x86_movmsk(__k) & __valid_bits) == 0;
        }
      else if constexpr (sizeof(__k) < 16)
        return __x86_vecmask_none<_Np>(__vec_zero_pad_to_16(__k));
      else
        return 0 != __x86_vec_testz(__k, _S_vec_implicit_mask<_Np, _TV>);
    }

  enum class _X86Cmp
  {
    _Eq = 0,
    _Lt = 1,
    _Le = 2,
    _Unord = 3,
    _Neq = 4,
    _Nlt = 5,
    _Nle = 6,
  };

  template <_X86Cmp _Cmp, __vec_builtin _TV, _ArchFlags _Flags = {}>
    requires is_floating_point_v<__vec_value_type<_TV>>
    [[__gnu__::__always_inline__]]
    inline auto
    __x86_bitmask_cmp(_TV __x, _TV __y)
    {
      constexpr int __c = int(_Cmp);
      using _Tp = __vec_value_type<_TV>;
      if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 8)
        return __builtin_ia32_cmppd512_mask(__x, __y, __c, -1, 4);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpps512_mask(__x, __y, __c, -1, 4);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 8)
        return __builtin_ia32_cmppd256_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpps256_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 8)
        return __builtin_ia32_cmppd128_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpps128_mask(__x, __y, __c, -1);
      else if constexpr (is_same_v<_Tp, _Float16>)
        {
          if constexpr (sizeof(_TV) == 64 and _Flags._M_have_avx512fp16())
            return __builtin_ia32_cmpph512_mask(__x, __y, __c, -1);
          else if constexpr (sizeof(_TV) == 32 and _Flags._M_have_avx512fp16())
            return __builtin_ia32_cmpph256_mask(__x, __y, __c, -1);
          else if constexpr (sizeof(_TV) == 16 and _Flags._M_have_avx512fp16())
            return __builtin_ia32_cmpph128_mask(__x, __y, __c, -1);
          else if constexpr (sizeof(_TV) < 16 and _Flags._M_have_avx512fp16())
            return __x86_bitmask_cmp<_Cmp>(__vec_zero_pad_to_16(__x), __vec_zero_pad_to_16(__y));
          else
            {
              // without AVX512_FP16, float16_t size needs to match float32_t size
              // (cf. __native_abi())
              static_assert(sizeof(_TV) <= 32);
              return __x86_bitmask_cmp<_Cmp>(__vec_cast<float>(__x), __vec_cast<float>(__y));
            }
        }
      else if constexpr (sizeof(_TV) < 16)
        return __x86_bitmask_cmp<_Cmp>(__vec_zero_pad_to_16(__x), __vec_zero_pad_to_16(__y));
      else
        static_assert(false);
    }

  template <typename _Tp>
    using __x86_intrin_int
      = decltype([] {
          if constexpr (sizeof(_Tp) == 1)
            return char();
          else
            return __integer_from<sizeof(_Tp)>();
        }());

  template <_X86Cmp _Cmp, __vec_builtin _TV, _ArchFlags _Flags = {}>
    requires is_integral_v<__vec_value_type<_TV>>
    [[__gnu__::__always_inline__]]
    inline auto
    __x86_bitmask_cmp(_TV __x, _TV __y)
    {
      constexpr int __c = int(_Cmp);
      using _Tp = __vec_value_type<_TV>;
      if constexpr (sizeof(_TV) < 16)
        return __x86_bitmask_cmp<_Cmp>(__vec_zero_pad_to_16(__x), __vec_zero_pad_to_16(__y));
      else if constexpr (is_signed_v<_Tp>)
        {
          const auto __xi = __vec_bit_cast<__x86_intrin_int<_Tp>>(__x);
          const auto __yi = __vec_bit_cast<__x86_intrin_int<_Tp>>(__y);
          if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 8)
            return __builtin_ia32_cmpq512_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 4)
            return __builtin_ia32_cmpd512_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 2)
            return __builtin_ia32_cmpw512_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 1)
            return __builtin_ia32_cmpb512_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 8)
            return __builtin_ia32_cmpq256_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 4)
            return __builtin_ia32_cmpd256_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 2)
            return __builtin_ia32_cmpw256_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 1)
            return __builtin_ia32_cmpb256_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 8)
            return __builtin_ia32_cmpq128_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 4)
            return __builtin_ia32_cmpd128_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 2)
            return __builtin_ia32_cmpw128_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 1)
            return __builtin_ia32_cmpb128_mask(__xi, __yi, __c, -1);
          else
            static_assert(false);
        }
      else
        {
          const auto __xi = __vec_bit_cast<__x86_intrin_int<_Tp>>(__x);
          const auto __yi = __vec_bit_cast<__x86_intrin_int<_Tp>>(__y);
          if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 8)
            return __builtin_ia32_ucmpq512_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 4)
            return __builtin_ia32_ucmpd512_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 2)
            return __builtin_ia32_ucmpw512_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 1)
            return __builtin_ia32_ucmpb512_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 8)
            return __builtin_ia32_ucmpq256_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 4)
            return __builtin_ia32_ucmpd256_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 2)
            return __builtin_ia32_ucmpw256_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 1)
            return __builtin_ia32_ucmpb256_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 8)
            return __builtin_ia32_ucmpq128_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 4)
            return __builtin_ia32_ucmpd128_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 2)
            return __builtin_ia32_ucmpw128_mask(__xi, __yi, __c, -1);
          else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 1)
            return __builtin_ia32_ucmpb128_mask(__xi, __yi, __c, -1);
          else
            static_assert(false);
        }
    }

  template <__vec_builtin _TV, _ArchFlags _Flags = {}>
    [[__gnu__::__always_inline__]]
    inline auto
    __x86_bitmask_isinf(_TV __x)
    {
      static_assert(_Flags._M_have_avx512dq());
      using _Tp = __vec_value_type<_TV>;
      static_assert(is_floating_point_v<_Tp>);
      if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 8)
        return __builtin_ia32_fpclasspd512_mask(__x, 0x18, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 8)
        return __builtin_ia32_fpclasspd256_mask(__x, 0x18, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 8)
        return __builtin_ia32_fpclasspd128_mask(__x, 0x18, -1);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 4)
        return __builtin_ia32_fpclassps512_mask(__x, 0x18, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 4)
        return __builtin_ia32_fpclassps256_mask(__x, 0x18, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 4)
        return __builtin_ia32_fpclassps128_mask(__x, 0x18, -1);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 2 and _Flags._M_have_avx512fp16())
        return __builtin_ia32_fpclassph512_mask(__x, 0x18, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 2 and _Flags._M_have_avx512fp16())
        return __builtin_ia32_fpclassph256_mask(__x, 0x18, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 2 and _Flags._M_have_avx512fp16())
        return __builtin_ia32_fpclassph128_mask(__x, 0x18, -1);
      else if constexpr (sizeof(_Tp) == 2 and not _Flags._M_have_avx512fp16())
        return __x86_bitmask_isinf(__vec_cast<float>(__x));
      else if constexpr (sizeof(_TV) < 16)
        return __x86_bitmask_isinf(__vec_zero_pad_to_16(__x));
      else
        static_assert(false);
    }

  template <__vec_builtin _KV, _ArchFlags _Flags = {}>
    [[__gnu__::__always_inline__]]
    inline _KV
    __x86_bit_to_vecmask(std::integral auto __bits)
    {
      using _Kp = __vec_value_type<_KV>;
      static_assert((sizeof(__bits) * __CHAR_BIT__ == __width_of<_KV>)
                      or (sizeof(__bits) == 1 and __CHAR_BIT__ > __width_of<_KV>));

      if constexpr (sizeof(_Kp) == 1 and sizeof(_KV) == 64)
        return __builtin_ia32_cvtmask2b512(__bits);
      else if constexpr (sizeof(_Kp) == 1 and sizeof(_KV) == 32)
        return __builtin_ia32_cvtmask2b256(__bits);
      else if constexpr (sizeof(_Kp) == 1 and sizeof(_KV) == 16)
        return __builtin_ia32_cvtmask2b128(__bits);
      else if constexpr (sizeof(_Kp) == 1 and sizeof(_KV) <= 8)
        return _VecOps<_KV>::_S_extract(__builtin_ia32_cvtmask2b128(__bits));

      else if constexpr (sizeof(_Kp) == 2 and sizeof(_KV) == 64)
        return __builtin_ia32_cvtmask2w512(__bits);
      else if constexpr (sizeof(_Kp) == 2 and sizeof(_KV) == 32)
        return __builtin_ia32_cvtmask2w256(__bits);
      else if constexpr (sizeof(_Kp) == 2 and sizeof(_KV) == 16)
        return __builtin_ia32_cvtmask2w128(__bits);
      else if constexpr (sizeof(_Kp) == 2 and sizeof(_KV) <= 8)
        return _VecOps<_KV>::_S_extract(__builtin_ia32_cvtmask2w128(__bits));

      else if constexpr (sizeof(_Kp) == 4 and sizeof(_KV) == 64)
        return __builtin_ia32_cvtmask2d512(__bits);
      else if constexpr (sizeof(_Kp) == 4 and sizeof(_KV) == 32)
        return __builtin_ia32_cvtmask2d256(__bits);
      else if constexpr (sizeof(_Kp) == 4 and sizeof(_KV) <= 16)
        return _VecOps<_KV>::_S_extract(__builtin_ia32_cvtmask2d128(__bits));

      else if constexpr (sizeof(_Kp) == 8 and sizeof(_KV) == 64)
        return __builtin_ia32_cvtmask2q512(__bits);
      else if constexpr (sizeof(_Kp) == 8 and sizeof(_KV) == 32)
        return __builtin_ia32_cvtmask2q256(__bits);
      else if constexpr (sizeof(_Kp) == 8 and sizeof(_KV) == 16)
        return __builtin_ia32_cvtmask2q128(__bits);

      else
        static_assert(false);
    }

  template <unsigned_integral _Kp, __vec_builtin _TV, _ArchFlags _Flags = {}>
    requires is_integral_v<__vec_value_type<_TV>>
    [[__gnu__::__always_inline__]]
    constexpr inline _TV
    __x86_bitmask_blend(_Kp __k, _TV __t, _TV __f)
    {
      using _Tp = __vec_value_type<_TV>;
      if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 8)
        return __builtin_ia32_blendmq_512_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmd_512_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 2)
        return __builtin_ia32_blendmw_512_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 1)
        return __builtin_ia32_blendmb_512_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 8)
        return __builtin_ia32_blendmq_256_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmd_256_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 2)
        return __builtin_ia32_blendmw_256_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 1)
        return __builtin_ia32_blendmb_256_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 8)
        return __builtin_ia32_blendmq_128_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmd_128_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 2)
        return __builtin_ia32_blendmw_128_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 1)
        return __builtin_ia32_blendmb_128_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) < 16)
        return _VecOps<_TV>::_S_extract(__x86_bitmask_blend(__k, __vec_zero_pad_to_16(__t),
                                                            __vec_zero_pad_to_16(__f)));
      else
        static_assert(false);
    }

  template <unsigned_integral _Kp, __vec_builtin _TV, _ArchFlags _Flags = {}>
    requires is_floating_point_v<__vec_value_type<_TV>>
    [[__gnu__::__always_inline__]]
    constexpr inline _TV
    __x86_bitmask_blend(_Kp __k, _TV __t, _TV __f)
    {
      using _Tp = __vec_value_type<_TV>;
      if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 8)
        return __builtin_ia32_blendmpd_512_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmps_512_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 8)
        return __builtin_ia32_blendmpd_256_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmps_256_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 8)
        return __builtin_ia32_blendmpd_128_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmps_128_mask (__f, __t, __k);
      else if constexpr (is_same_v<_Tp, _Float16>)
        {
          using _Up = __integer_from<sizeof(_Tp)>;
          return __vec_bit_cast<_Float16>(__x86_bitmask_blend(__k, __vec_bit_cast<_Up>(__t),
                                                              __vec_bit_cast<_Up>(__f)));
        }
      else if constexpr (sizeof(_TV) < 16)
        return __x86_bitmask_blend(__k, __vec_zero_pad_to_16(__t), __vec_zero_pad_to_16(__f));
      else
        static_assert(false);
    }

  template <int _OutputBits = 4, _ArchFlags _Flags = {}>
    constexpr _UInt<1>
    __bit_extract_even(_UInt<1> __x)
    {
      static_assert(_OutputBits <= 4);
      constexpr _UInt<1> __mask = 0x55u >> ((4 - _OutputBits) * 2);
#if __has_builtin(__builtin_ia32_pext_si)
      if constexpr (_Flags._M_have_bmi2())
        return __builtin_ia32_pext_si(__x, __mask);
#endif
      __x &= __mask;
      __x |= __x >> 1;
      __x &= 0x33u;
      __x |= __x >> 2;
      __x &= 0x0Fu;
      return __x;
    }

  template <int _OutputBits = 8, _ArchFlags _Flags = {}>
    constexpr _UInt<1>
    __bit_extract_even(_UInt<2> __x)
    {
      if constexpr (_OutputBits <= 4)
        return __bit_extract_even<_OutputBits>(_UInt<1>(__x));
      else
        {
          static_assert(_OutputBits <= 8);
          constexpr _UInt<2> __mask = 0x5555u >> ((8 - _OutputBits) * 2);
#if __has_builtin(__builtin_ia32_pext_si)
          if constexpr (_Flags._M_have_bmi2())
            return __builtin_ia32_pext_si(__x, __mask);
#endif
          __x &= __mask;
          __x |= __x >> 1;
          __x &= 0x3333u;
          __x |= __x >> 2;
          __x &= 0x0F0Fu;
          __x |= __x >> 4;
          return __x;
        }
    }

  template <int _OutputBits = 16, _ArchFlags _Flags = {}>
    constexpr _UInt<_OutputBits <= 8 ? 1 : 2>
    __bit_extract_even(_UInt<4> __x)
    {
      if constexpr (_OutputBits <= 4)
        return __bit_extract_even<_OutputBits>(_UInt<1>(__x));
      else if constexpr (_OutputBits <= 8)
        return __bit_extract_even<_OutputBits>(_UInt<2>(__x));
      else
        {
          static_assert(_OutputBits <= 16);
          constexpr _UInt<4> __mask = 0x5555'5555u >> ((16 - _OutputBits) * 2);
#if __has_builtin(__builtin_ia32_pext_si)
          if constexpr (_Flags._M_have_bmi2())
            return __builtin_ia32_pext_si(__x, __mask);
#endif
          __x &= __mask;
          __x |= __x >> 1;
          __x &= 0x3333'3333u;
          __x |= __x >> 2;
          __x &= 0x0F0F'0F0Fu;
          __x |= __x >> 4;
          __x &= 0x00FF'00FFu;
          __x |= __x >> 8;
          return __x;
        }
    }

  template <int _OutputBits = 32, _ArchFlags _Flags = {}>
    constexpr _UInt<_OutputBits <= 8 ? 1 : _OutputBits <= 16 ? 2 : 4>
    __bit_extract_even(_UInt<8> __x)
    {
      if constexpr (_OutputBits <= 4)
        return __bit_extract_even<_OutputBits>(_UInt<1>(__x));
      else if constexpr (_OutputBits <= 8)
        return __bit_extract_even<_OutputBits>(_UInt<2>(__x));
      else if constexpr (_OutputBits <= 16)
        return __bit_extract_even<_OutputBits>(_UInt<4>(__x));
      else
        {
          static_assert(_OutputBits <= 32);
          constexpr _UInt<8> __mask = 0x5555'5555'5555'5555ull >> ((32 - _OutputBits) * 2);
#if __has_builtin(__builtin_ia32_pext_si)
          if constexpr (_Flags._M_have_bmi2())
            {
#if __has_builtin(__builtin_ia32_pext_di)
              return __builtin_ia32_pext_di(__x, __mask);
#else
              return __builtin_ia32_pext_si(__x, static_cast<unsigned>(__mask))
                       | (__builtin_ia32_pext_si(__x >> 32, __mask >> 32) << 16);
#endif
            }
#endif
          __x &= __mask;
          __x |= __x >> 1;
          __x &= 0x3333'3333'3333'3333ull;
          __x |= __x >> 2;
          __x &= 0x0F0F'0F0F'0F0F'0F0Full;
          __x |= __x >> 4;
          __x &= 0x00FF'00FF'00FF'00FFull;
          __x |= __x >> 8;
          __x &= 0x0000'FFFF'0000'FFFFull;
          __x |= __x >> 16;
          return __x;
        }
    }

  // input bits must be 0 for all bits > _InputBits
  template <int _InputBits = 8, _ArchFlags _Flags = {}>
    constexpr _UInt<_InputBits <= 4 ? 1 : 2>
    __duplicate_each_bit(_UInt<1> __x)
    {
      static_assert(_InputBits <= 8);
      constexpr _UInt<2> __mask = 0x5555u >> ((8 - _InputBits) * 2);
      if constexpr (_InputBits == 1)
        return __x * 3;
#if __has_builtin(__builtin_ia32_pdep_si)
      else if constexpr (_Flags._M_have_bmi2())
        return 3 * __builtin_ia32_pdep_si(__x, __mask);
#endif
      else if constexpr (_InputBits == 2) // 0000'00BA
        return ((__x + 0b0010u) & 0b0101u) * 3; // 0B?A -> 0B0A -> BBAA
      else if constexpr (_InputBits <= 4) // 0000'DCBA
        {
          __x = ((__x << 2) | __x ) & 0b0011'0011u; // 00DC'??BA -> 00DC'00BA
          return ((__x + 0b0010'0010u) & __mask) * 3;     // -> DDCC'BBAA
        }
      else
        { // HGFE'DCBA
          _UInt<2> __y = ((__x << 4) | __x) & 0x0F0Fu; // HGFE'0000'DCBA
          __y |= __y << 2; // 00HG'??FE'00DC'??BA
          __y &= 0x3333u;  // 00HG'00FE'00DC'00BA
          __y += 0x2222u;  // 0H?G'0F?E'0D?C'0B?A
          return (__y & __mask) * 3; // HHGG'FFEE'DDCC'BBAA
        }
    }

  template <int _InputBits = 16, _ArchFlags _Flags = {}>
    constexpr _UInt<4>
    __duplicate_each_bit(_UInt<2> __x)
    {
      if constexpr (_InputBits <= 8)
        return __duplicate_each_bit<_InputBits>(_UInt<1>(__x));
      else
        {
          static_assert(_InputBits <= 16);
          constexpr _UInt<4> __mask = 0x5555'5555u >> ((16 - _InputBits) * 2);
#if __has_builtin(__builtin_ia32_pdep_si)
          if constexpr (_Flags._M_have_bmi2())
            return 3 * __builtin_ia32_pdep_si(__x, __mask);
#endif
          _UInt<4> __y = ((__x << 8) | __x) & 0x00FF00FFu;
          __y |= __y << 4;
          __y &= 0x0F0F'0F0Fu;
          __y |= __y << 2;
          __y &= 0x3333'3333u;
          return ((__y + 0x2222'2222u) & __mask) * 3;
        }
    }

  template <int _InputBits = 32, _ArchFlags _Flags = {}>
    constexpr _UInt<8>
    __duplicate_each_bit(_UInt<4> __x)
    {
      if constexpr (_InputBits <= 8)
        return __duplicate_each_bit<_InputBits>(_UInt<1>(__x));
      else if constexpr (_InputBits <= 16)
        return __duplicate_each_bit<_InputBits>(_UInt<2>(__x));
      else
        {
          static_assert(_InputBits <= 32);
          constexpr _UInt<8> __mask = 0x5555'5555'5555'5555u >> ((32 - _InputBits) * 2);
#if __has_builtin(__builtin_ia32_pdep_si)
          if constexpr (_Flags._M_have_bmi2())
            {
#if __has_builtin(__builtin_ia32_pdep_di)
              return 3 * __builtin_ia32_pdep_di(__x, __mask);
#else
              const _UInt<8> __hi = 3 * __builtin_ia32_pdep_si(__x >> 16, __mask >> 32);
              return (3 * __builtin_ia32_pdep_si(__x, static_cast<unsigned>(__mask))) | __hi << 32;
#endif
            }
#endif
          _UInt<8> __y = ((__x & 0xFFFF'0000ull) << 16) | (__x & 0x0000'FFFFu);
          __y |= __y << 8;
          __y &= 0x00FF'00FF'00FF'00FFull;
          __y |= __y << 4;
          __y &= 0x0F0F'0F0F'0F0F'0F0Full;
          __y |= __y << 2;
          __y &= 0x3333'3333'3333'3333ull;
          return ((__y + 0x2222'2222'2222'2222ull) & __mask) * 3;
        }
    }
}

#pragma GCC diagnostic pop

#endif  // INCLUDE_BITS_SIMD_X86_H_
