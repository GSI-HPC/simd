/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023-2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "unittest_pch.h"

using namespace vir::literals;

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    static constexpr int max = sizeof(T) == 8 ? 64 : 32;

    ADD_TEST_N(known_shift, max, std::is_integral_v<T>) {
      std::tuple {test_iota<V, 0, 0>},
      [](auto& t, auto _shift, const V x) {
        constexpr int shift = _shift;
        constexpr V vshift = T(shift);
        const V vshiftx = vshift ^ (x & 1_cw);
        V ref([](T i) -> T { return i << shift; });
        V refx([](T i) -> T { return i << (shift ^ (i & 1)); });
        t.verify_equal(x << shift, ref)(x, "<<", shift);
        t.verify_equal(x << vshift, ref)(x, "<<", vshift);
        t.verify_equal(x << vshiftx, refx)(x, "<<", vshiftx);
        const auto y = ~x;
        ref = V([](T i) -> T { return T(~i) << shift; });
        refx = V([](T i) -> T { return T(~i) << (shift ^ (i & 1)); });
        t.verify_equal(y << shift, ref)(y, "<<", shift);
        t.verify_equal(y << vshift, ref)(y, "<<", vshift);
        t.verify_equal(y << vshiftx, refx)(y, "<<", vshiftx);
      }
    };

    ADD_TEST(unknown_shift, std::is_integral_v<T>) {
      std::tuple {test_iota<V, 0, 0>},
      [](auto& t, const V x) {
        if not consteval
        {
          for (int shift : dp::iota<dp::simd<int, max>>)
            {
              const auto y = ~x;
              shift = make_value_unknown(shift);
              const V vshift = T(shift);
              V ref([=](T i) -> T { return i << shift; });
              t.verify_equal(x << shift, ref)(y, "<<", shift);
              t.verify_equal(x << vshift, ref)(y, "<<", vshift);
              ref = V([=](T i) -> T { return T(~i) << shift; });
              t.verify_equal(y << shift, ref)(y, "<<", shift);
              t.verify_equal(y << vshift, ref)(y, "<<", vshift);
            }
        }
      }
    };
  };

#include "unittest.h"
