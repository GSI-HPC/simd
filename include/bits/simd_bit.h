/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef INCLUDE_BITS_SIMD_BIT_H_
#define INCLUDE_BITS_SIMD_BIT_H_

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

// [simd.bit] -----------------------------------------------------------------
namespace std::simd
{
  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    byteswap(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    bit_ceil(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    bit_floor(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename _Vp::mask_type
    has_single_bit(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _V0, __simd_vec_type _V1>
    [[__gnu__::__always_inline__]]
    constexpr _V0
    rotl(const _V0& __v, const _V1& __s) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    rotl(const _Vp& __v, int __s) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _V0, __simd_vec_type _V1>
    [[__gnu__::__always_inline__]]
    constexpr _V0
    rotr(const _V0& __v, const _V1& __s) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    rotr(const _Vp& __v, int __s) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    bit_width(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_zero(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_one(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_zero(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_one(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    popcount(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }
}

namespace std
{
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
}

#pragma GCC diagnostic pop

#endif  // INCLUDE_BITS_SIMD_BIT_H_
