/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2026      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "unittest.h"
#include "complex_init.h"

template <typename V>
  struct Tests
  {
#if VIR_PATCH_PERMUTE_DYNAMIC
    using T = typename V::value_type;
    using M = typename V::mask_type;
    using P = simd::rebind_t<int, V>;

    ADD_TEST(shufN) {
      std::tuple{V(), init_vec<V, 1, 2, 3, 4, 5, 6, 7>, init_vec<P, 1, 3, 1, 2>},
      [](auto& t, V x, V y, P perm) {
	if constexpr (V::size() < 4)
	  t.verify_precondition_failure("a dynamic permute index is out of bounds",
					[&] { x[perm]; });
	else
	  {
	    t.verify_equal(x[perm], V());
	    t.verify_equal(y[perm], init_vec<V, 2, 4, 2, 3>);
	  }
      }
    };

    using P3 = simd::resize_t<3, P>;
    using V3 = simd::resize_t<3, V>;
    ADD_TEST(shuf3) {
      std::tuple{init_vec<V, 1, 2, 3, 4, 5, 6, 7>, init_vec<P3, 1, 5, 0> },
      [](auto& t, V x, P3 perm) {
	if constexpr (V::size() < 6)
	  t.verify_precondition_failure("a dynamic permute index is out of bounds",
					[&] { x[perm]; });
	else
	  t.verify_equal(x[perm], init_vec<V3, 2, 6, 1>);
      }
    };

    using P9 = simd::resize_t<9, P>;
    using V9 = simd::resize_t<9, V>;
    ADD_TEST(shuf9) {
      std::tuple{init_vec<V, 1, 2, 3, 4, 5, 6, 7>, init_vec<P9, 1, 0, 2, 7, 6, 5, 4, 2, 3>},
      [](auto& t, V x, P9 perm) {
	if constexpr (V::size() < 8)
	  t.verify_precondition_failure("a dynamic permute index is out of bounds",
					[&] { x[perm]; });
	else
	  t.verify_equal(x[perm], init_vec<V9, 2, 1, 3, 1, 7, 6, 5, 3, 4>);
      }
    };

#endif
  };
