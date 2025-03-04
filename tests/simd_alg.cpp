/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "unittest_pch.h"

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    static_assert(std::totally_ordered<T>);

    using M = typename V::mask_type;
    using pair = std::pair<V, V>;
    static constexpr std::conditional_t<std::is_floating_point_v<T>, short, T> x_max
      = test_iota_max<V, 1>;

    ADD_TEST(Min) {
      std::tuple{test_iota<V, 1> - T(1)},
      [](auto& t, const V x) {
        const V y = x_max - x;
        t.verify_equal(min(x, x), x);
        t.verify_equal(min(V(), x), V());
        t.verify_equal(min(x, V()), V());
        if constexpr (std::is_signed_v<T>)
          {
            t.verify_equal(min(-x, x), -x);
            t.verify_equal(min(x, -x), -x);
          }
        t.verify_equal(min(x + T(1), x), x);
        t.verify_equal(min(x, x + T(1)), x);
        t.verify_equal(min(x, y), min(y, x));
        t.verify_equal(min(x, y), V([](int i) { i %= x_max; return std::min(T(x_max - i), T(i)); }));
      }
    };

    ADD_TEST(Max) {
      std::tuple{test_iota<V, 1> - T(1)},
      [](auto& t, const V x) {
        const V y = x_max - x;
        t.verify_equal(max(x, x), x);
        t.verify_equal(max(V(), x), x);
        t.verify_equal(max(x, V()), x);
        if constexpr (std::is_signed_v<T>)
          {
            t.verify_equal(max(-x, x), x);
            t.verify_equal(max(x, -x), x);
          }
        t.verify_equal(max(x + T(1), x), x + T(1));
        t.verify_equal(max(x, x + T(1)), x + T(1));
        t.verify_equal(max(x, y), max(y, x));
        t.verify_equal(max(x, y), V([](int i) { i %= x_max; return std::max(T(x_max - i), T(i)); }));
      }
    };

    ADD_TEST(Minmax) {
      std::tuple{test_iota<V, 1> - T(1)},
      [](auto& t, V x) {
        const V y = x_max - x;
        t.verify_equal(minmax(x, x), pair{x, x});
        t.verify_equal(minmax(V(), x), pair{V(), x});
        t.verify_equal(minmax(x, V()), pair{V(), x});
        if constexpr (std::is_signed_v<T>)
          {
            t.verify_equal(minmax(-x, x), pair{-x, x});
            t.verify_equal(minmax(x, -x), pair{-x, x});
          }
        t.verify_equal(minmax(x + T(1), x), pair{x, x + T(1)});
        t.verify_equal(minmax(x, x + T(1)), pair{x, x + T(1)});
        t.verify_equal(minmax(x, y), minmax(y, x));
        t.verify_equal(minmax(x, y),
                       pair{V([](int i) { i %= x_max; return std::min(T(x_max - i), T(i)); }),
                            V([](int i) { i %= x_max; return std::max(T(x_max - i), T(i)); })});
      }
    };

    ADD_TEST(Clamp) {
      std::tuple{test_iota<V>},
      [](auto& t, const V x) {
        const V y = test_iota_max<V> - x;
        t.verify_equal(clamp(x, V(), x), x);
        t.verify_equal(clamp(x, x, x), x);
        t.verify_equal(clamp(V(), x, x), x);
        t.verify_equal(clamp(V(), V(), x), V());
        t.verify_equal(clamp(x, V(), V()), V());
        t.verify_equal(clamp(x, V(), y), min(x, y));
        t.verify_equal(clamp(y, V(), x), min(x, y));
        if constexpr (std::is_signed_v<T>)
          {
            t.verify_equal(clamp(V(T(-test_iota_max<V>)), -x, x), -x);
            t.verify_equal(clamp(V(T(test_iota_max<V>)), -x, x), x);
          }
      }
    };

    ADD_TEST(Select) {
      std::tuple{test_iota<V, 0, 63>, T(64)},
      [](auto& t, V x, V y) {
        y -= x;
        t.verify_equal(select(M(true), x, y), x);
        t.verify_equal(select(M(false), x, y), y);
        t.verify_equal(select(M(true), y, x), y);
        t.verify_equal(select(M(false), y, x), x);
        t.verify_equal(select(M([](int i) { return 1 == (i & 1); }), x, T()),
                       V([](int i) { return (1 == (i & 1)) ? T(i & 63) : T(); }));
      }
    };
  };

#include "unittest.h"
