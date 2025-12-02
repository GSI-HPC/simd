/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "unittest.h"

static constexpr bool is_iec559 =
#ifdef __GCC_IEC_559
      __GCC_IEC_559 >= 2;
#elif defined __STDC_IEC_559__
      __STDC_IEC_559__ == 1;
#else
      false;
#endif

// no tests for non-floating-point types
template <typename V>
  struct Tests {};

#if VIR_NEXT_PATCH
template <typename V>
  requires std::floating_point<typename V::value_type>
  struct Tests<V>
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    using L = std::numeric_limits<T>;

    static constexpr T min = L::lowest();
    static constexpr T denorm_min = L::denorm_min();
    static constexpr T norm_min = L::min();
    static constexpr T max = L::max();
    static constexpr T inf = L::infinity();
    static constexpr T nan = L::quiet_NaN();

    static constexpr T after_one = 1 + L::epsilon();
    static constexpr T before_one = (2 - L::epsilon()) / 2;

    ADD_TEST(Roundings) {
      make_packed_array<V>(+0., -0., 0.5, -0.5, 1, 1.5, -1.5, 2, 2.5, -2.5, 3, -3, 9, -9,
                           before_one, -before_one, after_one, -after_one,
                           2 * before_one, -2 * before_one, 2 * after_one, -2 * after_one,
                           inf, -inf, nan, denorm_min, norm_min / 3, norm_min, max,
                           0x1.fffffffffffffp52, -0x1.fffffffffffffp52,
                           0x1.ffffffffffffep52, -0x1.ffffffffffffep52,
                           0x1.ffffffffffffdp52, -0x1.ffffffffffffdp52,
                           0x1.fffffep21, -0x1.fffffep21,
                           0x1.fffffcp21, -0x1.fffffcp21,
                           0x1.fffffep22, -0x1.fffffep22,
                           0x1.fffffcp22, -0x1.fffffcp22,
                           0x1.fffffep23, -0x1.fffffep23,
                           0x1.fffffcp23, -0x1.fffffcp23,
                           0x1.8p23, -0x1.8p23),
      [](auto& t, V x) {
        t.verify_equal(fabs(x), V([&](int i) { return std::fabs(x[i]); }));
        t.verify_equal(copysign(x, x), x);
        t.verify_equal(copysign(-x, x), x);
        t.verify_equal(copysign(fabs(x), x), x);
        t.verify_equal(trunc(x), V([&](int i) { return std::trunc(x[i]); }));
        t.verify_equal(ceil(x), V([&](int i) { return std::ceil(x[i]); }));
        t.verify_equal(floor(x), V([&](int i) { return std::floor(x[i]); }));
        if !consteval
        {
          t.verify_equal(nearbyint(x), V([&](int i) { return std::nearbyint(x[i]); }));
        }
      }
    };
  };
#endif
