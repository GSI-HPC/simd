/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef INCLUDE_BITS_VEC_OPS_H_
#define INCLUDE_BITS_VEC_OPS_H_

#include "simd_details.h"

#include <bits/utility.h>

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

namespace std::simd
{
  template <std::signed_integral _Tp>
    constexpr bool
    __signed_has_single_bit(_Tp __x)
    { return __has_single_bit(make_unsigned_t<_Tp>(__x)); }

  /**
   * Alias for a vector builtin with given value type and total sizeof.
   */
  template <__vectorizable _Tp, size_t _Bytes>
    requires (__has_single_bit(_Bytes))
    using __vec_builtin_type_bytes [[__gnu__::__vector_size__(_Bytes)]] = _Tp;

  /**
   * Alias for a vector builtin with given value type \p _Tp and \p _Width.
   */
  template <__vectorizable _Tp, __simd_size_type _Width>
    requires (__signed_has_single_bit(_Width))
    using __vec_builtin_type = __vec_builtin_type_bytes<_Tp, sizeof(_Tp) * _Width>;

  /**
   * Constrain to any vector builtin with given value type and optional width.
   */
  template <typename _Tp, typename _ValueType,
            __simd_size_type _Width = sizeof(_Tp) / sizeof(_ValueType)>
    concept __vec_builtin_of
      = not is_arithmetic_v<_Tp> and __vectorizable<_ValueType>
          and _Width >= 1 and sizeof(_Tp) / sizeof(_ValueType) == _Width
          and same_as<__vec_builtin_type_bytes<_ValueType, sizeof(_Tp)>, _Tp>
          and requires(_Tp& __v, _ValueType __x) { __v[0] = __x; };

  /**
   * Constrain to any vector builtin.
   */
  template <typename _Tp>
    concept __vec_builtin
      = not is_class_v<_Tp> and requires(const _Tp& __x) {
        requires __vec_builtin_of<_Tp, remove_cvref_t<decltype(__x[0])>>;
      };

  /**
   * Alias for the value type of the given __vec_builtin type \p _Tp.
   */
  template <__vec_builtin _Tp>
    using __vec_value_type = remove_cvref_t<decltype(declval<const _Tp>()[0])>;

  /**
   * The width (number of value_type elements) of the given vector builtin or arithmetic type.
   */
  template <typename _Tp>
    inline constexpr __simd_size_type __width_of = 1;

  template <typename _Tp>
    requires __vec_builtin<_Tp>
    inline constexpr __simd_size_type __width_of<_Tp> = sizeof(_Tp) / sizeof(__vec_value_type<_Tp>);

  /**
   * Alias for a vector builtin with equal value type and new width \p _Np.
   */
  template <__simd_size_type _Np, __vec_builtin _TV>
    using __resize_vec_builtin_t = __vec_builtin_type<__vec_value_type<_TV>, _Np>;

  template <__vec_builtin _TV>
    requires (__width_of<_TV> > 1)
    using __half_vec_builtin_t = __resize_vec_builtin_t<__width_of<_TV> / 2, _TV>;

  template <__vec_builtin _TV>
    using __double_vec_builtin_t = __resize_vec_builtin_t<__width_of<_TV> * 2, _TV>;

  /**
   * Helper function to work around Clang not allowing v[i] in constant expressions.
   */
  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr __vec_value_type<_TV>
    __vec_get(_TV __v, int __i)
    {
#ifdef _GLIBCXX_CLANG
      if (__builtin_is_constant_evaluated())
        return __builtin_bit_cast(array<__vec_value_type<_TV>, __width_of<_TV>>, __v)[__i];
      else
#endif
        return __v[__i];
    }

  /**
   * Helper function to work around Clang and GCC not allowing assignment to v[i] in constant
   * expressions.
   */
  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr void
    __vec_set(_TV& __v, int __i, __vec_value_type<_TV> __x)
    {
      if (__builtin_is_constant_evaluated())
        {
#ifdef _GLIBCXX_CLANG
          auto __arr = __builtin_bit_cast(array<__vec_value_type<_TV>, __width_of<_TV>>, __v);
          __arr[__i] = __x;
          __v = __builtin_bit_cast(_TV, __arr);
#else
          __v = _GLIBCXX_SIMD_INT_PACK(__width_of<_TV>, __j, {
                  return _TV{(__i == __j ? __x : __v[__j])...};
                });
#endif
        }
      else
        __v[__i] = __x;
    }

