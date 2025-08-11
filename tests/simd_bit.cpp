/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "unittest_pch.h"

template <typename V>
  requires std::integral<typename V::value_type>
  struct Tests<V>
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    static constexpr T msb = T(std::make_unsigned_t<T>(1) << (sizeof(T) * CHAR_BIT - 1));

    ADD_TEST(Byteswap) {
      std::tuple {test_iota<V>, V(T(0x01'02'03'04'05'06'07'08LL))},
      [](auto& t, const V a, const V b) {
        if constexpr (sizeof(T) == 1)
          t.verify_equal(std::byteswap(a), a);
        else
          {
            auto x = std::byteswap(a);
            for (int i = 0; i < V::size(); ++i)
              t.verify_equal(x[i], std::byteswap(a[i]));
            auto y = std::datapar::byteswap(b);
            for (int i = 0; i < V::size(); ++i)
              t.verify_equal(y[i], std::byteswap(b[i]));
          }
      }
    };

    ADD_TEST(BitCeil, std::__unsigned_integer<T>) {
      std::tuple {test_iota<V, 0, msb < test_iota_max<V> ? msb : test_iota_max<V>>,
                  T(1024), T(msb + 1)},
      [](auto& t, const V a, const V b, const V c) {
        t.verify_precondition_failure("bit_ceil result is not representable", [&] {
          bit_ceil(c);
        });
        t.verify_equal(bit_ceil(b), select(b == T(), T(1), b));
        t.verify_equal(std::bit_ceil(a), bit_ceil(a));
        t.verify_equal(std::datapar::bit_ceil(a), bit_ceil(a));
        t.verify_equal(bit_ceil(a),
                       V([](T i) {
                         if (i > msb)
                           i -= msb + 1;
                         while (not std::has_single_bit(i))
                           i = (i | (i >> 1)) + 1;
                         return T(i);
                       }));
      }
    };

    ADD_TEST(BitFloor, std::__unsigned_integer<T>) {
      std::tuple {test_iota<V>, T(1024), T(msb + 1)},
      [](auto& t, const V a, const V b, const V c) {
        t.verify_equal(bit_floor(c), msb);
        t.verify_equal(bit_floor(b), b);
        t.verify_equal(std::bit_floor(a), bit_floor(a));
        t.verify_equal(std::datapar::bit_floor(a), bit_floor(a));
        t.verify_equal(bit_floor(a),
                       V([](T i) -> T {
                         if (i == 0)
                           return 0;
                         int shift = 0;
                         while ((i >> shift) > 1)
                           ++shift;
                         return T(1) << shift;
                       }));
      }
    };

    ADD_TEST(HasSingleBit, std::__unsigned_integer<T>) {
      std::tuple {test_iota<V>, msb, T(msb + 1)},
      [](auto& t, const V a, const V b, const V c) {
        t.verify(all_of(has_single_bit(b)));
        t.verify(none_of(has_single_bit(c)));
        t.verify_equal(std::has_single_bit(a), has_single_bit(a));
        t.verify_equal(std::datapar::has_single_bit(a), has_single_bit(a));
        t.verify_equal(has_single_bit(a), a != T() and a == bit_floor(a));
      }
    };

    ADD_TEST(RotateLeft, std::__unsigned_integer<T>) {
      std::tuple {test_iota<V, 0, 0>},
      [](auto& t, const V a) {
        t.verify_equal(rotl(a, sizeof(T) * CHAR_BIT), a);
        t.verify_equal(std::rotl(a, sizeof(T) * CHAR_BIT), a);
        t.verify_equal(std::datapar::rotl(a, sizeof(T) * CHAR_BIT), a);
      }
    };

    using I = std::make_signed_t<T>;
    using IV = std::datapar::rebind_t<I, V>;

    ADD_TEST_N(RotateN, 128, std::__unsigned_integer<T>) {
      std::tuple {test_iota<V, 0, 0>},
      [](auto& t, auto _shift, const V x) {
        constexpr int shift = _shift - 64;
        constexpr int rshift = I(sizeof(T) * CHAR_BIT) - shift;
        const IV vshift = I(shift);
        const IV vshiftx = vshift ^ IV(x & T(1));
        V ref([](T i) -> T { return std::rotl(i, shift); });
        V refx([](T i) -> T { return std::rotl(i, shift ^ (i & 1)); });
        const V l1 = rotl(x, shift);
        const V lv = rotl(x, vshift);
        const V lx = rotl(x, vshiftx);
        t.verify_equal(l1, ref);
        t.verify_equal(lv, ref);
        t.verify_equal(lx, refx);
        t.verify_equal(rotr(x, rshift), ref);
        t.verify_equal(rotr(x, I(rshift)), ref);
        t.verify_equal(rotr(x, I(sizeof(T) * CHAR_BIT) - vshiftx), refx);
      }
    };
  };

template <typename V>
  struct Tests
  {};

#include "unittest.h"
