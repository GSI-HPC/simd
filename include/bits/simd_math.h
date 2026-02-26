/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#if VIR_PATCH_MATH
#ifndef _GLIBCXX_SIMD_MATH_H
#define _GLIBCXX_SIMD_MATH_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include "simd_vec.h"

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

// [simd.math] ----------------------------------------------------------------
namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION
namespace simd
{
  template <typename _TV>
    concept __simd_clonable = __vec_builtin<_TV> && sizeof(_TV) >= 16
				&& (is_same_v<__vec_value_type<_TV>, float>
				      || is_same_v<__vec_value_type<_TV>, double>);

  /** \internal
   * Check whether \p __s matches "(( *__simd__". If it matches we assume it's a simd-clones
   * attribute declaration as used in glibc's <math.h>.
   */
  consteval bool
    __is_attribute_simd(const char* __s)
  {
    if (!__s) return false;
    while (*__s != '(' && *__s != '\0') ++__s;
    if (*__s == '\0') return false;
    if (*++__s != '(') return false;
    while (__s[1] == ' ') ++__s;
    if (*++__s != '_') return false;
    if (*++__s != '_') return false;
    if (*++__s != 's') return false;
    if (*++__s != 'i') return false;
    if (*++__s != 'm') return false;
    if (*++__s != 'd') return false;
    if (*++__s != '_') return false;
    if (*++__s != '_') return false;
    return true;
  }

#define _GLIBCXX_SIMD_HAS_SIMD_CLONE(fn)                                                           \
  __is_attribute_simd(_GLIBCXX_SIMD_TOSTRING(__SIMD_DECL(fn)))

  // __FAST_MATH__ must be defined when including <math.h> in order to get calls to simd-clones of
  // the math functions. Therefore, with __FAST_MATH__ we can inline everything, without it we need
  // to call into the library, which can be compiled with fast-math.
  // Also when building with __FAST_MATH__, we can ignore many corner cases (inf, NaN, negative
  // zero) and call to a simpler (faster) implementation. With __FAST_MATH__ simd::sin calls
  // simd::__fast_sin (unless inlined to a simd-clone), otherwise to simd::__sin (which internally
  // might call __fast_sin).
  //
  // The _GLIBCXX_SIMD_HAS_SIMD_CLONE macro determines whether a simd-clone is declared for the
  // fn(double) math function. We assume that a float simd-clone exists then, too. However, if for
  // some reason the body of __fast_fn does not compile to something GCC wants to inline, then the
  // gnu_inline attribute makes a call to the __fast_fn function in the library.

#if _GLIBCXX_X86

  // A pair<V0, V1> would always return via MEMORY, not SSE, according to the AMD64 psABI.
  // However, in this case we *really* could use return via two registers: [xyz]mm0 and [xyz]mm1. It
  // seems like we can do the trick via inline asm. It would be much better to have a different
  // calling convention function attribute, though.
  // (asm: it is important to fake a read-write to xmm0 in order to ensure no function gets called
  // without saving xmm1 to the stack - unless xmm1 is an argument to that function call)
  //
  // with this hack (hypot2 benchmark on Intel(R) Core(TM) Ultra 7 165U):
  //   TYPE                  Latency     Speedup     Throughput     Speedup
  //                   [cycles/call] [per value]  [cycles/call] [per value]
  //  float,                    56.2           1           11.9           1
  //  float, 8                    67        6.71           24.9        3.83
  //  float, 16                 69.8        12.9             46        4.15
  //  float, 32                 95.1        18.9           95.1        4.01
  //  float, 64                  193        18.6            194        3.93
  // ----------------------------------------------------------------------
  //   TYPE                  Latency     Speedup     Throughput     Speedup
  //                   [cycles/call] [per value]  [cycles/call] [per value]
  // double,                    97.4           1             18           1
  // double, 4                  79.9        4.88           21.1         3.4
  // double, 8                  81.5        9.56           42.9        3.34
  // double, 16                  100        15.6           91.9        3.13
  // double, 32                  190        16.4            188        3.05
  //
  // without this hack:
  //  TYPE                   Latency     Speedup     Throughput     Speedup
  //                   [cycles/call] [per value]  [cycles/call] [per value]
  //  float,                    56.2           1           11.9           1
  //  float, 8                  66.9        6.71           24.9        3.84
  //  float, 16                 79.3        11.3             47        4.06
  //  float, 32                 99.9          18            101        3.78
  //  float, 64                  206        17.4            208        3.68
  // ----------------------------------------------------------------------
  //  TYPE                   Latency     Speedup     Throughput     Speedup
  //                   [cycles/call] [per value]  [cycles/call] [per value]
  // double,                    97.5           1             18           1
  // double, 4                  79.8        4.89           21.1        3.41
  // double, 8                  91.1        8.56           46.9        3.07
  // double, 16                  107        14.5           99.1         2.9
  // double, 32                  203        15.3            202        2.85
  //
  // The Latency benchmark uses the result as one input to the next hypot call. The Throughput
  // benchmark completely discards the result. Consequently, the performance difference in
  // Throughput is only due to the library function having to store to memory rather than returning
  // in registers.

#define _GLIBCXX_SIMD_MATH_2X_CALL(fn, arg)                                                        \
  remove_cvref_t<decltype(arg._M_get_high()._M_get())> __hi;                                       \
  auto __lo = fn(arg._M_get_low()._M_get(), arg._M_get_high()._M_get());                           \
  asm("" : "={xmm1}"(__hi), "+{xmm0}"(__lo))

#define _GLIBCXX_SIMD_MATH_2X_CALL2(fn, arg0, arg1)                                                \
  remove_cvref_t<decltype(arg0._M_get_high()._M_get())> __hi;                                      \
  auto __lo = fn(arg0._M_get_low()._M_get(), arg1._M_get_low()._M_get(),                           \
		 arg0._M_get_high()._M_get(), arg1._M_get_high()._M_get());                        \
  asm("" : "={xmm1}"(__hi), "+{xmm0}"(__lo))

#define _GLIBCXX_SIMD_MATH_2X_CALL3(fn, arg0, arg1, arg2)                                          \
  remove_cvref_t<decltype(arg0._M_get_high()._M_get())> __hi;                                      \
  auto __lo = fn(arg0._M_get_low()._M_get(), arg1._M_get_low()._M_get(),                           \
		 arg2._M_get_low()._M_get(), arg0._M_get_high()._M_get(),                          \
		 arg1._M_get_high()._M_get(), arg2._M_get_high()._M_get());                        \
  asm("" : "={xmm1}"(__hi), "+{xmm0}"(__lo))

#define _GLIBCXX_SIMD_MATH_RET_TYPE(V0, V1) V0

#define _GLIBCXX_SIMD_MATH_RETURN(lo, hi)                                                          \
  asm("" :: "{xmm1}"(hi), "{xmm0}"(lo));                                                           \
  return lo

#else

#define _GLIBCXX_SIMD_MATH_2X_CALL(fn, arg)                                                        \
  const auto [__lo, __hi] = fn(arg._M_get_low()._M_get(), arg._M_get_high()._M_get())

#define _GLIBCXX_SIMD_MATH_2X_CALL2(fn, arg0, arg1)                                                \
  const auto [__lo, __hi] = fn(arg0._M_get_low()._M_get(), arg1._M_get_low()._M_get(),             \
			       arg0._M_get_high()._M_get(), arg1._M_get_high()._M_get())

#define _GLIBCXX_SIMD_MATH_2X_CALL3(fn, arg0, arg1, arg2)                                          \
  const auto [__lo, __hi] = fn(arg0._M_get_low()._M_get(), arg1._M_get_low()._M_get(),             \
			       arg2._M_get_low()._M_get(), arg0._M_get_high()._M_get(),            \
			       arg1._M_get_high()._M_get(), arg2._M_get_high()._M_get())

#define _GLIBCXX_SIMD_MATH_RET_TYPE(V0, V1) pair<V0, V1>

#define _GLIBCXX_SIMD_MATH_RETURN(lo, hi) return pair(lo, hi)

#endif

#define _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(decl, fn)                                                \
  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>                                  \
    [[__gnu__::__always_inline__]] decl                                                            \
    fn(const __deduced_vec_t<_Vp>& __x, const _Vp& __y)                                            \
    { return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y); }                                        \
												   \
  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>                                  \
    [[__gnu__::__always_inline__]] decl                                                            \
    fn(const _Vp& __x, const __deduced_vec_t<_Vp>& __y)                                            \
    { return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y); }

