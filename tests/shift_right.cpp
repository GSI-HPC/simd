/* SPDX-License-Identifier: BSD-3-Clause */
// Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
//                       Matthias Kretz <m.kretz@gsi.de>

#include "unittest_pch.h"

using namespace vir::literals;

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    static constexpr int max = sizeof(T) == 8 ? 64 : 32;

    ADD_TEST(constant_p) {
      std::tuple {},
      [](auto& t) {
        V x = {};
        t.verify(std::is_constant_evaluated() or x._M_is_constprop());
      }
    };

    ADD_TEST_N(known_shift, max, std::is_integral_v<T>) {
      std::tuple {test_iota<V>},
      [](auto& t, auto _shift, const V x) {
        constexpr T tmax = std::numeric_limits<T>::max();
        constexpr int shift = _shift;
        constexpr V vshift = T(shift);
        const V vshiftx = vshift ^ (x & 1_cw);
        t.verify(vshift._M_is_constprop());

        V ref([](T i) -> T { return i >> shift; });
        V refx([](T i) -> T { return i >> (shift ^ (i & 1)); });
        t.verify_equal(x >> shift, ref)(x, ">>", shift);
        t.verify_equal(x >> vshift, ref)(x, ">>", vshift);
        t.verify_equal(x >> vshiftx, refx)(x, ">>", vshiftx);

        const V y = ~x;
        ref = V([](T i) -> T { return T(~i) >> shift; });
        refx = V([](T i) -> T { return T(~i) >> (shift ^ (i & 1)); });
        t.verify_equal(y >> shift, ref)(y, ">>", shift);
        t.verify_equal(y >> vshift, ref)(y, ">>", vshift);
        t.verify_equal(y >> vshiftx, refx)(y, ">>", vshiftx);

        const V z = tmax - x;
        ref = V([](T i) -> T { return T(tmax - i) >> shift; });
        refx = V([](T i) -> T { return T(tmax - i) >> (shift ^ (i & 1)); });
        t.verify_equal(z >> shift, ref)(z, ">>", shift);
        t.verify_equal(z >> vshift, ref)(z, ">>", vshift);
        t.verify_equal(z >> vshiftx, refx)(z, ">>", vshiftx);
      }
    };

    ADD_TEST(unknown_shift, std::is_integral_v<T>) {
      std::tuple {test_iota<V>},
      [](auto& t, const V x) {
        for (int shift : dp::iota<dp::simd<int, max>>)
          {
            constexpr T tmax = std::numeric_limits<T>::max();
            const V vshift = T(shift);
            const V vshiftx = vshift ^ (x & 1_cw);
            t.verify(std::is_constant_evaluated()
                       or (not is_constprop(vshift) and not is_constprop(shift)));

            V ref([&](T i) -> T { return i >> shift; });
            V refx([&](T i) -> T { return i >> (shift ^ (i & 1)); });
            t.verify_equal(x >> shift, ref)(x, ">>", shift);
            t.verify_equal(x >> vshift, ref)(x, ">>", vshift);
            t.verify_equal(x >> vshiftx, refx)(x, ">>", vshiftx);

            const V y = ~x;
            ref = V([&](T i) -> T { return T(~i) >> shift; });
            refx = V([&](T i) -> T { return T(~i) >> (shift ^ (i & 1)); });
            t.verify_equal(y >> shift, ref)(y, ">>", shift);
            t.verify_equal(y >> vshift, ref)(y, ">>", vshift);
            t.verify_equal(y >> vshiftx, refx)(y, ">>", vshiftx);

            const V z = tmax - x;
            ref = V([&](T i) -> T { return T(tmax - i) >> shift; });
            refx = V([&](T i) -> T { return T(tmax - i) >> (shift ^ (i & 1)); });
            t.verify_equal(z >> shift, ref)(z, ">>", shift);
            t.verify_equal(z >> vshift, ref)(z, ">>", vshift);
            t.verify_equal(z >> vshiftx, refx)(z, ">>", vshiftx);
          }
      }
    };
  };

#include "unittest.h"
