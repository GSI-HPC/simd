/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_META_H_
#define PROTOTYPE_SIMD_META_H_

#include "fwddecl.h"
#include "flags.h"

namespace std::__detail
{
  template <typename _Tp>
    struct __assert_unreachable
    { static_assert(!is_same_v<_Tp, _Tp>, "this should be unreachable"); };

  template <typename _Tp>
    concept __arithmetic = integral<_Tp> || floating_point<_Tp>;

  template <typename _Tp, size_t _Np = 0>
    concept __array_type
      = same_as<_Tp, array<typename _Tp::value_type, _Np == 0 ? tuple_size_v<_Tp> : _Np>>;

  template <typename _Abi>
    concept __simd_abi_tag
      = not _Abi::template _IsValid<void>::value
          and requires { typename _Abi::_IsValidAbiTag; };

  template <typename _Abi, typename _Tp>
    concept __valid_abi_tag
      = __simd_abi_tag<_Abi> and _Abi::template _IsValid<_Tp>::value;

  static_assert(not __valid_abi_tag<int, int>);

  template<class T>
    concept __constexpr_wrapper_like
      = convertible_to<T, decltype(T::value)>
          and equality_comparable_with<T, decltype(T::value)>
          and bool_constant<T() == T::value>::value
          and bool_constant<static_cast<decltype(T::value)>(T()) == T::value>::value;

  template <typename _Tp, typename _Up>
    concept __bitcast_convertible
      = __vectorizable<_Tp> and __vectorizable<_Up> and not std::same_as<_Tp, _Up>
          and sizeof(_Tp) == sizeof(_Up)
          and ((integral<_Tp> and integral<_Up>)
                 or (floating_point<_Tp> and floating_point<_Up>));


#ifdef __clang__
  template <typename _Tp>
    static constexpr remove_cvref_t<_Tp> __static_sized_range_obj = {};
#endif

  template <typename _Tp, size_t _Np = 0>
    concept __static_sized_range
      = ranges::contiguous_range<_Tp> and ranges::sized_range<_Tp>
          and requires(_Tp&& __r) {
#if 0 // PR117849
            typename integral_constant<size_t, ranges::size(__r)>;
#else
            requires (decltype(std::span(__r))::extent != dynamic_extent);
#endif
#ifdef __clang__
            requires (_Np == 0 or ranges::size(__static_sized_range_obj<_Tp>) == _Np);
#else
            requires (_Np == 0 or ranges::size(__r) == _Np);
#endif
          };

  template <typename _From, typename _To>
    concept __arithmetic_only_value_preserving_convertible_to
      = convertible_to<_From, _To> and __arithmetic<_From> and __arithmetic<_To>
          and not (is_signed_v<_From> and is_unsigned_v<_To>)
          and numeric_limits<_From>::digits <= numeric_limits<_To>::digits
          and numeric_limits<_From>::max() <= numeric_limits<_To>::max()
          and numeric_limits<_From>::lowest() >= numeric_limits<_To>::lowest();

  template <typename _From, typename _To>
    concept __value_preserving_convertible_to
      = __arithmetic_only_value_preserving_convertible_to<_From, _To>
          or (__complex_like<_To> and __arithmetic_only_value_preserving_convertible_to<
                                        _From, typename _To::value_type>);

  template <typename _From, typename _To>
    concept __non_narrowing_constexpr_conversion
      = __constexpr_wrapper_like<_From> and convertible_to<_From, _To>
          and requires { { _From::value } -> std::convertible_to<_To>; }
          and static_cast<decltype(_From::value)>(_To(_From::value)) == _From::value
          and not (std::unsigned_integral<_To> and _From::value < decltype(_From::value)())
          and _From::value <= std::numeric_limits<_To>::max()
          and _From::value >= std::numeric_limits<_To>::lowest();

  template <typename _From, typename _To>
    concept __broadcast_constructible
      = convertible_to<_From, _To> // 4
          and ((not __arithmetic<remove_cvref_t<_From>>
                  and not __constexpr_wrapper_like<remove_cvref_t<_From>>) // 4.1
                 or __value_preserving_convertible_to<remove_cvref_t<_From>, _To> // 4.2
                 or __non_narrowing_constexpr_conversion<remove_cvref_t<_From>, _To>); // 4.3

