/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_INTERLEAVE_H_
#define PROTOTYPE_INTERLEAVE_H_

#include "simd.h"
#include "iota.h"

namespace std::datapar
{
  template <size_t _Offset, typename _Tp>
    constexpr const _Tp&
    __pack_subscript(const _Tp& __x0) noexcept
    {
      static_assert(_Offset == 0);
      return __x0;
    }

  template <size_t _Offset, typename _Tp, typename... _More>
    constexpr const _Tp&
    __pack_subscript(const _Tp& __x0, const _Tp& __x1, const _More&... __more) noexcept
    {
      if constexpr (_Offset == 0)
        return __x0;
      else if constexpr (_Offset == 1)
        return __x1;
      else
        return __pack_subscript<_Offset - 2>(__more...);
    }

  template <__detail::__simd_type _Vp, std::same_as<_Vp>... _More>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr std::tuple<_Vp, _More...>
    interleave(_Vp const& __a, _More const&... __more) noexcept
    {
      constexpr unsigned __n = 1 + sizeof...(_More);
      return [&]<size_t... _Offsets>(std::index_sequence<_Offsets...>) {
        return std::tuple{_Vp([&](auto __i) {
                            constexpr size_t __j = __i + _Offsets * _Vp::size();
                            return __pack_subscript<__j % __n>(__a, __more...)[__j / __n];
                          })...};
      }(std::make_index_sequence<__n>());
    }

}

#endif // PROTOTYPE_INTERLEAVE_H_
