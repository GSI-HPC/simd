/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023-2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

// [simd.alg]

#ifndef PROTOTYPE_SIMD_ALG_H_
#define PROTOTYPE_SIMD_ALG_H_

#include "simd.h"
#include "simd_mask.h"

namespace std
{
  template<totally_ordered _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    min(const basic_simd<_Tp, _Abi>& __a, const basic_simd<_Tp, _Abi>& __b) noexcept
    { return {__detail::__private_init, _Abi::_SimdImpl::_S_min(__a._M_data, __b._M_data)}; }

  template<totally_ordered _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    max(const basic_simd<_Tp, _Abi>& __a, const basic_simd<_Tp, _Abi>& __b) noexcept
    { return {__detail::__private_init, _Abi::_SimdImpl::_S_max(__a._M_data, __b._M_data)}; }

  template<totally_ordered _Tp, typename _Abi>
    constexpr pair<basic_simd<_Tp, _Abi>, basic_simd<_Tp, _Abi>>
    minmax(const basic_simd<_Tp, _Abi>& __a, const basic_simd<_Tp, _Abi>& __b) noexcept
    {
      pair<basic_simd<_Tp, _Abi>, basic_simd<_Tp, _Abi>> __r = {__a, __b};
      _Abi::_SimdImpl::_S_minmax(__r.first._M_data, __r.second._M_data);
      return __r;
    }

  template<totally_ordered _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    clamp(const basic_simd<_Tp, _Abi>& __v, const basic_simd<_Tp, _Abi>& __lo,
          const basic_simd<_Tp, _Abi>& __hi)
    {
      __glibcxx_simd_precondition(none_of(__lo > __hi), "lower bound is larger than upper bound");
      return {__detail::__private_init,
              _Abi::_SimdImpl::_S_clamp(__v._M_data, __lo._M_data, __hi._M_data)};
    }

  template <typename _Tp, typename _Up>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    simd_select(bool __c, const _Tp& __a, const _Up& __b)
    -> remove_cvref_t<decltype(__c ? __a : __b)>
    { return __c ? __a : __b; }

  template <size_t _Bytes, typename _Abi, typename _Tp, typename _Up>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    simd_select(const basic_simd_mask<_Bytes, _Abi>& __k, const _Tp& __a, const _Up& __b) noexcept
    -> decltype(__select_impl(__k, __a, __b))
    { return __select_impl(__k, __a, __b); }
}

#endif  // PROTOTYPE_SIMD_ALG_H_
