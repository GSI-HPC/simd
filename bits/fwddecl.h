/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_FWDDECL_H_
#define PROTOTYPE_FWDDECL_H_

//#include <experimental/bits/simd_detail.h>
#include "simd_config.h"

#include <complex> // needed for datapar::polar(r, theta) and nothing else!
#include <functional>
#include <stdfloat>
#include <type_traits>
#include <ranges>

namespace std
{
  struct _ScalarAbi;

  template <int _Width>
    struct _VecAbi;

  template <int _Width>
    struct _Avx512Abi;

  template <typename _Abi0, int _Np>
    struct _AbiArray;

  template <int _Np, typename _Tag>
    struct _AbiCombine;
}

namespace std::__detail
{
  template <size_t _Bytes>
    struct __make_unsigned_int
    { using type = void; };

#ifdef __UINT8_TYPE__
  template <>
    struct __make_unsigned_int<sizeof(__UINT8_TYPE__)>
    { using type = __UINT8_TYPE__; };
#endif

#ifdef __UINT16_TYPE__
  template <>
    struct __make_unsigned_int<sizeof(__UINT16_TYPE__)>
    { using type = __UINT16_TYPE__; };
#endif

#ifdef __UINT32_TYPE__
  template <>
    struct __make_unsigned_int<sizeof(__UINT32_TYPE__)>
    { using type = __UINT32_TYPE__; };
#endif

#ifdef __UINT64_TYPE__
  template <size_t _Bytes>
    requires (_Bytes == sizeof(__UINT64_TYPE__))
      and (sizeof(__UINT64_TYPE__) == sizeof(unsigned long long))
    struct __make_unsigned_int<_Bytes>
    { using type = unsigned long long; };

  template <size_t _Bytes>
    requires (_Bytes == sizeof(__UINT64_TYPE__))
      and (sizeof(__UINT64_TYPE__) != sizeof(unsigned long long)) // impossible, right?
    struct __make_unsigned_int<_Bytes>
    { using type = __UINT64_TYPE__; };
#endif

  // definition in vec_detail.h
  struct alignas(16) _MaskUInt128;

  template <>
    struct __make_unsigned_int<16>
    { using type = _MaskUInt128; };

  template <size_t _Bytes>
    using _UInt = typename __make_unsigned_int<_Bytes>::type;

  template <typename _Tp>
    using __make_unsigned_int_t = typename __make_unsigned_int<sizeof(_Tp)>::type;

  template <typename _Tp>
    using __make_signed_int_t = typename __make_signed<__make_unsigned_int_t<_Tp>>::__type;

  template <size_t _Bs>
    using __mask_integer_from
      = typename __make_signed<typename __make_unsigned_int<_Bs>::type>::__type;

  template <typename _Cp, auto __re, auto __im, typename _Tp = typename _Cp::value_type>
    constexpr _Cp __complex_object = _Cp {_Tp(__re), _Tp(__im)};

  template <typename _Tp>
    concept __complex_like_impl
      = requires(_Tp __x) {
        typename _Tp::value_type;
        { __x.real() } -> same_as<typename _Tp::value_type>;
        { __x.imag() } -> same_as<typename _Tp::value_type>;
        { real(__x) } -> same_as<typename _Tp::value_type>;
        { imag(__x) } -> same_as<typename _Tp::value_type>;
        { +__x } -> same_as<_Tp>;
        { -__x } -> same_as<_Tp>;
        { __x + __x } -> same_as<_Tp>;
        { __x - __x } -> same_as<_Tp>;
        { __x * __x } -> same_as<_Tp>;
        { __x / __x } -> same_as<_Tp>;
        { __x += __x } -> same_as<_Tp&>;
        { __x -= __x } -> same_as<_Tp&>;
        { __x *= __x } -> same_as<_Tp&>;
        { __x /= __x } -> same_as<_Tp&>;
        { abs(__x) } -> same_as<typename _Tp::value_type>;
        { arg(__x) } -> same_as<typename _Tp::value_type>;
        { norm(__x) } -> same_as<typename _Tp::value_type>;
        { conj(__x) } -> same_as<_Tp>;
        { proj(__x) } -> same_as<_Tp>;
      }
          and (__complex_object<_Tp, 1, 2> + _Tp {} == __complex_object<_Tp, 1, 2>)
          and (__complex_object<_Tp, -1, 5> - __complex_object<_Tp, -1, 5> == _Tp {})
          and (__complex_object<_Tp, 2, 3> * __complex_object<_Tp, 1, 1>
                 == __complex_object<_Tp, -1, 5>)
          and (__complex_object<_Tp, 5, 5> / __complex_object<_Tp, 1, 2>
                 == __complex_object<_Tp, 3, -1>)
          and (conj(__complex_object<_Tp, 5, 3>) == __complex_object<_Tp, 5, -3>)
          // not constexpr: and (abs(__complex_object<_Tp, 3, 4>) == typename _Tp::value_type(5))
          and (norm(__complex_object<_Tp, 5, 5>) == typename _Tp::value_type(50));