  // __higher_floating_point_rank_than<_Tp, U> (_Tp has higher or equal floating point rank than U)
  template <typename _From, typename _To>
    concept __higher_floating_point_rank_than
      = floating_point<_From> && floating_point<_To>
          && same_as<common_type_t<_From, _To>, _From>;

  // __higher_integer_rank_than<_Tp, U> (_Tp has higher or equal integer rank than U)
  template <typename _From, typename _To>
    concept __higher_integer_rank_than
      = integral<_From> && integral<_To>
          && (sizeof(_From) > sizeof(_To) || same_as<common_type_t<_From, _To>, _From>);

  template <typename _From, typename _To>
    concept __higher_rank_than
      = __higher_floating_point_rank_than<_From, _To> || __higher_integer_rank_than<_From, _To>;

  template <typename _From, typename _To, typename... _Flags>
    concept __loadstore_convertible_to
      = same_as<_From, _To>
          or (__vectorizable<_From> and __vectorizable<_To>
                and (__value_preserving_convertible_to<_From, _To>
#if SIMD_STD_BYTE
                       or (requires(_From __x) { static_cast<_To>(__x); }
#else
                       or (std::convertible_to<_From, _To>
#endif
                             and (std::same_as<_Flags, _Convert> or ...))));

  template <auto _Value>
    using _Ic = integral_constant<std::remove_const_t<decltype(_Value)>, _Value>;

  template <auto _Value>
    inline constexpr _Ic<_Value> __ic{};

  template <_SimdSizeType... _Is>
    using _SimdIndexSequence = std::integer_sequence<_SimdSizeType, _Is...>;

  template <_SimdSizeType _Np>
    using _MakeSimdIndexSequence = std::make_integer_sequence<_SimdSizeType, _Np>;

  template <typename _From, typename _To>
    concept __simd_generator_convertible_to
      = std::convertible_to<_From, _To>
          and (not __arithmetic<_From> or __value_preserving_convertible_to<_From, _To>);

  template <typename _Fp, typename _Tp, _SimdSizeType... _Is>
    requires (__simd_generator_convertible_to<decltype(declval<_Fp>()(__ic<_Is>)), _Tp> and ...)
    constexpr void
    __simd_generator_invokable_impl(_SimdIndexSequence<_Is...>);

  template <typename _Fp, typename _Tp, _SimdSizeType _Np>
    concept __simd_generator_invokable = requires {
      __simd_generator_invokable_impl<_Fp, _Tp>(_MakeSimdIndexSequence<_Np>());
    };

  template <typename _Fp, typename _Tp, _SimdSizeType... _Is>
    requires (not __simd_generator_convertible_to<decltype(declval<_Fp>()(__ic<_Is>)), _Tp> or ...)
    constexpr void
    __almost_simd_generator_invokable_impl(_SimdIndexSequence<_Is...>);

  template <typename _Fp, typename _Tp, _SimdSizeType _Np>
    concept __almost_simd_generator_invokable = requires(_Fp&& __gen) {
      __gen(__ic<0>);
      __almost_simd_generator_invokable_impl<_Fp, _Tp>(_MakeSimdIndexSequence<_Np>());
    };

  template <typename _Fp>
    concept __index_permutation_function_nosize = requires(_Fp const& __f)
      {
        { __f(0) } -> std::integral;
      };

  template <typename _Fp, typename _Simd>
    concept __index_permutation_function_size = requires(_Fp const& __f)
      {
        { __f(0, 0) } -> std::integral;
      };

  template <typename _Fp, typename _Simd>
    concept __index_permutation_function
      = __index_permutation_function_size<_Fp, _Simd> or __index_permutation_function_nosize<_Fp>;

  template <integral _Tp>
    _GLIBCXX_SIMD_INTRINSIC constexpr _Tp
    __div_roundup(_Tp __a, _Tp __b)
    { return (__a + __b - 1) / __b; }

#if SIMD_CONCEPTS
  template <typename T>
    concept __boolean_reducable_impl = requires(T&& x)
      {
        { datapar::all_of(x) } -> same_as<bool>;
        { datapar::none_of(x) } -> same_as<bool>;
        { datapar::any_of(x) } -> same_as<bool>;
        { datapar::reduce_count(x) } -> signed_integral;
        { datapar::reduce_min_index(x) } -> signed_integral;
        { datapar::reduce_max_index(x) } -> signed_integral;
      };

  template <typename T>
    concept __boolean_reducable = __boolean_reducable_impl<T> and requires(T&& x)
      {
        { !forward<T>(x) } -> __boolean_reducable_impl;
      };

  template <typename T, typename U>
    concept __simd_weakly_equality_comparable_with = requires(const remove_reference_t<T>& __t,
                                                              const remove_reference_t<U>& __u)
      {
        { __t == __u } -> __boolean_reducable;
        { __t != __u } -> __boolean_reducable;
        { __u == __t } -> __boolean_reducable;
        { __u != __t } -> __boolean_reducable;
      };

  template <typename _Tp, typename _Up>
    concept __simd_partially_ordered_with = requires(const remove_reference_t<_Tp>& __t,
                                                     const remove_reference_t<_Up>& __u)
      {
        { __t <  __u } -> __boolean_reducable;
        { __t >  __u } -> __boolean_reducable;
        { __t <= __u } -> __boolean_reducable;
        { __t >= __u } -> __boolean_reducable;
        { __u <  __t } -> __boolean_reducable;
        { __u >  __t } -> __boolean_reducable;
        { __u <= __t } -> __boolean_reducable;
        { __u >= __t } -> __boolean_reducable;
      };

  /* TODO
   * Do we need the common_reference checks for simd? The interesting scenarios must be user-defined
   * types that are convertible to simd, no? (operator simd() or derived from simd)
   */
  template <typename _Tp, typename _Up>
    concept __simd_comparison_common_type_with
      = common_reference_with<const remove_reference_t<_Tp>&, const remove_reference_t<_Up>&>;
}

namespace std::datapar
{
  template <typename _Tp>
    concept integral = __detail::__simd_integral<_Tp>;

