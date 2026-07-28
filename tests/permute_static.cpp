/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2026      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */
// no-fast-math

#include "unittest.h"
#include "complex_init.h"

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    ADD_TEST(permN) {
      std::tuple{V(), init_vec<V, 1, 2, 3, 4, 5, 6, 7>},
      [](auto& t, V x, V y) {
	static constexpr auto p = [](int i) {
	  return std::min(V::size() - 1, std::array{1, 3, 1, 2, 7}[i % 5]);
	};
	t.verify_equal(permute(x, p), V());
	if constexpr (V::size() == 1)
	  t.verify_equal(permute(y, p), T(1));
	else if constexpr (V::size() == 2)
	  t.verify_equal(permute(y, p), T(2));
	else if constexpr (V::size() == 3)
	  t.verify_equal(permute(y, p), init_vec<V, 2, 3, 2>);
	else if constexpr (V::size() == 4)
	  t.verify_equal(permute(y, p), init_vec<V, 2, 4, 2, 3>);
	else if constexpr (V::size() <= 7)
	  t.verify_equal(permute(y, p), init_vec<V, 2, 4, 2, 3, V::size()>);
	else
	  t.verify_equal(permute(y, p), init_vec<V, 2, 4, 2, 3, 1>);
      }
    };

    using V3 = simd::resize_t<3, V>;
    ADD_TEST(perm3) {
      std::tuple{init_vec<V, 1, 2, 3, 4, 5, 6, 7>},
      [](auto& t, V x) {
	static constexpr auto p3 = [](int i) {
	  return std::min(V::size() - 1, std::array{1, 5, 0}[i % 3]);
	};
	if constexpr (V::size() == 1)
	  t.verify_equal(simd::permute<3>(x, p3), T(1));
	else if constexpr (V::size() == 2)
	  t.verify_equal(simd::permute<3>(x, p3), init_vec<V3, 2, 2, 1>);
	else if constexpr (V::size() <= 5)
	  t.verify_equal(simd::permute<3>(x, p3), init_vec<V3, 2, V::size(), 1>);
	else
	  t.verify_equal(simd::permute<3>(x, p3), init_vec<V3, 2, 6, 1>);
      }
    };

    using V9 = simd::resize_t<9, V>;
    ADD_TEST(perm9) {
      std::tuple{init_vec<V, 1, 2, 3, 4, 5, 6, 7>},
      [](auto& t, V x) {
	static constexpr auto p9 = [](int i) {
	  return std::array{1, 0, 2, 7, 6, 5, 4, 2, 3}[i % 9] % V::size();
	};
	if constexpr (V::size() == 1)
	  t.verify_equal(simd::permute<9>(x, p9), T(1));
	else if constexpr (V::size() == 2)
	  t.verify_equal(simd::permute<9>(x, p9), init_vec<V9, 2, 1, 1, 2, 1, 2, 1, 1, 2>);
	else if constexpr (V::size() == 3)
	  t.verify_equal(simd::permute<9>(x, p9), init_vec<V9, 2, 1, 3, 2, 1, 3, 2, 3, 1>);
	else if constexpr (V::size() == 4)
	  t.verify_equal(simd::permute<9>(x, p9), init_vec<V9, 2, 1, 3, 4, 3, 2, 1, 3, 4>);
	else if constexpr (V::size() == 5)
	  t.verify_equal(simd::permute<9>(x, p9), init_vec<V9, 2, 1, 3, 3, 2, 1, 5, 3, 4>);
	else if constexpr (V::size() == 6)
	  t.verify_equal(simd::permute<9>(x, p9), init_vec<V9, 2, 1, 3, 2, 1, 6, 5, 3, 4>);
	else
	  t.verify_equal(simd::permute<9>(x, p9), init_vec<V9, 2, 1, 3, 1, 7, 6, 5, 3, 4>);
      }
    };
  };
