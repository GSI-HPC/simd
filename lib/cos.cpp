/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#define VIR_EXTENSIONS 1

#include "support.h"

namespace std::simd
{
  template <_TargetTraits _Traits = _TargetTraits()._M_math_abi(), typename TV>
    [[gnu::flatten]]
    TV
    __cos(TV x)
    { return __fast_cos<_ArchTraits(_Traits), TV>(x); }

  template <_TargetTraits _Traits = _TargetTraits()._M_math_abi(), typename V0, typename V1>
    [[gnu::flatten]]
    _GLIBCXX_SIMD_MATH_RET_TYPE(V0, V1)
    __2x_cos(V0 x, V1 y)
    { return __fast_2x_cos<_ArchTraits(_Traits), V0, V1>(x, y); }

#define FN cos
#define NO_FAST 1
#include "instantiate_1arg.h"
}
