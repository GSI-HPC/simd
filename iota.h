/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_IOTA_H_
#define PROTOTYPE_IOTA_H_

#include "simd.h"

#if not SIMD_IS_A_RANGE
#error "iota is implemented such that it requires simd to be a range"
#endif

namespace std::datapar
{
  template <typename _Tp>
    requires is_arithmetic_v<_Tp>
      or (__detail::__valid_simd<_Tp> and is_arithmetic_v<typename _Tp::value_type>)
    constexpr _Tp iota = _Tp();

  template <typename _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    iota<basic_simd<_Tp, _Abi>>([](_Tp __i) -> _Tp {
      static_assert (__simd_size_v<_Tp, _Abi> - 1 <= numeric_limits<_Tp>::max(), "iota object would overflow");
      return __i;
    });
}

#endif  // PROTOTYPE_IOTA_H_
