/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2026      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#define VIR_EXTENSIONS 1
#include "support.h"

namespace std::simd
{
  template <_ArchTraits = _ArchTraits()._M_math_abi(), typename TV>
    [[gnu::flatten]]
    TV
    __fast_round(TV x)
    {
      using T = __vec_value_type<TV>;
      constexpr auto [...is] = _IotaArray<__width_of<TV>>;
      return TV{T(__builtin_round(x[is]))...};
    }

  template <_ArchTraits _Traits = _ArchTraits()._M_math_abi(), typename V0, typename V1>
    [[gnu::flatten]]
    _GLIBCXX_SIMD_MATH_RET_TYPE(V0, V1)
    __fast_2x_round(V0 x, V1 y)
    {
      using T = __vec_value_type<V0>;
      constexpr auto [...is] = _IotaArray<__width_of<V0>>;
      constexpr auto [...js] = _IotaArray<__width_of<V1>>;
      V0 lo{T(__builtin_round(x[is]))...};
      V1 hi{T(__builtin_round(y[js]))...};
      _GLIBCXX_SIMD_MATH_RETURN(lo, hi);
    }

#define FN round
#define ONLY_FAST 1
#include "instantiate_1arg.h"
}
