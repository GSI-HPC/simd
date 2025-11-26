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
        t.verify(!all_of(k))(k);
        if constexpr (V::size() > 1)
          {
            t.verify(any_of(k))(k);
            t.verify(!none_of(k))(k);
          }

        t.verify(all_of(tr));
        t.verify(any_of(tr));
        t.verify(!none_of(tr));

        t.verify(!all_of(fa));
        t.verify(!any_of(fa));
        t.verify(none_of(fa));

        /* TODO:
        t.verify_equal(reduce_count(tr), V::size());
        t.verify_equal(reduce_count(fa), 0);
        t.verify_equal(reduce_count(k), V::size() / 2);
         */
      }
    };

    ADD_TEST(CvtToInt, (sizeof(T) <= sizeof(0ull))) {
      std::tuple{M([](int i) { return 1 == (i & 1); }), M(true), M(false), M([](int i) {
                   return i % 13 == 0 || i % 7 == 0;
      })},
      [](auto& t, const M k, const M tr, const M fa, const M k2) {
        t.verify_equal(V(+tr), V(1));
        t.verify_equal(V(+fa), V());
        t.verify_equal(V(+k), vec<V, 0, 1>);

        if constexpr (std::is_integral_v<T>)
          {
            t.verify_equal(V(~tr), ~V(1));
            t.verify_equal(V(~fa), ~V(0));
            t.verify_equal(V(~k), ~vec<V, 0, 1>);
          }

        t.verify(all_of(simd::rebind_t<char, M>(tr)));
        t.verify(!all_of(simd::rebind_t<char, M>(fa)));
        t.verify(!all_of(simd::rebind_t<char, M>(k)));

        t.verify_equal(fa.to_ullong(), 0ull);
        t.verify_equal(fa.to_bitset(), std::bitset<V::size()>());

        // test whether 'M -> bitset -> M' is an identity transformation
        t.verify_equal(M(fa.to_bitset()), fa)(fa.to_bitset());
        t.verify_equal(M(tr.to_bitset()), tr)(tr.to_bitset());
        t.verify_equal(M(k.to_bitset()), k)(k.to_bitset());
        t.verify_equal(M(k2.to_bitset()), k2)(k2.to_bitset());

        static_assert(sizeof(0ull) * CHAR_BIT == 64);
        if constexpr (V::size() <= 64)
          {
            constexpr unsigned long long full = -1ull >> (64 - V::size());
            t.verify_equal(tr.to_ullong(), full)(std::hex, tr.to_ullong(), '^', full, "->",
                                                 tr.to_ullong() ^ full);
            t.verify_equal(tr.to_bitset(), full);

            constexpr unsigned long long alternating = 0xaaaa'aaaa'aaaa'aaaaULL & full;
            t.verify_equal(k.to_ullong(), alternating)(std::hex, k.to_ullong(), '^', alternating,
                                                       "->", k.to_ullong() ^ alternating);
            t.verify_equal(k.to_bitset(), alternating);

            // 0, 7, 13, 14, 21, 26, 28, 35, 39, 42, 49, 52, 56, 63, 65, ...
            constexpr unsigned long long bits7_13 = 0x8112'0488'1420'6081ULL & full;
            t.verify_equal(k2.to_ullong(), bits7_13)(std::hex, k2.to_ullong());
          }
        else
          {
            constexpr unsigned long long full = -1ull;
            constexpr unsigned long long alternating = 0xaaaa'aaaa'aaaa'aaaaULL;
            int shift = M::size() - 64;
            t.verify_equal((tr.to_bitset() >> shift).to_ullong(), full);
            t.verify_equal((k.to_bitset() >> shift).to_ullong(), alternating);
          }

        t.verify_equal(+tr, -(-tr));
        t.verify_equal(-+tr, -tr);
      }
    };

    // TODO: test mask conversions

    ADD_TEST(mask_reductions0) {
      std::tuple {M(true)},
      [](auto& t, M x) {
        t.verify_equal(std::simd::reduce_min_index(x), 0);
        t.verify_equal(std::simd::reduce_max_index(x), V::size - 1);
        t.verify_precondition_failure("An empty mask does not have a min_index.", [&] {
          std::simd::reduce_min_index(!x);
        });
        t.verify_precondition_failure("An empty mask does not have a max_index.", [&] {
          std::simd::reduce_max_index(!x);
        });
      }
    };

#if 0 // TODO
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
        t.verify_not_equal(!k, k);
        t.verify_equal(k | k, k);
        t.verify_equal(k & k, k);
        t.verify(none_of(k ^ k));
        t.verify_equal(std::simd::reduce_count(k), nk);
        if constexpr (sizeof(T) <= sizeof(0ULL))
          t.verify_equal(-std::simd::reduce(-k), nk)(k, -k);
        t.verify_equal(std::simd::reduce_count(!k), V::size - nk)(!k);
        if constexpr (V::size <= 128 and sizeof(T) <= sizeof(0ULL))
          t.verify_equal(-std::simd::reduce(-!k), V::size - nk)(-!k);
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

    ADD_TEST(Cat_n_Chunk) {
      std::tuple{M([](int i) { return 1 == (i & 1); }), M([](int i) { return 1 == (i % 3); })},
      [](auto& t, const M k0, const M k1) {
        auto c = cat(k0, k1);
        t.verify_equal(c.size(), V::size() * 2);
        for (int i = 0; i < V::size(); ++i)
          t.verify_equal(c[i], k0[i])(i);
        for (int i = 0; i < V::size(); ++i)
          t.verify_equal(c[i + V::size()], k1[i])(i);
        const auto [c0, c1] = simd::chunk<M>(c);
        t.verify_equal(c0, k0);
        t.verify_equal(c1, k1);
        if constexpr (V::size() <= 35)
          {
            auto d = cat(k1, c, k0);
            for (int i = 0; i < V::size(); ++i)
              {
                t.verify_equal(d[i], k1[i])(i);
                t.verify_equal(d[i + V::size()], k0[i])(i);
                t.verify_equal(d[i + 2 * V::size()], k1[i])(i);
                t.verify_equal(d[i + 3 * V::size()], k0[i])(i);
              }
            const auto [...chunked] = simd::chunk<3>(d);
            t.verify_equal(cat(chunked...), d);
          }
      }
    };
  };

#include "unittest.h"
