/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

// no-fast-math

#include "unittest.h"
#include <numeric>

#ifdef __SSE__
#include <x86intrin.h>
#endif

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    ADD_TEST(misc, !simd::__scalar_abi_tag<typename V::abi_type>) {
      std::tuple{init_vec<V, 0, 100, 2, 54, 3>},
      [](auto& t, V x) {
	t.verify_equal(x, x);
	if !consteval
	{
#ifdef __SSE__
	  V r = x;
	  if constexpr (sizeof(x) == 16 && std::is_same_v<T, float>)
	    t.verify_equal(r = _mm_and_ps(x, x), x);
#endif
#ifdef __SSE2__
	  if constexpr (sizeof(x) == 16 && std::is_integral_v<T>)
	    t.verify_equal(r = _mm_and_si128(x, x), x);
	  if constexpr (sizeof(x) == 16 && std::is_same_v<T, double>)
	    t.verify_equal(r = _mm_and_pd(x, x), x);
#endif
	}
      }
    };
  };