#define _GLIBCXX_SIMD_MATH_3ARG_OVERLOADS(decl, fn)                                                \
  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>                                  \
    [[__gnu__::__always_inline__]] decl                                                            \
    fn(const __deduced_vec_t<_Vp>& __x, const _Vp& __y, const _Vp& __z)                            \
    { return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __z); }                                   \
												   \
  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>                                  \
    [[__gnu__::__always_inline__]] decl                                                            \
    fn(const _Vp& __x, const __deduced_vec_t<_Vp>& __y, const _Vp& __z)                            \
    { return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __z); }                                   \
												   \
  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>                                  \
    [[__gnu__::__always_inline__]] decl                                                            \
    fn(const _Vp& __x, const _Vp& __y, const __deduced_vec_t<_Vp>& __z)                            \
    { return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __z); }                                   \
												   \
  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>                                  \
    [[__gnu__::__always_inline__]] decl                                                            \
    fn(const __deduced_vec_t<_Vp>& __x, const __deduced_vec_t<_Vp>& __y, const _Vp& __z)           \
    { return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __z); }                                   \
												   \
  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>                                  \
    [[__gnu__::__always_inline__]] decl                                                            \
    fn(const __deduced_vec_t<_Vp>& __x, const _Vp& __y, const __deduced_vec_t<_Vp>& __z)           \
    { return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __z); }                                   \
												   \
  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>                                  \
    [[__gnu__::__always_inline__]] decl                                                            \
    fn(const _Vp& __x, const __deduced_vec_t<_Vp>& __y, const __deduced_vec_t<_Vp>& __z)           \
    { return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __z); }

