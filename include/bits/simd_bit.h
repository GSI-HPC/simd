/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#if VIR_NEXT_PATCH
#ifndef _GLIBCXX_SIMD_BIT_H
#define _GLIBCXX_SIMD_BIT_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include "simd_vec.h"

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

// [simd.bit] -----------------------------------------------------------------
namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION
namespace simd
{
  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    byteswap(const _Vp& __v) noexcept
    { return _Vp([&](int __i) { return std::byteswap(__v[__i]); }); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    bit_ceil(const _Vp& __v)
    {
      using _Tp = typename _Vp::value_type;
      constexpr _Tp __max = _Tp(1) << (sizeof(_Tp) * __CHAR_BIT__ - 1);
      __glibcxx_simd_precondition(all_of(__v <= __max), "bit_ceil result is not representable");
      return _Vp([&](int __i) { return std::bit_ceil(__v[__i]); });
    }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    bit_floor(const _Vp& __v) noexcept
    { return _Vp([&](int __i) { return std::bit_floor(__v[__i]); }); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename _Vp::mask_type
    has_single_bit(const _Vp& __v) noexcept
    { return typename _Vp::mask_type([&](int __i) { return std::has_single_bit(__v[__i]); }); }

  template<__simd_vec_type _V0, __simd_vec_type _V1>
    [[__gnu__::__always_inline__]]
    constexpr _V0
    rotl(const _V0& __v, const _V1& __s) noexcept
    { return _V0([&](int __i) { return std::rotl(__v[__i], __s[__i]); }); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    rotl(const _Vp& __v, int __s) noexcept
    { return _Vp([&](int __i) { return std::rotl(__v[__i], __s); }); }

  template<__simd_vec_type _V0, __simd_vec_type _V1>
    [[__gnu__::__always_inline__]]
    constexpr _V0
    rotr(const _V0& __v, const _V1& __s) noexcept
    { return _V0([&](int __i) { return std::rotr(__v[__i], __s[__i]); }); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    rotr(const _Vp& __v, int __s) noexcept
    { return _Vp([&](int __i) { return std::rotr(__v[__i], __s); }); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    bit_width(const _Vp& __v) noexcept
    {
      return rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>([&](int __i) {
	       return std::bit_width(__v[__i]);
	     });
    }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_zero(const _Vp& __v) noexcept
    {
      return rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>([&](int __i) {
	       return std::countl_zero(__v[__i]);
	     });
    }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_one(const _Vp& __v) noexcept
    {
      return rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>([&](int __i) {
	       return std::countl_one(__v[__i]);
	     });
    }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_zero(const _Vp& __v) noexcept
    {
      return rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>([&](int __i) {
	       return std::countr_zero(__v[__i]);
	     });
    }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_one(const _Vp& __v) noexcept
    {
      return rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>([&](int __i) {
	       return std::countr_one(__v[__i]);
	     });
    }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    popcount(const _Vp& __v) noexcept
    {
      return rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>([&](int __i) {
	       return std::popcount(__v[__i]);
	     });
    }
} // namespace simd

  using simd::byteswap;
  using simd::bit_ceil;
  using simd::bit_floor;
  using simd::has_single_bit;
  using simd::rotl;
  using simd::rotr;
  using simd::bit_width;
  using simd::countl_zero;
  using simd::countl_one;
  using simd::countr_zero;
  using simd::countr_one;
  using simd::popcount;
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std

#pragma GCC diagnostic pop
#endif // C++26
#endif // _GLIBCXX_SIMD_BIT_H
#endif
