/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "unittest_pch.h"

template <std::size_t B, typename A>
  consteval std::size_t
  element_size(const simd::basic_mask<B, A>&)
  { return B; }

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    ADD_TEST(Sanity) {
      std::tuple{M([](int i) { return 1 == (i & 1); })},
      [](auto& t, const M k) {
        t.verify_equal(element_size(k), sizeof(T));
        for (int i = 0; i < k.size(); i += 2)
          t.verify_equal(k[i], false)(k);
        for (int i = 1; i < k.size(); i += 2)
          t.verify_equal(k[i], true)(k);
      }
    };

    ADD_TEST(Reductions) {
      std::tuple{M([](int i) { return 1 == (i & 1); }), M(true), M(false)},
      [](auto& t, const M k, const M tr, const M fa) {
        t.verify(not all_of(k))(k);
        if constexpr (V::size() > 1)
          {
            t.verify(any_of(k))(k);
            t.verify(not none_of(k))(k);
          }

        t.verify(all_of(tr));
        t.verify(any_of(tr));
        t.verify(not none_of(tr));

        t.verify(not all_of(fa));
        t.verify(not any_of(fa));
        t.verify(none_of(fa));

        /* TODO:
        t.verify_equal(reduce_count(tr), V::size());
        t.verify_equal(reduce_count(fa), 0);
        t.verify_equal(reduce_count(k), V::size() / 2);
         */
      }
    };

#if 0 // TODO
    ADD_TEST(mask_reductions0) {
      std::tuple {test_iota<V>},
      [](auto& t, V x) {
        t.verify_equal(std::simd::reduce_min_index(x == x), 0);
        t.verify_equal(std::simd::reduce_max_index(x == x), V::size - 1);
      }
    };

    ADD_TEST_N(mask_reductions, int(test_iota_max<V>) + 1, requires(T x) { x + x; }) {
      std::tuple{test_iota<V>, test_iota<V> == T(0)},
      [](auto& t, auto ii, V v, M k0) {
        constexpr int i = ii;
        M k = v == T(i);

        // Caveat:
        // k0[n0 * (test_iota_max<V> + 1)] is true if it exists
        // k[n * (test_iota_max<V> + 1) + i] is true if it exists
        // none_of(k) is true if i > test_iota_max<V>
        // => by test construction:
        static_assert(i <= test_iota_max<V>);
        // also by construction of test_iota_max:
        static_assert(test_iota_max<V> < V::size());

        constexpr int nk = 1 + (V::size() - i - 1) / (test_iota_max<V> + 1);
        constexpr int maxk = (nk - 1) * (test_iota_max<V> + 1) + i;
        static_assert(maxk < V::size());

        constexpr int nk0 = 1 + (V::size() - 1) / (test_iota_max<V> + 1);
        constexpr int maxk0 = (nk0 - 1) * (test_iota_max<V> + 1);
        static_assert(maxk0 < V::size());

        constexpr int maxkork0 = std::max(maxk, maxk0);

        t.verify_equal(k[i], true);
        t.verify_equal(std::as_const(k)[i], true);
        t.verify_equal(std::simd::reduce_min_index(k), i)(k);
        t.verify_equal(std::simd::reduce_max_index(k), maxk)(k);
        t.verify_equal(std::simd::reduce_min_index(k || k0), 0);
        t.verify_equal(std::simd::reduce_max_index(k || k0), maxkork0);
        t.verify_equal(k, k);
        t.verify_not_equal(not k, k);
        t.verify_equal(k | k, k);
        t.verify_equal(k & k, k);
        t.verify(none_of(k ^ k));
        t.verify_equal(std::simd::reduce_count(k), nk);
        if constexpr (sizeof(T) <= sizeof(0ULL))
          t.verify_equal(-std::simd::reduce(-k), nk)(k, -k);
        t.verify_equal(std::simd::reduce_count(not k), V::size - nk)(not k);
        if constexpr (V::size <= 128 and sizeof(T) <= sizeof(0ULL))
          t.verify_equal(-std::simd::reduce(-not k), V::size - nk)(-not k);
        t.verify(any_of(k));
        t.verify(bool(any_of(k & k0) ^ (i != 0)));
        k = M([&](int i) { return i == 0 ? true : k[i]; });
        t.verify_equal(k[i], true);
        t.verify_equal(std::as_const(k)[i], true);
        t.verify_equal(k[0], true);
        t.verify_equal(std::as_const(k)[0], true);
        t.verify_equal(std::simd::reduce_min_index(k), 0)(k);
        t.verify_equal(std::simd::reduce_max_index(k), maxk)(k);
      }
    };
#endif
  };

#include "unittest.h"
