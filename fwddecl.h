/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_FWDDECL_H_
#define PROTOTYPE_FWDDECL_H_

//#include <experimental/bits/simd_detail.h>
#include "simd_config.h"

#include <functional>
#include <stdfloat>
#include <type_traits>
#include <ranges>

namespace std
{
  template <int _Width>
    struct _VecAbi;

  template <int _Width>
    struct _Avx512Abi;

  struct _ScalarAbi;

  namespace __detail
  {
    template <size_t _Np, bool _Sanitized = false>
      struct _BitMask;

    template <size_t _Np>
      using _SanitizedBitMask = _BitMask<_Np, true>;

    template <size_t _Bytes>
      struct __make_unsigned_int;

    template <>
      struct __make_unsigned_int<sizeof(unsigned int)>
      { using type = unsigned int; };

    template <>
      struct __make_unsigned_int<sizeof(unsigned long)
                                   + (sizeof(unsigned long) == sizeof(unsigned int))>
      { using type = unsigned long; };

    template <>
      struct __make_unsigned_int<sizeof(unsigned long long)
                                   + (sizeof(unsigned long long) == sizeof(unsigned long))>
      { using type = unsigned long long; };

    template <>
      struct __make_unsigned_int<sizeof(unsigned short)>
      { using type = unsigned short; };

    template <>
      struct __make_unsigned_int<sizeof(unsigned char)>
      { using type = unsigned char; };

    template <typename _Tp>
      using __make_unsigned_int_t = typename __make_unsigned_int<sizeof(_Tp)>::type;

    template <typename _Tp>
      using __make_signed_int_t = make_signed_t<__make_unsigned_int_t<_Tp>>;

    template <size_t _Bs>
      using __mask_integer_from = make_signed_t<typename __make_unsigned_int<_Bs>::type>;

    template <typename _Tp>
      struct __is_vectorizable
      : bool_constant<false>
      {};

    // TODO
    //template <> struct __is_vectorizable<std::byte> : bool_constant<true> {};

    template <> struct __is_vectorizable<char> : bool_constant<true> {};
    template <> struct __is_vectorizable<wchar_t> : bool_constant<true> {};
    template <> struct __is_vectorizable<char8_t> : bool_constant<true> {};
    template <> struct __is_vectorizable<char16_t> : bool_constant<true> {};
    template <> struct __is_vectorizable<char32_t> : bool_constant<true> {};

    template <> struct __is_vectorizable<  signed char> : bool_constant<true> {};
    template <> struct __is_vectorizable<unsigned char> : bool_constant<true> {};
    template <> struct __is_vectorizable<  signed short> : bool_constant<true> {};
    template <> struct __is_vectorizable<unsigned short> : bool_constant<true> {};
    template <> struct __is_vectorizable<  signed int> : bool_constant<true> {};
    template <> struct __is_vectorizable<unsigned int> : bool_constant<true> {};
    template <> struct __is_vectorizable<  signed long> : bool_constant<true> {};
    template <> struct __is_vectorizable<unsigned long> : bool_constant<true> {};
    template <> struct __is_vectorizable<  signed long long> : bool_constant<true> {};
    template <> struct __is_vectorizable<unsigned long long> : bool_constant<true> {};

    template <> struct __is_vectorizable<float> : bool_constant<true> {};
    template <> struct __is_vectorizable<double> : bool_constant<true> {};
#ifdef __STDCPP_FLOAT16_T__
    //template <> struct __is_vectorizable<std::float16_t> : bool_constant<true> {};
#endif
#ifdef __STDCPP_FLOAT32_T__
    template <> struct __is_vectorizable<std::float32_t> : bool_constant<true> {};
#endif
#ifdef __STDCPP_FLOAT64_T__
    template <> struct __is_vectorizable<std::float64_t> : bool_constant<true> {};
#endif

    template <typename _Tp>
      concept __vectorizable = __is_vectorizable<_Tp>::value;

    template <typename _Tp, typename>
      struct __make_dependent
      { using type = _Tp; };

    template <typename _Tp, typename _Up>
      using __make_dependent_t = typename __make_dependent<_Tp, _Up>::type;

    template <int _Bs, typename _Tp>
      consteval auto
      __native_abi_impl_recursive()
      {
        constexpr int _Width = _Bs / sizeof(_Tp);
        if constexpr (_Avx512Abi<_Width>::template _IsValid<_Tp>::value)
          return _Avx512Abi<_Width>();
        else if constexpr (_VecAbi<_Width>::template _IsValid<_Tp>::value)
          return _VecAbi<_Width>();
        else if constexpr (_Bs > sizeof(_Tp))
          return __native_abi_impl_recursive<_Bs / 2, _Tp>();
        else
          return __make_dependent_t<_ScalarAbi, _Tp>();
      }

