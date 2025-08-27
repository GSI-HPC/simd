/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef INCLUDE_BITS_SIMD_MATH_H_
#define INCLUDE_BITS_SIMD_MATH_H_

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

// [simd.math] ----------------------------------------------------------------
namespace std::simd
{
  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<int, __deduced_vec_t<_Vp>>
    ilogb(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    ldexp(const _Vp& __x, const rebind_t<int, __deduced_vec_t<_Vp>>& exp)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    scalbn(const _Vp& __x, const rebind_t<int, __deduced_vec_t<_Vp>>& n)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    scalbln(const _Vp& __x, const rebind_t<long int, __deduced_vec_t<_Vp>>& n)
    { static_assert(false, "TODO"); }

  template <signed_integral T, typename Abi>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<T, Abi>
    abs(const basic_vec<T, Abi>& j)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    abs(const _Vp& j)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    fabs(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    ceil(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    floor(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    nearbyint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    rint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    rebind_t<long int, __deduced_vec_t<_Vp>>
    lrint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    rebind_t<long long int, __deduced_vec_t<_Vp>>
    llrint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    round(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<long int, __deduced_vec_t<_Vp>>
    lround(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<long long int, __deduced_vec_t<_Vp>>
    llround(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    fmod(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    trunc(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    remainder(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    copysign(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    nextafter(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    fdim(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    fmax(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    fmin(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1, typename _V2>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1, _V2>
    fma(const _V0& __x, const _V1& __y, const _V2& __z)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<int, __deduced_vec_t<_Vp>>
    fpclassify(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isfinite(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isinf(const _Vp& __x)
    { return static_cast<const __deduced_vec_t<_Vp>&>(__x)._M_isinf(); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isnan(const _Vp& __x)
    { return static_cast<const __deduced_vec_t<_Vp>&>(__x)._M_isnan(); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isnormal(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    signbit(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    isgreater(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    isgreaterequal(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    isless(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    islessequal(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    islessgreater(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    isunordered(const _V0& __x, const _V1& __y)
    {
      using _Vp = __math_common_simd_t<_V0, _V1>;
      if constexpr (__simd_integral<_V0> or is_integral_v<_V0>)
        return __y._M_isnan();
      else if constexpr (__simd_integral<_V1> or is_integral_v<_V1>)
        return __x._M_isnan();
      else
        return static_cast<const _Vp&>(__x)._M_isunordered(static_cast<const _Vp&>(__y));
    }

#if false and _GLIBCXX_SIMD_HAVE_SSE and defined __x86_64__
#define _GLIBCXX_SIMD_MATH_2X_CALL(fn, arg)                                                        \
  const auto __lo = __##fn##_2x(arg._M_get_low()._M_get(), arg._M_get_high()._M_get());            \
  remove_cvref_t<decltype(arg._M_get_high()._M_get())> __hi;                                       \
  asm("": "={xmm1}"(__hi))

#define _GLIBCXX_SIMD_MATH_2X_CALL2(fn, arg0, arg1)                                                \
  const auto __lo = __##fn##_2x(arg0._M_get_low()._M_get(), arg1._M_get_low()._M_get(),            \
                                arg0._M_get_high()._M_get(), arg1._M_get_high()._M_get());         \
  remove_cvref_t<decltype(arg0._M_get_high()._M_get())> __hi;                                      \
  asm("": "={xmm1}"(__hi))

#define _GLIBCXX_SIMD_MATH_RET_TYPE _V0
#else
#define _GLIBCXX_SIMD_MATH_2X_CALL(fn, arg)                                                        \
  const auto [__lo, __hi] = __##fn##_2x(arg._M_get_low()._M_get(), arg._M_get_high()._M_get())

#define _GLIBCXX_SIMD_MATH_2X_CALL2(fn, arg0, arg1)                                                \
  const auto [__lo, __hi] = __##fn##_2x(arg0._M_get_low()._M_get(), arg1._M_get_low()._M_get(),    \
                                        arg0._M_get_high()._M_get(), arg1._M_get_high()._M_get())

#define _GLIBCXX_SIMD_MATH_RET_TYPE pair<_V0, _V1>
#endif

  // __FAST_MATH__ must be defined when including <math.h> in order to get calls to simdclones of
  // the math functions. Therefore, with __FAST_MATH__ we can inline everything, without it we need
  // to call into the library, which can be compiled with fast-math.
#if __FAST_MATH__ && __OPTIMIZE__
#define _GLIBCXX_SIMD_MATH_IMPL(fn)                                                                \
  template <typename _TV>                                                                          \
    inline _TV                                                                                     \
    __##fn(_TV __x)                                                                                \
    {                                                                                              \
      return _GLIBCXX_SIMD_INT_PACK(__width_of<_TV>, _Is, {                                        \
               return _TV{std::fn(__x[_Is])...};                                                   \
             });                                                                                   \
    }                                                                                              \
                                                                                                   \
  template <typename _V0, typename _V1>                                                            \
    [[__gnu__::__always_inline__]]                                                                 \
    inline pair<_V0, _V1>                                                                          \
    __##fn##_2x(_V0 __x, _V1 __y)                                                                  \
    { return pair { __##fn(__x), __##fn(__y) }; }

#else
#define _GLIBCXX_SIMD_MATH_IMPL(fn)                                                                \
  template <typename _Vp, _TargetTraits = {}>                                                      \
    _Vp                                                                                            \
    __##fn(_Vp);                                                                                   \
                                                                                                   \
  template <typename _V0, typename _V1, _TargetTraits = {}>                                        \
    _GLIBCXX_SIMD_MATH_RET_TYPE                                                                    \
    __##fn##_2x(_V0, _V1);

#endif

#define _GLIBCXX_SIMD_MATH_CALL(fn)                                                                \
  _GLIBCXX_SIMD_MATH_IMPL(fn)                                                                      \
                                                                                                   \
  template<__math_floating_point _Vp, _ArchFlags _Flags = {}>                                      \
    [[__gnu__::__always_inline__]]                                                                 \
    constexpr __deduced_vec_t<_Vp>                                                                 \
    fn(const _Vp& __x)                                                                             \
    {                                                                                              \
      if constexpr (not is_same_v<_Vp, __deduced_vec_t<_Vp>>)                                      \
        return fn(static_cast<const __deduced_vec_t<_Vp>&>(__x));                                  \
      else if (__builtin_is_constant_evaluated() or __x._M_is_constprop())                         \
        return _Vp([&] [[__gnu__::__always_inline__]] (int __i) {                                  \
                 return std::fn(__x[__i]);                                                         \
               });                                                                                 \
      else if constexpr (_Vp::size() == 1)                                                         \
        return std::fn(__x[0]);                                                                    \
      else if constexpr (_Flags.template _M_eval_as_f32<typename _Vp::value_type>())               \
        return _Vp(fn(rebind_t<float, _Vp>(__x)));                                                 \
      else if constexpr (_Vp::abi_type::_S_nreg == 1)                                              \
        return __##fn(__x._M_get());                                                               \
      else if constexpr (_Vp::abi_type::_S_nreg == 2)                                              \
        {                                                                                          \
          _GLIBCXX_SIMD_MATH_2X_CALL(fn, __x);                                                     \
          return _Vp::_S_init(__lo, __hi);                                                         \
        }                                                                                          \
      else                                                                                         \
        return _Vp::_S_init(fn(__x._M_get_low()), fn(__x._M_get_high()));                          \
    }

#define _GLIBCXX_SIMD_MATH_CALL2(fn)                                                               \
  template <typename _Vp, _TargetTraits = {}>                                                      \
    _Vp                                                                                            \
    __##fn(_Vp, _Vp);                                                                              \
                                                                                                   \
  template <typename _V0, typename _V1, _TargetTraits = {}>                                        \
    _GLIBCXX_SIMD_MATH_RET_TYPE                                                                    \
    __##fn##_2x(_V0, _V0, _V1, _V1);                                                               \
                                                                                                   \
  template <typename _V0, typename _V1>                                                            \
    [[__gnu__::__always_inline__]]                                                                 \
    constexpr __math_common_simd_t<_V0, _V1>                                                       \
    fn(const _V0& __x, const _V1& __y)                                                             \
    {                                                                                              \
      if constexpr (not is_same_v<_V0, __math_common_simd_t<_V0, _V1>>                             \
                      or not is_same_v<_V1, __math_common_simd_t<_V0, _V1>>)                       \
        return fn(static_cast<const __math_common_simd_t<_V0, _V1>&>(__x),                         \
                  static_cast<const __math_common_simd_t<_V0, _V1>&>(__y));                        \
      else if (__builtin_is_constant_evaluated()                                                   \
                 or (__x._M_is_constprop() and __y._M_is_constprop()))                             \
        return _V0([&] [[__gnu__::__always_inline__]] (int __i) {                                  \
                 return std::fn(__x[__i], __y[__i]);                                               \
               });                                                                                 \
      else if constexpr (_V0::size() == 1)                                                         \
        return std::fn(__x[0], __y[0]);                                                            \
      else if constexpr (_V0::abi_type::_S_nreg == 1)                                              \
        return __##fn(__x._M_get(), __y._M_get());                                                 \
      else if constexpr (_V0::abi_type::_S_nreg == 2)                                              \
        {                                                                                          \
          _GLIBCXX_SIMD_MATH_2X_CALL2(fn, __x, __y);                                               \
          return _V0::_S_init(__lo, __hi);                                                         \
        }                                                                                          \
      else                                                                                         \
        return _V0::_S_init(fn(__x._M_get_low(), __y._M_get_low()),                                \
                            fn(__x._M_get_high(), __y._M_get_high()));                             \
    }

  _GLIBCXX_SIMD_MATH_CALL(acos)
  _GLIBCXX_SIMD_MATH_CALL(asin)
  _GLIBCXX_SIMD_MATH_CALL(atan)
  _GLIBCXX_SIMD_MATH_CALL2(atan2)
  _GLIBCXX_SIMD_MATH_CALL(cos)
  _GLIBCXX_SIMD_MATH_CALL(sin)
  _GLIBCXX_SIMD_MATH_CALL(tan)
  _GLIBCXX_SIMD_MATH_CALL(acosh)
  _GLIBCXX_SIMD_MATH_CALL(asinh)
  _GLIBCXX_SIMD_MATH_CALL(atanh)
  _GLIBCXX_SIMD_MATH_CALL(cosh)
  _GLIBCXX_SIMD_MATH_CALL(sinh)
  _GLIBCXX_SIMD_MATH_CALL(tanh)

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    exp(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    exp2(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    expm1(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    log(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    log10(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    log1p(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    log2(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    logb(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    cbrt(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    hypot(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1, class _V2>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1, _V2>
    hypot(const _V0& __x, const _V1& __y, const _V2& __z)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    pow(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    sqrt(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    erf(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    erfc(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    lgamma(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    tgamma(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1, class _V2>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1, _V2>
    lerp(const _V0& __a, const _V1& __b, const _V2& __t) noexcept
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    assoc_laguerre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __n,
                   const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __m, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    assoc_legendre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __l,
                   const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __m, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    beta(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    comp_ellint_1(const _Vp& __k)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    comp_ellint_2(const _Vp& __k)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    comp_ellint_3(const _V0& __k, const _V1& __nu)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    cyl_bessel_i(const _V0& __nu, const _V1& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    cyl_bessel_j(const _V0& __nu, const _V1& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    cyl_bessel_k(const _V0& __nu, const _V1& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    cyl_neumann(const _V0& __nu, const _V1& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    ellint_1(const _V0& __k, const _V1& __phi)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    ellint_2(const _V0& __k, const _V1& __phi)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1, class _V2>
    __math_common_simd_t<_V0, _V1, _V2>
    ellint_3(const _V0& __k, const _V1& __nu, const _V2& __phi)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    expint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    hermite(const rebind_t<unsigned,
                           __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    laguerre(const rebind_t<unsigned,
                            __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    legendre(const rebind_t<unsigned,
                            __deduced_vec_t<_Vp>>& __l, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    riemann_zeta(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    sph_bessel(const rebind_t<unsigned,
                              __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    sph_legendre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __l,
                 const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __m, const _Vp& __theta)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    sph_neumann(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    frexp(const _Vp& __value, rebind_t<int, __deduced_vec_t<_Vp>>* __exp)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    remquo(const _V0& __x, const _V1& __y, rebind_t<int, __math_common_simd_t<_V0, _V1>>* __quo)
    { static_assert(false, "TODO"); }

  template<class T, class Abi>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<T, Abi>
    modf(const type_identity_t<basic_vec<T, Abi>>& __value, basic_vec<T, Abi>* __iptr)
    { static_assert(false, "TODO"); }
}

namespace std
{
  using simd::acos;
  using simd::asin;
  using simd::atan;
  using simd::atan2;
  using simd::cos;
  using simd::sin;
  using simd::tan;
  using simd::acosh;
  using simd::asinh;
  using simd::atanh;
  using simd::cosh;
  using simd::sinh;
  using simd::tanh;
  using simd::exp;
  using simd::exp2;
  using simd::expm1;
  using simd::frexp;
  using simd::ilogb;
  using simd::ldexp;
  using simd::log;
  using simd::log10;
  using simd::log1p;
  using simd::log2;
  using simd::logb;
  using simd::modf;
  using simd::scalbn;
  using simd::scalbln;
  using simd::cbrt;
  using simd::abs;
  using simd::abs;
  using simd::fabs;
  using simd::hypot;
  using simd::pow;
  using simd::sqrt;
  using simd::erf;
  using simd::erfc;
  using simd::lgamma;
  using simd::tgamma;
  using simd::ceil;
  using simd::floor;
  using simd::nearbyint;
  using simd::rint;
  using simd::lrint;
  using simd::llrint;
  using simd::round;
  using simd::lround;
  using simd::llround;
  using simd::trunc;
  using simd::fmod;
  using simd::remainder;
  using simd::remquo;
  using simd::copysign;
  using simd::nextafter;
  using simd::fdim;
  using simd::fmax;
  using simd::fmin;
  using simd::fma;
  using simd::lerp;
  using simd::fpclassify;
  using simd::isfinite;
  using simd::isinf;
  using simd::isnan;
  using simd::isnormal;
  using simd::signbit;
  using simd::isgreater;
  using simd::isgreaterequal;
  using simd::isless;
  using simd::islessequal;
  using simd::islessgreater;
  using simd::isunordered;
  using simd::assoc_laguerre;
  using simd::assoc_legendre;
  using simd::beta;
  using simd::comp_ellint_1;
  using simd::comp_ellint_2;
  using simd::comp_ellint_3;
  using simd::cyl_bessel_i;
  using simd::cyl_bessel_j;
  using simd::cyl_bessel_k;
  using simd::cyl_neumann;
  using simd::ellint_1;
  using simd::ellint_2;
  using simd::ellint_3;
  using simd::expint;
  using simd::hermite;
  using simd::laguerre;
  using simd::legendre;
  using simd::riemann_zeta;
  using simd::sph_bessel;
  using simd::sph_legendre;
  using simd::sph_neumann;
}

// [simd.complex.math] --------------------------------------------------------
namespace std::simd
{
  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    real(const _Vp& __x) noexcept
    { return __x.real(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    imag(const _Vp& __x) noexcept
    { return __x.imag(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    abs(const _Vp& __x) noexcept
    { return __x._M_abs(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    arg(const _Vp& __x) noexcept
    { return __x._M_arg(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    norm(const _Vp& __x) noexcept
    { return __x._M_norm(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    conj(const _Vp& __x) noexcept
    { return __x._M_conj(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    proj(const _Vp& __x) noexcept
    { return __x._M_proj(); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    exp(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    log(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    log10(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    sqrt(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    sin(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    asin(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    cos(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    acos(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    tan(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    atan(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    sinh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    asinh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    cosh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    acosh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    tanh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    atanh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_floating_point _Vp>
    rebind_t<complex<typename _Vp::value_type>, _Vp>
    polar(const _Vp& __x, const _Vp& __y = {})
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    pow(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }
}

namespace std
{
  using simd::real;
  using simd::imag;
  using simd::arg;
  using simd::norm;
  using simd::conj;
  using simd::proj;
  using simd::polar;
}

#pragma GCC diagnostic pop

#endif  // INCLUDE_BITS_SIMD_MATH_H_
