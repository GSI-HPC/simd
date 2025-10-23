/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef _GLIBCXX_VEC_OPS_H
#define _GLIBCXX_VEC_OPS_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include "simd_details.h"

#include <bit>
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
      = !is_arithmetic_v<_Tp> && __vectorizable<_ValueType>
          && _Width >= 1 && sizeof(_Tp) / sizeof(_ValueType) == _Width
          && same_as<__vec_builtin_type_bytes<_ValueType, sizeof(_Tp)>, _Tp>
          && requires(_Tp& __v, _ValueType __x) { __v[0] = __x; };

  /**
   * Constrain to any vector builtin.
   */
  template <typename _Tp>
    concept __vec_builtin
      = !is_class_v<_Tp> && requires(const _Tp& __x) {
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

  template <typename _Up, __vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr __vec_builtin_type_bytes<_Up, sizeof(_TV)>
    __vec_bit_cast(_TV __v)
    { return reinterpret_cast<__vec_builtin_type_bytes<_Up, sizeof(_TV)>>(__v); }

  template <int _Np, __vec_builtin _TV>
    requires signed_integral<__vec_value_type<_TV>>
    static constexpr _TV _S_vec_implicit_mask = []<int... _Is> (integer_sequence<int, _Is...>) {
      return _TV{ (_Is < _Np ? -1 : 0)... };
    } (make_integer_sequence<int, __width_of<_TV>>());

  /**
   * Helper function to work around Clang not allowing v[i] in constant expressions.
   */
  template <__vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr __vec_value_type<_TV>
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
    [[__gnu__::__always_inline__]]
    constexpr void
    __vec_set(_TV& __v, int __i, __vec_value_type<_TV> __x)
    {
      if (__builtin_is_constant_evaluated())
        {
#ifdef _GLIBCXX_CLANG
          auto __arr = __builtin_bit_cast(array<__vec_value_type<_TV>, __width_of<_TV>>, __v);
          __arr[__i] = __x;
          __v = __builtin_bit_cast(_TV, __arr);
#else
          constexpr auto [...__j] = __iota<int[__width_of<_TV>]>;
          __v = _TV{(__i == __j ? __x : __v[__j])...};
#endif
        }
      else
        __v[__i] = __x;
    }

  /**
   * Return vector builtin with all values from \p __a and \p __b.
   */
  template <__vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr __vec_builtin_type<__vec_value_type<_TV>, __width_of<_TV> * 2>
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
      else
        static_assert(false);
#elif __has_builtin(__integer_pack)
      return __builtin_shufflevector(__a, __b, __integer_pack(2 * _N0)...);
#else
#error "Neither Clang nor GCC?"
#endif
    }

  template <int _N0, int _N1, int... _Ns, __vec_builtin _TV0, __vec_builtin _TV1,
           __vec_builtin... _TVs>
    [[__gnu__::__always_inline__]]
    constexpr __vec_builtin_type<__vec_value_type<_TV0>,
                                 __bit_ceil(unsigned(_N0 + (_N1 + ... + _Ns)))>
    __vec_concat_sized(const _TV0& __a, const _TV1& __b, const _TVs&... __rest)
    {
      constexpr auto [...__is] = __iota<int[__bit_ceil(unsigned(_N0 + _N1))]>;
      const auto __ab = __builtin_shufflevector(
                          __a, __b, (__is < _N0 ? __is
                                                : __is < _N0 + _N1 ? __is - _N0 + __width_of<_TV0>
                                                                   : -1)...);
      if constexpr (sizeof...(__rest) == 0)
        return __ab;
      else
        return __vec_concat_sized<_N0 + _N1, _Ns...>(__ab, __rest...);
    }

  template <__vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr __half_vec_builtin_t<_TV>
    __vec_split_lo(_TV __v)
    { return __builtin_shufflevector(__v, __v, __integer_pack(__width_of<_TV> / 2)...); }

  template <__vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr __half_vec_builtin_t<_TV>
    __vec_split_hi(_TV __v)
    {
      constexpr int __n = __width_of<_TV> / 2;
      constexpr auto [...__is] = __iota<int[__n]>;
      return __half_vec_builtin_t<_TV> {__v[(__n + __is)]...};
    }

  /**
   * Return a type with sizeof 16. If the input type is smaller, add zero-padding to \p __x.
   */
  template <__vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr auto
    __vec_zero_pad_to_16(_TV __x)
    {
      static_assert(sizeof(_TV) < 16);
      using _Up = _UInt<sizeof(_TV)>;
      __vec_builtin_type_bytes<_Up, 16> __tmp = {__builtin_bit_cast(_Up, __x)};
      return __builtin_bit_cast(__vec_builtin_type_bytes<__vec_value_type<_TV>, 16>, __tmp);
    }

  /// Return \p __x zero-padded to \p _Bytes bytes.
  template <size_t _Bytes, __vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr auto
    __vec_zero_pad_to(_TV __x)
    {
      static_assert(sizeof(_TV) <= _Bytes);
      if constexpr (sizeof(_TV) == _Bytes)
        return __x;
      else
        return __vec_zero_pad_to<_Bytes>(__vec_concat(__x, _TV()));
    }

  // work around __builtin_constant_p returning false unless passed a variable
  // (__builtin_constant_p(x[0]) is false while __is_constprop(x[0]) is true)
  template <typename _Tp>
    [[__gnu__::__always_inline__]]
    constexpr bool
    __is_constprop(const _Tp& __x)
    {
      if constexpr (__complex_like<_Tp>)
        return __is_constprop(__x.real()) && __is_constprop(__x.imag());
      else
        return __builtin_constant_p(__x);
    }

  [[__gnu__::__always_inline__]]
  constexpr bool
  __is_constprop(const auto&... __xs) requires(sizeof...(__xs) >= 2)
  {
    if consteval
      {
        return true;
      }
    else
      {
        return (__is_constprop(__xs) && ...);
      }
  }

  [[__gnu__::__always_inline__]]
  constexpr bool
  __is_constprop_equal_to(const auto& __x, const auto& __expect)
  { return __is_constprop(__x) && __x == __expect; }

