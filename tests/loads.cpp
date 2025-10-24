/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

// no-fast-math

#include "unittest.h"

template <typename T, std::size_t N, std::size_t Alignment>
  class alignas(Alignment) aligned_array
    : public std::array<T, N>
  {};

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    static_assert(simd::alignment_v<V> <= 256);

    ADD_TEST(loads) {
      std::tuple {aligned_array<T, V::size * 2, 256> {}, aligned_array<int, V::size * 2, 256> {}},
      [](auto& t, auto mem, auto ints) {
        t.verify_equal(simd::unchecked_load<V>(mem), V());
        t.verify_equal(simd::partial_load<V>(mem), V());

        t.verify_equal(simd::unchecked_load<V>(mem, simd::flag_aligned), V());
        t.verify_equal(simd::partial_load<V>(mem, simd::flag_aligned), V());

        t.verify_equal(simd::unchecked_load<V>(mem, simd::flag_overaligned<256>), V());
        t.verify_equal(simd::partial_load<V>(mem, simd::flag_overaligned<256>), V());

        t.verify_equal(simd::unchecked_load<V>(mem.begin() + 1, mem.end()), V());
        t.verify_equal(simd::partial_load<V>(mem.begin() + 1, mem.end()), V());
        t.verify_equal(simd::partial_load<V>(mem.begin() + 1, mem.begin() + 1), V());
        t.verify_equal(simd::partial_load<V>(mem.begin() + 1, mem.begin() + 2), V());

        t.verify_equal(simd::unchecked_load<V>(ints, simd::flag_convert), V());
        t.verify_equal(simd::partial_load<V>(ints, simd::flag_convert), V());
      }
    };

    ADD_TEST(loads_iota, requires {T() + T(1);}) {
      std::tuple {[] {
        aligned_array<T, V::size * 2, simd::alignment_v<V>> arr = {};
        T init = 0;
        for (auto& x : arr) x = (init += T(1));
        return arr;
      }(), [] {
        aligned_array<int, V::size * 2, simd::alignment_v<V, int>> arr = {};
        std::iota(arr.begin(), arr.end(), 1);
        return arr;
      }()},
      [](auto& t, auto mem, auto ints) {
        constexpr V ref = test_iota<V, 1, 0>;
        constexpr V ref1 = V([](int i) { return i == 0 ? T(1): T(); });

        t.verify_equal(simd::unchecked_load<V>(mem), ref);
        t.verify_equal(simd::partial_load<V>(mem), ref);

        t.verify_equal(simd::unchecked_load<V>(mem.begin() + 1, mem.end()), ref + T(1));
        t.verify_equal(simd::partial_load<V>(mem.begin() + 1, mem.end()), ref + T(1));
        t.verify_equal(simd::partial_load<V>(mem.begin(), mem.begin() + 1), ref1);

        t.verify_equal(simd::unchecked_load<V>(mem, simd::flag_aligned), ref);
        t.verify_equal(simd::partial_load<V>(mem, simd::flag_aligned), ref);

        t.verify_equal(simd::unchecked_load<V>(ints, simd::flag_convert), ref);
        t.verify_equal(simd::partial_load<V>(ints, simd::flag_convert), ref);
        t.verify_equal(simd::partial_load<V>(
                         ints.begin(), ints.begin(), simd::flag_convert), V());
        t.verify_equal(simd::partial_load<V>(
                         ints.begin(), ints.begin() + 1, simd::flag_convert), ref1);
      }
    };
  };
