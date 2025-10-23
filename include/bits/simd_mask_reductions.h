/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef _GLIBCXX_SIMD_MASK_REDUCTIONS_H
#define _GLIBCXX_SIMD_MASK_REDUCTIONS_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include "simd_mask.h"

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

// [simd.mask.reductions] -----------------------------------------------------
namespace std::simd
{
  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr bool
    all_of(const basic_mask<_Bytes, _Ap>& __k) noexcept
    { return __k._M_all_of(); }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr bool
    any_of(const basic_mask<_Bytes, _Ap>& __k) noexcept
    { return __k._M_any_of(); }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr bool
    none_of(const basic_mask<_Bytes, _Ap>& __k) noexcept
    { return __k._M_none_of(); }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr __simd_size_type
    reduce_count(const basic_mask<_Bytes, _Ap>& __k) noexcept
    {
      if constexpr (_Ap::_S_size == 1)
        return +__k[0];
      else if constexpr (__flags_test(_Ap::_S_variant, _AbiVariant::_VecMask))
        return -reduce(-__k);
      else
        return __k._M_reduce_count();
    }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr __simd_size_type
    reduce_min_index(const basic_mask<_Bytes, _Ap>& __k)
    { return __k._M_reduce_min_index(); }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr __simd_size_type
    reduce_max_index(const basic_mask<_Bytes, _Ap>& __k)
    { return __k._M_reduce_max_index(); }

  constexpr bool
  all_of(same_as<bool> auto __x) noexcept
  { return __x; }

  constexpr bool
  any_of(same_as<bool> auto __x) noexcept
  { return __x; }

  constexpr bool
  none_of(same_as<bool> auto __x) noexcept
  { return !__x; }

  constexpr __simd_size_type
  reduce_count(same_as<bool> auto __x) noexcept
  { return __x; }

  constexpr __simd_size_type
  reduce_min_index(same_as<bool> auto __x)
  { return 0; }

  constexpr __simd_size_type
  reduce_max_index(same_as<bool> auto __x)
  { return 0; }
}

#pragma GCC diagnostic pop
#endif // C++26
#endif // _GLIBCXX_SIMD_MASK_REDUCTIONS_H