#define _GLIBCXX_SIMD_MATH_CALL(fn)                                                                \
  template <_ArchTraits, __simd_clonable _TV>                                                      \
    requires (_GLIBCXX_SIMD_HAS_SIMD_CLONE(fn))                                                    \
    [[__gnu__::__gnu_inline__]]                                                                    \
    inline _TV                                                                                     \
    __fast_##fn(_TV __x)                                                                           \
    {                                                                                              \
      constexpr auto [...__is] = _IotaArray<__width_of<_TV>>;                                      \
      return _TV{std::fn(__x[__is])...};                                                           \
    }                                                                                              \
												   \
  template <_ArchTraits, typename _Vp>                                                             \
    requires (!__simd_clonable<_Vp> || !_GLIBCXX_SIMD_HAS_SIMD_CLONE(fn))                          \
    extern _Vp                                                                                     \
    __fast_##fn(_Vp);                                                                              \
												   \
  template <_ArchTraits, typename _V0, typename _V1>                                               \
    extern _GLIBCXX_SIMD_MATH_RET_TYPE(_V0, _V1)                                                   \
    __fast_2x_##fn(_V0, _V1);                                                                      \
												   \
  template <_TargetTraits, typename _Vp>                                                           \
    extern _Vp                                                                                     \
    __##fn(_Vp);                                                                                   \
												   \
  template <_TargetTraits, typename _V0, typename _V1>                                             \
    extern _GLIBCXX_SIMD_MATH_RET_TYPE(_V0, _V1)                                                   \
    __2x_##fn(_V0, _V1);                                                                           \
												   \
  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>                                  \
    [[__gnu__::__always_inline__]]                                                                 \
    constexpr __deduced_vec_t<_Vp>                                                                 \
    fn(const _Vp& __x)                                                                             \
    {                                                                                              \
      if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)                                         \
	return fn<_Traits, __deduced_vec_t<_Vp>>(__x);                                             \
      else if (__is_const_known(__x))                                                              \
	return _Vp([&] [[__gnu__::__always_inline__]] (int __i) {                                  \
		 return std::fn(__x[__i]);                                                         \
	       });                                                                                 \
      else if constexpr (_Vp::size() == 1)                                                         \
	return std::fn(__x[0]);                                                                    \
      else if constexpr (_Traits.template _M_eval_as_f32<typename _Vp::value_type>())              \
	return _Vp(fn<_Traits, rebind_t<float, _Vp>>(__x));                                        \
      else if constexpr (_Vp::abi_type::_S_nreg == 1 && _Traits._M_fast_math())                    \
	return __fast_##fn<_ArchTraits(_Traits)._M_math_abi()>(__x._M_get());                      \
      else if constexpr (_Vp::abi_type::_S_nreg == 1)                                              \
	return __##fn<_Traits._M_math_abi()>(__x._M_get());                                        \
      else if constexpr (_Vp::abi_type::_S_nreg == 2 && _Traits._M_fast_math()                     \
			   && !_GLIBCXX_SIMD_HAS_SIMD_CLONE(fn))                                   \
	{                                                                                          \
	  _GLIBCXX_SIMD_MATH_2X_CALL(__fast_2x_##fn<_ArchTraits(_Traits)._M_math_abi()>, __x);     \
	  return _Vp::_S_init(__lo, __hi);                                                         \
	}                                                                                          \
      else if constexpr (_Vp::abi_type::_S_nreg == 2 && !_Traits._M_fast_math())                   \
	{                                                                                          \
	  _GLIBCXX_SIMD_MATH_2X_CALL(__2x_##fn<_Traits._M_math_abi()>, __x);                       \
	  return _Vp::_S_init(__lo, __hi);                                                         \
	}                                                                                          \
      else                                                                                         \
	return _Vp::_S_init(fn<_Traits>(__x._M_get_low()), fn(__x._M_get_high()));                 \
    }

#if 1
#define _GLIBCXX_SIMD_MATH_CALL2_HANDLE_2X(fn)                                                     \
      else if constexpr (_Vp::abi_type::_S_nreg == 2 && _Traits._M_fast_math()                     \
			   && !_GLIBCXX_SIMD_HAS_SIMD_CLONE(fn))                                   \
	{                                                                                          \
	  _GLIBCXX_SIMD_MATH_2X_CALL2(__fast_2x_##fn<_ArchTraits(_Traits)._M_math_abi()>,          \
				      __x, __y);                                                   \
	  return _Vp::_S_init(__lo, __hi);                                                         \
	}                                                                                          \
      else if constexpr (_Vp::abi_type::_S_nreg == 2 && !_Traits._M_fast_math())                   \
	{                                                                                          \
	  _GLIBCXX_SIMD_MATH_2X_CALL2(__2x_##fn<_Traits._M_math_abi()>, __x, __y);                 \
	  return _Vp::_S_init(__lo, __hi);                                                         \
	}
#else
#define _GLIBCXX_SIMD_MATH_CALL2_HANDLE_2X(fn)
#endif

#define _GLIBCXX_SIMD_MATH_CALL2(fn, allow_clone)                                                  \
  template <_ArchTraits, __simd_clonable _TV>                                                      \
    requires (allow_clone && _GLIBCXX_SIMD_HAS_SIMD_CLONE(fn))                                     \
    [[__gnu__::__gnu_inline__]]                                                                    \
    inline _TV                                                                                     \
    __fast_##fn(_TV __x0, _TV __x1) noexcept                                                       \
    {                                                                                              \
      constexpr auto [...__is] = _IotaArray<__width_of<_TV>>;                                      \
      return _TV{std::fn(__x0[__is], __x1[__is])...};                                              \
    }                                                                                              \
												   \
  template <_ArchTraits, typename _TV>                                                             \
    requires (!allow_clone || !__simd_clonable<_TV> || !_GLIBCXX_SIMD_HAS_SIMD_CLONE(fn))          \
    [[__gnu__::__const__]]                                                                         \
    extern _TV                                                                                     \
    __fast_##fn(_TV, _TV) noexcept;                                                                \
												   \
  template <_ArchTraits, typename _V0, typename _V1>                                               \
    [[__gnu__::__const__]]                                                                         \
    extern _GLIBCXX_SIMD_MATH_RET_TYPE(_V0, _V1)                                                   \
    __fast_2x_##fn(_V0, _V0, _V1, _V1) noexcept;                                                   \
												   \
  template <_TargetTraits, typename _TV>                                                           \
    _TV                                                                                            \
    __##fn(_TV, _TV) noexcept;                                                                     \
												   \
  template <_TargetTraits, typename _V0, typename _V1>                                             \
    _GLIBCXX_SIMD_MATH_RET_TYPE(_V0, _V1)                                                          \
    __2x_##fn(_V0, _V0, _V1, _V1) noexcept;                                                        \
												   \
  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>                                 \
    [[__gnu__::__always_inline__]]                                                                 \
    constexpr __deduced_vec_t<_Vp>                                                                 \
    fn(const _Vp& __x, const _Vp& __y)                                                             \
    {                                                                                              \
      if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)                                         \
	return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y);                                        \
      else if (__is_const_known(__x, __y))                                                         \
	return _Vp([&] [[__gnu__::__always_inline__]] (int __i) {                                  \
		 return std::fn(__x[__i], __y[__i]);                                               \
	       });                                                                                 \
      else if constexpr (_Vp::size() == 1)                                                         \
	return std::fn(__x[0], __y[0]);                                                            \
      else if constexpr (_Traits.template _M_eval_as_f32<typename _Vp::value_type>())              \
	return _Vp(fn<_Traits, rebind_t<float, _Vp>>(__x, __y));                                   \
      else if constexpr (_Vp::abi_type::_S_nreg == 1 && _Traits._M_fast_math())                    \
	return __fast_##fn<_ArchTraits(_Traits)._M_math_abi()>(__x._M_get(), __y._M_get());        \
      else if constexpr (_Vp::abi_type::_S_nreg == 1)                                              \
	return __##fn<_Traits._M_math_abi()>(__x._M_get(), __y._M_get());                          \
      _GLIBCXX_SIMD_MATH_CALL2_HANDLE_2X(fn)                                                       \
      else                                                                                         \
	return _Vp::_S_init(fn<_Traits>(__x._M_get_low(), __y._M_get_low()),                       \
			    fn<_Traits>(__x._M_get_high(), __y._M_get_high()));                    \
    }                                                                                              \
												   \
  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, fn)

