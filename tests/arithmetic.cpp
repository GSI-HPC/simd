/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "unittest.h"

using namespace vir::literals;

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    static constexpr T min = std::numeric_limits<T>::lowest();
    static constexpr T norm_min = std::numeric_limits<T>::min();
    static constexpr T max = std::numeric_limits<T>::max();
    static constexpr bool is_iec559 =
#ifdef __GCC_IEC_559
      __GCC_IEC_559 >= 2;
#elif defined __STDC_IEC_559__
      __STDC_IEC_559__ == 1;
#else
      false;
#endif

    ADD_TEST(plus0) {
      std::tuple{V(), vec<V, 1, 2, 3, 4, 5, 6, 7>},
      [](auto& t, V x, V y) {
        t.verify_equal(x + x, x);
        t.verify_equal(x = x + T(1), T(1));
        t.verify_equal(x + x, T(2));
        t.verify_equal(x = x + y, vec<V, 2, 3, 4, 5, 6, 7, 8>);
        t.verify_equal(x = x + -y, T(1));
        t.verify_equal(x += y, vec<V, 2, 3, 4, 5, 6, 7, 8>);
        t.verify_equal(x, vec<V, 2, 3, 4, 5, 6, 7, 8>);
        t.verify_equal(x += -y, T(1));
        t.verify_equal(x, T(1));
      }
    };

    ADD_TEST(plus1) {
      std::tuple{test_iota<V>},
      [](auto& t, V x) {
        t.verify_equal(x + 0_cw, x);
        t.verify_equal(0_cw + x, x);
        t.verify_equal(x + T(), x);
        t.verify_equal(T() + x, x);
        t.verify_equal(x + -x, V());
        t.verify_equal(-x + x, V());
      }
    };

    ADD_TEST(minus0) {
      std::tuple{T(1), T(0), vec<V, 1, 2, 3, 4, 5, 6, 7>},
      [](auto& t, V x, V y, V z) {
        t.verify_equal(x - y, x);
        t.verify_equal(x - T(1), y);
        t.verify_equal(y, x - T(1));
        t.verify_equal(x - x, y);
        t.verify_equal(x = z - x, vec<V, 0, 1, 2, 3, 4, 5, 6>);
        t.verify_equal(x = z - x, V(1));
        t.verify_equal(z -= x, vec<V, 0, 1, 2, 3, 4, 5, 6>);
        t.verify_equal(z, vec<V, 0, 1, 2, 3, 4, 5, 6>);
        t.verify_equal(z -= z, V(0));
        t.verify_equal(z, V(0));
      }
    };

    ADD_TEST(minus1) {
      std::tuple{test_iota<V>},
      [](auto& t, V x) {
        t.verify_equal(x - x, V());
        t.verify_equal(x - 0_cw, x);
        t.verify_equal(0_cw - x, -x);
        t.verify_equal(x - T(), x);
        t.verify_equal(T() - x, -x);
      }
    };

    ADD_TEST(times0) {
      std::tuple{T(0), T(1), T(2)},
      [](auto& t, T v0, T v1, T v2) {
        V x = v1;
        V y = v0;
        t.verify_equal(x * y, y);
        t.verify_equal(x = x * T(2), T(2));
        t.verify_equal(x * x, T(4));
        y = vec<V, 1, 2, 3, 4, 5, 6, 7>;
        t.verify_equal(x = x * y, vec<V, 2, 4, 6, 8, 10, 12, 14>);
        y = v2;
        // don't test norm_min/2*2 in the following. There's no guarantee, in
        // general, that the result isn't flushed to zero (e.g. NEON without
        // subnormals)
        for (T n : {T(max - 1), std::is_floating_point_v<T> ? T(norm_min * 3) : min})
          {
            x = T(n / 2);
            t.verify_equal(x * y, V(n));
          }
        if (std::is_integral<T>::value && std::is_unsigned<T>::value)
          {
            // test modulo arithmetics
            T n = max;
            x = n;
            for (T m : {T(2), T(7), T(max / 127), max})
              {
                y = m;
                // if T is of lower rank than int, `n * m` will promote to int
                // before executing the multiplication. In this case an overflow
                // will be UB (and ubsan will warn about it). The solution is to
                // cast to uint in that case.
                using U
                  = std::conditional_t<(sizeof(T) < sizeof(int)), unsigned, T>;
                t.verify_equal(x * y, V(T(U(n) * U(m))));
              }
          }
        x = v2;
        t.verify_equal(x *= vec<V, 1, 2, 3>, vec<V, 2, 4, 6>);
        t.verify_equal(x, vec<V, 2, 4, 6>);
      }
    };

    ADD_TEST(times1) {
      std::tuple{test_iota<V, 0, 11>},
      [](auto& t, V x) {
        t.verify_equal(x * x, V([](int i) { return T(T(i % 12) * T(i % 12)); }));
        t.verify_equal(x * 1_cw, x);
        t.verify_equal(1_cw * x, x);
        t.verify_equal(x * T(1), x);
        t.verify_equal(T(1) * x, x);
        t.verify_equal(x * T(-1), -x);
        t.verify_equal(T(-1) * x, -x);
      }
    };

    // avoid testing subnormals and expect minor deltas for non-IEC559 float
    ADD_TEST(divide0, std::is_floating_point_v<T> and not is_iec559) {
      std::tuple{T(2), vec<V, 1, 2, 3, 4, 5, 6, 7>},
      [](auto& t, V x, V y) {
        t.verify_equal_to_ulp(x / x, V(T(1)), 1);
        t.verify_equal_to_ulp(T(3) / x, V(T(3) / T(2)), 1);
        t.verify_equal_to_ulp(x / T(3), V(T(2) / T(3)), 1);
        t.verify_equal_to_ulp(y / x, vec<V, T(.5), T(1), T(1.5), T(2), T(2.5), T(3), T(3.5)>, 1);
      }
    };

    // avoid testing subnormals and expect minor deltas for non-IEC559 float
    ADD_TEST(divide1, std::is_floating_point_v<T> and not is_iec559) {
      std::array{T{norm_min * 1024}, T{1}, T{}, T{-1}, T{max / 1024}, T{max / T(4.1)}, max, min},
      [](auto& t, V a) {
        V b = 2_cw;
        V ref([&](int i) { return a[i] / 2; });
        t.verify_equal_to_ulp(a / b, ref, 1);
        a = select(a == 0_cw, T(1), a);
        // -freciprocal-math together with flush-to-zero makes
        // the following range restriction necessary (i.e.
        // 1/|a| must be >= min). Intel vrcpps and vrcp14ps
        // need some extra slack (use 1.1 instead of 1).
        a = select(fabs(a) >= T(1.1) / norm_min, T(1), a);
        t.verify_equal_to_ulp(a / a, V(1), 1)("\na = ", a);
        ref = V([&](int i) { return 2 / a[i]; });
        t.verify_equal_to_ulp(b / a, ref, 1)("\na = ", a);
        t.verify_equal_to_ulp(b /= a, ref, 1);
        t.verify_equal_to_ulp(b, ref, 1);
      }
    };

    ADD_TEST(divide2, is_iec559 or not std::is_floating_point_v<T>) {
      std::tuple{T(2), vec<V, 1, 2, 3, 4, 5, 6, 7>, vec<V, max, norm_min>, vec<V, norm_min, max>,
                 vec<V, max, T(norm_min + 1)>},
      [](auto& t, V x, V y, V z, V a, V b) {
        t.verify_equal(x / x, V(1));
        t.verify_equal(T(3) / x, V(T(3) / T(2)));
        t.verify_equal(x / T(3), V(T(2) / T(3)));
        t.verify_equal(y / x, vec<V, T(.5), T(1), T(1.5), T(2), T(2.5), T(3), T(3.5)>);
        V ref = vec<V, T(max / 2), T(norm_min / 2)>;
        t.verify_equal(z / x, ref);
        ref = vec<V, T(norm_min / 2), T(max / 2)>;
        t.verify_equal(a / x, ref);
        t.verify_equal(b / b, V(1));
        ref = vec<V, T(2 / max), T(2 / (norm_min + 1))>;
        t.verify_equal(x / b, ref);
        t.verify_equal(x /= b, ref);
        t.verify_equal(x, ref);
      }
    };

    static constexpr V from0 = test_iota<V, 0, 63>;
    static constexpr V from1 = test_iota<V, 1, 64>;
    static constexpr V from2 = test_iota<V, 2, 65>;

    ADD_TEST(incdec) {
      std::tuple{from0},
      [](auto& t, V x) {
        t.verify_equal(x++, from0);
        t.verify_equal(x, from1);
        t.verify_equal(++x, from2);
        t.verify_equal(x, from2);

        t.verify_equal(x--, from2);
        t.verify_equal(x, from1);
        t.verify_equal(--x, from0);
        t.verify_equal(x, from0);
      }
    };
  };