  template <typename _Tp>
    concept __complex_like = __complex_like_impl<remove_cvref_t<_Tp>>;

  template <typename _Tp>
    struct __is_vectorizable
    : bool_constant<false>
    {};

#if SIMD_STD_BYTE
  template <> struct __is_vectorizable<std::byte> : bool_constant<true> {};
#endif

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

  // necessary / helpful for complex<double>:
  //template <> struct __is_vectorizable<  signed __int128> : bool_constant<true> {};
  //template <> struct __is_vectorizable<unsigned __int128> : bool_constant<true> {};

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

  // internal (for complex<double>)
  // DO NOT USE THIS OUTSIDE OF THE SIMD IMPLEMENTATION!
  template <> struct __is_vectorizable<_MaskUInt128> : bool_constant<true> {};

  // Also allows user-defined types that implement the std::complex interface
  template <__complex_like _Tp>
    struct __is_vectorizable<_Tp> : __is_vectorizable<typename _Tp::value_type> {};

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

  template <typename _Tp>
    struct __canonical_vec_type
    { using type = _Tp; };

  template <typename _Tp>
    using __canonical_vec_type_t = typename __canonical_vec_type<_Tp>::type;

  template <_SimdSizeType>
    class _NoAbiPreference;

  template <typename _Tp, _SimdSizeType _Np,
            template<_SimdSizeType> class _PrefAbi = _NoAbiPreference>
    struct _DeduceAbi;

  template <typename _Tp, _SimdSizeType _Np,
            template<_SimdSizeType> class _PrefAbi = _NoAbiPreference>
    using __deduce_t = typename _DeduceAbi<__canonical_vec_type_t<_Tp>, _Np, _PrefAbi>::type;

  template <typename _Tp>
    struct __is_simd
    : bool_constant<false>
    {};

  template <typename _Tp>
    inline constexpr bool __is_simd_v = __is_simd<_Tp>::value;

  template <typename _Tp>
    struct __is_mask
    : bool_constant<false>
    {};

  template <typename _Tp>
    inline constexpr bool __is_mask_v = __is_mask<_Tp>::value;

  template <typename _Tp, typename _Abi = __detail::_NativeAbi<_Tp>>
    struct __simd_size : integral_constant<__detail::_SimdSizeType, 0>
    {};

  template <typename _Tp, typename _Abi>
    inline constexpr __detail::_SimdSizeType __simd_size_v = __simd_size<_Tp, _Abi>::value;

  template <typename _Tp>
    concept __simd_type = __is_simd_v<_Tp>;

  template <typename _Tp>
    concept __mask_type = __is_mask_v<_Tp>;

  template <typename _Vp>
    concept __simd_or_mask = __simd_type<_Vp> or __mask_type<_Vp>;

  template <typename _Tp>
    concept __simd_integral
      = __simd_type<_Tp> and integral<typename _Tp::value_type>;

  template <typename _Tp>
    concept __simd_signed_integral
      = __simd_integral<_Tp> and is_signed_v<typename _Tp::value_type>;

  template <typename _Tp>
    concept __simd_unsigned_integral
      = __simd_integral<_Tp> and not __simd_signed_integral<_Tp>;

  template <typename _Tp>
    concept __simd_unsigned_integer
      = __simd_type<_Tp> and std::__unsigned_integer<typename _Tp::value_type>;

  template <typename _Tp, _SimdSizeType _Np = 0>
    concept __simd_floating_point
      = __simd_type<_Tp> and floating_point<typename _Tp::value_type>
          and (_Np == 0 or _Tp::size.value == _Np);

  template <typename _Tp>
    concept __simd_complex
      = __simd_type<_Tp> and __complex_like<typename _Tp::value_type>;

  template <typename _T0, typename _T1>
    concept __simd_matching_width
      = __simd_type<_T0> and __simd_type<_T1> and (_T0::size() == _T1::size())
          and (sizeof(typename _T0::value_type) == sizeof(typename _T1::value_type));
}

namespace std::datapar
{
  template <typename _Tp, typename _Abi = __detail::_NativeAbi<_Tp>>
    class basic_simd;

  template <size_t _Bytes,
            typename _Abi = __detail::_NativeAbi<__detail::__mask_integer_from<_Bytes>>>
    class basic_simd_mask;

  template <typename... _Flags>
    struct flags;