#define _GLIBCXX_SIMD_MATH_1ARG_IMPL(fn)                                                           \
      if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)                                         \
	return fn<_Traits, __deduced_vec_t<_Vp>>(__x);                                             \
      else if (__is_const_known(__x))                                                              \
	return _Vp([&] [[__gnu__::__always_inline__]] (int __i) { return std::fn(__x[__i]); });    \
      else if constexpr (_Vp::size() == 1)                                                         \
	return std::fn(__x[0]);                                                                    \
      else if constexpr (_Traits.template _M_eval_as_f32<typename _Vp::value_type>())              \
	return _Vp(fn<_Traits, rebind_t<float, _Vp>>(__x));                                        \
      else if constexpr (_Vp::abi_type::_S_nreg > 1)                                               \
	return _Vp::_S_init(fn<_Traits>(__x._M_get_low()), fn<_Traits>(__x._M_get_high()));        \
      else                                                                                         \
	return __##fn<_Traits>(__x._M_get())

#define _GLIBCXX_SIMD_MATH_2ARG_IMPL(fn)                                                           \
      if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)                                         \
	return fn<_Traits, __deduced_vec_t<_Vp>>(__x, __y);                                        \
      else if (__is_const_known(__x, __y))                                                         \
	return _Vp([&] [[__gnu__::__always_inline__]] (int __i) {                                  \
		 return std::fn(__x[__i], __y[__i]);                                               \
	       });                                                                                 \
      else if constexpr (_Vp::size() == 1)                                                         \
	return std::fn(__x[0], __y[0]);                                                            \
      else if constexpr (_Traits.template _M_eval_as_f32<typename _Vp::value_type>())              \
	return _Vp(fn<_Traits, rebind_t<float, _Vp>>(__x, __y));                                   \
      else if constexpr (_Vp::abi_type::_S_nreg > 1)                                               \
	return _Vp::_S_init(fn<_Traits>(__x._M_get_low(), __y._M_get_low()),                       \
			    fn<_Traits>(__x._M_get_high(), __y._M_get_high()));                    \
      else                                                                                         \
	return __##fn<_Traits>(__x._M_get(), __y._M_get())

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<int, __deduced_vec_t<_Vp>>
    ilogb(const _Vp& __x)
    { _GLIBCXX_SIMD_MATH_1ARG_IMPL(ilogb); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    ldexp(const _Vp& __x, const rebind_t<int, __deduced_vec_t<_Vp>>& exp)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    scalbn(const _Vp& __x, const rebind_t<int, __deduced_vec_t<_Vp>>& n)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    scalbln(const _Vp& __x, const rebind_t<long int, __deduced_vec_t<_Vp>>& n)
    { static_assert(false, "TODO"); }

  template <signed_integral T, typename Abi>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<T, Abi>
    abs(const basic_vec<T, Abi>& __x)
    { return __x._M_abs(); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    abs(const _Vp& __x)
    { return static_cast<const __deduced_vec_t<_Vp>&>(__x)._M_fabs(); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    fabs(const _Vp& __x)
    { return static_cast<const __deduced_vec_t<_Vp>&>(__x)._M_fabs(); }

  template <_TargetTraits _Traits, __vec_builtin _TV>
    _TV
    __fabs(_TV __x)
    { return __vec_and(__vec_not(_S_signmask<_TV>), __x); }

  template <_TargetTraits _Traits, __vec_builtin _TV>
    _TV
    __ceil(_TV __x)
    {
      using _Tp = __vec_value_type<_TV>;
      constexpr unsigned long long __digits = numeric_limits<_Tp>::digits;
      static_assert(__CHAR_BIT__ * sizeof(1ull) >= __digits);
      constexpr _Tp __shifter = 1ull << (__digits - 1);
      const _TV __absx = __fabs<_Traits>(__x);
      _TV __truncated = __vec_cast<_Tp>(__vec_cast<__integer_from<sizeof(_Tp)>>(__x));
      __truncated += __truncated < __x ? _Tp(1) : _Tp();
      return __absx < __shifter ? __truncated : __x;
    }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    ceil(const _Vp& __x)
    { _GLIBCXX_SIMD_MATH_1ARG_IMPL(ceil); }

  template <_TargetTraits _Traits, __vec_builtin _TV>
    _TV
    __floor(_TV __x)
    {
      using _Tp = __vec_value_type<_TV>;
      constexpr unsigned long long __digits = numeric_limits<_Tp>::digits;
      const _TV __absx = __fabs<_Traits>(__x);
      static_assert(__CHAR_BIT__ * sizeof(1ull) >= __digits);
      constexpr _Tp __shifter = 1ull << (__digits - 1);
      _TV __truncated = __vec_cast<_Tp>(__vec_cast<__integer_from<sizeof(_Tp)>>(__x));
      __truncated -= __truncated > __x ? _Tp(1) : _Tp();
      return __absx < __shifter ? __truncated : __x;
    }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    floor(const _Vp& __x)
    { _GLIBCXX_SIMD_MATH_1ARG_IMPL(floor); }

  template <_TargetTraits _Traits, __vec_builtin _TV>
    _TV
    __nearbyint(_TV __x)
    {
      using _Tp = __vec_value_type<_TV>;
      constexpr unsigned long long __digits = numeric_limits<_Tp>::digits;
      static_assert(__CHAR_BIT__ * sizeof(1ull) >= __digits);
      constexpr _Tp __shifter = 1ull << (__digits - 1);
      const _TV __absx = __fabs<_Traits>(__x);
      _TV __rounded = __builtin_assoc_barrier(__absx + __shifter) - __shifter;
      __rounded = __vec_xor(__vec_and(_S_signmask<_TV>, __x), __rounded);
      if constexpr (_Traits._M_finite_math_only())
	return __rounded;
      else
	return __absx < __shifter ? __rounded : __x;
    }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    inline __deduced_vec_t<_Vp>
    nearbyint(const _Vp& __x)
    { _GLIBCXX_SIMD_MATH_1ARG_IMPL(nearbyint); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    inline __deduced_vec_t<_Vp>
    rint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    inline rebind_t<long int, __deduced_vec_t<_Vp>>
    lrint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    inline rebind_t<long long int, __deduced_vec_t<_Vp>>
    llrint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    round(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<long int, __deduced_vec_t<_Vp>>
    lround(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<long long int, __deduced_vec_t<_Vp>>
    llround(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    fmod(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, fmod)

  template <_TargetTraits _Traits, __vec_builtin _TV>
    _TV
    __trunc(_TV __x)
    {
      using _Tp = __vec_value_type<_TV>;
      constexpr unsigned long long __digits = numeric_limits<_Tp>::digits;
      const _TV __absx = __fabs<_Traits>(__x);
      static_assert(__CHAR_BIT__ * sizeof(1ull) >= __digits);
      constexpr _Tp __shifter = 1ull << (__digits - 1);
      _TV __truncated = __vec_cast<_Tp>(__vec_cast<__integer_from<sizeof(_Tp)>>(__x));
      return __absx < __shifter ? __truncated : __x;
    }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    trunc(const _Vp& __x)
    { _GLIBCXX_SIMD_MATH_1ARG_IMPL(trunc); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    remainder(const _Vp& __x, const _Vp& __y)
    { _GLIBCXX_SIMD_MATH_2ARG_IMPL(remainder); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, remainder)

  template <_TargetTraits _Traits, __vec_builtin _TV>
    _TV
    __copysign(_TV __x, _TV __y)
    {
      constexpr _TV __absmask = __vec_not(_S_signmask<_TV>);
      return __vec_or(__vec_andnot(__absmask, __y), __vec_and(__absmask, __x));
    }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    copysign(const _Vp& __x, const _Vp& __y)
    { _GLIBCXX_SIMD_MATH_2ARG_IMPL(copysign); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, copysign)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    nextafter(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, nextafter)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    fdim(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, fdim)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    fmax(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, fmax)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    fmin(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, fmin)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    fma(const _Vp& __x, const _Vp& __y, const _Vp& __z)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_3ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, fma)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<int, __deduced_vec_t<_Vp>>
    fpclassify(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isfinite(const _Vp& __x)
    {
      using _Kp = typename __deduced_vec_t<_Vp>::mask_type;
      if constexpr (_Traits._M_finite_math_only())
	return _Kp(true);
      else if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)
	return isfinite<_Traits, __deduced_vec_t<_Vp>>(__x);
      else if (__is_const_known(__x))
	return _Kp([&] [[__gnu__::__always_inline__]] (int __i) {
		 return std::isfinite(__x[__i]);
	       });
      else if constexpr (_Vp::size() == 1)
	return std::isfinite(__x[0]);
      else if constexpr (_Traits.template _M_eval_as_f32<typename _Vp::value_type>())
	return _Kp(isfinite<_Traits, rebind_t<float, _Vp>>(__x));
      else if constexpr (_Vp::abi_type::_S_nreg > 1)
	return _Kp::_S_concat(isfinite<_Traits>(__x._M_get_low()),
			      isfinite<_Traits>(__x._M_get_high()));
      else
	{
	  using _Tp = typename _Vp::value_type;
	  // use integer compare to avoid raising FE_INVALID
	  using _Ip = __integer_from<sizeof(_Tp)>;
	  using _IV = rebind_t<_Ip, _Vp>;
	  const _IV __ai = __builtin_bit_cast(_IV, fabs<_Traits>(__x));
	  return __ai <= __builtin_bit_cast(_Ip, numeric_limits<_Tp>::max());
	}
    }

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

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isnormal(const _Vp& __x)
    {
      using _Kp = typename __deduced_vec_t<_Vp>::mask_type;
      if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)
	return isnormal<_Traits, __deduced_vec_t<_Vp>>(__x);
      else if (__is_const_known(__x))
	return _Kp([&] [[__gnu__::__always_inline__]] (int __i) {
		 return std::isnormal(__x[__i]);
	       });
      else if constexpr (_Vp::size() == 1)
	return std::isnormal(__x[0]);
      else if constexpr (_Traits.template _M_eval_as_f32<typename _Vp::value_type>())
	return _Kp(isnormal<_Traits, rebind_t<float, _Vp>>(__x));
      else if constexpr (_Vp::abi_type::_S_nreg > 1)
	return _Kp::_S_concat(isnormal<_Traits>(__x._M_get_low()),
			      isnormal<_Traits>(__x._M_get_high()));
      // TODO: Use fpclassp[ds] instruction with AVX512
      else
	{
	  using _Tp = typename _Vp::value_type;
	  using _Lm = numeric_limits<_Tp>;
	  const _Vp __a = fabs<_Traits>(__x);
	  if constexpr (_Traits._M_finite_math_only())
	    return __a >= _Lm::min();
	  else
	    { // use integer compare to avoid raising FE_INVALID
	      using _Ip = __integer_from<sizeof(_Tp)>;
	      using _IV = rebind_t<_Ip, _Vp>;
	      const _IV __ai = __builtin_bit_cast(_IV, __a);
	      return __ai >= __builtin_bit_cast(_Ip, _Lm::min())
		       && __ai <= __builtin_bit_cast(_Ip, _Lm::max());
	    }
	}
    }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    signbit(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isgreater(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr typename __deduced_vec_t<_Vp>::mask_type,
				    isgreater)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isgreaterequal(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr typename __deduced_vec_t<_Vp>::mask_type,
				    isgreaterequal)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isless(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr typename __deduced_vec_t<_Vp>::mask_type,
				    isless)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    islessequal(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr typename __deduced_vec_t<_Vp>::mask_type,
				    islessequal)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    islessgreater(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr typename __deduced_vec_t<_Vp>::mask_type,
				    islessgreater)

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isunordered(const _Vp& __x, const _Vp& __y)
    {
      if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)
	return isunordered<_Traits, __deduced_vec_t<_Vp>>(__x, __y);
      else
	return __x._M_isunordered(__y);
    }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(constexpr typename __deduced_vec_t<_Vp>::mask_type,
				    isunordered)

  _GLIBCXX_SIMD_MATH_CALL(acos)
  _GLIBCXX_SIMD_MATH_CALL(asin)
  _GLIBCXX_SIMD_MATH_CALL(atan)
  _GLIBCXX_SIMD_MATH_CALL2(atan2, true)
  _GLIBCXX_SIMD_MATH_CALL(cos)
  _GLIBCXX_SIMD_MATH_CALL(sin)
  _GLIBCXX_SIMD_MATH_CALL(tan)
  _GLIBCXX_SIMD_MATH_CALL(acosh)
  _GLIBCXX_SIMD_MATH_CALL(asinh)
  _GLIBCXX_SIMD_MATH_CALL(atanh)
  _GLIBCXX_SIMD_MATH_CALL(cosh)
  _GLIBCXX_SIMD_MATH_CALL(sinh)
  _GLIBCXX_SIMD_MATH_CALL(tanh)

  _GLIBCXX_SIMD_MATH_CALL(exp)
  _GLIBCXX_SIMD_MATH_CALL(exp2)
  _GLIBCXX_SIMD_MATH_CALL(expm1)

  _GLIBCXX_SIMD_MATH_CALL(log)
  _GLIBCXX_SIMD_MATH_CALL(log10)
  _GLIBCXX_SIMD_MATH_CALL(log1p)
  _GLIBCXX_SIMD_MATH_CALL(log2)
  _GLIBCXX_SIMD_MATH_CALL(logb)

  _GLIBCXX_SIMD_MATH_CALL(cbrt)

  _GLIBCXX_SIMD_MATH_CALL2(hypot, false)

  template <_ArchTraits, typename _TV>
    [[__gnu__::__const__]]
    extern _TV
    __fast_hypot(_TV, _TV, _TV) noexcept;

  template <_ArchTraits, typename _V0, typename _V1>
    [[__gnu__::__const__]]
    extern _GLIBCXX_SIMD_MATH_RET_TYPE(_V0, _V1)
    __fast_2x_hypot(_V0, _V0, _V0, _V1, _V1, _V1) noexcept;

  template <_TargetTraits, typename _TV>
    extern _TV
    __hypot(_TV, _TV, _TV) noexcept;

  template <_TargetTraits, typename _V0, typename _V1>
    extern _GLIBCXX_SIMD_MATH_RET_TYPE(_V0, _V1)
    __2x_hypot(_V0, _V0, _V0, _V1, _V1, _V1) noexcept;

  template<_TargetTraits _Traits = {}, typename _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    hypot(const _Vp& __x, const _Vp& __y, const _Vp& __z)
    {
      static_assert(!_Traits._M_math_abi()._M_have_mmx());
      if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)
	return hypot<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __z);
      else if (__is_const_known(__x, __y, __z))
	return _Vp([&] [[__gnu__::__always_inline__]] (int __i) {
		 return std::hypot(__x[__i], __y[__i], __z[__i]);
	       });
      else if constexpr (_Vp::size() == 1)
	return std::hypot(__x[0], __y[0], __z[0]);
      else if constexpr (_Traits.template _M_eval_as_f32<typename _Vp::value_type>())
	return _Vp(hypot<_Traits, rebind_t<float, _Vp>>(__x, __y, __z));
      else if constexpr (_Vp::abi_type::_S_nreg > 2)
	return _Vp::_S_init(hypot<_Traits>(__x._M_get_low(), __y._M_get_low()),
			    hypot<_Traits>(__x._M_get_high(), __y._M_get_high()));
      else if constexpr (_Vp::abi_type::_S_nreg == 2 && _Traits._M_fast_math())
	{
	  _GLIBCXX_SIMD_MATH_2X_CALL3(__fast_2x_hypot<_ArchTraits(_Traits)._M_math_abi()>,
				      __x, __y, __z);
	  return _Vp::_S_init(__lo, __hi);
	}
      else if constexpr (_Vp::abi_type::_S_nreg == 2 && !_Traits._M_fast_math())
	{
	  _GLIBCXX_SIMD_MATH_2X_CALL3(__2x_hypot<_Traits._M_math_abi()>, __x, __y, __z);
	  return _Vp::_S_init(__lo, __hi);
	}
      else if constexpr (_Vp::abi_type::_S_nreg == 1 && _Traits._M_fast_math())
	return __fast_hypot<_ArchTraits(_Traits)._M_math_abi()>(__x._M_get(), __y._M_get(),
								__z._M_get());
      else if constexpr (_Vp::abi_type::_S_nreg == 1)
	return __hypot<_Traits._M_math_abi()>(__x._M_get(), __y._M_get(), __z._M_get());
      else
	static_assert(false);
    }

  _GLIBCXX_SIMD_MATH_3ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, hypot)

  _GLIBCXX_SIMD_MATH_CALL2(pow, true)

  template <_TargetTraits _Traits, __vec_builtin _TV>
    [[__gnu__::__always_inline__]]
    inline _TV
    __sqrt(_TV __x)
    {
      constexpr bool __is_double = is_same_v<__vec_value_type<_TV>, double>;
      constexpr bool __is_float = is_same_v<__vec_value_type<_TV>, float>;
      constexpr bool __is_flt16 = is_same_v<__vec_value_type<_TV>, _Float16>;
#if _GLIBCXX_X86
      if constexpr (sizeof(__x) < 16)
	return _VecOps<_TV>::_S_extract(__sqrt<_Traits>(__vec_zero_pad_to_16(__x)));
      else if constexpr (__is_double && sizeof(__x) == 16)
	return __builtin_ia32_sqrtpd(__x);
      else if constexpr (__is_double && sizeof(__x) == 32)
	return __builtin_ia32_sqrtpd256(__x);
      else if constexpr (__is_double && sizeof(__x) == 64)
	return __builtin_ia32_sqrtpd512_mask(__x, _TV(), -1, 0x04);
      else if constexpr (__is_float && sizeof(__x) == 16)
	return __builtin_ia32_sqrtps(__x);
      else if constexpr (__is_float && sizeof(__x) == 32)
	return __builtin_ia32_sqrtps256(__x);
      else if constexpr (__is_float && sizeof(__x) == 64)
	return __builtin_ia32_sqrtps512_mask(__x, _TV(), -1, 0x04);
      else if constexpr (__is_flt16 && sizeof(__x) == 16)
	return __builtin_ia32_sqrtph128_mask(__x, _TV(), -1);
      else if constexpr (__is_flt16 && sizeof(__x) == 32)
	return __builtin_ia32_sqrtph256_mask(__x, _TV(), -1);
      else if constexpr (__is_flt16 && sizeof(__x) == 64)
	return __builtin_ia32_sqrtph512_mask_round (__x, _TV(), -1, 0x04);
      else
	static_assert(false);
#endif
    }

  template <_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    sqrt(const _Vp& __x)
    { _GLIBCXX_SIMD_MATH_1ARG_IMPL(sqrt); }

  _GLIBCXX_SIMD_MATH_CALL(erf)
  _GLIBCXX_SIMD_MATH_CALL(erfc)
  _GLIBCXX_SIMD_MATH_CALL(lgamma)
  _GLIBCXX_SIMD_MATH_CALL(tgamma)

  template <__simd_floating_point _Vp, _TargetTraits = {}>
    inline _Vp
    __lerp(_Vp __a, _Vp __b, _Vp __t) noexcept
    {
      constexpr _Vp __zero = {};
      constexpr _Vp __one(1);
      using _Mp = typename _Vp::mask_type;

      // TODO: benchmark which method of computing the mask is better
#if 1
      const _Mp __different_sign = (__a * __b <= __zero);
#elif 0
      const _Mp __different_sign
	= (__a <= __zero && __b >= __zero) || (__a >= __zero && __b <= __zero);
#else
      using _IV = rebind_t<__integer_from<sizeof(typename _Vp::value_type)>, _Vp>;
      const _Mp __different_sign
	= ((bit_cast<_IV>(__a) ^ bit_cast<_IV>(__b)) < _IV()) || __a == __zero || __b == __zero;
#endif

      const _Vp __r = __t * __b + (__one - __t) * __a; // __r is also exact at __t=1
      if (all_of(__different_sign))
	return __r;
      else
	{
	  // Exact at __t=0, monotonic except near __t=1,
	  // bounded, determinate, and consistent:
	  const _Vp __x = __a + __t * (__b - __a);
	  return select(__different_sign || __t == __one, __r,
			select((__t > __one) == (__b > __a),
			       select(__b < __x, __x, __b),
			       select(__b > __x, __x, __b)));  // monotonic near __t=1
	}
    }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    lerp(const _Vp& __a, const _Vp& __b, const _Vp& __t) noexcept
    {
      if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)
	return lerp<_Traits, __deduced_vec_t<_Vp>>(__a, __b, __t);
      else if (__is_const_known(__a, __b, __t))
	return _Vp([&] [[__gnu__::__always_inline__]] (int __i) {
		 return std::lerp(__a[__i], __b[__i], __t[__i]);
	       });
      else if constexpr (_Vp::size() == 1)
	return std::lerp(__a[0], __b[0], __t[0]);
      else if constexpr (_Traits.template _M_eval_as_f32<typename _Vp::value_type>())
	{ // Any operation on value_type converts to float anyway (and back). Thus, it's more
	  // efficient if we can reduce the number of conversions (and use a simpler formula).
	  //
	  // Sketch of a proof that ta+(1-t)a == a for all t (consistency):
	  //
	  // Large t, such that (1-t) == -t would be problematic, but _Float16 max is 65504
	  //   => no issue
	  // Small t, such that (1-t) is inexact would be problematic.
	  // - Consider _Float16 sub-min (2^-24) => 1-2^24, which is exact in binary32.
	  //   => For any number larger than 2^-24, (1-t) cannot be inexact either.
	  // - Consider negative sub-min (-2^-24) => 1+2^24, which is off by .5 ULP in binary32.
	  // - The ULP of the smallest normal negative number also is -2^-24, which leads to the
	  // same .5 ULP loss of precision for (1-t).
	  // This .5 ULP loss does not matter when computing ta+(1-t)a because ta is smaller by
	  // a factor of 2^24 and thus after rounding to binary16, no error is left.
	  using _Vf = rebind_t<float, _Vp>;
	  const _Vf __th = _Vf(__t);
	  return _Vp(__th * __b + (_Vf(1) - __th) * __a);
	}
      else if constexpr (_Vp::abi_type::_S_nreg > 1)
	return _Vp::_S_init(lerp<_Traits>(__a._M_get_low(), __b._M_get_low(), __t._M_get_low()),
			    lerp<_Traits>(__a._M_get_high(), __b._M_get_high(), __t._M_get_high()));
      else
	return __lerp(__a, __b, __t);
    }

  _GLIBCXX_SIMD_MATH_3ARG_OVERLOADS(constexpr __deduced_vec_t<_Vp>, lerp)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    assoc_laguerre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __n,
		   const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __m, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    assoc_legendre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __l,
		   const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __m, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    beta(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(__deduced_vec_t<_Vp>, beta)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    comp_ellint_1(const _Vp& __k)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    comp_ellint_2(const _Vp& __k)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    comp_ellint_3(const _Vp& __k, const _Vp& __nu)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(__deduced_vec_t<_Vp>, comp_ellint_3)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    cyl_bessel_i(const _Vp& __nu, const _Vp& __x)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(__deduced_vec_t<_Vp>, cyl_bessel_i)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    cyl_bessel_j(const _Vp& __nu, const _Vp& __x)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(__deduced_vec_t<_Vp>, cyl_bessel_j)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    cyl_bessel_k(const _Vp& __nu, const _Vp& __x)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(__deduced_vec_t<_Vp>, cyl_bessel_k)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    cyl_neumann(const _Vp& __nu, const _Vp& __x)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(__deduced_vec_t<_Vp>, cyl_neumann)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    ellint_1(const _Vp& __k, const _Vp& __phi)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(__deduced_vec_t<_Vp>, ellint_1)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    ellint_2(const _Vp& __k, const _Vp& __phi)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_2ARG_OVERLOADS(__deduced_vec_t<_Vp>, ellint_2)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    ellint_3(const _Vp& __k, const _Vp& __nu, const _Vp& __phi)
    { static_assert(false, "TODO"); }

  _GLIBCXX_SIMD_MATH_3ARG_OVERLOADS(__deduced_vec_t<_Vp>, ellint_3)

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    expint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    hermite(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    laguerre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    legendre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __l, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    riemann_zeta(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    sph_bessel(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    sph_legendre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __l,
		 const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __m, const _Vp& __theta)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    sph_neumann(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    frexp(const _Vp& __value, rebind_t<int, __deduced_vec_t<_Vp>>* __exp)
    { static_assert(false, "TODO"); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    remquo(const _Vp& __x, const _Vp& __y, rebind_t<int, __deduced_vec_t<_Vp>>* __quo)
    {
      if constexpr (!is_same_v<_Vp, __deduced_vec_t<_Vp>>)
	return remquo<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __quo);
      else //if (__is_const_known(__x, __y))
	{
	  int __tmp[_Vp::size()] = {};
	  const _Vp __r([&] [[__gnu__::__always_inline__]] (int __i) {
		      return std::remquo(__x[__i], __y[__i], &__tmp[__i]);
		    });
	  *__quo = rebind_t<int, __deduced_vec_t<_Vp>>(__tmp);
	  return __r;
	}
#if 0
      else if constexpr (_Vp::size() == 1)
	{
	  const _Vp __r = std::remquo(__x[0], __y[0], &__tmp[0]);
	  *__quo = __tmp[0];
	  return __r;
	}
      else if constexpr (_Traits.template _M_eval_as_f32<typename _Vp::value_type>())
	// TODO: is __quo always of the correct type?
	return _Vp(remquo<_Traits, rebind_t<float, _Vp>>(__x, __y, __quo));
      else
	static_assert(false, "TODO");
#endif
    }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    remquo(const __deduced_vec_t<_Vp>& __x, const _Vp& __y,
	   rebind_t<int, __deduced_vec_t<_Vp>>* __quo)
    { return remquo<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __quo); }

  template<_TargetTraits _Traits = {}, __math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    remquo(const _Vp& __x, const __deduced_vec_t<_Vp>& __y,
	   rebind_t<int, __deduced_vec_t<_Vp>>* __quo)
    { return remquo<_Traits, __deduced_vec_t<_Vp>>(__x, __y, __quo); }

  template<class T, class Abi>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<T, Abi>
    modf(const type_identity_t<basic_vec<T, Abi>>& __value, basic_vec<T, Abi>* __iptr)
    { static_assert(false, "TODO"); }
} // namespace simd

// clean up internal macros
#undef _GLIBCXX_SIMD_HAS_SIMD_CLONE
#undef _GLIBCXX_SIMD_FN_NAME
#undef _GLIBCXX_SIMD_MATH_2X_CALL
#undef _GLIBCXX_SIMD_MATH_2X_CALL2
#undef _GLIBCXX_SIMD_MATH_CALL
#undef _GLIBCXX_SIMD_MATH_CALL2

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

// [simd.complex.math] --------------------------------------------------------
namespace simd
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
    polar(const _Vp& __r, const _Vp& __theta = {})
    {
      // TODO: use sincos instead of two calls
      return {__r * cos(__theta), __r * sin(__theta)};
    }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    pow(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }
} // namespace simd

  using simd::real;
  using simd::imag;
  using simd::arg;
  using simd::norm;
  using simd::conj;
  using simd::proj;
  using simd::polar;

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std

#pragma GCC diagnostic pop
#endif // C++26
#endif // _GLIBCXX_SIMD_MATH_H
#endif
