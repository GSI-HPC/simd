/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef BITS_SIMD_BIT_H_
#define BITS_SIMD_BIT_H_

#include <bit>

namespace std::datapar
{
  template<__detail::__simd_integral _Vp>
    constexpr _Vp
    byteswap(const _Vp& __v) noexcept
    { return _Vp([&](int __i) { return std::byteswap(__v[__i]); }); }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr _Vp
    bit_ceil(const _Vp& __v)
    {
      using _Tp = typename _Vp::value_type;
      constexpr _Tp __max = _Tp(1) << (sizeof(_Tp) * __CHAR_BIT__ - 1);
      __glibcxx_simd_precondition(all_of(__v <= __max), "bit_ceil result is not representable");
      return _Vp([&](int __i) { return std::bit_ceil(__v[__i]); });
    }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr _Vp
    bit_floor(const _Vp& __v) noexcept
    { return _Vp([&](int __i) { return std::bit_floor(__v[__i]); }); }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr typename _Vp::mask_type
    has_single_bit(const _Vp& __v) noexcept
    { return typename _Vp::mask_type([&](int __i) { return std::has_single_bit(__v[__i]); }); }

  template<__detail::__simd_unsigned_integer _V0, __detail::__simd_integral _V1>
    requires __detail::__simd_matching_width<_V0, _V1>
    constexpr _V0
    rotl(const _V0& __v, const _V1& __s) noexcept
    { return _V0([&](int __i) { return std::rotl(__v[__i], __s[__i]); }); }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr _Vp
    rotl(const _Vp& __v, int __s) noexcept
    { return _Vp([&](int __i) { return std::rotl(__v[__i], __s); }); }

  template<__detail::__simd_unsigned_integer _V0, __detail::__simd_integral _V1>
    requires __detail::__simd_matching_width<_V0, _V1>
    constexpr _V0
    rotr(const _V0& __v, const _V1& __s) noexcept
    { return _V0([&](int __i) { return std::rotr(__v[__i], __s[__i]); }); }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr _Vp
    rotr(const _Vp& __v, int __s) noexcept
    { return _Vp([&](int __i) { return std::rotr(__v[__i], __s); }); }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    bit_width(const _Vp& __v) noexcept
    {
      using _Rp = rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>;
      return _Rp([&](int __i) { return std::bit_width(__v[__i]); });
    }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_zero(const _Vp& __v) noexcept
    {
      using _Rp = rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>;
      return _Rp([&](int __i) { return std::countl_zero(__v[__i]); });
    }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_one(const _Vp& __v) noexcept
    {
      using _Rp = rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>;
      return _Rp([&](int __i) { return std::countl_one(__v[__i]); });
    }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_zero(const _Vp& __v) noexcept
    {
      using _Rp = rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>;
      return _Rp([&](int __i) { return std::countr_zero(__v[__i]); });
    }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_one(const _Vp& __v) noexcept
    {
      using _Rp = rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>;
      return _Rp([&](int __i) { return std::countr_one(__v[__i]); });
    }

  template<__detail::__simd_unsigned_integer _Vp>
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    popcount(const _Vp& __v) noexcept
    {
      using _Rp = rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>;
      return _Rp([&](int __i) { return std::popcount(__v[__i]); });
    }
}

#endif  // BITS_SIMD_BIT_H_
