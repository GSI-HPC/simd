/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

// Implements non-members of P2664

#ifndef PROTOTYPE_PERMUTE_H_
#define PROTOTYPE_PERMUTE_H_

#include "simd.h"
#include "iota.h"

namespace std::datapar
{
  namespace permutations
  {
    struct _DuplicateEven
    {
      consteval unsigned
      operator()(unsigned __i) const
      { return __i & ~1u; }
    };

    inline constexpr _DuplicateEven duplicate_even {};

    struct _DuplicateOdd
    {
      consteval unsigned
      operator()(unsigned __i) const
      { return __i | 1u; }
    };

    inline constexpr _DuplicateOdd duplicate_odd {};

    template <unsigned _Np>
      struct _SwapNeighbors
      {
        consteval unsigned
        operator()(unsigned __i, unsigned __size) const
        {
          if (__size % (2 * _Np) != 0)
            abort(); // swap_neighbors<N> permutation requires a multiple of 2N elements
          else if (std::has_single_bit(_Np))
            return __i ^ _Np;
          else if (__i % (2 * _Np) >= _Np)
            return __i - _Np;
          else
            return __i + _Np;
        }
      };

    template <unsigned _Np = 1u>
      inline constexpr _SwapNeighbors<_Np> swap_neighbors {};

    template <int _Position>
      struct _Broadcast
      {
        consteval int
        operator()(int, int __size) const
        { return _Position < 0 ? __size + _Position : _Position; }
      };

    template <int _Position>
      inline constexpr _Broadcast<_Position> broadcast {};

    inline constexpr _Broadcast<0> broadcast_first {};

    inline constexpr _Broadcast<-1> broadcast_last {};

    struct _Reverse
    {
      consteval int
      operator()(int __i, int __size) const
      { return __size - 1 - __i; }
    };

    inline constexpr _Reverse reverse {};

    template <int _Offset>
      struct _Rotate
      {
        consteval int
        operator()(int __i, int __size) const
        {
          __i += _Offset;
          __i %= __size;
          if (__i < 0)
            __i += __size;
          return __i;
        }
      };

    template <int _Offset>
      inline constexpr _Rotate<_Offset> rotate {};

    template <int _Offset>
      struct _Shift
      {
        consteval int
        operator()(int __i, int __size) const
        {
          const int __j = __i + _Offset;
          if (__j >= __size or -__j > __size)
            return zero_element;
          else if (__j < 0)
            return __size + __j;
          else
            return __j;
        }
      };

    template <int _Offset>
      inline constexpr _Shift<_Offset> shift {};
  }

  template <__detail::_SimdSizeType _Np = 0, __detail::__simd_or_mask _Vp,
            __detail::__index_permutation_function<_Vp> _Fp>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
    resize_t<_Np == 0 ? _Vp::size() : _Np, _Vp>
    permute(_Vp const& __v, _Fp const __idx_perm) noexcept
    {
      using _Tp = typename _Vp::value_type;
      if constexpr ((_Np == 0 or _Np == _Vp::size())
                      and requires { _Vp::_Impl::_S_permute(__v._M_data, __idx_perm);})
        return {__detail::__private_init, _Vp::_Impl::_S_permute(__v._M_data, __idx_perm)};
      else
        return resize_t<_Np == 0 ? _Vp::size() : _Np, _Vp>(
                 [&] [[__gnu__::__always_inline__]] (auto __i0) -> _Tp {
                   constexpr __detail::_SimdSizeType __i = __i0;
                   constexpr __detail::_SimdSizeType __j = [&] {
                     if constexpr (__detail::__index_permutation_function_nosize<_Fp>)
                       return __idx_perm(__i);
                     else
                       return __idx_perm(__i, _Vp::size());
                   }();
                   if constexpr (__j == zero_element)
                     return 0;
                   else if constexpr (__j < 0)
                     {
                       static_assert(-__j <= int(_Vp::size()));
                       return __v[__v.size() + __j];
                     }
                   else
                     {
                       static_assert(__j < int(_Vp::size()));
                       return __v[__j];
                     }
                 });
    }
}

#endif  // PROTOTYPE_PERMUTE_H_
