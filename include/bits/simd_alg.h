/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef _GLIBCXX_SIMD_ALG_H
#define _GLIBCXX_SIMD_ALG_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include "simd_vec.h"

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

// [simd.alg] -----------------------------------------------------------------
namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION
namespace simd
{
  template<typename _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<_Tp, _Ap>
    min(const basic_vec<_Tp, _Ap>& __a, const basic_vec<_Tp, _Ap>& __b) noexcept
    { return __select_impl(__a < __b, __a, __b); }

  template<typename _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<_Tp, _Ap>
    max(const basic_vec<_Tp, _Ap>& __a, const basic_vec<_Tp, _Ap>& __b) noexcept
    { return __select_impl(__a < __b, __b, __a); }

  template<typename _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr pair<basic_vec<_Tp, _Ap>, basic_vec<_Tp, _Ap>>
    minmax(const basic_vec<_Tp, _Ap>& __a, const basic_vec<_Tp, _Ap>& __b) noexcept
    { return {min(__a, __b), max(__a, __b)}; }

  template<typename _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<_Tp, _Ap>
    clamp(const basic_vec<_Tp, _Ap>& __v, const basic_vec<_Tp, _Ap>& __lo,
	  const basic_vec<_Tp, _Ap>& __hi)
    {
      __glibcxx_simd_precondition(none_of(__lo > __hi), "lower bound is larger than upper bound");
      return max(__lo, min(__hi, __v));
    }

  template<typename _Tp, typename _Up>
    constexpr auto
    select(bool __c, const _Tp& __a, const _Up& __b)
    -> remove_cvref_t<decltype(__c ? __a : __b)>
    { return __c ? __a : __b; }

  template<size_t _Bytes, typename _Ap, typename _Tp, typename _Up>
    [[__gnu__::__always_inline__]]
    constexpr auto
    select(const basic_mask<_Bytes, _Ap>& __c, const _Tp& __a, const _Up& __b)
    noexcept -> decltype(__select_impl(__c, __a, __b))
    { return __select_impl(__c, __a, __b); }
} // namespace simd

  using simd::min;
  using simd::max;
  using simd::minmax;
  using simd::clamp;

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std

#pragma GCC diagnostic pop
#endif // C++26
#endif // _GLIBCXX_SIMD_ALG_H
