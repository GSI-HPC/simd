/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */
// requires int

#include "unittest_pch.h"

template <typename V>
  requires (V::size() * sizeof(typename V::value_type) <= 70 * 4) // avoid exploding RAM usage
  struct Tests<V>
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    static constexpr int max = sizeof(T) == 8 ? 64 : 32;

    ADD_TEST_N(known_shift, 4, std::is_integral_v<T>) {
      std::tuple {test_iota<V, 0, 0>},
      []<int N>(auto& t, const V x) {
	constexpr int shift = max * (N + 1) / 4 - 1;
	constexpr V vshift = T(shift);
	const V vshiftx = vshift ^ (x & std::cw<1>);
	V ref([](T i) -> T { return i << shift; });
	V refx([](T i) -> T { return i << (shift ^ (i & 1)); });
	t.verify_equal(x << shift, ref)("{:d} << {:d}", x, shift);
	t.verify_equal(x << vshift, ref)("{:d} << {:d}", x, vshift);
	t.verify_equal(x << vshiftx, refx)("{:d} << {:d}", x, vshiftx);
	const auto y = ~x;
	ref = V([](T i) -> T { return T(~i) << shift; });
	refx = V([](T i) -> T { return T(~i) << (shift ^ (i & 1)); });
	t.verify_equal(y << shift, ref)("{:d} << {:d}", y, shift);
	t.verify_equal(y << vshift, ref)("{:d} << {:d}", y, vshift);
	t.verify_equal(y << vshiftx, refx)("{:d} << {:d}", y, vshiftx);
      }
    };

    ADD_TEST(unknown_shift, std::is_integral_v<T>) {
      std::tuple {test_iota<V, 0, 0>},
      [](auto& t, const V x) {
	if !consteval
	{
	  for (int shift = 0; shift < max; ++shift)
	    {
	      const auto y = ~x;
	      shift = make_value_unknown(shift);
	      const V vshift = T(shift);
	      V ref([=](T i) -> T { return i << shift; });
	      t.verify_equal(x << shift, ref)("{:d} << {:d}", y, shift);
	      t.verify_equal(x << vshift, ref)("{:d} << {:d}", y, vshift);
	      ref = V([=](T i) -> T { return T(~i) << shift; });
	      t.verify_equal(y << shift, ref)("{:d} << {:d}", y, shift);
	      t.verify_equal(y << vshift, ref)("{:d} << {:d}", y, vshift);
	    }
	}
      }
    };
  };

template <typename V>
  struct Tests
  {};

#include "unittest.h"
