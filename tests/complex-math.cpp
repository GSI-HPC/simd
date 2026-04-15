/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2024–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */
// requires complex

#include "unittest.h"

static constexpr bool is_iec559 =
#ifdef __GCC_IEC_559
      __GCC_IEC_559 >= 2;
#elif defined __STDC_IEC_559__
      __STDC_IEC_559__ == 1;
#else
      false;
#endif

// no tests for non-complex types
template <typename V>
  struct Tests {};

template <typename V>
  requires complex_like<typename V::value_type>
  struct Tests<V>
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;
    using L = std::numeric_limits<T>;
    using RV = simd::rebind_t<typename T::value_type, V>;

    ADD_TEST(Norm) {
      std::tuple {test_iota<V>},
      [](auto& t, const V x) {
	const V y = {x.real(), x.real() / std::cw<3>};
	t.verify_equal(norm(x), RV([&](int i) { return std::norm(x[i]); }));
	t.verify_equal(norm(y), RV([&](int i) { return std::norm(y[i]); }));
      }
    };

    ADD_TEST(Abs) {
      std::tuple {test_iota<V>},
      [](auto& t, const V x) {
	if !consteval
	  {
	    const V y = {x.real(), x.real() / std::cw<3>};
	    t.verify_equal(abs(x), RV([&](int i) { return std::abs(x[i]); }));
	    t.verify_equal(abs(y), RV([&](int i) { return std::abs(y[i]); }));
	  }
      }
    };
  };
