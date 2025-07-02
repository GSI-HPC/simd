/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef INCLUDE_BITS_SIMD_X86_H_
#define INCLUDE_BITS_SIMD_X86_H_

#include "vec_ops.h"

#ifndef __SSE__
#error "wrong include for this target"
#endif

#pragma GCC push_options
// ensure GCC knows about the __builtin_ia32_* calls
#pragma GCC target("avx2,bmi,bmi2,avx512vl,avx512bw,avx512dq,avx10.2")
#pragma GCC pop_options

namespace std::simd
{
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
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 2)
        return __builtin_ia32_cmpph512_mask(__x, __y, __c, -1, 4);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 8)
        return __builtin_ia32_cmppd256_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpps256_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 2)
        return __builtin_ia32_cmpph256_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 8)
        return __builtin_ia32_cmppd128_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpps128_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 2)
        return __builtin_ia32_cmpph128_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) <= 8 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpps128_mask(__vec_zero_pad_to_16(__x),
                                            __vec_zero_pad_to_16(__y), __c, -1);
      else if constexpr (sizeof(_TV) <= 8 and sizeof(_Tp) == 2)
        return __builtin_ia32_cmpph128_mask(__vec_zero_pad_to_16(__x),
                                            __vec_zero_pad_to_16(__y), __c, -1);
      else
        static_assert(false);
    }

  template <_X86Cmp _Cmp, __vec_builtin _TV, _ArchFlags _Flags = {}>
    requires is_integral_v<__vec_value_type<_TV>>
    [[__gnu__::__always_inline__]]
    inline auto
    __x86_bitmask_cmp(_TV __x, _TV __y)
    {
      constexpr int __c = int(_Cmp);
      using _Tp = __vec_value_type<_TV>;
      if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 8)
        return __builtin_ia32_cmpq512_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpd512_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 2)
        return __builtin_ia32_cmpw512_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 1)
        return __builtin_ia32_cmpb512_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 8)
        return __builtin_ia32_cmpq256_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpd256_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 2)
        return __builtin_ia32_cmpw256_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 1)
        return __builtin_ia32_cmpb256_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 8)
        return __builtin_ia32_cmpq128_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpd128_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 2)
        return __builtin_ia32_cmpw128_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 1)
        return __builtin_ia32_cmpb128_mask(__x, __y, __c, -1);
      else if constexpr (sizeof(_TV) <= 8 and sizeof(_Tp) == 4)
        return __builtin_ia32_cmpd128_mask(__vec_zero_pad_to_16(__x),
                                           __vec_zero_pad_to_16(__y), __c, -1);
      else if constexpr (sizeof(_TV) <= 8 and sizeof(_Tp) == 2)
        return __builtin_ia32_cmpw128_mask(__vec_zero_pad_to_16(__x),
                                           __vec_zero_pad_to_16(__y), __c, -1);
      else if constexpr (sizeof(_TV) <= 8 and sizeof(_Tp) == 1)
        return __builtin_ia32_cmpb128_mask(__vec_zero_pad_to_16(__x),
                                           __vec_zero_pad_to_16(__y), __c, -1);
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
      else if constexpr (sizeof(_TV) == 64 and sizeof(_Tp) == 2)
        return __builtin_ia32_blendmph_512_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 8)
        return __builtin_ia32_blendmpd_256_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmps_256_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 32 and sizeof(_Tp) == 2)
        return __builtin_ia32_blendmph_256_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 8)
        return __builtin_ia32_blendmpd_128_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmps_128_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 16 and sizeof(_Tp) == 2)
        return __builtin_ia32_blendmph_128_mask (__f, __t, __k);
      else if constexpr (sizeof(_TV) == 8 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmps_128_mask (__vec_zero_pad_to_16(__f),
                                                 __vec_zero_pad_to_16(__t), __k);
      else if constexpr (sizeof(_TV) <= 8 and sizeof(_Tp) == 2)
        return __builtin_ia32_blendmph_128_mask (__vec_zero_pad_to_16(__f),
                                                 __vec_zero_pad_to_16(__t), __k);
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
      else if constexpr (sizeof(_TV) == 8 and sizeof(_Tp) == 4)
        return __builtin_ia32_blendmd_128_mask (__vec_zero_pad_to_16(__f),
                                                __vec_zero_pad_to_16(__t), __k);
      else if constexpr (sizeof(_TV) <= 8 and sizeof(_Tp) == 2)
        return __builtin_ia32_blendmw_128_mask (__vec_zero_pad_to_16(__f),
                                                __vec_zero_pad_to_16(__t), __k);
      else if constexpr (sizeof(_TV) <= 8 and sizeof(_Tp) == 1)
        return __builtin_ia32_blendmb_128_mask (__vec_zero_pad_to_16(__f),
                                                __vec_zero_pad_to_16(__t), __k);
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

#endif  // INCLUDE_BITS_SIMD_X86_H_