    struct _InvalidAbi
    {};

    template <typename _Tp>
      consteval auto
      __native_abi_impl()
      {
        if constexpr (__is_vectorizable<_Tp>::value)
          {
            // __one is used to make _VecAbi a dependent type
            constexpr int __one = sizeof(_Tp) / sizeof(_Tp);
            return __native_abi_impl_recursive<__one * 256, _Tp>();
          }
        else
          return _InvalidAbi();
      }

    template <typename _Tp>
      using _NativeAbi = decltype(__native_abi_impl<_Tp>());

    using _SimdSizeType = int;

    template <typename _Tp, _SimdSizeType _Np>
      struct _DeduceAbi;

    template <typename _Tp, _SimdSizeType _Np>
      using __deduce_t = typename _DeduceAbi<_Tp, _Np>::type;
  }

  template <typename _Abi0, int _Np>
    struct _AbiArray;

  template <__detail::_SimdSizeType _Np, typename _Tag>
    struct _AbiCombine;

  template <typename _Tp, typename _Abi = __detail::_NativeAbi<_Tp>>
    class basic_simd;

  template <size_t _Bytes,
            typename _Abi = __detail::_NativeAbi<__detail::__mask_integer_from<_Bytes>>>
    class basic_simd_mask;

  template <typename... _Flags>
    struct simd_flags;

  template <typename _Tp>
    struct is_simd
    : bool_constant<false>
    {};

  template <typename _Tp>
    inline constexpr bool is_simd_v = is_simd<_Tp>::value;

  template <typename _Tp>
    struct is_mask
    : bool_constant<false>
    {};

  template <typename _Tp>
    inline constexpr bool is_mask_v = is_mask<_Tp>::value;

  template <typename _Tp, typename _Abi = __detail::_NativeAbi<_Tp>>
    struct __simd_size : integral_constant<__detail::_SimdSizeType, 0>
    {};

  template <typename _Tp, typename _Abi>
    inline constexpr __detail::_SimdSizeType __simd_size_v = __simd_size<_Tp, _Abi>::value;

  template <typename _Tp, typename _Up = typename _Tp::value_type>
    struct simd_alignment
    {};

  template <typename _Tp, typename _Up = typename _Tp::value_type>
    inline constexpr size_t simd_alignment_v = simd_alignment<_Tp, _Up>::value;

  template <typename _Tp, typename _Vp>
    struct rebind_simd
    {};

  template <typename _Tp, typename _Vp>
    using rebind_simd_t = typename rebind_simd<_Tp, _Vp>::type;

  template <__detail::_SimdSizeType _Np, typename _Vp>
    struct resize_simd
    {};

  template <__detail::_SimdSizeType _Np, typename _Vp>
    using resize_simd_t = typename resize_simd<_Np, _Vp>::type;

  template <typename _Tp,
            __detail::_SimdSizeType _Np = __simd_size_v<_Tp, __detail::_NativeAbi<_Tp>>>
    using simd = basic_simd<_Tp, __detail::__deduce_t<_Tp, _Np>>;

  template <typename _Tp,
            __detail::_SimdSizeType _Np = __simd_size_v<_Tp, __detail::_NativeAbi<_Tp>>>
    using simd_mask = basic_simd_mask<sizeof(_Tp), __detail::__deduce_t<_Tp, _Np>>;

  // mask_reductions.h
  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
    all_of(const basic_simd_mask<_Bs, _Abi>& __k) noexcept;

  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
    any_of(const basic_simd_mask<_Bs, _Abi>& __k) noexcept;

  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
    none_of(const basic_simd_mask<_Bs, _Abi>& __k) noexcept;

  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
    reduce_count(const basic_simd_mask<_Bs, _Abi>& __k) noexcept;

  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
    reduce_min_index(const basic_simd_mask<_Bs, _Abi>& __k);

  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
    reduce_max_index(const basic_simd_mask<_Bs, _Abi>& __k);

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
  all_of(same_as<bool> auto __x) noexcept;

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
  any_of(same_as<bool> auto __x) noexcept;

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
  none_of(same_as<bool> auto __x) noexcept;

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
  reduce_count(same_as<bool> auto __x) noexcept;

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
  reduce_min_index(same_as<bool> auto __x) noexcept;

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
  reduce_max_index(same_as<bool> auto __x) noexcept;

