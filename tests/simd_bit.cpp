/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2025–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */
// requires int

#include "unittest_pch.h"

#include <climits>

template <typename V>
  struct CheckInvocable
  {
    using T = typename V::value_type;
    static constexpr bool unsigned_integer
      = any_type_of<T, unsigned char, unsigned short, unsigned int, unsigned long,
		    unsigned long long>;
    static_assert(std::integral<T> == requires(V x) { std::byteswap(x); });
    static_assert(unsigned_integer == requires(V x) { std::bit_ceil(x); });
    static_assert(unsigned_integer == requires(V x) { std::bit_floor(x); });
    static_assert(unsigned_integer == requires(V x) { std::has_single_bit(x); });
    static_assert(unsigned_integer == requires(V x, V y) { std::rotl(x, y); });
    static_assert(unsigned_integer == requires(V x, int y) { std::rotl(x, y); });
    static_assert(unsigned_integer == requires(V x, V y) { std::rotr(x, y); });
    static_assert(unsigned_integer == requires(V x, int y) { std::rotr(x, y); });
    static_assert(unsigned_integer == requires(V x) { std::bit_width(x); });
    static_assert(unsigned_integer == requires(V x) { std::countl_zero(x); });
    static_assert(unsigned_integer == requires(V x) { std::countl_one(x); });
    static_assert(unsigned_integer == requires(V x) { std::countr_zero(x); });
    static_assert(unsigned_integer == requires(V x) { std::countr_one(x); });
    static_assert(unsigned_integer == requires(V x) { std::popcount(x); });
  };

template <typename V>
  requires std::integral<typename V::value_type>
    && (V::size() * sizeof(typename V::value_type) <= 70 * 4) // avoid exploding RAM usage
  struct Tests<V> : CheckInvocable<V>
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
	    auto y = std::simd::byteswap(b);
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
	t.verify_equal(std::simd::bit_ceil(a), bit_ceil(a));
	t.verify_equal(bit_ceil(a), V([&](int i) { return std::bit_ceil(a[i]); }));
      }
    };

    ADD_TEST(BitFloor, std::__unsigned_integer<T>) {
      std::tuple {test_iota<V>, T(1024), T(msb + 1)},
      [](auto& t, const V a, const V b, const V c) {
	t.verify_equal(bit_floor(c), msb);
	t.verify_equal(bit_floor(b), b);
	t.verify_equal(std::bit_floor(a), bit_floor(a));
	t.verify_equal(std::simd::bit_floor(a), bit_floor(a));
	t.verify_equal(bit_floor(a), V([&](int i) { return std::bit_floor(a[i]); }));
      }
    };

    ADD_TEST(HasSingleBit, std::__unsigned_integer<T>) {
      std::tuple {test_iota<V>, msb, T(msb + 1)},
      [](auto& t, const V a, const V b, const V c) {
	t.verify(all_of(has_single_bit(b)));
	t.verify(none_of(has_single_bit(c)));
	t.verify_equal(std::has_single_bit(a), has_single_bit(a));
	t.verify_equal(std::simd::has_single_bit(a), has_single_bit(a));
	t.verify_equal(has_single_bit(a), a != T() && a == bit_floor(a));
      }
    };

    ADD_TEST(RotateLeft, std::__unsigned_integer<T>) {
      std::tuple {test_iota<V, 0, 0>},
      [](auto& t, const V a) {
	t.verify_equal(rotl(a, sizeof(T) * CHAR_BIT), a);
	t.verify_equal(std::rotl(a, sizeof(T) * CHAR_BIT), a);
	t.verify_equal(std::simd::rotl(a, sizeof(T) * CHAR_BIT), a);
      }
    };

    using I = std::make_signed_t<T>;
    using IV = std::simd::rebind_t<I, V>;

    ADD_TEST_N(RotateN, 128, std::__unsigned_integer<T>) {
      std::tuple {test_iota<V, 0, 0>},
      []<int shift>(auto& t, const V x) {
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
  struct Tests : CheckInvocable<V>
  {};

#include "unittest.h"
