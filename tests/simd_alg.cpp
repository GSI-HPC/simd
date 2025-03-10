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
    static constexpr int x_max_int = static_cast<int>(x_max);

    static constexpr V
    reverse_iota(const V x)
    {
      if constexpr (std::is_enum_v<T>)
        {
          using Vu = dp::rebind_t<std::underlying_type_t<T>, V>;
          return static_cast<V>(std::to_underlying(x_max) - static_cast<Vu>(x));
        }
      else
        return x_max - x;
    }

    ADD_TEST(Min) {
      std::tuple{test_iota<V, 0, -1>, reverse_iota(test_iota<V, 0, -1>), test_iota<V, 1>},
      [](auto& t, const V x, const V y, const V x1) {
        t.verify_equal(min(x, x), x);
        t.verify_equal(min(V(), x), V());
        t.verify_equal(min(x, V()), V());
        if constexpr (std::is_signed_v<T>)
          {
            t.verify_equal(min(-x, x), -x);
            t.verify_equal(min(x, -x), -x);
          }
        t.verify_equal(min(x1, x), x);
        t.verify_equal(min(x, x1), x);
        t.verify_equal(min(x, y), min(y, x));
        t.verify_equal(min(x, y), V([](int i) {
                                    i %= x_max_int;
                                    return std::min(T(x_max_int - i), T(i));
                                  }));
      }
    };

    ADD_TEST(Max) {
      std::tuple{test_iota<V, 0, -1>, reverse_iota(test_iota<V, 0, -1>), test_iota<V, 1>},
      [](auto& t, const V x, const V y, const V x1) {
        t.verify_equal(max(x, x), x);
        t.verify_equal(max(V(), x), x);
        t.verify_equal(max(x, V()), x);
        if constexpr (std::is_signed_v<T>)
          {
            t.verify_equal(max(-x, x), x);
            t.verify_equal(max(x, -x), x);
          }
        t.verify_equal(max(x1, x), x1);
        t.verify_equal(max(x, x1), x1);
        t.verify_equal(max(x, y), max(y, x));
        t.verify_equal(max(x, y), V([](int i) {
                                    i %= x_max_int;
                                    return std::max(T(x_max_int - i), T(i));
                                  }));
      }
    };

    ADD_TEST(Minmax) {
      std::tuple{test_iota<V, 0, -1>, reverse_iota(test_iota<V, 0, -1>), test_iota<V, 1>},
      [](auto& t, const V x, const V y, const V x1) {
        t.verify_equal(minmax(x, x), pair{x, x});
        t.verify_equal(minmax(V(), x), pair{V(), x});
        t.verify_equal(minmax(x, V()), pair{V(), x});
        if constexpr (std::is_signed_v<T>)
          {
            t.verify_equal(minmax(-x, x), pair{-x, x});
            t.verify_equal(minmax(x, -x), pair{-x, x});
          }
        t.verify_equal(minmax(x1, x), pair{x, x1});
        t.verify_equal(minmax(x, x1), pair{x, x1});
        t.verify_equal(minmax(x, y), minmax(y, x));
        t.verify_equal(minmax(x, y),
                       pair{V([](int i) {
                              i %= x_max_int;
                              return std::min(T(x_max_int - i), T(i));
                            }),
                            V([](int i) {
                              i %= x_max_int;
                              return std::max(T(x_max_int - i), T(i));
                            })});
      }
    };

    ADD_TEST(Clamp) {
      std::tuple{test_iota<V>, reverse_iota(test_iota<V>)},
      [](auto& t, const V x, const V y) {
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
      std::tuple{test_iota<V, 0, 63>, test_iota<V, 1, 64>},
      [](auto& t, const V x, const V y) {
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
