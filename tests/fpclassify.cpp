/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2024–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */
// requires float

#include "unittest.h"

#if VIR_PATCH_MATH
template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    static_assert(std::floating_point<T>);

    using L = std::numeric_limits<T>;

    static constexpr T min = L::lowest();
    static constexpr T norm_min = L::min();
    static constexpr T max = L::max();
#if !_GLIBCXX_FAST_MATH
    static constexpr T denorm_min = L::denorm_min();
#endif
#if !__FINITE_MATH_ONLY__
    static constexpr T inf = L::infinity();
    static constexpr T nan = L::quiet_NaN();
#endif
    static constexpr T zero = 0;

    static constexpr T after_one = 1 + L::epsilon();
    static constexpr T before_one = (2 - L::epsilon()) / 2;

    static constexpr std::array test_values = {
      T(+0.),
#if !__NO_SIGNED_ZEROS__
      T(-0.),
#endif
      T(0.45), T(0.5), T(0.55),
      T(-0.45), T(-0.5), T(-0.55),
      T(1.45), T(1.5), T(1.55),
      T(-1.45), T(-1.5), T(-1.55),
      T(2), T(2.5), T(-2.5),
      T(3), T(-3), T(9), T(-9),
      before_one, -before_one, after_one, -after_one,
      2 * before_one, -2 * before_one, 2 * after_one, -2 * after_one,
#if !__FINITE_MATH_ONLY__
      inf, -inf, nan,
#if __SUPPORT_SNAN__
      L::signaling_NaN(),
#endif
#endif
#if !_GLIBCXX_FAST_MATH
      denorm_min, norm_min / 3,
      -denorm_min, -norm_min / 3,
#endif
      norm_min, max, -max, -norm_min
    };

    static constexpr auto test_isinf = make_math_test {
      test_values,
      1000,
      [](auto x) { return x = simd::select(std::isinf(x), T(1), T(0)); },
      require_no_fpexcept
    };
  };
#endif
