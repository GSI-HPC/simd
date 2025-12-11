/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef _GLIBCXX_SIMD_REDUCTIONS_H
#define _GLIBCXX_SIMD_REDUCTIONS_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include "simd_vec.h"

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

// [simd.reductions] ----------------------------------------------------------
namespace std::simd
{
  template <typename _Tp, typename _Ap, __reduction_binary_operation<_Tp> _BinaryOperation = plus<>>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce(const basic_vec<_Tp, _Ap>& __x, _BinaryOperation __binary_op = {})
    { return __x._M_reduce(__binary_op); }

  template <typename _Tp, typename _Ap, __reduction_binary_operation<_Tp> _BinaryOperation = plus<>>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce(const basic_vec<_Tp, _Ap>& __x, const typename basic_vec<_Tp, _Ap>::mask_type& __mask,
           _BinaryOperation __binary_op = {}, type_identity_t<_Tp> __identity_element
             = __default_identity_element<_Tp, _BinaryOperation>())
    { return reduce(__select_impl(__mask, __x, __identity_element), __binary_op); }

  template <totally_ordered _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce_min(const basic_vec<_Tp, _Ap>& __x) noexcept
    {
      return reduce(__x, []<typename _UV>(const _UV& __a, const _UV& __b) {
               return __select_impl(__a < __b, __a, __b);
             });
    }

  template <totally_ordered _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce_min(const basic_vec<_Tp, _Ap>& __x,
               const typename basic_vec<_Tp, _Ap>::mask_type& __mask) noexcept
    {
      return reduce(__select_impl(__mask, __x, numeric_limits<_Tp>::max()),
                    []<typename _UV>(const _UV& __a, const _UV& __b) {
                      return __select_impl(__a < __b, __a, __b);
                    });
    }

  template <totally_ordered _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce_max(const basic_vec<_Tp, _Ap>& __x) noexcept
    {
      return reduce(__x, []<typename _UV>(const _UV& __a, const _UV& __b) {
               return __select_impl(__a < __b, __b, __a);
             });
    }

  template <totally_ordered _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce_max(const basic_vec<_Tp, _Ap>& __x,
               const typename basic_vec<_Tp, _Ap>::mask_type& __mask) noexcept
    {
      return reduce(__select_impl(__mask, __x, numeric_limits<_Tp>::lowest()),
                    []<typename _UV>(const _UV& __a, const _UV& __b) {
                      return __select_impl(__a < __b, __b, __a);
                    });
    }
}

#pragma GCC diagnostic pop
#endif // C++26
#endif // _GLIBCXX_SIMD_REDUCTIONS_H
