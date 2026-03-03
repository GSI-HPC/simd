/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

// no-fast-math

#include "unittest.h"

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    static_assert(simd::alignment_v<V> <= 256);

    ADD_TEST(stores) {
      std::tuple {test_iota<V, 1, 0>},
      [](auto& t, const V v) {
	alignas(256) std::array<T, V::size * 2> mem = {};

	simd::unchecked_store(v, mem, simd::flag_aligned);
	simd::partial_store(v, mem.begin() + V::size(), mem.begin() + V::size() + 2);
	simd::partial_store(v, mem.end(), mem.end());
	for (int i = 0; i < V::size; ++i)
	  {
	    const T ref(i + 1);
	    t.verify_equal(mem[i], ref);
	    t.verify_equal(mem[V::size + i], i < 2 ? ref : T())(ref);
	  }
      }
    };

    using Other = std::conditional_t<complex_like<T>,
				     std::conditional_t<sizeof(T) == sizeof(std::complex<float>),
							std::complex<double>, std::complex<float>>,
				     std::conditional_t<sizeof(T) == sizeof(int),
							unsigned short, int>>;

    ADD_TEST(converting_stores) {
      std::tuple {test_iota<V, 1, 0>},
      [](auto& t, const V v) {
	alignas(256) std::array<Other, V::size * 2> mem2 = {};
	simd::unchecked_store(v, mem2, simd::flag_convert);
	simd::partial_store(v, mem2.begin() + V::size() + 1, mem2.end(),
			    simd::flag_convert | simd::flag_overaligned<alignof(Other)>);
	for (int i = 0; i < V::size; ++i)
	  {
	    t.verify_equal(mem2[i], Other(T(i + 1)));
	    t.verify_equal(mem2[V::size + i], Other(T(i)));
	  }

	simd::unchecked_store(V(), mem2.begin(), V::size(), simd::flag_convert);
	simd::unchecked_store(V(), mem2.begin() + V::size(), V::size(), simd::flag_convert);
	for (int i = 0; i < 2 * V::size; ++i)
	  t.verify_equal(mem2[i], Other())("i =", i);

	if constexpr (V::size() > 1)
	  {
	    simd::partial_store(v, mem2.begin() + 1, V::size() - 2, simd::flag_convert);
	    for (int i = 0; i < V::size - 2; ++i)
	      t.verify_equal(mem2[i], Other(T(i)));
	    t.verify_equal(mem2[V::size - 1], Other());
	    t.verify_equal(mem2[V::size], Other());
	  }
	else
	  {
	    simd::partial_store(v, mem2.begin() + 1, 0, simd::flag_convert);
	    t.verify_equal(mem2[0], Other());
	    t.verify_equal(mem2[1], Other());
	  }
      }
    };

    static constexpr M alternating = M([](int i) { return 1 == (i & 1); });

    ADD_TEST(masked_stores) {
      std::tuple {test_iota<V, 1, 0>, alternating, M(true), M(false)},
      [](auto& t, const V v, const M al, const M tr, const M fa) {
	alignas(256) std::array<T, V::size * 2> mem = {};

	simd::unchecked_store(v, mem, fa, simd::flag_aligned);
	simd::unchecked_store(v, mem.begin() + V::size(), mem.end(), fa);
	for (int i = 0; i < V::size; ++i)
	  {
	    t.verify_equal(mem[i], T());
	    t.verify_equal(mem[V::size + i], T());
	  }
	simd::unchecked_store(v, mem, tr, simd::flag_aligned);
	simd::unchecked_store(v, mem.begin() + V::size(), mem.end(), tr);
	for (int i = 0; i < V::size; ++i)
	  {
	    t.verify_equal(mem[i], T(i + 1));
	    t.verify_equal(mem[V::size + i], T(i + 1));
	    mem[i] = mem[V::size + i] = T();
	  }
	simd::unchecked_store(v, mem, al, simd::flag_aligned);
	simd::unchecked_store(v, mem.begin() + V::size(), mem.end(), al);
	for (int i = 0; i < V::size; ++i)
	  {
	    t.verify_equal(mem[i], i % 2 ? T(i + 1) : T() );
	    t.verify_equal(mem[V::size + i], i % 2 ? T(i + 1) : T());
	  }
      }
    };
  };