  template <typename _Tp, typename _Up = typename _Tp::value_type>
    struct alignment
    {};

  template <typename _Tp, typename _Up = typename _Tp::value_type>
    inline constexpr size_t alignment_v = alignment<_Tp, _Up>::value;

  template <typename _Tp, typename _Vp>
    struct rebind
    {};

  template <typename _Tp, typename _Vp>
    using rebind_t = typename rebind<_Tp, _Vp>::type;

  template <__detail::_SimdSizeType _Np, typename _Vp>
    struct resize
    {};

  template <__detail::_SimdSizeType _Np, typename _Vp>
    using resize_t = typename resize<_Np, _Vp>::type;

  template <typename _Tp,
            __detail::_SimdSizeType _Np = __detail::__simd_size_v<_Tp, __detail::_NativeAbi<_Tp>>>
    using simd = basic_simd<_Tp, __detail::__deduce_t<_Tp, _Np>>;

  template <typename _Tp,
            __detail::_SimdSizeType _Np = __detail::__simd_size_v<_Tp, __detail::_NativeAbi<_Tp>>>
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
    chunk(const basic_simd<_Tp, _Abi>& __x) noexcept;

  template <typename _M, size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    chunk(const basic_simd_mask<_Bs, _Abi>& __x) noexcept;

  template <size_t _Np, typename _Tp, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    chunk(const basic_simd<_Tp, _Abi>& __x) noexcept;

  template <size_t _Np, size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    chunk(const basic_simd_mask<_Bs, _Abi>& __x) noexcept;

  template <typename _Tp, typename... _Abis>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
    simd<_Tp, (__detail::__simd_size_v<_Tp, _Abis> + ...)>
    cat(const basic_simd<_Tp, _Abis>&... __xs) noexcept;

  template <size_t _Bs, typename... _Abis>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
    simd_mask<__detail::__mask_integer_from<_Bs>, (basic_simd_mask<_Bs, _Abis>::size.value + ...)>
    cat(const basic_simd_mask<_Bs, _Abis>&... __xs) noexcept;
}

namespace std::__detail
{
  template <typename _BinaryOperation, typename _Tp>
    concept __binary_operation
      = requires (_BinaryOperation __binary_op, std::datapar::simd<_Tp, 1> __v) {
        { __binary_op(__v, __v) } -> same_as<std::datapar::simd<_Tp, 1>>;
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

  template <typename _Tp>
    constexpr bool __is_simd_specialization = false;

  template <typename _Tp, typename _Abi>
    constexpr bool __is_simd_specialization<std::datapar::basic_simd<_Tp, _Abi>> = true;

  template <typename _Tp>
    using __plus_result_t = decltype(declval<const _Tp&>() + declval<const _Tp&>());

  template <typename _Tp>
    struct __deduced_simd
    { using type = void; };

  template <typename _Tp>
    requires __is_simd_specialization<__plus_result_t<_Tp>>
      and is_destructible_v<__plus_result_t<_Tp>>
    struct __deduced_simd<_Tp>
    { using type = __plus_result_t<_Tp>; };

  template <typename _Tp>
    using __deduced_simd_t = typename __deduced_simd<_Tp>::type;

  template <typename... _Ts>
    concept __math_floating_point
      = (__simd_floating_point<__deduced_simd_t<_Ts>> or ...);

  template <typename... _Ts>
    struct __math_common_simd;

  template <typename... _Ts>
    requires __math_floating_point<_Ts...>
    using __math_common_simd_t = typename __math_common_simd<_Ts...>::type;
}

namespace std::datapar
{
  template <typename _Tp, typename _Abi,
            __detail::__binary_operation<_Tp> _BinaryOperation = plus<>>
    constexpr _Tp
    reduce(const basic_simd<_Tp, _Abi>& __x, _BinaryOperation __binary_op = {});

  template <typename _Tp, typename _Abi,
            __detail::__binary_operation<_Tp> _BinaryOperation = plus<>>
    requires same_as<_BinaryOperation, plus<>>
      or same_as<_BinaryOperation, multiplies<>>
      or same_as<_BinaryOperation, bit_and<>>
      or same_as<_BinaryOperation, bit_or<>>
      or same_as<_BinaryOperation, bit_xor<>>
    constexpr _Tp
    reduce(const basic_simd<_Tp, _Abi>& __x, const typename basic_simd<_Tp, _Abi>::mask_type& __k,
           _BinaryOperation __binary_op = {})
    {
      return reduce(__x, __k, __binary_op,
                    __detail::__default_identity_element<_Tp, _BinaryOperation>());
    }

