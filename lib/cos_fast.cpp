/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#define VIR_EXTENSIONS 1

#include "support.h"

namespace std::simd
{
  template <_ArchTraits = _ArchTraits()._M_math_abi(), typename TV>
    [[gnu::flatten]]
    TV
    __fast_cos(TV x)
    {
#if __has_builtin(__builtin_elementwise_cos)
      return __builtin_elementwise_cos(x);
#else
      constexpr auto [...is] = _IotaArray<__width_of<TV>>;
      return TV{std::cos(x[is])...};
#endif
    }

  template <_ArchTraits = _ArchTraits()._M_math_abi(), typename V0, typename V1>
    [[gnu::flatten]]
    _GLIBCXX_SIMD_MATH_RET_TYPE(V0, V1)
    __fast_2x_cos(V0 x, V1 y)
    {
#if __has_builtin(__builtin_elementwise_cos)
      V0 lo = __builtin_elementwise_cos(x);
      V1 hi = __builtin_elementwise_cos(y);
#else
      constexpr auto [...is] = _IotaArray<__width_of<V0>>;
      constexpr auto [...js] = _IotaArray<__width_of<V1>>;
      V0 lo{std::cos(x[is])...};
      V1 hi{std::cos(y[js])...};
#endif
      _GLIBCXX_SIMD_MATH_RETURN(lo, hi);
    }

#define FN cos
#define ONLY_FAST 1
#include "instantiate_1arg.h"
}