  template <typename _Tp>
    concept signed_integral = __detail::__simd_signed_integral<_Tp>;

  template <typename _Tp>
    concept unsigned_integral = __detail::__simd_unsigned_integral<_Tp>;

  template <typename _Tp>
    concept floating_point = __detail::__simd_floating_point<_Tp>;

  template <typename _Tp>
    concept arithmetic = integral<_Tp> or floating_point<_Tp>;

  template <typename _Tp>
    concept equality_comparable
      = __detail::__simd_weakly_equality_comparable_with<_Tp, _Tp>;

  template <typename _Tp, typename _Up>
    concept equality_comparable_with
      = equality_comparable<_Tp> and equality_comparable<_Up>
          and __detail::__simd_comparison_common_type_with<_Tp, _Up>
          and equality_comparable<common_reference_t<const remove_reference_t<_Tp>&,
                                                     const remove_reference_t<_Up>&>>
          and __detail::__simd_weakly_equality_comparable_with<_Tp, _Up>;

  template <typename _Tp>
    concept totally_ordered
      = equality_comparable<_Tp> and __detail::__simd_partially_ordered_with<_Tp, _Tp>;

  template <typename _Tp, typename _Up>
    concept totally_ordered_with
      = totally_ordered<_Tp> and totally_ordered<_Up>
          and equality_comparable_with<_Tp, _Up>
          and totally_ordered<common_reference_t<const remove_reference_t<_Tp>&,
                                                 const remove_reference_t<_Up>&>>
          and __detail::__simd_partially_ordered_with<_Tp, _Up>;

  template <typename _Tp>
    concept regular = semiregular<_Tp> and equality_comparable<_Tp>;
#endif
}

#endif  // PROTOTYPE_SIMD_META_H_
