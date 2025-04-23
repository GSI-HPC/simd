/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_VEC_DETAIL_H_
#define PROTOTYPE_VEC_DETAIL_H_

#include "simd_config.h"
#include "simd_meta.h"

#include <cstdint>

#define _GLIBCXX_SIMD_TOSTRING_IMPL(x) #x
#define _GLIBCXX_SIMD_TOSTRING(x) _GLIBCXX_SIMD_TOSTRING_IMPL(x)
#define _GLIBCXX_SIMD_LOC __FILE__ ":" _GLIBCXX_SIMD_TOSTRING(__LINE__) ": "

#define __glibcxx_simd_erroneous_unless(expr)                                                      \
  do {                                                                                             \
    const bool __cond = !bool(expr);                                                               \
    if (__builtin_is_constant_evaluated() && __cond)                                               \
      __builtin_trap(); /* erroneous behavior in a constant expression */                          \
    else if (__builtin_constant_p(__cond) && __cond)                                               \
      []() __attribute__((__noinline__, __noipa__, __error__("erroneous behavior detected."        \
        "\n" _GLIBCXX_SIMD_LOC "note: '" #expr "' does not hold")))                                \
      { __builtin_unreachable(); }();                                                              \
  } while(false)

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

namespace std::__detail
{
  template <typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC constexpr bool
    __is_power2_minus_1(_Tp __x)
    {
      using _Up = __make_unsigned_int_t<_Tp>;
      const _Up __y = _Up(__x);
      return _Up() == (__y and _Up(__y + 1u));
    }

  template <std::signed_integral _Tp>
    constexpr bool
    __signed_has_single_bit(_Tp __x)
    { return __has_single_bit(make_unsigned_t<_Tp>(__x)); }

  template <std::signed_integral _Tp>
    constexpr _Tp
    __signed_bit_ceil(_Tp __x)
    {
      const _Tp __r = __bit_ceil(make_unsigned_t<_Tp>(__x));
      __glibcxx_simd_erroneous_unless(__r > 0);
      return __r;
    }

  template <std::signed_integral _Tp>
    constexpr _Tp
    __signed_bit_floor(_Tp __x)
    {
      __glibcxx_simd_erroneous_unless(__x >= 0);
      const _Tp __r = __bit_floor(make_unsigned_t<_Tp>(__x));
      return __r;
    }

  ///////////////////////////////////////////////////////////////////////////////////////////////
  /////////////// tools for working with gnu::vector_size types (vector builtins) ///////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////
  struct alignas(16) _MaskUInt128
  {
    // this is little endian; but as long as all we do is the bit-ops below it doesn't matter
    unsigned long long _M_lo;
    unsigned long long _M_hi;

    constexpr
    _MaskUInt128() = default;

    constexpr
    _MaskUInt128(unsigned long long __lo, unsigned long long __hi)
    : _M_lo(__lo), _M_hi(__hi)
    {}

    constexpr
    _MaskUInt128(int __lo)
    : _M_lo(__lo), _M_hi(__lo < 0 ? -1 : 0)
    {}

    constexpr
    _MaskUInt128(unsigned long long __lo)
    : _M_lo(__lo), _M_hi(0)
    {}

    constexpr friend _MaskUInt128
    operator&(const _MaskUInt128& __x, const _MaskUInt128& __y)
    { return {__x._M_lo & __y._M_lo, __x._M_hi & __y._M_hi}; }

    constexpr friend _MaskUInt128
    operator|(const _MaskUInt128& __x, const _MaskUInt128& __y)
    { return {__x._M_lo | __y._M_lo, __x._M_hi | __y._M_hi}; }

    constexpr friend _MaskUInt128
    operator^(const _MaskUInt128& __x, const _MaskUInt128& __y)
    { return {__x._M_lo ^ __y._M_lo, __x._M_hi ^ __y._M_hi}; }

    constexpr friend _MaskUInt128
    operator~(const _MaskUInt128& __x)
    { return {~__x._M_lo, ~__x._M_hi}; }

    constexpr friend bool
    operator==(const _MaskUInt128&, const _MaskUInt128&) = default;
  };

  /**
   * Reduce template instantiations for internal code by folding different types with same value
   * representation and semantics to a single common type.
   *
   * The primary template is in fwddecl.h.
   */
  template <same_as<long> _Tp>
    requires (sizeof(_Tp) == sizeof(int))
    struct __canonical_vec_type<_Tp>
    { using type = int; };

  template <same_as<long> _Tp>
    requires (sizeof(_Tp) == sizeof(long long))
    struct __canonical_vec_type<_Tp>
    { using type = long long; };

  template <same_as<unsigned long> _Tp>
    requires (sizeof(_Tp) == sizeof(unsigned int))
    struct __canonical_vec_type<_Tp>
    { using type = unsigned int; };

  template <same_as<unsigned long> _Tp>
    requires (sizeof(_Tp) == sizeof(unsigned long long))
    struct __canonical_vec_type<_Tp>
    { using type = unsigned long long; };

  template <typename _Tp>
    requires std::is_enum_v<_Tp>
    struct __canonical_vec_type<_Tp>
    { using type = __canonical_vec_type<std::underlying_type_t<_Tp>>::type; };

  template <>
    struct __canonical_vec_type<char>
    { using type = conditional_t<is_signed_v<char>, signed char, unsigned char>; };

  template <>
    struct __canonical_vec_type<char8_t>
    { using type = unsigned char; };

  template <>
    struct __canonical_vec_type<char16_t>
    { using type = uint_least16_t; };

  template <>
    struct __canonical_vec_type<char32_t>
    { using type = uint_least32_t; };

  template <>
    struct __canonical_vec_type<wchar_t>
    {
      using type = conditional_t<is_signed_v<wchar_t>,
                                 __make_signed_int_t<wchar_t>, __make_unsigned_int_t<wchar_t>>;
    };

  template <>
    struct __canonical_vec_type<_Float64>
    { using type = double; };

  template <>
    struct __canonical_vec_type<_Float32>
    { using type = float; };

  template <typename _Tp, __complex_like _Cp>
    struct __rebind_complex;

  template <typename _Tp, template <typename> class _ComplexTemplate, typename _Up>
    struct __rebind_complex<_Tp, _ComplexTemplate<_Up>>
    { using type = _ComplexTemplate<_Tp>; };

  template <__complex_like _Tp>
    struct __canonical_vec_type<_Tp>
    {
      using type = typename __rebind_complex<
                     typename __canonical_vec_type<typename _Tp::value_type>::type, _Tp>::type;
    };

  /**
   * For use in implementation code in place of __vectorizable. This ensures the implementation code
   * is leaner and simpler.
   */
  template <typename _Tp>
    concept __vectorizable_canon
      = __vectorizable<_Tp> and same_as<_Tp, __canonical_vec_type_t<_Tp>>;

  template <typename _Tp>
    concept __vectorizable_canon_no_complex
      = __vectorizable_canon<_Tp> and not __complex_like<_Tp>;

  /**
   * Alias for a vector builtin with given value type and total sizeof.
   */
  template <__vectorizable _Tp, size_t _Bytes>
    requires (__has_single_bit(_Bytes))
    using __vec_builtin_type_bytes [[__gnu__::__vector_size__(_Bytes)]] = _Tp;

  /**
   * Alias for a vector builtin with given value type \p _Tp and \p _Width.
   */
  template <__vectorizable _Tp, _SimdSizeType _Width>
    requires (__signed_has_single_bit(_Width))
    using __vec_builtin_type = __vec_builtin_type_bytes<_Tp, sizeof(_Tp) * _Width>;

  /**
   * Constrain to any vector builtin with given value type and optional width.
   */
  template <typename _Tp, typename _ValueType,
            _SimdSizeType _Width = sizeof(_Tp) / sizeof(_ValueType)>
    concept __vec_builtin_of
      = not __arithmetic<_Tp> and __vectorizable<_ValueType>
          and _Width >= 1 and sizeof(_Tp) / sizeof(_ValueType) == _Width
          and same_as<__vec_builtin_type_bytes<_ValueType, sizeof(_Tp)>, _Tp>
          and requires(_Tp& __v, _ValueType __x) { __v[0] = __x; };

  static_assert(    __vec_builtin_of<__vec_builtin_type<int, 4>, int>);
  static_assert(not __vec_builtin_of<__vec_builtin_type<int, 4>, char>);
  static_assert(not __vec_builtin_of<int, int>);

  /**
   * Constrain to any vector builtin.
   */
  template <typename _Tp>
    concept __vec_builtin
      = not std::is_class_v<_Tp> and requires(const _Tp& __x) {
        requires __vec_builtin_of<_Tp, remove_cvref_t<decltype(__x[0])>>;
      };

  static_assert(not __vec_builtin<int>);
  static_assert(    __vec_builtin<__vec_builtin_type<int, 4>>);

  template <typename _Tp>
    struct __value_type_of_impl;

  template <typename _Tp>
    concept __has_value_type_member = requires { typename _Tp::value_type; };

  /**
   * Alias for the value type of the given type \p _Tp.
   */
  template <typename _Tp>
    requires __vec_builtin<_Tp> or __arithmetic<_Tp> or __has_value_type_member<_Tp>
      or same_as<_Tp, _MaskUInt128>
    using __value_type_of = typename __value_type_of_impl<_Tp>::type;

  template <__vec_builtin _Tp>
    struct __value_type_of_impl<_Tp>
    { using type = remove_cvref_t<decltype(std::declval<const _Tp>()[0])>; };

  template <__arithmetic _Tp>
    struct __value_type_of_impl<_Tp>
    { using type = _Tp; };

  template <>
    struct __value_type_of_impl<_MaskUInt128>
    { using type = _MaskUInt128; };

  template <__has_value_type_member _Tp>
    struct __value_type_of_impl<_Tp>
    { using type = typename _Tp::value_type; };

  /**
   * The width (number of value_type elements) of the given vector builtin or arithmetic type.
   */
  template <typename _Tp>
    requires __vec_builtin<_Tp> or __arithmetic<_Tp>
    inline constexpr _SimdSizeType __width_of = sizeof(_Tp) / sizeof(__value_type_of<_Tp>);

  /**
   * Alias for a vector builtin with value type \p _Up and equal width as \p _TV.
   */
  template <__vectorizable _Up, __vec_builtin _TV>
    using __rebind_vec_builtin_t = __vec_builtin_type<_Up, __width_of<_TV>>;

  /**
   * Alias for a vector builtin with equal value type and new width \p _Np.
   */
  template <_SimdSizeType _Np, __vec_builtin _TV>
    using __resize_vec_builtin_t = __vec_builtin_type<__value_type_of<_TV>, _Np>;

  template <__vec_builtin _TV>
    requires (__width_of<_TV> > 1)
    using __half_vec_builtin_t = __resize_vec_builtin_t<__width_of<_TV> / 2, _TV>;

  template <__vec_builtin _TV>
    using __double_vec_builtin_t = __resize_vec_builtin_t<__width_of<_TV> * 2, _TV>;

  /**
   * Alias for a vector builtin with value type of \p _TV transformed using \p _Trait and equal
   * width as \p _TV.
   */
  template <template <typename> class _Trait, __vec_builtin _TV>
    using __transform_vec_builtin_t
      = __vec_builtin_type<_Trait<__value_type_of<_TV>>, __width_of<_TV>>;

  /**
   * Alias for a vector mask type matching the given \p _TV type.
   */
  template <__vec_builtin _TV>
    using __mask_vec_from = __transform_vec_builtin_t<__make_signed_int_t, _TV>;

  /**
   * Constrain to vector builtins with given value_type sizeof and optionally vector type sizeof.
   */
  template <typename _Tp, size_t _ValueTypeSize, size_t _VecSize = sizeof(_Tp)>
    concept __vec_builtin_sizeof
      = __vec_builtin<_Tp> and sizeof(_Tp) == _VecSize
          and sizeof(__value_type_of<_Tp>) == _ValueTypeSize;

  static_assert(    __vec_builtin_sizeof<__vec_builtin_type<int, 4>, sizeof(int)>);
  static_assert(not __vec_builtin_sizeof<int, sizeof(int)>);

  using __v2double [[__gnu__::__vector_size__(16)]] = double;
  using __v4double [[__gnu__::__vector_size__(32)]] = double;
  using __v8double [[__gnu__::__vector_size__(64)]] = double;

  using __v4float [[__gnu__::__vector_size__(16)]] = float;
  using __v8float [[__gnu__::__vector_size__(32)]] = float;
  using __v16float [[__gnu__::__vector_size__(64)]] = float;

  using __v8float16 [[__gnu__::__vector_size__(16)]] = _Float16;
  using __v16float16 [[__gnu__::__vector_size__(32)]] = _Float16;
  using __v32float16 [[__gnu__::__vector_size__(64)]] = _Float16;

  using __v16char [[__gnu__::__vector_size__(16)]] = char;
  using __v32char [[__gnu__::__vector_size__(32)]] = char;
  using __v64char [[__gnu__::__vector_size__(64)]] = char;

  using __v16schar [[__gnu__::__vector_size__(16)]] = signed char;
  using __v32schar [[__gnu__::__vector_size__(32)]] = signed char;
  using __v64schar [[__gnu__::__vector_size__(64)]] = signed char;

  using __v16uchar [[__gnu__::__vector_size__(16)]] = unsigned char;
  using __v32uchar [[__gnu__::__vector_size__(32)]] = unsigned char;
  using __v64uchar [[__gnu__::__vector_size__(64)]] = unsigned char;

  using __v8int16 [[__gnu__::__vector_size__(16)]] = int16_t;
  using __v16int16 [[__gnu__::__vector_size__(32)]] = int16_t;
  using __v32int16 [[__gnu__::__vector_size__(64)]] = int16_t;

  using __v8uint16 [[__gnu__::__vector_size__(16)]] = uint16_t;
  using __v16uint16 [[__gnu__::__vector_size__(32)]] = uint16_t;
  using __v32uint16 [[__gnu__::__vector_size__(64)]] = uint16_t;

  using __v4int32 [[__gnu__::__vector_size__(16)]] = int32_t;
  using __v8int32 [[__gnu__::__vector_size__(32)]] = int32_t;
  using __v16int32 [[__gnu__::__vector_size__(64)]] = int32_t;

  using __v4uint32 [[__gnu__::__vector_size__(16)]] = uint32_t;
  using __v8uint32 [[__gnu__::__vector_size__(32)]] = uint32_t;
  using __v16uint32 [[__gnu__::__vector_size__(64)]] = uint32_t;

  using __v2uint64 [[__gnu__::__vector_size__(16)]] = uint64_t;
  using __v4uint64 [[__gnu__::__vector_size__(32)]] = uint64_t;
  using __v8uint64 [[__gnu__::__vector_size__(64)]] = uint64_t;

  using __v2int64 [[__gnu__::__vector_size__(16)]] = int64_t;
  using __v4int64 [[__gnu__::__vector_size__(32)]] = int64_t;
  using __v8int64 [[__gnu__::__vector_size__(64)]] = int64_t;

  using __v2llong [[__gnu__::__vector_size__(16)]] = long long;
  using __v4llong [[__gnu__::__vector_size__(32)]] = long long;
  using __v8llong [[__gnu__::__vector_size__(64)]] = long long;

  using __v2ullong [[__gnu__::__vector_size__(16)]] = unsigned long long;
  using __v4ullong [[__gnu__::__vector_size__(32)]] = unsigned long long;
  using __v8ullong [[__gnu__::__vector_size__(64)]] = unsigned long long;

  /**
   * Helper function to work around Clang not allowing v[i] in constant expressions.
   */
  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr __value_type_of<_TV>
    __vec_get(_TV __v, int __i)
    {
#ifdef __clang__
      if (__builtin_is_constant_evaluated())
        return __builtin_bit_cast(array<__value_type_of<_TV>, __width_of<_TV>>, __v)[__i];
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
    __vec_set(_TV& __v, int __i, __value_type_of<_TV> __x)
    {
      if (__builtin_is_constant_evaluated())
        {
#ifdef __clang__
          auto __arr = __builtin_bit_cast(array<__value_type_of<_TV>, __width_of<_TV>>, __v);
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
   * Returns a permutation of the given vector builtin. _Indices work like for
   * __builtin_shufflevector, except that -1 signifies a 0.
   */
  template <int... _Indices, __vec_builtin _Tp>
    _GLIBCXX_SIMD_INTRINSIC constexpr _Tp
    __vec_permute(_Tp __x)
    {
      static_assert(sizeof...(_Indices) == __width_of<_Tp>);
      return __builtin_shufflevector(__x, _Tp(),
                                     (_Indices == -1 ? __width_of<_Tp> : _Indices)...);
    }

  /**
   * Split \p __x into \p _Total parts and return the part at index \p _Index. Optionally combine
   * multiple parts into the return value (\p _Combine).
   */
  template <int _Index, int _Total, int _Combine = 1, __vec_builtin _TV,
            typename _Width = _Ic<__width_of<_TV>>>
    _GLIBCXX_SIMD_INTRINSIC _GLIBCXX_CONST constexpr
    __vec_builtin_type<__value_type_of<_TV>, _Width::value / _Total * _Combine>
    __vec_extract_part(_TV __x, _Width __width = {})
    {
      using _Tp = __value_type_of<_TV>;
      constexpr int __values_per_part = __width / _Total;
      constexpr int __values_to_skip = _Index * __values_per_part;
      constexpr int __return_size = _Combine * __values_per_part;
      static_assert((_Index + _Combine) * __values_per_part * sizeof(_Tp) <= sizeof(__x),
                    "out of bounds __vec_extract_part");
#ifdef __clang__
      using _RV = __vec_builtin_type<_Tp, __return_size>;
      return _GLIBCXX_SIMD_INT_PACK(__return_size, _Ind, {
               return _RV{__vec_get(__x, __values_to_skip + _Ind)...};
             });
#else
      return _GLIBCXX_SIMD_INT_PACK(__return_size, _Ind, {
               return __builtin_shufflevector(__x, __x, (__values_to_skip + _Ind)...);
             });
#endif
    }

  template <int _Index, int _Total, int _Combine = 1, integral _Tp,
            typename _Width = decltype(__ic<sizeof(_Tp) * __CHAR_BIT__>)>
    _GLIBCXX_SIMD_INTRINSIC _GLIBCXX_CONST constexpr integral auto
    __vec_extract_part(_Tp __x, _Width __width = {})
    {
      constexpr int __values_per_part = __width / _Total;
      constexpr int __values_to_skip = _Index * __values_per_part;
      constexpr int __return_size = __values_per_part * _Combine;
      static_assert((_Index + _Combine) * __values_per_part * sizeof(_Tp) <= sizeof(__x),
                    "out of bounds __vec_extract_part");
      return _GLIBCXX_SIMD_INT_PACK(__return_size, _Ind, {
               return __builtin_shufflevector(__x, __x, (__values_to_skip + _Ind)...);
             });
    }

  template <size_t _Bytes, __vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr __vec_builtin_type_bytes<__value_type_of<_TV>, _Bytes>
    __vec_lo(_TV __x)
    {
      using _Tp = __value_type_of<_TV>;
      constexpr int __new_width = _Bytes / sizeof(_Tp);
      static_assert(sizeof(_TV) >= _Bytes);
      return _GLIBCXX_SIMD_INT_PACK(__new_width, _Is, {
               return __builtin_shufflevector(__x, __x, _Is...);
             });
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr __vec_builtin_type_bytes<__value_type_of<_TV>, 16>
    __vec_lo128(_TV __x)
    {
      using _Tp = __value_type_of<_TV>;
      constexpr int __new_width = 16 / sizeof(_Tp);
      static_assert(sizeof(_TV) >= 32);
      return _GLIBCXX_SIMD_INT_PACK(__new_width, _Is, {
               return __builtin_shufflevector(__x, __x, _Is...);
             });
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr __vec_builtin_type_bytes<__value_type_of<_TV>, 16>
    __vec_hi128(_TV __x)
    {
      using _Tp = __value_type_of<_TV>;
      constexpr int __new_width = 16 / sizeof(_Tp);
      static_assert(sizeof(_TV) >= 32);
      return _GLIBCXX_SIMD_INT_PACK(__new_width, _Is, {
               return __builtin_shufflevector(__x, __x, (__width_of<_TV> - __new_width + _Is)...);
             });
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr __vec_builtin_type_bytes<__value_type_of<_TV>, 32>
    __vec_lo256(_TV __x)
    {
      using _Tp = __value_type_of<_TV>;
      constexpr int __new_width = 32 / sizeof(_Tp);
      static_assert(sizeof(_TV) >= 64);
      return _GLIBCXX_SIMD_INT_PACK(__new_width, _Is, {
               return __builtin_shufflevector(__x, __x, _Is...);
             });
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr __vec_builtin_type_bytes<__value_type_of<_TV>, 32>
    __vec_hi256(_TV __x)
    {
      using _Tp = __value_type_of<_TV>;
      constexpr int __new_width = 32 / sizeof(_Tp);
      static_assert(sizeof(_TV) >= 64);
      return _GLIBCXX_SIMD_INT_PACK(__new_width, _Is, {
               return __builtin_shufflevector(__x, __x, (__width_of<_TV> - __new_width + _Is)...);
             });
    }

  /**
   * Return vector builtin with all values from \p __a and \p __b.
   */
  template <__vec_builtin _Tp>
    _GLIBCXX_SIMD_INTRINSIC constexpr
    __vec_builtin_type<__value_type_of<_Tp>, __width_of<_Tp> * 2>
    __vec_concat(_Tp __a, _Tp __b)
    {
      constexpr int _N0 = __width_of<_Tp>;
#ifdef __clang__
      using _RV = __vec_builtin_type<__value_type_of<_Tp>, _N0 * 2>;
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

  template <int _Offset, __vec_builtin _Tp>
    _GLIBCXX_SIMD_INTRINSIC constexpr auto
    __vec_concat_from_pack(_Tp __a)
    {
      static_assert(_Offset == 0);
      return __vec_concat(__a, _Tp{});
    }

  template <int _Offset, __vec_builtin _Tp, __vec_builtin... _More>
    _GLIBCXX_SIMD_INTRINSIC constexpr auto
    __vec_concat_from_pack(_Tp __a, _Tp __b, _More... __more)
    {
      if constexpr (_Offset == 0)
        return __vec_concat(__a, __b);
      else
        return __vec_concat_from_pack<_Offset - 1>(__more...);
    }

  template <__vec_builtin _Tp, __vec_builtin... _More>
    requires (sizeof...(_More) >= 1)
    _GLIBCXX_SIMD_INTRINSIC constexpr auto
    __vec_concat(_Tp __a, _Tp __b, _More... __more)
    {
      static_assert((std::is_same_v<_Tp, _More> and ...));
      return _GLIBCXX_SIMD_INT_PACK((sizeof...(_More) + 1) / 2, _Is, {
               return __vec_concat(__vec_concat(__a, __b),
                                   __vec_concat_from_pack<_Is>(__more...)...);
             });
    }

  /**
   * Convert \p __a to _To.
   * Prefer this function over calling __builtin_convertvector directly so that the library can
   * improve code-gen (until the relevant PRs on GCC get resolved).
   */
  template <__vec_builtin _To, __vec_builtin _From>
    _GLIBCXX_SIMD_INTRINSIC _To
    __vec_convert(_From __a)
    { return __builtin_convertvector(__a, _To); }

  template <__vectorizable _To, __vec_builtin _From>
    _GLIBCXX_SIMD_INTRINSIC __rebind_vec_builtin_t<_To, _From>
    __vec_convert(_From __a)
    { return __builtin_convertvector(__a, __rebind_vec_builtin_t<_To, _From>); }

  template <__vec_builtin _To, __vec_builtin... _From>
    requires (sizeof...(_From) >= 2)
    _GLIBCXX_SIMD_INTRINSIC _To
    __vec_convert(_From... __pack)
    {
      using _T2 = __vec_builtin_type_bytes<__value_type_of<_To>,
                                           sizeof(_To) / std::__bit_ceil(sizeof...(__pack))>;
      return __vec_concat(__vec_convert<_T2>(__pack)...);
    }

  /**
   * Converts __v into array<_To, N>, where N is _NParts if non-zero or otherwise deduced from _To
   * such that N * #elements(_To) <= #elements(__v). Note: this function may return less than all
   * converted elements
   * \tparam _NParts allows to convert fewer or more (only last _To, to be partially filled) than
   *                 all
   * \tparam _Offset where to start, number of elements (not Bytes or Parts)
   */
  template <typename _To, int _NParts = 0, int _Offset = 0, int _FromSize = 0, __vec_builtin _From>
    _GLIBCXX_SIMD_INTRINSIC auto
    __vec_convert_all(_From __v)
    {
      static_assert(_FromSize < __width_of<_From>);
      constexpr int __input_size = _FromSize == 0 ? __width_of<_From> : _FromSize;
      if constexpr (is_arithmetic_v<_To> && _NParts != 1)
        {
          static_assert(_Offset < __width_of<_From>);
          constexpr int _Np = _NParts == 0 ? __input_size - _Offset : _NParts;
          using _Rp = array<_To, _Np>;
          return _GLIBCXX_SIMD_INT_PACK(_Np, _Is, {
                   return _Rp{(static_cast<_To>(__v[_Is + _Offset]))...};
                 });
        }
      else
        {
          static_assert(__vec_builtin<_To>);
          if constexpr (_NParts == 1)
            {
              static_assert(_Offset % __width_of<_To> == 0);
              return array<_To, 1>{
                __vec_convert<_To>(__vec_extract_part<
                                     _Offset / __width_of<_To>,
                                     __div_roundup(__input_size, __width_of<_To>)>(__v))
              };
            }
          else if constexpr ((__input_size - _Offset) > __width_of<_To>)
            {
              constexpr size_t _NTotal = (__input_size - _Offset) / __width_of<_To>;
              constexpr size_t _Np = _NParts == 0 ? _NTotal : _NParts;
              static_assert(_Np <= _NTotal
                              or (_Np == _NTotal + 1
                                    and (__input_size - _Offset) % __width_of<_To> > 0));
              using _Rp = array<_To, _Np>;
              if constexpr (_Np == 1)
                return _Rp{__vec_convert<_To>(__vec_extract_part<_Offset, __input_size,
                                                                 __width_of<_To>>(__v))};
              else
                return _GLIBCXX_SIMD_INT_PACK(_Np, _Is, {
                         return _Rp {
                           __vec_convert<_To>(
                             __vec_extract_part<_Is * __width_of<_To> + _Offset, __input_size,
                                                __width_of<_To>>(__v))...
                         };
                       });
            }
          else if constexpr (_Offset == 0)
            return array<_To, 1>{__vec_convert<_To>(__v)};
          else
            return array<_To, 1>{__vec_convert<_To>(
                                   __vec_extract_part<_Offset, __input_size,
                                                      __input_size - _Offset>(__v))};
        }
    }

  /**
   * Generator "ctor" for __vec_builtin types.
   */
#define _GLIBCXX_SIMD_VEC_GEN(_Tp, width, pack, code)                                              \
  _GLIBCXX_SIMD_INT_PACK(width, pack, { return _Tp code; })

  template <__vec_builtin _Tp, int _Width = __width_of<_Tp>>
    _GLIBCXX_SIMD_INTRINSIC constexpr _Tp
    __vec_generate(auto&& __gen)
    { return _GLIBCXX_SIMD_VEC_GEN(_Tp, _Width, _Is, {__gen(__ic<_Is>)...}); }

  template <int _Width, typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC constexpr __vec_builtin_type<_Tp, __signed_bit_ceil(_Width)>
    __vec_broadcast(_Tp __x)
    {
      using _Rp = __vec_builtin_type<_Tp, __signed_bit_ceil(_Width)>;
      return _GLIBCXX_SIMD_VEC_GEN(_Rp, _Width, __is, {(__is < _Width ? __x : _Tp())...});
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    __vec_xor(_TV __a, _TV __b)
    {
      using _Tp = __value_type_of<_TV>;
      if constexpr (is_floating_point_v<_Tp>)
        {
          using _UV = __vec_builtin_type<__make_unsigned_int_t<_Tp>, __width_of<_TV>>;
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
      using _Tp = __value_type_of<_TV>;
      if constexpr (is_floating_point_v<_Tp>)
        {
          using _UV = __vec_builtin_type<__make_unsigned_int_t<_Tp>, __width_of<_TV>>;
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
      using _Tp = __value_type_of<_TV>;
      if constexpr (is_floating_point_v<_Tp>)
        {
          using _UV = __vec_builtin_type<__make_unsigned_int_t<_Tp>, __width_of<_TV>>;
          return __builtin_bit_cast(
                   _TV, __builtin_bit_cast(_UV, __a) & __builtin_bit_cast(_UV, __b));
        }
      else
        return __a & __b;
    }


  //overloaded in x86_detail.h
  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    __vec_andnot(_TV __a, _TV __b)
    {
      using _Tp = __value_type_of<_TV>;
      using _UV = __vec_builtin_type<__make_unsigned_int_t<_Tp>, __width_of<_TV>>;
      return __builtin_bit_cast(
               _TV, ~__builtin_bit_cast(_UV, __a) & __builtin_bit_cast(_UV, __b));
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    __vec_not(_TV __a)
    {
      using _UV = __vec_builtin_type_bytes<unsigned, sizeof(_TV)>;
      if constexpr (is_floating_point_v<__value_type_of<_TV>>)
        return __builtin_bit_cast(_TV, ~__builtin_bit_cast(_UV, __a));
      else
        return ~__a;
    }

  /**
   * Bit-cast \p __x to a vector type with equal sizeof but value-type \p _Up.
   * Optionally, the width of the return type can be constrained, making the cast ill-formed if it
   * doesn't match.
   */
  template <typename _Up, int _Np = 0, __vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr auto
    __vec_bitcast(_TV __x)
    {
      if constexpr (_Np == 0)
        return __builtin_bit_cast(__vec_builtin_type_bytes<_Up, sizeof(__x)>, __x);
      else
        return __builtin_bit_cast(__vec_builtin_type<_Up, _Np>, __x);
    }

  /**
   * Bit-cast \p __x to the vector type \p _UV. sizeof(_UV) may be smaller than sizeof(__x).
   */
  template <__vec_builtin _UV, __vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr _UV
    __vec_bitcast_trunc(_TV __x)
    {
      static_assert(sizeof(_UV) <= sizeof(_TV));
      if constexpr (sizeof(_UV) == sizeof(_TV))
        return __builtin_bit_cast(_UV, __x);
      else if constexpr (sizeof(_UV) <= 8)
        {
          using _Ip = __make_signed_int_t<_UV>;
          return __builtin_bit_cast(
                   _UV, __builtin_bit_cast(__vec_builtin_type_bytes<_Ip, sizeof(__x)>, __x)[0]);
        }
      else
        {
          const auto __y
            = __builtin_bit_cast(__vec_builtin_type_bytes<__value_type_of<_UV>, sizeof(__x)>, __x);
          return _GLIBCXX_SIMD_INT_PACK(__width_of<_UV>, _Is, {
                   return __builtin_shufflevector(__y, __y, _Is...);
                 });
        }
    }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC _TV
    __vec_optimizer_barrier(_TV __x)
    {
      asm("":"+v,x,g"(__x));
      return __x;
    }

  /**
   * Return a type with sizeof 16. If the input type is smaller, add zero-padding to \p __x.
   */
  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC constexpr auto
    __vec_zero_pad_to_16(_TV __x)
    {
      static_assert(sizeof(_TV) <= 16);
      if constexpr (sizeof(_TV) == 16)
        return __x;
      else
        {
          using _Up = __make_signed_int_t<_TV>;
          __vec_builtin_type_bytes<_Up, 16> __tmp = {__builtin_bit_cast(_Up, __x)};
          return __builtin_bit_cast(__vec_builtin_type_bytes<__value_type_of<_TV>, 16>, __tmp);
        }
    }

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
   * An object of given type where all bits are 1.
   */
  template <__vec_builtin _V>
    constexpr _V _S_allbits = __builtin_bit_cast(_V, ~__vec_builtin_type_bytes<char, sizeof(_V)>());

  /**
   * An object of given type where only the sign bits are 1.
   */
  template <__vec_builtin _V>
    requires std::floating_point<__value_type_of<_V>>
    constexpr _V _S_signmask = __vec_xor(_V() + 1, _V() - 1);

  /**
   * An object of given type where only the sign bits are 0 (complement of _S_signmask).
   */
  template <__vec_builtin _V>
    requires std::floating_point<__value_type_of<_V>>
    constexpr _V _S_absmask = __vec_andnot(_S_signmask<_V>, _S_allbits<_V>);

  template <__vec_builtin _V, __value_type_of<_V> _Offset = {}>
    constexpr _V _S_vec_iota = []<int... _Is>(integer_sequence<int, _Is...>) {
      return _V{static_cast<__value_type_of<_V>>(_Is + _Offset)...};
    }(make_integer_sequence<int, __width_of<_V>>());

  // work around __builtin_constant_p returning false unless passed a variable
  // (__builtin_constant_p(x[0]) is false while __is_constprop(x[0]) is true)
  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
  __is_constprop(const auto& __x)
  { return __builtin_is_constant_evaluated() or __builtin_constant_p(__x); }

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
  __is_constprop(const __complex_like auto& __x)
  {
    return __builtin_is_constant_evaluated()
             or (__is_constprop(__x.real()) and __is_constprop(__x.imag()));
  }

  _GLIBCXX_SIMD_INTRINSIC constexpr bool
  __is_constprop_equal_to(const auto& __x, const auto& __expect)
  { return (__builtin_is_constant_evaluated() or __builtin_constant_p(__x)) and __x == __expect; }

  template <__vec_builtin _TV>
    _GLIBCXX_SIMD_INTRINSIC _TV
    __hadd_insn(_TV __x, _TV __y) = delete;

  template <__vec_builtin _TV, int _Np = __width_of<_TV>,
            typename = make_integer_sequence<int, _Np>>
    struct _VecOps;

  template <__vec_builtin _TV, int _Np, int... _Is>
    struct _VecOps<_TV, _Np, integer_sequence<int, _Is...>>
    {
      static_assert(_Np <= __width_of<_TV>);

      using _Tp = __value_type_of<_TV>;

      using _HV = __half_vec_builtin_t<conditional_t<_Np >= 2, _TV, __double_vec_builtin_t<_TV>>>;

      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_broadcast(_Tp __init)
      { return _TV {((void)_Is, __init)...}; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_broadcast_to_even(_Tp __init)
      { return _TV {((_Is & 1) == 0 ? __init : _Tp())...}; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_broadcast_to_odd(_Tp __init)
      { return _TV {((_Is & 1) == 1 ? __init : _Tp())...}; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_all_of(_TV __k) noexcept
      { return (... and (__k[_Is] != 0)); }

      template <typename _Offset = _Ic<0>>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_extract(__vec_builtin auto __x, _Offset = {})
      { return __builtin_shufflevector(__x, __x, (_Is + _Offset::value)...); }

      // swap neighboring elements
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_swap_neighbors(_TV __x)
      { return __builtin_shufflevector(__x, __x, (_Is ^ 1)...); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_move_carry(_TV __x)
      {
        return __builtin_shufflevector(__x, _TV(), ((_Is & 1) == 0 ? __width_of<_TV>
                                                                   : _Is - 1)...);
      }

      // duplicate each element, duplicating the vector in sizeof
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_dup_each(_HV __x) requires (_Np > 1)
      { return __builtin_shufflevector(__x, __x, (_Is >> 1)...); }

      // duplicate even indexed elements, dropping the odd ones
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_dup_even(_TV __x)
      { return __builtin_shufflevector(__x, __x, (_Is & ~1)...); }

      // duplicate odd indexed elements, dropping the even ones
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_dup_odd(_TV __x)
      { return __builtin_shufflevector(__x, __x, (_Is | 1)...); }

      // return even indexed elements alternating from x and y
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_blend_even(_TV __x, _TV __y)
      { return __builtin_shufflevector(__x, __y, ((_Is & ~1) + __width_of<_TV> * (_Is & 1))...); }

      // return odd indexed elements alternating from x and y
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_blend_odd(_TV __x, _TV __y)
      {
        return __builtin_shufflevector(__x, __y,
                                       ((_Is & ~1) + 1 + __width_of<_TV> * (_Is & 1))...);
      }

      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_overwrite_even_elements(_TV& __x, _HV __y) requires (_Np > 1)
      {
        constexpr _SimdSizeType __n = __width_of<_TV>;
        __x = __builtin_shufflevector(__x,
#ifdef __clang__
                                      __vec_concat(__y, __y),
#else
                                      __y,
#endif
                                      ((_Is & 1) == 0 ? __n + _Is / 2 : _Is)...);
      }

      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_overwrite_even_elements(_TV& __xl, _TV& __xh, _TV __y)
      {
        constexpr _SimdSizeType __nl = __width_of<_TV>;
        constexpr _SimdSizeType __nh = __nl * 3 / 2;
        __xl = __builtin_shufflevector(__xl, __y, ((_Is & 1) == 0 ? __nl + _Is / 2 : _Is)...);
        __xh = __builtin_shufflevector(__xh, __y, ((_Is & 1) == 0 ? __nh + _Is / 2 : _Is)...);
      }

      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_overwrite_odd_elements(_TV& __x, _HV __y) requires (_Np > 1)
      {
        constexpr _SimdSizeType __n = __width_of<_TV>;
        __x = __builtin_shufflevector(__x,
#ifdef __clang__
                                      __vec_concat(__y, __y),
#else
                                      __y,
#endif
                                      ((_Is & 1) == 1 ? __n + _Is / 2 : _Is)...);
      }

      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_overwrite_odd_elements(_TV& __xl, _TV& __xh, _TV __y)
      {
        constexpr _SimdSizeType __nl = __width_of<_TV>;
        constexpr _SimdSizeType __nh = __nl * 3 / 2;
        __xl = __builtin_shufflevector(__xl, __y, ((_Is & 1) == 1 ? __nl + _Is / 2 : _Is)...);
        __xh = __builtin_shufflevector(__xh, __y, ((_Is & 1) == 1 ? __nh + _Is / 2 : _Is)...);
      }

      // implementation in bits/detail.h
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_hadd(_TV __x, _TV __y);

      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_compress_even(__double_vec_builtin_t<_TV> __x)
      { return __builtin_shufflevector(__x, __x, (_Is * 2)...); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_compress_odd(__double_vec_builtin_t<_TV> __x)
      { return __builtin_shufflevector(__x, __x, (_Is * 2 + 1)...); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_compress_even(_TV __x, _TV __y)
      { return __builtin_shufflevector(__x, __y, (_Is * 2)...); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_compress_odd(_TV __x, _TV __y)
      { return __builtin_shufflevector(__x, __y, (_Is * 2 + 1)...); }

      // negate every even element (real part of interleaved complex)
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_complex_negate_real(_TV __x)
      { return __vec_xor(_S_broadcast_to_even(_S_signmask<_TV>[0]), __x); }

      // negate every odd element (imaginary part of interleaved complex)
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
      _S_complex_negate_imag(_TV __x)
      { return __vec_xor(_S_broadcast_to_odd(_S_signmask<_TV>[0]), __x); }

      // Subtract elements with even index, add elements with odd index.
      _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
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
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_is_constprop_equal_to(_TV __x, _Tp __ref)
      { return (__is_constprop_equal_to(__x[_Is], __ref) and ...); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_complex_real_is_zero(_TV __x)
      { return (((_Is & 1) == 1 or (__x[_Is] == _Tp())) and ...); }

      // True iff all elements at even indexes are zero. This includes signed zeros only when
      // -fno-signed-zeros is in effect.
      template <__detail::_OptFlags _Flags = {}>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
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

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_complex_real_is_one(_TV __x)
      { return (((_Is & 1) == 1 or (__x[_Is] == _Tp(1))) and ...); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_complex_real_is_constprop_one(_TV __x)
      { return (((_Is & 1) == 1 or __is_constprop_equal_to(__x[_Is], _Tp(1))) and ...); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_complex_imag_is_zero(_TV __x)
      { return (((_Is & 1) == 0 or (__x[_Is] == _Tp())) and ...); }

      // True iff all elements at odd indexes are zero. This includes signed zeros only when
      // -fno-signed-zeros is in effect.
      template <__detail::_OptFlags _Flags = {}>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
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

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_complex_imag_is_one(_TV __x)
      { return (((_Is & 1) == 0 or (__x[_Is] == _Tp(1))) and ...); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_complex_imag_is_constprop_one(_TV __x)
      { return (((_Is & 1) == 0 or __is_constprop_equal_to(__x[_Is], _Tp(1))) and ...); }
    };
}

#pragma GCC diagnostic pop

#endif  // PROTOTYPE_VEC_DETAIL_H_