#if _GLIBCXX_X86
  template <__vec_builtin _UV, __vec_builtin _TV>
    inline _UV
    __x86_cvt_f16c(_TV __v);
#endif

  /** \internal
   * Simple wrapper around __builtin_convertvector to provide static_cast-like syntax.
   *
   * Works around GCC failing to use the F16C/AVX512F cvtps2ph/cvtph2ps instructions.
   */
  template <__vec_builtin _UV, __vec_builtin _TV, _ArchTraits _Traits = {}>
    [[__gnu__::__always_inline__]]
    constexpr _UV
    __vec_cast(_TV __v)
    {
      static_assert(__width_of<_UV> == __width_of<_TV>);
#if _GLIBCXX_X86
      constexpr bool __to_f16 = is_same_v<__vec_value_type<_UV>, _Float16>;
      constexpr bool __from_f16 = is_same_v<__vec_value_type<_TV>, _Float16>;
      constexpr bool __needs_f16c = _Traits._M_have_f16c() && !_Traits._M_have_avx512fp16()
                                      && (__to_f16 || __from_f16);
      if (__needs_f16c && !__is_constprop(__v))
        { // Work around PR121688
          if constexpr (__needs_f16c)
            return __x86_cvt_f16c<_UV>(__v);
        }
      if constexpr (is_floating_point_v<__vec_value_type<_TV>>
                      && is_integral_v<__vec_value_type<_UV>> && sizeof(_UV) < sizeof(_TV)
                      && sizeof(__vec_value_type<_UV>) < sizeof(int))
        {
          using _Ip = __integer_from<std::min(sizeof(int), sizeof(__vec_value_type<_TV>))>;
          using _IV = __vec_builtin_type<_Ip, __width_of<_TV>>;
          return __vec_cast<_UV>(__vec_cast<_IV>(__v));
        }
#endif
      return __builtin_convertvector(__v, _UV);
    }

  /** \internal
   * Overload of the above cast function that determines the destination vector type from a given
   * element type \p _Up and the `__width_of` the argument type.
   *
   * Calls the above overload.
   */
  template <__vectorizable _Up, __vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr __vec_builtin_type<_Up, __width_of<_TV>>
    __vec_cast(_TV __v)
    { return __vec_cast<__vec_builtin_type<_Up, __width_of<_TV>>>(__v); }

  /** \internal
   * As above, but with additional precondition on possible values of the argument.
   *
   * Precondition: __k[i] is either 0 or -1 for all i.
   */
  template <__vec_builtin _UV, __vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr _UV
    __vec_mask_cast(_TV __k)
    {
      static_assert(signed_integral<__vec_value_type<_UV>>);
      static_assert(signed_integral<__vec_value_type<_TV>>);
      // TODO: __builtin_convertvector cannot be optimal because it doesn't consider input and
      // output can only be 0 or -1.
      return __builtin_convertvector(__k, _UV);
    }

  template <__vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr _TV
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
    [[__gnu__::__always_inline__]]
    constexpr _TV
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
    [[__gnu__::__always_inline__]]
    constexpr _TV
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
    [[__gnu__::__always_inline__]]
    constexpr _TV
    __vec_andnot(_TV __a, _TV __b)
    {
      using _Tp = __vec_value_type<_TV>;
      using _UV = __vec_builtin_type<__integer_from<sizeof(_Tp)>, __width_of<_TV>>;
      return __builtin_bit_cast(
               _TV, ~__builtin_bit_cast(_UV, __a) & __builtin_bit_cast(_UV, __b));
    }

  template <__vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    constexpr _TV
    __vec_not(_TV __a)
    {
      using _UV = __vec_builtin_type_bytes<unsigned, sizeof(_TV)>;
      if constexpr (is_floating_point_v<__vec_value_type<_TV>>)
        return __builtin_bit_cast(_TV, ~__builtin_bit_cast(_UV, __a));
      else
        return ~__a;
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
      _S_broadcast_to_even(_Tp __init)
      { return _TV {((_Is & 1) == 0 ? __init : _Tp())...}; }

      [[__gnu__::__always_inline__]]
      static constexpr _TV
      _S_broadcast_to_odd(_Tp __init)
      { return _TV {((_Is & 1) == 1 ? __init : _Tp())...}; }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_all_of(_TV __k) noexcept
      { return (... && (__k[_Is] != 0)); }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_any_of(_TV __k) noexcept
      { return (... || (__k[_Is] != 0)); }

      [[__gnu__::__always_inline__]]
      static constexpr bool
      _S_none_of(_TV __k) noexcept
      { return (... && (__k[_Is] == 0)); }

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
      { return (__is_constprop_equal_to(__x[_Is], __ref) && ...); }

      // True iff all elements at even indexes are zero. This includes signed zeros only when
      // -fno-signed-zeros is in effect.
      template <_OptTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
      static constexpr bool
        _S_complex_real_is_constprop_zero(_TV __x)
        {
          if constexpr (_Traits._M_conforming_to_STDC_annex_G())
            {
              using _Up = _UInt<sizeof(_Tp)>;
              return (((_Is & 1) == 1 || __is_constprop_equal_to(__builtin_bit_cast(_Up, __x[_Is]),
                                                                 _Up())) && ...);
            }
          else
            return (((_Is & 1) == 1 || __is_constprop_equal_to(__x[_Is], _Tp())) && ...);
      }

      // True iff all elements at odd indexes are zero. This includes signed zeros only when
      // -fno-signed-zeros is in effect.
      template <_OptTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        static constexpr bool
        _S_complex_imag_is_constprop_zero(_TV __x)
        {
          if constexpr (_Traits._M_conforming_to_STDC_annex_G())
            {
              using _Up = _UInt<sizeof(_Tp)>;
              return (((_Is & 1) == 0 || __is_constprop_equal_to(__builtin_bit_cast(_Up, __x[_Is]),
                                                                 _Up())) && ...);
            }
          else
            return (((_Is & 1) == 0 || __is_constprop_equal_to(__x[_Is], _Tp())) && ...);
        }
    };
}

#pragma GCC diagnostic pop
#endif // C++26
#endif // _GLIBCXX_VEC_OPS_H