  /**
   * Return vector builtin with all values from \p __a and \p __b.
   */
  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr
    __vec_builtin_type<__vec_value_type<_TV>, __width_of<_TV> * 2>
    __vec_concat(_TV __a, _TV __b)
    {
      constexpr int _N0 = __width_of<_TV>;
#ifdef _GLIBCXX_CLANG
      using _RV = __vec_builtin_type<__vec_value_type<_TV>, _N0 * 2>;
      if constexpr (_N0 == 1)
        return _RV{__a[0], __b[0]};
      else if constexpr (_N0 == 2)
        return _RV{__a[0], __a[1], __b[0], __b[1]};
      else if constexpr (_N0 == 4)
        return _RV{__a[0], __a[1], __a[2], __a[3],
                   __b[0], __b[1], __b[2], __b[3]};
      else if constexpr (_N0 == 8)
        return _RV{__a[0], __a[1], __a[2], __a[3], __a[4], __a[5], __a[6], __a[7],
                   __b[0], __b[1], __b[2], __b[3], __b[4], __b[5], __b[6], __b[7]};
      else if constexpr (_N0 == 16)
        return _RV{__a[0], __a[1], __a[2], __a[3], __a[4], __a[5], __a[6], __a[7],
                   __a[8], __a[9], __a[10], __a[11], __a[12], __a[13], __a[14], __a[15],
                   __b[0], __b[1], __b[2], __b[3], __b[4], __b[5], __b[6], __b[7],
                   __b[8], __b[9], __b[10], __b[11], __b[12], __b[13], __b[14], __b[15]};
      else if constexpr (_N0 == 32)
        return _RV{__a[0], __a[1], __a[2], __a[3], __a[4], __a[5], __a[6], __a[7],
                   __a[8], __a[9], __a[10], __a[11], __a[12], __a[13], __a[14], __a[15],
                   __a[16], __a[17], __a[18], __a[19], __a[20], __a[21], __a[22], __a[23],
                   __a[24], __a[25], __a[26], __a[27], __a[28], __a[29], __a[30], __a[31],
                   __b[0], __b[1], __b[2], __b[3], __b[4], __b[5], __b[6], __b[7],
                   __b[8], __b[9], __b[10], __b[11], __b[12], __b[13], __b[14], __b[15],
                   __b[16], __b[17], __b[18], __b[19], __b[20], __b[21], __b[22], __b[23],
                   __b[24], __b[25], __b[26], __b[27], __b[28], __b[29], __b[30], __b[31]};
#else
      if constexpr (_N0 == 1)
        return __builtin_shufflevector(__a, __b, 0, 1);
      else if constexpr (_N0 == 2)
        return __builtin_shufflevector(__a, __b, 0, 1, 2, 3);
      else if constexpr (_N0 == 4)
        return __builtin_shufflevector(__a, __b, 0, 1, 2, 3, 4, 5, 6, 7);
      else if constexpr (_N0 == 8)
        return __builtin_shufflevector(__a, __b,
                                       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      else if constexpr (_N0 == 16)
        return __builtin_shufflevector(__a, __b,
                                       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                                       18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
      else if constexpr (_N0 == 32)
        return __builtin_shufflevector(__a, __b,
                                       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                                       18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
                                       33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
                                       48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
                                       63);
#endif
      else
        static_assert(false);
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    __vec_xor(_TV __a, _TV __b)
    {
      using _Tp = __vec_value_type<_TV>;
      if constexpr (is_floating_point_v<_Tp>)
        {
          using _UV = __vec_builtin_type<__integer_from<sizeof(_Tp)>, __width_of<_TV>>;
          return __builtin_bit_cast(
                   _TV, __builtin_bit_cast(_UV, __a) ^ __builtin_bit_cast(_UV, __b));
        }
      else
        return __a ^ __b;
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    __vec_or(_TV __a, _TV __b)
    {
      using _Tp = __vec_value_type<_TV>;
      if constexpr (is_floating_point_v<_Tp>)
        {
          using _UV = __vec_builtin_type<__integer_from<sizeof(_Tp)>, __width_of<_TV>>;
          return __builtin_bit_cast(
                   _TV, __builtin_bit_cast(_UV, __a) | __builtin_bit_cast(_UV, __b));
        }
      else
        return __a | __b;
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    __vec_and(_TV __a, _TV __b)
    {
      using _Tp = __vec_value_type<_TV>;
      if constexpr (is_floating_point_v<_Tp>)
        {
          using _UV = __vec_builtin_type<__integer_from<sizeof(_Tp)>, __width_of<_TV>>;
          return __builtin_bit_cast(
                   _TV, __builtin_bit_cast(_UV, __a) & __builtin_bit_cast(_UV, __b));
        }
      else
        return __a & __b;
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    __vec_andnot(_TV __a, _TV __b)
    {
      using _Tp = __vec_value_type<_TV>;
      using _UV = __vec_builtin_type<__integer_from<sizeof(_Tp)>, __width_of<_TV>>;
      return __builtin_bit_cast(
               _TV, ~__builtin_bit_cast(_UV, __a) & __builtin_bit_cast(_UV, __b));
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    __vec_not(_TV __a)
    {
      using _UV = __vec_builtin_type_bytes<unsigned, sizeof(_TV)>;
      if constexpr (is_floating_point_v<__vec_value_type<_TV>>)
        return __builtin_bit_cast(_TV, ~__builtin_bit_cast(_UV, __a));
      else
        return ~__a;
    }

  /**
   * Return a type with sizeof 16. If the input type is smaller, add zero-padding to \p __x.
   */
  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr auto
    __vec_zero_pad_to_16(_TV __x)
    {
      static_assert(sizeof(_TV) < 16);
      using _Up = _UInt<sizeof(_TV)>;
      __vec_builtin_type_bytes<_Up, 16> __tmp = {__builtin_bit_cast(_Up, __x)};
      return __builtin_bit_cast(__vec_builtin_type_bytes<__vec_value_type<_TV>, 16>, __tmp);
    }

  /// Return \p __x zero-padded to \p _Bytes bytes.
  template <size_t _Bytes, __vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr auto
    __vec_zero_pad_to(_TV __x)
    {
      static_assert(sizeof(_TV) <= _Bytes);
      if constexpr (sizeof(_TV) == _Bytes)
        return __x;
      else
        return __vec_zero_pad_to<_Bytes>(__vec_concat(__x, _TV()));
    }

  /**
   * An object of given type where only the sign bits are 1.
   */
  template <__vec_builtin _V>
    requires std::floating_point<__vec_value_type<_V>>
    constexpr _V _S_signmask = __vec_xor(_V() + 1, _V() - 1);

  template <__vec_builtin _TV, int _Np = __width_of<_TV>,
            typename = make_integer_sequence<int, _Np>>
    struct _VecOps;

  template <__vec_builtin _TV, int _Np, int... _Is>
    struct _VecOps<_TV, _Np, integer_sequence<int, _Is...>>
    {
      static_assert(_Np <= __width_of<_TV>);

      using _Tp = __vec_value_type<_TV>;

      using _HV = __half_vec_builtin_t<conditional_t<_Np >= 2, _TV, __double_vec_builtin_t<_TV>>>;

      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_broadcast(_Tp __init)
      { return _TV {((void)_Is, __init)...}; }

      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_broadcast_to_even(_Tp __init)
      { return _TV {((_Is & 1) == 0 ? __init : _Tp())...}; }

      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_broadcast_to_odd(_Tp __init)
      { return _TV {((_Is & 1) == 1 ? __init : _Tp())...}; }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_all_of(_TV __k) noexcept
      { return (... and (__k[_Is] != 0)); }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_any_of(_TV __k) noexcept
      { return (... or (__k[_Is] != 0)); }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_none_of(_TV __k) noexcept
      { return (... and (__k[_Is] == 0)); }

      template <typename _Offset = integral_constant<int, 0>>
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_extract(__vec_builtin auto __x, _Offset = {})
      {
        static_assert(is_same_v<__vec_value_type<_TV>, __vec_value_type<decltype(__x)>>);
        return __builtin_shufflevector(__x, decltype(__x)(), (_Is + _Offset::value)...);
      }

      // swap neighboring elements
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_swap_neighbors(_TV __x)
      { return __builtin_shufflevector(__x, __x, (_Is ^ 1)...); }

      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_move_carry(_TV __x)
      {
        return __builtin_shufflevector(__x, _TV(), ((_Is & 1) == 0 ? __width_of<_TV>
                                                                   : _Is - 1)...);
      }

      // duplicate each element, duplicating the vector in sizeof
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_dup_each(_HV __x) requires (_Np > 1)
      { return __builtin_shufflevector(__x, __x, (_Is >> 1)...); }

      // duplicate even indexed elements, dropping the odd ones
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_dup_even(_TV __x)
      { return __builtin_shufflevector(__x, __x, (_Is & ~1)...); }

      // duplicate odd indexed elements, dropping the even ones
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_dup_odd(_TV __x)
      { return __builtin_shufflevector(__x, __x, (_Is | 1)...); }

      // return even indexed elements alternating from x and y
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_blend_even(_TV __x, _TV __y)
      { return __builtin_shufflevector(__x, __y, ((_Is & ~1) + __width_of<_TV> * (_Is & 1))...); }

      // return odd indexed elements alternating from x and y
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_blend_odd(_TV __x, _TV __y)
      {
        return __builtin_shufflevector(__x, __y,
                                       ((_Is & ~1) + 1 + __width_of<_TV> * (_Is & 1))...);
      }

      [[__gnu__::__always_inline__]]
      static constexpr void
      _S_overwrite_even_elements(_TV& __x, _HV __y) requires (_Np > 1)
      {
        constexpr __simd_size_type __n = __width_of<_TV>;
        __x = __builtin_shufflevector(__x,
#ifdef _GLIBCXX_CLANG
                                      __vec_concat(__y, __y),
#else
                                      __y,
#endif
                                      ((_Is & 1) == 0 ? __n + _Is / 2 : _Is)...);
      }

      [[__gnu__::__always_inline__]]
      static constexpr void
      _S_overwrite_even_elements(_TV& __xl, _TV& __xh, _TV __y)
      {
        constexpr __simd_size_type __nl = __width_of<_TV>;
        constexpr __simd_size_type __nh = __nl * 3 / 2;
        __xl = __builtin_shufflevector(__xl, __y, ((_Is & 1) == 0 ? __nl + _Is / 2 : _Is)...);
        __xh = __builtin_shufflevector(__xh, __y, ((_Is & 1) == 0 ? __nh + _Is / 2 : _Is)...);
      }

      [[__gnu__::__always_inline__]]
      static constexpr void
      _S_overwrite_odd_elements(_TV& __x, _HV __y) requires (_Np > 1)
      {
        constexpr __simd_size_type __n = __width_of<_TV>;
        __x = __builtin_shufflevector(__x,
#ifdef _GLIBCXX_CLANG
                                      __vec_concat(__y, __y),
#else
                                      __y,
#endif
                                      ((_Is & 1) == 1 ? __n + _Is / 2 : _Is)...);
      }

      [[__gnu__::__always_inline__]]
      static constexpr void
      _S_overwrite_odd_elements(_TV& __xl, _TV& __xh, _TV __y)
      {
        constexpr __simd_size_type __nl = __width_of<_TV>;
        constexpr __simd_size_type __nh = __nl * 3 / 2;
        __xl = __builtin_shufflevector(__xl, __y, ((_Is & 1) == 1 ? __nl + _Is / 2 : _Is)...);
        __xh = __builtin_shufflevector(__xh, __y, ((_Is & 1) == 1 ? __nh + _Is / 2 : _Is)...);
      }

      // implementation in bits/detail.h
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_hadd(_TV __x, _TV __y);

      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_compress_even(__double_vec_builtin_t<_TV> __x)
      { return __builtin_shufflevector(__x, __x, (_Is * 2)...); }

      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_compress_odd(__double_vec_builtin_t<_TV> __x)
      { return __builtin_shufflevector(__x, __x, (_Is * 2 + 1)...); }

      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_compress_even(_TV __x, _TV __y)
      { return __builtin_shufflevector(__x, __y, (_Is * 2)...); }

      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_compress_odd(_TV __x, _TV __y)
      { return __builtin_shufflevector(__x, __y, (_Is * 2 + 1)...); }

      // negate every even element (real part of interleaved complex)
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_complex_negate_real(_TV __x)
      { return __vec_xor(_S_broadcast_to_even(_S_signmask<_TV>[0]), __x); }

      // negate every odd element (imaginary part of interleaved complex)
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_complex_negate_imag(_TV __x)
      { return __vec_xor(_S_broadcast_to_odd(_S_signmask<_TV>[0]), __x); }

      // Subtract elements with even index, add elements with odd index.
      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_addsub(_TV __x, _TV __y)
      {
#if 0
        return __x + _S_complex_negate_imag(__y);
#else
        // GCC recognizes this pattern as addsub
        return __builtin_shufflevector(__x - __y, __x + __y,
                                       (_Is + (_Is & 1) * __width_of<_TV>)...);
#endif
      }

      // true if all elements are know to be equal to __ref at compile time
      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_is_constprop_equal_to(_TV __x, _Tp __ref)
      { return (__is_constprop_equal_to(__x[_Is], __ref) and ...); }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_complex_real_is_zero(_TV __x)
      { return (((_Is & 1) == 1 or (__x[_Is] == _Tp())) and ...); }

      // True iff all elements at even indexes are zero. This includes signed zeros only when
      // -fno-signed-zeros is in effect.
      template <_OptFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
      static constexpr bool
        _S_complex_real_is_constprop_zero(_TV __x)
        {
          if constexpr (_Flags._M_conforming_to_STDC_annex_G())
            {
              using _Up = _UInt<sizeof(_Tp)>;
              return (((_Is & 1) == 1 or __is_constprop_equal_to(__builtin_bit_cast(_Up, __x[_Is]),
                                                                 _Up())) and ...);
            }
          else
            return (((_Is & 1) == 1 or __is_constprop_equal_to(__x[_Is], _Tp())) and ...);
      }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_complex_real_is_one(_TV __x)
      { return (((_Is & 1) == 1 or (__x[_Is] == _Tp(1))) and ...); }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_complex_real_is_constprop_one(_TV __x)
      { return (((_Is & 1) == 1 or __is_constprop_equal_to(__x[_Is], _Tp(1))) and ...); }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_complex_imag_is_zero(_TV __x)
      { return (((_Is & 1) == 0 or (__x[_Is] == _Tp())) and ...); }

      // True iff all elements at odd indexes are zero. This includes signed zeros only when
      // -fno-signed-zeros is in effect.
      template <_OptFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        static constexpr bool
        _S_complex_imag_is_constprop_zero(_TV __x)
        {
          if constexpr (_Flags._M_conforming_to_STDC_annex_G())
            {
              using _Up = _UInt<sizeof(_Tp)>;
              return (((_Is & 1) == 0 or __is_constprop_equal_to(__builtin_bit_cast(_Up, __x[_Is]),
                                                                 _Up())) and ...);
            }
          else
            return (((_Is & 1) == 0 or __is_constprop_equal_to(__x[_Is], _Tp())) and ...);
        }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_complex_imag_is_one(_TV __x)
      { return (((_Is & 1) == 0 or (__x[_Is] == _Tp(1))) and ...); }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_complex_imag_is_constprop_one(_TV __x)
      { return (((_Is & 1) == 0 or __is_constprop_equal_to(__x[_Is], _Tp(1))) and ...); }
    };
}

#pragma GCC diagnostic pop

#endif  // INCLUDE_BITS_VEC_OPS_H_
