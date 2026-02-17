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

    static constexpr T min = std::numeric_limits<T>::lowest();
    static constexpr T norm_min = std::numeric_limits<T>::min();
    static constexpr T max = std::numeric_limits<T>::max();

    ADD_TEST(shuf0) {
      std::tuple{V(), init_vec<V, 1, 2, 3, 4, 5, 6, 7>, init_vec<P, 3, 2, 1, 0>},
      [](auto& t, V x, V y, P perm) {
	if constexpr (V::size() < 4)
	  t.verify_precondition_failure("subscript is out of bounds", [&] { x[perm]; });
	else
	  {
	    t.verify_equal(x[perm], V());
	    t.verify_equal(y[perm], init_vec<V, 4, 3, 2, 1>);
	  }
      }
    };
#endif
  };
