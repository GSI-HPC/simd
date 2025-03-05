/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_MATH_H_
#define PROTOTYPE_SIMD_MATH_H_

#include "simd.h"
#include <cmath>

namespace std::__detail
{
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
      = (ext::simd_floating_point<__deduced_simd_t<_Ts>> or ...);

  template <typename... _Ts>
    struct __math_common_simd;

  template <typename... _Ts>
    requires __math_floating_point<_Ts...>
    using __math_common_simd_t = typename __math_common_simd<_Ts...>::type;

  template <typename _T0>
    requires __math_floating_point<_T0>
    struct __math_common_simd<_T0>
    { using type = __deduced_simd_t<_T0>; };

  template <typename _T0>
    requires (not __math_floating_point<_T0>)
    struct __math_common_simd<_T0>
    { using type = _T0; };

  template <typename _T0, typename _T1>
    requires __math_floating_point<_T0, _T1>
    struct __math_common_simd<_T0, _T1>
    : std::common_type<typename __math_common_simd<_T0>::type,
                       typename __math_common_simd<_T1>::type>
    {};

  template <typename _T0, typename _T1, typename _T2, typename... _Ts>
    requires requires { typename __math_common_simd<_T0, _T1>::type; }
    struct __math_common_simd<_T0, _T1, _T2, _Ts...>
    : std::common_type<typename __math_common_simd<_T0, _T1>::type, _T2, _Ts...>
    {};

  template <typename _T0, typename _T1, typename _T2, typename... _Ts>
    requires (not requires { typename __math_common_simd<_T0, _T1>::type; })
    struct __math_common_simd<_T0, _T1, _T2, _Ts...>
    : std::common_type<typename __math_common_simd<_T2, _Ts...>::type, _T0, _T1>
    {};
}

