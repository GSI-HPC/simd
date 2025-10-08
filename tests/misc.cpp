/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

// no-fast-math

#include "unittest.h"
#include <numeric>

using namespace vir::literals;

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    ADD_TEST(misc) {
      std::tuple{vec<V, 0, 100, 2, 54, 3>},
      [](auto& t, V x) {
        t.verify_equal(x, x);
        if not consteval
        {
#ifdef __SSE__
          if constexpr (sizeof(x) == 16 and std::is_same_v<T, float>)
            t.verify_equal(_mm_and_ps(x, x), x);
#endif
#ifdef __SSE2__
          if constexpr (sizeof(x) == 16 and std::is_integral_v<T>)
            t.verify_equal(_mm_and_si128(x, x), x);
          if constexpr (sizeof(x) == 16 and std::is_same_v<T, double>)
            t.verify_equal(_mm_and_pd(x, x), x);
#endif
        }
      }
    };

    static_assert(dp::alignment_v<V> <= 256);

    ADD_TEST(loads) {
      std::tuple {std::array<T, V::size * 2> {}, std::array<int, V::size * 2> {}},
      [](auto& t, auto mem, auto ints) {
        //alignas(256) std::array<T, V::size * 2> mem = {};
        t.verify_equal(dp::unchecked_load<V>(mem), V());
        t.verify_equal(dp::partial_load<V>(mem), V());

        t.verify_equal(dp::unchecked_load<V>(mem, dp::flag_aligned), V());
        t.verify_equal(dp::partial_load<V>(mem, dp::flag_aligned), V());

        t.verify_equal(dp::unchecked_load<V>(mem, dp::flag_overaligned<256>), V());
        t.verify_equal(dp::partial_load<V>(mem, dp::flag_overaligned<256>), V());

        t.verify_equal(dp::unchecked_load<V>(mem.begin() + 1, mem.end()), V());
        t.verify_equal(dp::partial_load<V>(mem.begin() + 1, mem.end()), V());
        t.verify_equal(dp::partial_load<V>(mem.begin() + 1, mem.begin() + 1), V());
        t.verify_equal(dp::partial_load<V>(mem.begin() + 1, mem.begin() + 2), V());

        t.verify_equal(dp::unchecked_load<V>(ints, dp::flag_convert), V());
        t.verify_equal(dp::partial_load<V>(ints, dp::flag_convert), V());
      }
    };

    ADD_TEST(loads_iota, requires {T() + T(1);}) {
      std::tuple {[] {
        std::array<T, V::size * 2> arr = {};
        T init = 0;
        for (auto& x : arr) x = (init += T(1));
        return arr;
      }(), [] {
        std::array<int, V::size * 2> arr = {};
        simd::__iota(arr.begin(), arr.end(), 1);
        return arr;
      }()},
      [](auto& t, auto mem, auto ints) {
        constexpr V ref = test_iota<V, 1, 0>;
        constexpr V ref1 = V([](int i) { return i == 0 ? T(1): T(); });

        t.verify_equal(dp::unchecked_load<V>(mem), ref);
        t.verify_equal(dp::partial_load<V>(mem), ref);

        t.verify_equal(dp::unchecked_load<V>(mem.begin() + 1, mem.end()), ref + T(1));
        t.verify_equal(dp::partial_load<V>(mem.begin() + 1, mem.end()), ref + T(1));
        t.verify_equal(dp::partial_load<V>(mem.begin(), mem.begin() + 1), ref1);

        t.verify_equal(dp::unchecked_load<V>(mem, dp::flag_aligned), ref);
        t.verify_equal(dp::partial_load<V>(mem, dp::flag_aligned), ref);

        t.verify_equal(dp::unchecked_load<V>(ints, dp::flag_convert), ref);
        t.verify_equal(dp::partial_load<V>(ints, dp::flag_convert), ref);
        t.verify_equal(dp::partial_load<V>(
                         ints.begin(), ints.begin(), dp::flag_convert), V());
        t.verify_equal(dp::partial_load<V>(
                         ints.begin(), ints.begin() + 1, dp::flag_convert), ref1);
      }
    };

    ADD_TEST(stores, requires {T() + T(1);}) {
      std::tuple {test_iota<V, 1, 0>, std::array<T, V::size * 2> {}, std::array<int, V::size * 2> {}},
      [](auto& t, const V v, const auto& mem_init, const auto& ints_init) {
        alignas(256) std::array<T, V::size * 2> mem = mem_init;
        alignas(256) std::array<int, V::size * 2> ints = ints_init;

        dp::unchecked_store(v, mem, dp::flag_aligned);
        dp::unchecked_store(v, mem.begin() + V::size(), mem.end());
        for (int i = 0; i < V::size; ++i)
          {
            t.verify_equal(mem[i], T(i + 1));
            t.verify_equal(mem[V::size + i], T(i + 1));
          }
        if constexpr (complex_like<T>)
          {
          }
        else
          {
            dp::unchecked_store(v, ints, dp::flag_convert);
            dp::partial_store(v, ints.begin() + V::size() + 1, ints.end(),
                              dp::flag_convert | dp::flag_overaligned<alignof(T)>);
            for (int i = 0; i < V::size; ++i)
              {
                t.verify_equal(ints[i], int(T(i + 1)));
                t.verify_equal(ints[V::size + i], int(T(i)));
              }

            dp::unchecked_store(V(), ints.begin(), V::size(), dp::flag_convert);
            dp::unchecked_store(V(), ints.begin() + V::size(), V::size(), dp::flag_convert);
            for (int i = 0; i < 2 * V::size; ++i)
              t.verify_equal(ints[i], 0)("i =", i);

            if constexpr (V::size() > 1)
              {
                dp::partial_store(v, ints.begin() + 1, V::size() - 2, dp::flag_convert);
                for (int i = 0; i < V::size - 2; ++i)
                  t.verify_equal(ints[i], int(T(i)));
                t.verify_equal(ints[V::size - 1], 0);
                t.verify_equal(ints[V::size], 0);
              }
            else
              {
                dp::partial_store(v, ints.begin() + 1, 0, dp::flag_convert);
                t.verify_equal(ints[0], 0);
                t.verify_equal(ints[1], 0);
              }
          }
      }
    };
  };
