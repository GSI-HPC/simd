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

namespace std
{
  constexpr int permute_zero = INT_MIN;

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
        operator()(unsigned __i, auto __size) const
        {
          static_assert(__size % (2 * _Np) == 0,
                        "swap_neighbors<N> permutation requires a multiple of 2N elements");
          if (std::has_single_bit(_Np))
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
        operator()(int) const
        { return _Position; }
      };

    template <int _Position>
      inline constexpr _Broadcast<_Position> broadcast {};

    inline constexpr _Broadcast<0> broadcast_first {};

    inline constexpr _Broadcast<-1> broadcast_last {};

    struct _Reverse
    {
      consteval int
      operator()(int __i) const
      { return -1 - __i; }
    };

    inline constexpr _Reverse reverse {};

    template <int _Offset>
      struct _Rotate
      {
        consteval int
        operator()(int __i, auto __size) const
        { return (__i + _Offset) % int(__size()); }
      };

    template <int _Offset>
      inline constexpr _Rotate<_Offset> rotate {};

    template <int _Offset>
      struct _Shift
      {
        consteval int
        operator()(int __i, auto __size) const
        {
          const int __j = __i + _Offset;
          if (__j >= __size or -__j > __size)
            return permute_zero;
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
    std::resize_simd_t<_Np == 0 ? _Vp::size() : _Np, _Vp>
    permute(_Vp const& __v, _Fp const __idx_perm) noexcept
    {
      using _Tp = typename _Vp::value_type;
      using _Rp = resize_simd_t<_Np == 0 ? _Vp::size() : _Np, _Vp>;
      return _Rp([&] [[__gnu__::__always_inline__]] (auto __i) -> _Tp {
               constexpr int __j = [&] {
                 if constexpr (__detail::__index_permutation_function_nosize<_Fp>)
                   return __idx_perm(__i);
                 else
                   return __idx_perm(__i, _Vp::size);
               }();
               if constexpr (__j == permute_zero)
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