#define _GLIBCXX_SIMD_MATH_1ARG(name)                                                              \
namespace std::datapar                                                                             \
{                                                                                                  \
  template <__detail::__math_floating_point _Up>                                                   \
    _GLIBCXX_ALWAYS_INLINE constexpr __detail::__deduced_simd_t<_Up>                               \
    name(const _Up& __xx)                                                                          \
    {                                                                                              \
      using _Vp = __detail::__deduced_simd_t<_Up>;                                                 \
      using _Tp [[maybe_unused]] = __detail::__canonical_vec_type_t<typename _Vp::value_type>;     \
      const _Vp& __x = __xx;                                                                       \
      if consteval                                                                                 \
        {                                                                                          \
          return _Vp([&] (int __i) {                                                               \
                   if constexpr (sizeof(__x[0]) == sizeof(float))                                  \
                     return __builtin_##name##f(__x[__i]);                                         \
                   else if constexpr (sizeof(__x[0]) == sizeof(double))                            \
                     return __builtin_##name(__x[__i]);                                            \
                   else                                                                            \
                     static_assert(false);                                                         \
                 });                                                                               \
        }                                                                                          \
      else                                                                                         \
        {                                                                                          \
          if constexpr (requires { _Vp::_Impl::_S_##name(__x._M_data); })                          \
            return _Vp::_Impl::_S_##name(__x._M_data);                                             \
          else if constexpr (requires { typename _Vp::abi_type::_AbiCombineTag; })                 \
            {                                                                                      \
              using _Tup = typename _Vp::abi_type::template _SimdMember<_Tp>;                      \
              return _Vp(__detail::__private_init,                                                 \
                         _Tup::_S_generate_persimd(                                                \
                           [&] [[gnu::always_inline]]                                              \
                             (__detail::__constexpr_wrapper_like auto __i) {                       \
                             return __data(name(__x._M_data._M_simd_at(__i)));                     \
                           }));                                                                    \
            }                                                                                      \
          else if constexpr (requires { typename _Vp::abi_type::_Abi0Type; })                      \
            {                                                                                      \
              using _VPart = basic_simd<_Tp, typename _Vp::abi_type::_Abi0Type>;                   \
              _Vp __r;                                                                             \
              const auto& __arr0 = __x._M_data;                                                    \
              _GLIBCXX_SIMD_INT_PACK(__arr0.size(), _Is, {                                         \
                ((__r._M_data[_Is] = name(_VPart(__arr0[_Is]))._M_data), ...);                     \
              });                                                                                  \
              return __r;                                                                          \
            }                                                                                      \
          else                                                                                     \
            {                                                                                      \
              using _VB = typename _Vp::_MemberType;                                               \
              return _GLIBCXX_SIMD_INT_PACK(_Vp::size(), _Is, {                                    \
                     if constexpr (sizeof(_Tp) == sizeof(float))                                   \
                       return _VB{__builtin_##name##f(__x[_Is])...};                               \
                     else if constexpr (sizeof(_Tp) == sizeof(double))                             \
                       return _VB{__builtin_##name(__x[_Is])...};                                  \
                     else                                                                          \
                       static_assert(false);                                                       \
                   });                                                                             \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
}                                                                                                  \
namespace std                                                                                      \
{                                                                                                  \
  using std::datapar::name;                                                                        \
}

#define _GLIBCXX_SIMD_MATH_2ARG(name)                                                              \
namespace std::datapar                                                                             \
{                                                                                                  \
  template <typename _V0, typename _V1>                                                            \
    _GLIBCXX_ALWAYS_INLINE constexpr __detail::__math_common_simd_t<_V0, _V1>                      \
    name(const _V0& __x0, const _V1& __x1)                                                         \
    {                                                                                              \
      using _Vp = __detail::__math_common_simd_t<_V0, _V1>;                                        \
      using _Tp [[maybe_unused]] = __detail::__canonical_vec_type_t<typename _Vp::value_type>;     \
      const _Vp& __x = __x0;                                                                       \
      const _Vp& __y = __x1;                                                                       \
      if consteval                                                                                 \
        {                                                                                          \
          return _Vp([&] (int __i) {                                                               \
                   if constexpr (sizeof(__x[0]) == sizeof(float))                                  \
                     return __builtin_##name##f(__x[__i], __y[__i]);                               \
                   else if constexpr (sizeof(__x[0]) == sizeof(double))                            \
                     return __builtin_##name(__x[__i], __y[__i]);                                  \
                   else                                                                            \
                     static_assert(false);                                                         \
                 });                                                                               \
        }                                                                                          \
      else                                                                                         \
        {                                                                                          \
          if constexpr (requires { _Vp::_Impl::_S_##name(__x._M_data, __y._M_data); })             \
            return _Vp::_Impl::_S_##name(__x._M_data, __y._M_data);                                \
          else if constexpr (requires { typename _Vp::abi_type::_AbiCombineTag; })                 \
            {                                                                                      \
              using _Tup = typename _Vp::abi_type::template _SimdMember<_Tp>;                      \
              return _Vp(__detail::__private_init,                                                 \
                         _Tup::_S_generate_persimd(                                                \
                           [&] [[gnu::always_inline]]                                              \
                             (__detail::__constexpr_wrapper_like auto __i) {                       \
                             return __data(name(__x._M_data._M_simd_at(__i),                       \
                                                __y._M_data._M_simd_at(__i)));                     \
                           }));                                                                    \
            }                                                                                      \
          else if constexpr (requires { typename _Vp::abi_type::_Abi0Type; })                      \
            {                                                                                      \
              using _VPart = basic_simd<_Tp, typename _Vp::abi_type::_Abi0Type>;                   \
              _Vp __r;                                                                             \
              const auto& __arr0 = __x._M_data;                                                    \
              const auto& __arr1 = __y._M_data;                                                    \
              _GLIBCXX_SIMD_INT_PACK(__arr0.size(), _Is, {                                         \
                ((__r._M_data[_Is] = name(_VPart(__arr0[_Is]), _VPart(__arr1[_Is]))._M_data), ...);\
              });                                                                                  \
              return __r;                                                                          \
            }                                                                                      \
          else                                                                                     \
            {                                                                                      \
              using _VB = typename _Vp::_MemberType;                                               \
              return _GLIBCXX_SIMD_INT_PACK(_Vp::size(), _Is, {                                    \
                     if constexpr (sizeof(_Tp) == sizeof(float))                                   \
                       return _VB{__builtin_##name##f(__x[_Is], __y[_Is])...};                     \
                     else if constexpr (sizeof(_Tp) == sizeof(double))                             \
                       return _VB{__builtin_##name(__x[_Is], __y[_Is])...};                        \
                     else                                                                          \
                       static_assert(false);                                                       \
                   });                                                                             \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
}                                                                                                  \
namespace std                                                                                      \
{                                                                                                  \
  using std::datapar::name;                                                                        \
}

#define _GLIBCXX_SIMD_MATH_3ARG(name)                                                              \
namespace std::datapar                                                                             \
{                                                                                                  \
  template <typename _V0, typename _V1, typename _V2>                                              \
    _GLIBCXX_ALWAYS_INLINE constexpr __detail::__math_common_simd_t<_V0, _V1, _V2>                 \
    name(const _V0& __x0, const _V1& __x1, const _V2& __x2)                                        \
    {                                                                                              \
      using _Vp = __detail::__math_common_simd_t<_V0, _V1, _V2>;                                   \
      using _Tp [[maybe_unused]] = __detail::__canonical_vec_type_t<typename _Vp::value_type>;     \
      const _Vp& __x = __x0;                                                                       \
      const _Vp& __y = __x1;                                                                       \
      const _Vp& __z = __x2;                                                                       \
      if consteval                                                                                 \
        {                                                                                          \
           return _Vp([&] (int __i) {                                                              \
                    if constexpr (sizeof(__x[0]) == sizeof(float))                                 \
                      return __builtin_##name##f(__x[__i], __y[__i], __z[__i]);                    \
                    else if constexpr (sizeof(__x[0]) == sizeof(double))                           \
                      return __builtin_##name(__x[__i], __y[__i], __z[__i]);                       \
                    else                                                                           \
                      static_assert(false);                                                        \
                  });                                                                              \
         }                                                                                         \
      else                                                                                         \
        {                                                                                          \
          if constexpr (requires { _Vp::_Impl::_S_##name(__x._M_data, __y._M_data, __z._M_data); })\
            return _Vp::_Impl::_S_##name(__x._M_data, __y._M_data, __z._M_data);                   \
          else if constexpr (requires { typename _Vp::abi_type::_AbiCombineTag; })                 \
            {                                                                                      \
              using _Tup = typename _Vp::abi_type::template _SimdMember<_Tp>;                      \
              return _Vp(__detail::__private_init,                                                 \
                         _Tup::_S_generate_persimd(                                                \
                           [&] [[gnu::always_inline]]                                              \
                             (__detail::__constexpr_wrapper_like auto __i) {                       \
                             return __data(name(__x._M_data._M_simd_at(__i),                       \
                                                __y._M_data._M_simd_at(__i),                       \
                                                __z._M_data._M_simd_at(__i)));                     \
                           }));                                                                    \
            }                                                                                      \
          else if constexpr (requires { typename _Vp::abi_type::_Abi0Type; })                      \
            {                                                                                      \
              using _VPart = basic_simd<_Tp, typename _Vp::abi_type::_Abi0Type>;                   \
              _Vp __r;                                                                             \
              const auto& __arr0 = __x._M_data;                                                    \
              const auto& __arr1 = __y._M_data;                                                    \
              const auto& __arr2 = __z._M_data;                                                    \
              _GLIBCXX_SIMD_INT_PACK(__arr0.size(), _Is, {                                         \
                ((__r._M_data[_Is] = name(_VPart(__arr0[_Is]), _VPart(__arr1[_Is]),                \
                                          _VPart(__arr2[_Is]))._M_data), ...);                     \
              });                                                                                  \
              for (size_t __i = 0; __i < __arr0.size(); ++__i)                                     \
                __r._M_data[__i] = name(_VPart(__arr0[__i]), _VPart(__arr1[__i]),                  \
                                               _VPart(__arr2[__i]))._M_data;                       \
              return __r;                                                                          \
            }                                                                                      \
          else                                                                                     \
            {                                                                                      \
              using _VB = typename _Vp::_MemberType;                                               \
              return _GLIBCXX_SIMD_INT_PACK(_Vp::size(), _Is, {                                    \
                     if constexpr (sizeof(_Tp) == sizeof(float))                                   \
                       return _VB{__builtin_##name##f(__x[_Is], __y[_Is], __z[_Is])...};           \
                     else if constexpr (sizeof(_Tp) == sizeof(double))                             \
                       return _VB{__builtin_##name(__x[_Is], __y[_Is], __z[_Is])...};              \
                     else                                                                          \
                       static_assert(false);                                                       \
                   });                                                                             \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
}                                                                                                  \
namespace std                                                                                      \
{                                                                                                  \
  using std::datapar::name;                                                                        \
}

_GLIBCXX_SIMD_MATH_1ARG(acos)
_GLIBCXX_SIMD_MATH_1ARG(asin)
_GLIBCXX_SIMD_MATH_1ARG(atan)
_GLIBCXX_SIMD_MATH_2ARG(atan2)
_GLIBCXX_SIMD_MATH_1ARG(cos)
_GLIBCXX_SIMD_MATH_1ARG(sin)
_GLIBCXX_SIMD_MATH_1ARG(tan)
_GLIBCXX_SIMD_MATH_1ARG(acosh)
_GLIBCXX_SIMD_MATH_1ARG(asinh)
_GLIBCXX_SIMD_MATH_1ARG(atanh)
_GLIBCXX_SIMD_MATH_1ARG(cosh)
_GLIBCXX_SIMD_MATH_1ARG(sinh)
_GLIBCXX_SIMD_MATH_1ARG(tanh)
_GLIBCXX_SIMD_MATH_1ARG(exp)
_GLIBCXX_SIMD_MATH_1ARG(exp2)
_GLIBCXX_SIMD_MATH_1ARG(expm1)
_GLIBCXX_SIMD_MATH_1ARG(log)
_GLIBCXX_SIMD_MATH_1ARG(log10)
_GLIBCXX_SIMD_MATH_1ARG(log1p)
_GLIBCXX_SIMD_MATH_1ARG(log2)
_GLIBCXX_SIMD_MATH_1ARG(logb)
_GLIBCXX_SIMD_MATH_1ARG(cbrt)
_GLIBCXX_SIMD_MATH_1ARG(sqrt)
_GLIBCXX_SIMD_MATH_1ARG(abs) // FIXME: there's no __builtin_abs
_GLIBCXX_SIMD_MATH_1ARG(fabs)
_GLIBCXX_SIMD_MATH_2ARG(hypot)
_GLIBCXX_SIMD_MATH_3ARG(hypot)
_GLIBCXX_SIMD_MATH_2ARG(pow)
_GLIBCXX_SIMD_MATH_1ARG(erf)
_GLIBCXX_SIMD_MATH_1ARG(erfc)
_GLIBCXX_SIMD_MATH_1ARG(lgamma)
_GLIBCXX_SIMD_MATH_1ARG(tgamma)
_GLIBCXX_SIMD_MATH_1ARG(ceil)
_GLIBCXX_SIMD_MATH_1ARG(floor)
_GLIBCXX_SIMD_MATH_1ARG(round)
_GLIBCXX_SIMD_MATH_1ARG(trunc)
_GLIBCXX_SIMD_MATH_2ARG(fmod)
_GLIBCXX_SIMD_MATH_2ARG(remainder)
_GLIBCXX_SIMD_MATH_2ARG(copysign)
_GLIBCXX_SIMD_MATH_2ARG(nextafter)
_GLIBCXX_SIMD_MATH_2ARG(fdim)
_GLIBCXX_SIMD_MATH_2ARG(fmax)
_GLIBCXX_SIMD_MATH_2ARG(fmin)
_GLIBCXX_SIMD_MATH_3ARG(fma)
_GLIBCXX_SIMD_MATH_3ARG(lerp) // missing noexcept

#undef _GLIBCXX_SIMD_MATH_1ARG
#undef _GLIBCXX_SIMD_MATH_2ARG
#undef _GLIBCXX_SIMD_MATH_3ARG

#define _GLIBCXX_SIMD_MATH_CLASSIFICATION_1ARG(name)                                               \
namespace std::datapar                                                                             \
{                                                                                                  \
  template <__detail::__math_floating_point _Up>                                                   \
    _GLIBCXX_ALWAYS_INLINE constexpr typename __detail::__deduced_simd_t<_Up>::mask_type           \
    name(const _Up& __xx)                                                                          \
    {                                                                                              \
      using _Vp = __detail::__deduced_simd_t<_Up>;                                                 \
      using _Kp = typename _Vp::mask_type;                                                         \
      using _Tp [[maybe_unused]] = typename _Vp::value_type;                                       \
      const _Vp& __x = __xx;                                                                       \
      if consteval                                                                                 \
        {                                                                                          \
          return _Kp([&] (int __i) -> bool {                                                       \
                   if constexpr (sizeof(__x[0]) == sizeof(float))                                  \
                     return __builtin_##name##f(__x[__i]);                                         \
                   else if constexpr (sizeof(__x[0]) == sizeof(double))                            \
                     return __builtin_##name(__x[__i]);                                            \
                   else                                                                            \
                     static_assert(false);                                                         \
                 });                                                                               \
        }                                                                                          \
      else                                                                                         \
        {                                                                                          \
          return _Kp(__detail::__private_init, _Vp::_Impl::_S_##name(__x._M_data));                \
        }                                                                                          \
    }                                                                                              \
}                                                                                                  \
namespace std                                                                                      \
{                                                                                                  \
  using std::datapar::name;                                                                        \
}

_GLIBCXX_SIMD_MATH_CLASSIFICATION_1ARG(isfinite)
_GLIBCXX_SIMD_MATH_CLASSIFICATION_1ARG(isinf)
_GLIBCXX_SIMD_MATH_CLASSIFICATION_1ARG(isnan)
_GLIBCXX_SIMD_MATH_CLASSIFICATION_1ARG(isnormal)
_GLIBCXX_SIMD_MATH_CLASSIFICATION_1ARG(signbit)

#undef _GLIBCXX_SIMD_MATH_CLASSIFICATION_1ARG


// the following depend on the global rounding mode (not constexpr):
//template<@\mathfloatingpoint@ V> @\deducedsimd@<V> nearbyint(const V& x);
//template<@\mathfloatingpoint@ V> @\deducedsimd@<V> rint(const V& x);
//template<@\mathfloatingpoint@ V> rebind_t<long int, @\deducedsimd@<V>> lrint(const V& x);
//template<@\mathfloatingpoint@ V> rebind_t<long long int, V> llrint(const @\deducedsimd@<V>& x);

#if 0
frexp(const V& value, rebind_t<int, @\deducedsimd@<V>>* exp);
constexpr rebind_t<int, @\deducedsimd@<V>> ilogb(const V& x);
constexpr @\deducedsimd@<V> ldexp(const V& x, const rebind_t<int, @\deducedsimd@<V>>& exp);
constexpr basic_simd<T, Abi> modf(const type_identity_t<basic_simd<T, Abi>>& value, basic_simd<T, Abi>* iptr);
constexpr @\deducedsimd@<V> scalbn(const V& x, const rebind_t<int, @\deducedsimd@<V>>& n);
constexpr @\deducedsimd@<V> scalbln(const V& x, const rebind_t<long int, @\deducedsimd@<V>>& n);
template<signed_integral T, class Abi> constexpr basic_simd<T, Abi> abs(const basic_simd<T, Abi>& j);

  template<@\mathfloatingpoint@ V> constexpr rebind_t<long int, @\deducedsimd@<V>> lround(const V& x);
  template<@\mathfloatingpoint@ V> constexpr rebind_t<long long int, @\deducedsimd@<V>> llround(const V& x);
  template<class V0, class V1> constexpr @\mathcommonsimd@<V0, V1> remquo(const V0& x, const V1& y, rebind_t<int, @\mathcommonsimd@<V0, V1>>* quo);
  template<@\mathfloatingpoint@ V> constexpr rebind_t<int, @\deducedsimd@<V>> fpclassify(const V& x);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type isgreater(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type isgreaterequal(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type isless(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type islessequal(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type islessgreater(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type isunordered(const V0& x, const V1& y);

  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> assoc_laguerre(const rebind_t<unsigned, @\deducedsimd@<V>>& n, const
      rebind_t<unsigned, @\deducedsimd@<V>>& m,
                     const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> assoc_legendre(const rebind_t<unsigned, @\deducedsimd@<V>>& l, const
      rebind_t<unsigned, @\deducedsimd@<V>>& m,
                     const V& x);
  template<class V0, class V1>
    @\mathcommonsimd@<V0, V1> beta(const V0& x, const V1& y);
  template<@\mathfloatingpoint@ V> @\deducedsimd@<V> comp_ellint_1(const V& k);
  template<@\mathfloatingpoint@ V> @\deducedsimd@<V> comp_ellint_2(const V& k);
  template<class V0, class V1>
    @\mathcommonsimd@<V0, V1> comp_ellint_3(const V0& k, const V1& nu);
  template<class V0, class V1>
    @\mathcommonsimd@<V0, V1> cyl_bessel_i(const V0& nu, const V1& x);
  template<class V0, class V1>
    @\mathcommonsimd@<V0, V1> cyl_bessel_j(const V0& nu, const V1& x);
  template<class V0, class V1>
    @\mathcommonsimd@<V0, V1> cyl_bessel_k(const V0& nu, const V1& x);
  template<class V0, class V1>
    @\mathcommonsimd@<V0, V1> cyl_neumann(const V0& nu, const V1& x);
  template<class V0, class V1>
    @\mathcommonsimd@<V0, V1> ellint_1(const V0& k, const V1& phi);
  template<class V0, class V1>
    @\mathcommonsimd@<V0, V1> ellint_2(const V0& k, const V1& phi);
  template<class V0, class V1, class V2>
    @\mathcommonsimd@<V0, V1, V2> ellint_3(const V0& k, const V1& nu, const V2& phi);
  template<@\mathfloatingpoint@ V> @\deducedsimd@<V> expint(const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> hermite(const rebind_t<unsigned, @\deducedsimd@<V>>& n, const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> laguerre(const rebind_t<unsigned, @\deducedsimd@<V>>& n, const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> legendre(const rebind_t<unsigned, @\deducedsimd@<V>>& l, const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> riemann_zeta(const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> sph_bessel(const rebind_t<unsigned, @\deducedsimd@<V>>& n, const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> sph_legendre(const rebind_t<unsigned, @\deducedsimd@<V>>& l,
      const rebind_t<unsigned, @\deducedsimd@<V>>& m, const V& theta);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V>
      sph_neumann(const rebind_t<unsigned, @\deducedsimd@<V>>& n, const V& x);
#endif

#endif  // PROTOTYPE_SIMD_MATH_H_
