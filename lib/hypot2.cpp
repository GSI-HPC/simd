/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#define VIR_NEXT_PATCH 1
#define VIR_EXTENSIONS 1

#include "support.h"

namespace std::simd
{
  // With fast-math, ignore precision of subnormals and inputs from
  // __finite_max_v/2 to __finite_max_v. This removes all
  // branching/masking.
  template <_ArchTraits = _ArchTraits()._M_math_abi(), typename TV>
    [[gnu::flatten, gnu::optimize("Ofast")]]
    TV
    __fast_hypot(TV x0, TV y0) noexcept
    {
      using T = __vec_value_type<TV>;
      constexpr int N = __width_of<TV>;
      using V = vec<T, N>;
      const V x = x0;
      const V y = y0;
      if constexpr (!is_void_v<IncreasePrecision<T>>)
        if constexpr (N <= vec<IncreasePrecision<T>>::size())
          {
            using V2 = rebind_t<IncreasePrecision<T>, V>;
            return V(sqrt(V2(x) * V2(x) + V2(y) * V2(y)));
          }
      const V absx = fabs(x); // no error
      const V absy = fabs(y); // no error
      V hi = max(absx, absy); // no error
      V lo = min(absx, absy); // no error
      const auto huge_diff = is_large_diff(hi, lo);
      if (all_of(huge_diff)) [[unlikely]]
        return hi;
      // avoid denormals:
      lo = select(huge_diff, V(), lo);
      const V scale_back = rescale_factors(hi, lo);
      return scale_back * sqrt((lo * lo)._M_assoc_barrier() + hi * hi);
    }

  template <_ArchTraits = _ArchTraits()._M_math_abi(), typename V0, typename V1>
    [[gnu::flatten, gnu::optimize("Ofast")]]
    _GLIBCXX_SIMD_MATH_RET_TYPE(V0, V1)
    __fast_2x_hypot(V0 x0, V0 y0, V1 x1, V1 y1) noexcept
    {
      V0 lo = __fast_hypot(x0, y0);
      V1 hi = __fast_hypot(x1, y1);
      _GLIBCXX_SIMD_MATH_RETURN(lo, hi);
    }

  template <_TargetTraits Traits = _TargetTraits()._M_math_abi(), typename TV>
    [[gnu::flatten]]
    TV
    __hypot(TV x0, TV y0) noexcept
    {
      using T = __vec_value_type<TV>;
      using L = numeric_limits<T>;
      constexpr int N = __width_of<TV>;
      using V = vec<T, N>;
      const V x = x0;
      const V y = y0;
      if constexpr (!is_void_v<IncreasePrecision<T>>)
        if constexpr (N <= vec<IncreasePrecision<T>>::size())
          {
            using V2 = rebind_t<IncreasePrecision<T>, V>;
            V r = V(sqrt(V2(x) * V2(x) + V2(y) * V2(y)));
            if (all_of(isfinite(x) && isfinite(y))) [[likely]]
              return r;
            return select(isinf(x) || isinf(y), inf_v<V>, r);
          }
      const V absx = fabs(x); // no error
      const V absy = fabs(y); // no error
      V hi = max(absx, absy); // no error
      V lo = min(absx, absy); // no error
      const auto huge_diff = is_large_diff(hi, lo);
      if (all_of(huge_diff)) [[unlikely]]
        return hi;
      lo = select(huge_diff, V(), lo);
      if (all_of(isnormal(x)) && all_of(isnormal(y))) [[likely]]
        {
          const V scale_back = rescale_factors(hi, lo);
          return scale_back * sqrt((lo * lo)._M_assoc_barrier() + hi * hi);
        }
      else
        {
          // slower path to support subnormals
          // if hi is subnormal, avoid scaling by inf & final mul by 0
          // (which yields NaN) by using min()
          constexpr V subnorm_scale = 1 / norm_min_v<V>;
          // invert exponent w/o error and w/o using the slow divider
          // unit: xor inverts the exponent but off by 1. Multiplication
          // with .5 adjusts for the discrepancy.
          const V scale = select(hi >= norm_min_v<V>,
                                 (hi & inf_v<V>) ^ inf_v<V> * T(.5),
                                 subnorm_scale);
          // adjust final exponent for subnormal inputs
          V hi_exp = select(hi >= norm_min_v<V>, V(hi & inf_v<V>),
                            V(norm_min_v<V>)); // no error
          V h1 = hi * scale; // no error
          lo *= scale;       // no error
          V r = hi_exp * sqrt((lo * lo)._M_assoc_barrier() + h1 * h1);
          if constexpr (Traits._M_finite_math_only())
            return r;

          V fixup = hi; // lo == 0
          // where(lo == 0, fixup)                   = hi;
          fixup = select(isunordered(x, y), L::quiet_NaN(), fixup);
          fixup = select(isinf(absx) || isinf(absy), L::infinity(), fixup);
          // Instead of lo == 0, the following could depend on h1² ==
          // h1² + lo (i.e. hi is so much larger than the other two
          // inputs that the result is exactly hi). While this may
          // improve precision, it is likely to reduce efficiency if the
          // ISA has FMAs (because h1² + lo is an FMA, but the
          // intermediate
          // h1² must be kept)
          return select(lo == 0 || isunordered(x, y) || isinf(absx) || isinf(absy), fixup, r);
        }
    }

  template <_TargetTraits = _TargetTraits()._M_math_abi(), typename V0, typename V1>
    [[gnu::flatten]]
    _GLIBCXX_SIMD_MATH_RET_TYPE(V0, V1)
    __2x_hypot(V0 x0, V0 y0, V1 x1, V1 y1) noexcept
    {
      V0 lo = __hypot(x0, y0);
      V1 hi = __hypot(x1, y1);
      _GLIBCXX_SIMD_MATH_RETURN(lo, hi);
    }

#define FN hypot
#include "instantiate_2arg.h"
}
