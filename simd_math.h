/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2024      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_MATH_H_
#define PROTOTYPE_SIMD_MATH_H_

#include "simd.h"
#include <cmath>

namespace SIMD_NSPC
{
  namespace __detail
  {
    template <typename _Tp>
      constexpr bool __is_simd_specialization = false;

    template <typename _Tp, typename _Abi>
      constexpr bool __is_simd_specialization<basic_vec<_Tp, _Abi>> = true;

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
        = (SIMD_NSPC::floating_point<__deduced_simd_t<_Ts>> or ...);

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
}

#define _GLIBCXX_SIMD_MATH_1ARG(name)                                                              \
namespace SIMD_NSPC                                                                                \
{                                                                                                  \
  namespace __detail                                                                               \
  {                                                                                                \
    template <__vec_builtin _Vp, auto = _BuildFlags()>                                             \
      [[gnu::flatten,                                                                              \
        gnu::optimize("tree-vectorize", "no-math-errno", "unsafe-math-optimizations")]] _Vp        \
      __##name(_Vp __v)                                                                            \
      {                                                                                            \
        _Vp __ret;                                                                                 \
        _Pragma("GCC ivdep")                                                                       \
        for (int __i = 0; __i < __width_of<_Vp>; ++__i)                                            \
          {                                                                                        \
            if constexpr (sizeof(__v[0]) == sizeof(float))                                         \
              __ret[__i] = __builtin_##name##f(__v[__i]);                                          \
            else if constexpr (sizeof(__v[0]) == sizeof(double))                                   \
              __ret[__i] = __builtin_##name(__v[__i]);                                             \
            else                                                                                   \
              static_assert(false);                                                                \
          }                                                                                        \
        return __ret;                                                                              \
      }                                                                                            \
  }                                                                                                \
                                                                                                   \
  template <__detail::__math_floating_point _Up>                                                   \
    _GLIBCXX_ALWAYS_INLINE constexpr __detail::__deduced_simd_t<_Up>                               \
    name(const _Up& __xx)                                                                          \
    {                                                                                              \
      using _Vp = __detail::__deduced_simd_t<_Up>;                                                 \
      using _Tp [[maybe_unused]] = typename _Vp::value_type;                                       \
      const _Vp& __x = __xx;                                                                       \
      auto __gen = [&] (int __i) {                                                                 \
        if constexpr (sizeof(__x[0]) == sizeof(float))                                             \
          return __builtin_##name##f(__x[__i]);                                                    \
        else if constexpr (sizeof(__x[0]) == sizeof(double))                                       \
          return __builtin_##name(__x[__i]);                                                       \
        else                                                                                       \
          static_assert(false);                                                                    \
      };                                                                                           \
      if consteval                                                                                 \
        { return _Vp(__gen); }                                                                     \
      else                                                                                         \
        {                                                                                          \
          if (__x._M_is_constprop())                                                               \
            return _Vp(__gen);                                                                     \
          else if constexpr (requires { typename _Vp::abi_type::_AbiCombineTag; })                 \
            {                                                                                      \
              using _Tup = typename _Vp::abi_type::template _SimdMember<_Tp>;                      \
              return _Vp(__detail::__private_init,                                                 \
                         _Tup::_S_generate_pervec(                                                 \
                           [&] [[gnu::always_inline]] (vir::constexpr_value auto __i) {            \
                             return __data(name(__x._M_data._M_simd_at(__i)));                     \
                           }));                                                                    \
            }                                                                                      \
          else if constexpr (requires { typename _Vp::abi_type::_Abi0Type; })                      \
            {                                                                                      \
              using _VPart = basic_vec<_Tp, typename _Vp::abi_type::_Abi0Type>;                    \
              _Vp __r;                                                                             \
              const auto& __arr0 = __x._M_data;                                                    \
              _GLIBCXX_SIMD_INT_PACK(__arr0.size(), _Is, {                                         \
                ((__r._M_data[_Is] = name(_VPart(__arr0[_Is]))._M_data), ...);                     \
              });                                                                                  \
              return __r;                                                                          \
            }                                                                                      \
          else if constexpr (requires { _Vp::_Impl::_S_##name(__x._M_data); })                     \
            return _Vp::_Impl::_S_##name(__x._M_data);                                             \
          else                                                                                     \
            return _Vp(__detail::__##name(__x._M_data));                                           \
        }                                                                                          \
    }                                                                                              \
}                                                                                                  \
                                                                                                   \
namespace SIMD_TOPLEVEL_NSPC                                                                       \
{                                                                                                  \
  using SIMD_NSPC::name;                                                                           \
}

#define _GLIBCXX_SIMD_MATH_2ARG(name)                                                              \
namespace SIMD_NSPC                                                                                \
{                                                                                                  \
  namespace __detail                                                                               \
  {                                                                                                \
    template <__vec_builtin _Vp, auto = _BuildFlags()>                                             \
      [[gnu::flatten,                                                                              \
        gnu::optimize("tree-vectorize", "no-math-errno", "unsafe-math-optimizations")]] _Vp        \
      __##name(_Vp __v0, _Vp __v1)                                                                 \
      {                                                                                            \
        _Vp __ret;                                                                                 \
        _Pragma("GCC ivdep")                                                                       \
        for (int __i = 0; __i < __width_of<_Vp>; ++__i)                                            \
          {                                                                                        \
            if constexpr (sizeof(__v0[0]) == sizeof(float))                                        \
              __ret[__i] = __builtin_##name##f(__v0[__i], __v1[__i]);                              \
            else if constexpr (sizeof(__v0[0]) == sizeof(double))                                  \
              __ret[__i] = __builtin_##name(__v0[__i], __v1[__i]);                                 \
            else                                                                                   \
              static_assert(false);                                                                \
          }                                                                                        \
        return __ret;                                                                              \
      }                                                                                            \
  }                                                                                                \
                                                                                                   \
  template <typename _V0, typename _V1>                                                            \
    _GLIBCXX_ALWAYS_INLINE constexpr __detail::__math_common_simd_t<_V0, _V1>                      \
    name(const _V0& __x0, const _V1& __x1)                                                         \
    {                                                                                              \
      using _Vp = __detail::__math_common_simd_t<_V0, _V1>;                                        \
      using _Tp [[maybe_unused]] = typename _Vp::value_type;                                       \
      const _Vp& __x = __x0;                                                                       \
      const _Vp& __y = __x1;                                                                       \
      auto __gen = [&] (int __i) {                                                                 \
        if constexpr (sizeof(__x[0]) == sizeof(float))                                             \
          return __builtin_##name##f(__x[__i], __y[__i]);                                          \
        else if constexpr (sizeof(__x[0]) == sizeof(double))                                       \
          return __builtin_##name(__x[__i], __y[__i]);                                             \
        else                                                                                       \
          static_assert(false);                                                                    \
      };                                                                                           \
      if consteval                                                                                 \
        { return _Vp(__gen); }                                                                     \
      else                                                                                         \
        {                                                                                          \
          if (__x._M_is_constprop() and __y._M_is_constprop())                                     \
            return _Vp(__gen);                                                                     \
          else if constexpr (requires { typename _Vp::abi_type::_AbiCombineTag; })                 \
            {                                                                                      \
              using _Tup = typename _Vp::abi_type::template _SimdMember<_Tp>;                      \
              return _Vp(__detail::__private_init,                                                 \
                         _Tup::_S_generate_pervec(                                                 \
                           [&] [[gnu::always_inline]] (vir::constexpr_value auto __i) {            \
                             return __data(name(__x._M_data._M_simd_at(__i),                       \
                                                __y._M_data._M_simd_at(__i)));                     \
                           }));                                                                    \
            }                                                                                      \
          else if constexpr (requires { typename _Vp::abi_type::_Abi0Type; })                      \
            {                                                                                      \
              using _VPart = basic_vec<_Tp, typename _Vp::abi_type::_Abi0Type>;                    \
              _Vp __r;                                                                             \
              const auto& __arr0 = __x._M_data;                                                    \
              const auto& __arr1 = __y._M_data;                                                    \
              _GLIBCXX_SIMD_INT_PACK(__arr0.size(), _Is, {                                         \
                ((__r._M_data[_Is] = name(_VPart(__arr0[_Is]), _VPart(__arr1[_Is]))._M_data), ...);\
              });                                                                                  \
              return __r;                                                                          \
            }                                                                                      \
          else                                                                                     \
            return _Vp(__detail::__##name(__x._M_data, __y._M_data));                              \
        }                                                                                          \
    }                                                                                              \
}                                                                                                  \
                                                                                                   \
namespace SIMD_TOPLEVEL_NSPC                                                                       \
{                                                                                                  \
  using SIMD_NSPC::name;                                                                           \
}

#define _GLIBCXX_SIMD_MATH_3ARG(name)                                                              \
namespace SIMD_NSPC                                                                                \
{                                                                                                  \
  namespace __detail                                                                               \
  {                                                                                                \
    template <__vec_builtin _Vp, auto = _BuildFlags()>                                             \
      [[gnu::flatten,                                                                              \
        gnu::optimize("tree-vectorize", "no-math-errno", "unsafe-math-optimizations")]] _Vp        \
      __##name(_Vp __v0, _Vp __v1, _Vp __v2)                                                       \
      {                                                                                            \
        _Vp __ret;                                                                                 \
        _Pragma("GCC ivdep")                                                                       \
        for (int __i = 0; __i < __width_of<_Vp>; ++__i)                                            \
          {                                                                                        \
            if constexpr (sizeof(__v0[0]) == sizeof(float))                                        \
              __ret[__i] = __builtin_##name##f(__v0[__i], __v1[__i], __v2[__i]);                   \
            else if constexpr (sizeof(__v0[0]) == sizeof(double))                                  \
              __ret[__i] = __builtin_##name(__v0[__i], __v1[__i], __v2[__i]);                      \
            else                                                                                   \
              static_assert(false);                                                                \
          }                                                                                        \
        return __ret;                                                                              \
      }                                                                                            \
  }                                                                                                \
                                                                                                   \
  template <typename _V0, typename _V1, typename _V2>                                              \
    _GLIBCXX_ALWAYS_INLINE constexpr __detail::__math_common_simd_t<_V0, _V1, _V2>                 \
    name(const _V0& __x0, const _V1& __x1, const _V2& __x2)                                        \
    {                                                                                              \
      using _Vp = __detail::__math_common_simd_t<_V0, _V1, _V2>;                                   \
      using _Tp [[maybe_unused]] = typename _Vp::value_type;                                       \
      const _Vp& __x = __x0;                                                                       \
      const _Vp& __y = __x1;                                                                       \
      const _Vp& __z = __x2;                                                                       \
      auto __gen = [&] (int __i) {                                                                 \
        if constexpr (sizeof(__x[0]) == sizeof(float))                                             \
          return __builtin_##name##f(__x[__i], __y[__i], __z[__i]);                                \
        else if constexpr (sizeof(__x[0]) == sizeof(double))                                       \
          return __builtin_##name(__x[__i], __y[__i], __z[__i]);                                   \
        else                                                                                       \
          static_assert(false);                                                                    \
      };                                                                                           \
      if consteval                                                                                 \
        { return _Vp(__gen); }                                                                     \
      else                                                                                         \
        {                                                                                          \
          if (__x._M_is_constprop() and __y._M_is_constprop())                                     \
            return _Vp(__gen);                                                                     \
          else if constexpr (requires { typename _Vp::abi_type::_AbiCombineTag; })                 \
            {                                                                                      \
              using _Tup = typename _Vp::abi_type::template _SimdMember<_Tp>;                      \
              return _Vp(__detail::__private_init,                                                 \
                         _Tup::_S_generate_pervec(                                                 \
                           [&] [[gnu::always_inline]] (vir::constexpr_value auto __i) {            \
                             return __data(name(__x._M_data._M_simd_at(__i),                       \
                                                __y._M_data._M_simd_at(__i),                       \
                                                __z._M_data._M_simd_at(__i)));                     \
                           }));                                                                    \
            }                                                                                      \
          else if constexpr (requires { typename _Vp::abi_type::_Abi0Type; })                      \
            {                                                                                      \
              using _VPart = basic_vec<_Tp, typename _Vp::abi_type::_Abi0Type>;                    \
              _Vp __r;                                                                             \
              const auto& __arr0 = __x._M_data;                                                    \
              const auto& __arr1 = __y._M_data;                                                    \
              const auto& __arr2 = __z._M_data;                                                    \
              _GLIBCXX_SIMD_INT_PACK(__arr0.size(), _Is, {                                         \
                ((__r._M_data[_Is] = name(_VPart(__arr0[_Is]), _VPart(__arr1[_Is]),                \
                                          _VPart(__arr2[_Is]))._M_data), ...);          \
              });                                                                                  \
              for (size_t __i = 0; __i < __arr0.size(); ++__i)                                     \
                __r._M_data[__i] = name(_VPart(__arr0[__i]), _VPart(__arr1[__i]),                  \
                                               _VPart(__arr2[__i]))._M_data;                       \
              return __r;                                                                          \
            }                                                                                      \
          else                                                                                     \
            return _Vp(__detail::__##name(__x._M_data, __y._M_data, __z._M_data));                 \
        }                                                                                          \
    }                                                                                              \
}                                                                                                  \
                                                                                                   \
namespace SIMD_TOPLEVEL_NSPC                                                                       \
{                                                                                                  \
  using SIMD_NSPC::name;                                                                           \
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
_GLIBCXX_SIMD_MATH_1ARG(abs)
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

// the following depend on the global rounding mode (not constexpr):
//template<@\mathfloatingpoint@ V> @\deducedsimd@<V> nearbyint(const V& x);
//template<@\mathfloatingpoint@ V> @\deducedsimd@<V> rint(const V& x);
//template<@\mathfloatingpoint@ V> rebind_simd_t<long int, @\deducedsimd@<V>> lrint(const V& x);
//template<@\mathfloatingpoint@ V> rebind_simd_t<long long int, V> llrint(const @\deducedsimd@<V>& x);

#if 0
frexp(const V& value, rebind_simd_t<int, @\deducedsimd@<V>>* exp);
constexpr rebind_simd_t<int, @\deducedsimd@<V>> ilogb(const V& x);
constexpr @\deducedsimd@<V> ldexp(const V& x, const rebind_simd_t<int, @\deducedsimd@<V>>& exp);
constexpr basic_simd<T, Abi> modf(const type_identity_t<basic_simd<T, Abi>>& value, basic_simd<T, Abi>* iptr);
constexpr @\deducedsimd@<V> scalbn(const V& x, const rebind_simd_t<int, @\deducedsimd@<V>>& n);
constexpr @\deducedsimd@<V> scalbln(const V& x, const rebind_simd_t<long int, @\deducedsimd@<V>>& n);
template<signed_integral T, class Abi> constexpr basic_simd<T, Abi> abs(const basic_simd<T, Abi>& j);

  template<@\mathfloatingpoint@ V> constexpr rebind_simd_t<long int, @\deducedsimd@<V>> lround(const V& x);
  template<@\mathfloatingpoint@ V> constexpr rebind_simd_t<long long int, @\deducedsimd@<V>> llround(const V& x);
  template<class V0, class V1> constexpr @\mathcommonsimd@<V0, V1> remquo(const V0& x, const V1& y, rebind_simd_t<int, @\mathcommonsimd@<V0, V1>>* quo);
  template<@\mathfloatingpoint@ V> constexpr rebind_simd_t<int, @\deducedsimd@<V>> fpclassify(const V& x);
  template<@\mathfloatingpoint@ V> constexpr typename @\deducedsimd@<V>::mask_type isfinite(const V& x);
  template<@\mathfloatingpoint@ V> constexpr typename @\deducedsimd@<V>::mask_type isinf(const V& x);
  template<@\mathfloatingpoint@ V> constexpr typename @\deducedsimd@<V>::mask_type isnan(const V& x);
  template<@\mathfloatingpoint@ V> constexpr typename @\deducedsimd@<V>::mask_type isnormal(const V& x);
  template<@\mathfloatingpoint@ V> constexpr typename @\deducedsimd@<V>::mask_type signbit(const V& x);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type isgreater(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type isgreaterequal(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type isless(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type islessequal(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type islessgreater(const V0& x, const V1& y);
  template<class V0, class V1> constexpr typename @\mathcommonsimd@<V0, V1>::mask_type isunordered(const V0& x, const V1& y);

  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> assoc_laguerre(const rebind_simd_t<unsigned, @\deducedsimd@<V>>& n, const
      rebind_simd_t<unsigned, @\deducedsimd@<V>>& m,
                     const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> assoc_legendre(const rebind_simd_t<unsigned, @\deducedsimd@<V>>& l, const
      rebind_simd_t<unsigned, @\deducedsimd@<V>>& m,
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
    @\deducedsimd@<V> hermite(const rebind_simd_t<unsigned, @\deducedsimd@<V>>& n, const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> laguerre(const rebind_simd_t<unsigned, @\deducedsimd@<V>>& n, const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> legendre(const rebind_simd_t<unsigned, @\deducedsimd@<V>>& l, const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> riemann_zeta(const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> sph_bessel(const rebind_simd_t<unsigned, @\deducedsimd@<V>>& n, const V& x);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V> sph_legendre(const rebind_simd_t<unsigned, @\deducedsimd@<V>>& l,
      const rebind_simd_t<unsigned, @\deducedsimd@<V>>& m, const V& theta);
  template<@\mathfloatingpoint@ V>
    @\deducedsimd@<V>
      sph_neumann(const rebind_simd_t<unsigned, @\deducedsimd@<V>>& n, const V& x);
#endif

#endif  // PROTOTYPE_SIMD_MATH_H_
