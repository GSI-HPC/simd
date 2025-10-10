/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

// no-fast-math

#include "unittest.h"
#include <numeric>

using namespace vir::literals;

template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    ADD_TEST(misc) {
      std::tuple{vec<V, 0, 100, 2, 54, 3>},
      [](auto& t, V x) {
        t.verify_equal(x, x);
        if not consteval
        {
#ifdef __SSE__
          if constexpr (sizeof(x) == 16 and std::is_same_v<T, float>)
            t.verify_equal(_mm_and_ps(x, x), x);
#endif
#ifdef __SSE2__
          if constexpr (sizeof(x) == 16 and std::is_integral_v<T>)
            t.verify_equal(_mm_and_si128(x, x), x);
          if constexpr (sizeof(x) == 16 and std::is_same_v<T, double>)
            t.verify_equal(_mm_and_pd(x, x), x);
#endif
        }
      }
    };
  };