  template <typename _Tp, typename _Abi, __detail::__binary_operation<_Tp> _BinaryOperation>
    constexpr _Tp
    reduce(const basic_simd<_Tp, _Abi>& __x, const typename basic_simd<_Tp, _Abi>::mask_type& __k,
           _BinaryOperation __binary_op, __type_identity_t<_Tp> __identity_element);

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

  // [simd.math]
  template <typename _V0>
    constexpr typename __detail::__deduced_simd_t<_V0>::mask_type
    isfinite(const _V0& __x0);

  template <typename _V0, typename _V1>
    constexpr typename __detail::__math_common_simd_t<_V0, _V1>::mask_type
    isunordered(const _V0& __x, const _V1& __y);

  // [simd.alg], Algorithms
  template <std::totally_ordered _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    min(const basic_simd<_Tp, _Abi>& __a, const basic_simd<_Tp, _Abi>& __b) noexcept;

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    max(const basic_simd<_Tp, _Abi>& __a, const basic_simd<_Tp, _Abi>& __b) noexcept;

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr pair<basic_simd<_Tp, _Abi>, basic_simd<_Tp, _Abi>>
    minmax(const basic_simd<_Tp, _Abi>& __a, const basic_simd<_Tp, _Abi>& __b) noexcept;

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    clamp(const basic_simd<_Tp, _Abi>& __v, const basic_simd<_Tp, _Abi>& __lo,
          const basic_simd<_Tp, _Abi>& __hi);

  template <typename _Tp, typename _Up>
    constexpr auto
    select(bool __c, const _Tp& __a, const _Up& __b)
    -> remove_cvref_t<decltype(__c ? __a : __b)>;

  template <size_t _Bytes, typename _Abi, typename _Tp, typename _Up>
    constexpr auto
    select(const basic_simd_mask<_Bytes, _Abi>& __k, const _Tp& __a, const _Up& __b) noexcept
    -> decltype(__select_impl(__k, __a, __b));

  // [simd.bit]
  template<__detail::__simd_integral _Vp>
    constexpr _Vp
    byteswap(const _Vp& __v) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr _Vp
    bit_ceil(const _Vp& __v);

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr _Vp
    bit_floor(const _Vp& __v) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr typename _Vp::mask_type
    has_single_bit(const _Vp& __v) noexcept;

  template<__detail::__simd_unsigned_integer _V0, __detail::__simd_integral _V1>
    requires __detail::__simd_matching_width<_V0, _V1>
    constexpr _V0
    rotl(const _V0& __v, const _V1& __s) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr _Vp
    rotl(const _Vp& __v, int __s) noexcept;

  template<__detail::__simd_unsigned_integer _V0, __detail::__simd_integral _V1>
    requires __detail::__simd_matching_width<_V0, _V1>
    constexpr _V0
    rotr(const _V0& __v, const _V1& __s) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr _Vp
    rotr(const _Vp& __v, int __s) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    bit_width(const _Vp& __v) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_zero(const _Vp& __v) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_one(const _Vp& __v) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_zero(const _Vp& __v) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_one(const _Vp& __v) noexcept;

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    popcount(const _Vp& __v) noexcept;

  constexpr int zero_element = -1 << (sizeof(int) * __CHAR_BIT__ - 1);
  constexpr int uninit_element = zero_element + 1;

  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    real(const _Vp&) noexcept;

  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    imag(const _Vp&) noexcept;

  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    abs(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    arg(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    norm(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    conj(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    proj(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    exp(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    log(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    log10(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    sqrt(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    sin(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    asin(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    cos(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    acos(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    tan(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    atan(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    sinh(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    asinh(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    cosh(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    acosh(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    tanh(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    atanh(const _Vp&);

  template <__detail::__simd_floating_point _Vp>
    constexpr rebind_t<complex<typename _Vp::value_type>, _Vp>
    polar(const _Vp& __r, const _Vp& __theta = {});

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    pow(const _Vp& __x, const _Vp& __y);
}

namespace std
{
  // [simd.math]
  using datapar::isfinite;
  using datapar::isunordered;

  // [simd.alg]
  using datapar::min;
  using datapar::max;
  using datapar::minmax;
  using datapar::clamp;

  // [simd.bit]
  using datapar::byteswap;
  using datapar::bit_ceil;
  using datapar::bit_floor;
  using datapar::has_single_bit;
  using datapar::rotl;
  using datapar::rotr;
  using datapar::bit_width;
  using datapar::countl_zero;
  using datapar::countl_one;
  using datapar::countr_zero;
  using datapar::countr_one;
  using datapar::popcount;

  // [simd.complex.math]
  using datapar::real;
  using datapar::imag;
  using datapar::arg;
  using datapar::norm;
  using datapar::conj;
  using datapar::proj;
  using datapar::polar;
}

#endif  // PROTOTYPE_FWDDECL_H_
