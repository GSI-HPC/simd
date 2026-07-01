/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
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

    ADD_TEST(CallIntrinsic, !simd::__scalar_abi_tag<typename V::abi_type>) {
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

    ADD_TEST(ConvertToGnuVec, std::is_arithmetic_v<T>) {
      std::tuple {test_iota<V>},
      [](auto& t, V x) {
	constexpr unsigned N2 = std::bit_ceil(sizeof(T) * V::size());
	using GnuV [[gnu::vector_size(N2)]] = T;
	GnuV y = x;
	for (int i = 0; i < V::size(); ++i)
	  t.verify_equal(y[i], x[i]);
	x = y;
	for (int i = 0; i < V::size(); ++i)
	  t.verify_equal(x[i], y[i]);
      }
    };

    static constexpr M alternating = M([](int i) { return 1 == (i & 1); });

    ADD_TEST(ConvertMaskToGnuVec, M::size() == 1 || M::abi_type::_S_is_vecmask) {
      std::tuple {alternating},
      [](auto &t, M x) {
	// complex masks are special:
	// 1. rather than sizeof(complex) integers, the masks are made up of
	//    sizeof(complex::value_type) integers
	// 2. _CxIleav stores one integer *per complex::value_type*
	// 3. _CxCtgus stores one integer *per complete complex*
	constexpr bool interleaved = M::abi_type::_S_is_cx_ileav && M::size() > 1;
	constexpr unsigned Iwidth = sizeof(T) / (1 + simd::__complex_like<T>);
	constexpr unsigned N2 = std::bit_ceil(Iwidth * M::size() * (1 + interleaved));
	using GnuV [[gnu::vector_size(N2)]] = simd::__integer_from<Iwidth>;
	GnuV y = x;
	for (int i = 0; i < V::size(); ++i)
	  {
	    if constexpr (interleaved)
	      {
		t.verify_equal(y[i * 2] == 0, x[i] == 0);
		t.verify_equal(y[i * 2 + 1] == 0, x[i] == 0);
	      }
	    else
	      t.verify_equal(y[i] == 0, x[i] == 0);
	  }
	x = y;
	for (int i = 0; i < V::size(); ++i)
	  {
	    if constexpr (interleaved)
	      {
		t.verify_equal(x[i] == 0, y[i * 2] == 0);
		t.verify_equal(x[i] == 0, y[i * 2 + 1] == 0);
	      }
	    else
	      t.verify_equal(x[i] == 0, y[i] == 0);
	  }
      }
    };
  };
