/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef BITS_SIMD_COMPLEX_H_
#define BITS_SIMD_COMPLEX_H_

#include "simd.h"

namespace std::datapar
{
  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    real(const _Vp& __x) noexcept
    { return __x.real(); }

  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    imag(const _Vp& __x) noexcept
    { return __x.imag(); }

  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    abs(const _Vp& __x)
    { return __x._M_abs(); }

  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    arg(const _Vp&);

  template <__detail::__simd_complex _Vp>
    constexpr rebind_t<typename _Vp::value_type::value_type, _Vp>
    norm(const _Vp& __x)
    { return __x._M_norm(); }

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    conj(const _Vp& __x)
    { return __x._M_conj(); }

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
    polar(const _Vp& __r, const _Vp& __theta);

  template <__detail::__simd_complex _Vp>
    constexpr _Vp
    pow(const _Vp& __x, const _Vp& __y);
}

#endif  // BITS_SIMD_COMPLEX_H_