  template <typename _V, typename _Tp, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    simd_chunk(const basic_simd<_Tp, _Abi>& __x) noexcept;

  template <typename _M, size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    simd_chunk(const basic_simd_mask<_Bs, _Abi>& __x) noexcept;

  template <size_t _Np, typename _Tp, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    simd_chunk(const basic_simd<_Tp, _Abi>& __x) noexcept;

  template <size_t _Np, size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    simd_chunk(const basic_simd_mask<_Bs, _Abi>& __x) noexcept;

  template <typename _Tp, typename... _Abis>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
    simd<_Tp, (__simd_size_v<_Tp, _Abis> + ...)>
    simd_cat(const basic_simd<_Tp, _Abis>&... __xs) noexcept;

  template <size_t _Bs, typename... _Abis>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
    simd_mask<__detail::__mask_integer_from<_Bs>, (basic_simd_mask<_Bs, _Abis>::size.value + ...)>
    simd_cat(const basic_simd_mask<_Bs, _Abis>&... __xs) noexcept;

  namespace __detail
  {
    template <typename _BinaryOperation, typename _Tp>
      concept __binary_operation = requires (_BinaryOperation __binary_op, simd<_Tp, 1> __v) {
        { __binary_op(__v, __v) } -> same_as<simd<_Tp, 1>>;
      };

    template <typename _Tp, typename _BinaryOperation>
      requires same_as<_BinaryOperation, plus<>>
        or same_as<_BinaryOperation, multiplies<>>
        or same_as<_BinaryOperation, bit_and<>>
        or same_as<_BinaryOperation, bit_or<>>
        or same_as<_BinaryOperation, bit_xor<>>
      constexpr _Tp __default_identity_element(); /*= [] {
        static_assert(false, "You need to provide an identity element on masked reduce with custom "
                             "binary operation.");
      }();*/
  }

  template <typename _Tp, typename _Abi,
            __detail::__binary_operation<_Tp> _BinaryOperation = plus<>>
    constexpr _Tp
    reduce(const basic_simd<_Tp, _Abi>& __x, _BinaryOperation __binary_op = {});

  template <typename _Tp, typename _Abi,
            __detail::__binary_operation<_Tp> _BinaryOperation = plus<>>
    constexpr _Tp
    reduce(const basic_simd<_Tp, _Abi>& __x, const typename basic_simd<_Tp, _Abi>::mask_type& __k,
           _BinaryOperation __binary_op = {},
           __type_identity_t<_Tp> __identity_element
             = __detail::__default_identity_element<_Tp, _BinaryOperation>());

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr _Tp
    reduce_min(const basic_simd<_Tp, _Abi>& __x) noexcept;

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr _Tp
    reduce_min(const basic_simd<_Tp, _Abi>& __x,
               const typename basic_simd<_Tp, _Abi>::mask_type& __k) noexcept;

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr _Tp
    reduce_max(const basic_simd<_Tp, _Abi>& __x) noexcept;

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr _Tp
    reduce_max(const basic_simd<_Tp, _Abi>& __x,
               const typename basic_simd<_Tp, _Abi>::mask_type& __k) noexcept;

  // [simd.alg], Algorithms
  template <totally_ordered _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    min(const basic_simd<_Tp, _Abi>& __a, const basic_simd<_Tp, _Abi>& __b) noexcept;

  template <totally_ordered _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    max(const basic_simd<_Tp, _Abi>& __a, const basic_simd<_Tp, _Abi>& __b) noexcept;

  template <totally_ordered _Tp, typename _Abi>
    constexpr pair<basic_simd<_Tp, _Abi>, basic_simd<_Tp, _Abi>>
    minmax(const basic_simd<_Tp, _Abi>& __a, const basic_simd<_Tp, _Abi>& __b) noexcept;

  template <totally_ordered _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    clamp(const basic_simd<_Tp, _Abi>& __v, const basic_simd<_Tp, _Abi>& __lo,
          const basic_simd<_Tp, _Abi>& __hi);

  template <typename _Tp, typename _Up>
    constexpr auto
    simd_select(bool __c, const _Tp& __a, const _Up& __b)
    -> remove_cvref_t<decltype(__c ? __a : __b)>;

  template <size_t _Bytes, typename _Abi, typename _Tp, typename _Up>
    constexpr auto
    simd_select(const basic_simd_mask<_Bytes, _Abi>& __k, const _Tp& __a, const _Up& __b) noexcept
    -> decltype(__select_impl(__k, __a, __b));
}

#endif  // PROTOTYPE_FWDDECL_H_
