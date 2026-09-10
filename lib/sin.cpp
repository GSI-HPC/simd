/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2026      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#define VIR_EXTENSIONS 1
#include "support.h"

namespace std::simd
{
  template <_TargetTraits = _TargetTraits()._M_math_abi(), typename TV>
    [[gnu::flatten]]
    TV
    __sin(TV x)
    {
      constexpr int N = __width_of<TV>;
      using T = __vec_value_type<TV>;
      if constexpr (is_same_v<T, float>)
	{
	  using float_v = simd::vec<T, N>;
	  using signed_v = simd::rebind_t<__integer_from<sizeof(T)>, float_v>;
	  using unsigned_v = simd::rebind_t<_UInt<sizeof(T)>, float_v>;

	  static constexpr T inv_pi = 0x1.45f306p-2f;
	  static constexpr T pi_1   =  0x1.921fb6p+1f;
	  static constexpr T pi_2   = -0x1.777a5cp-24f;
	  static constexpr T pi_3   = -0x1.ee59dap-49f;
	  static constexpr T c0     = -0x1.55554ep-3f;
	  static constexpr T c1     =  0x1.110f22p-7f;
	  static constexpr T c2     = -0x1.9f7cd6p-13f;
	  static constexpr T c3     =  0x1.5e6364p-19f;
	  static constexpr T nan    = numeric_limits<T>::quiet_NaN();

	  float_v abs_x = simd::abs(float_v(x));

	  // n = round(|x| / pi)
	  float_v n_f = simd::nearbyint(abs_x * inv_pi);
	  signed_v n_i = static_cast<signed_v>(n_f);

	  // r = |x| - n*pi (multi-part subtraction matching vfmsq_laneq_f32 chain)
	  float_v r = abs_x - n_f * pi_1;
	  r -= n_f * pi_2;
	  r -= n_f * pi_3;

	  // Polynomial (Horner): y = c3; y = c2+r2*y; y = c1+r2*y; y = c0+r2*y; y = r+r3*y
	  float_v r2 = r * r;
	  float_v r3 = r2 * r;
	  float_v y = c3;
	  y = c2 + r2 * y;
	  y = c1 + r2 * y;
	  y = c0 + r2 * y;
	  y = r + r3 * y;

	  // Sign correction: octant parity XOR original input sign
	  unsigned_v x_bits = std::bit_cast<unsigned_v>(x);
	  unsigned_v sign_correction = ((unsigned_v(n_i) & 1u) << 31) ^ (x_bits & 0x80000000u);
	  float_v result = std::bit_cast<float_v>(std::bit_cast<unsigned_v>(y) ^ sign_correction);

	  // Large values (|x| >= 2^20) and Inf/NaN → NaN (matches advsimd range_val)
	  auto is_large   = abs_x >= 0x1p20f;
	  auto is_special = simd::isinf(float_v(x)) || simd::isnan(float_v(x));
	  result = simd::select(is_large || is_special, nan, result);

	  return result;
	}
      else
	{
	  constexpr auto [...is] = _IotaArray<N>;
	  return TV{std::sin(x[is])...};
	}
    }

  template <_TargetTraits _Traits = _TargetTraits()._M_math_abi(), typename V0, typename V1>
    [[gnu::flatten]]
    _GLIBCXX_SIMD_MATH_RET_TYPE(V0, V1)
    __2x_sin(V0 x, V1 y)
    {
      V0 lo = __sin<_Traits>(x);
      V1 hi = __sin<_Traits>(y);
      _GLIBCXX_SIMD_MATH_RETURN(lo, hi);
    }

#define FN sin
#define NO_FAST
#include "instantiate_1arg.h"
}
