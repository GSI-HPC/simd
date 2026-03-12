/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"
#include <climits>
#include <complex>

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr Info<2> info = {"Latency", "Throughput"};

    template <simd_vec_type T>
      [[gnu::always_inline]]
      static void
      d_from_r(T& d, simd_vec_type auto& r)
      {
	vir::fake_modify(r);
	d = std::bit_cast<T>(cat(r, r));
	vir::fake_modify(d);
      }

    template <complex_like T>
      [[gnu::always_inline]]
      static void
      d_from_r(T& d, typename T::value_type& r)
      {
	vir::fake_modify(r);
	d = {r, r};
	vir::fake_modify(d);
      }


    template <class T>
      [[gnu::flatten]]
      static Times<info.size()>
      run()
      {
	using TT = value_type_t<T>;
	using TTT = typename TT::value_type;

	constexpr TT init = TT(1, 0);

	T zerov = T();
	T b = zerov + TT(TTT(0), TTT(1));
	vir::fake_modify(zerov, b);

	T data[6];
	for (T& a : data)
	  {
	    a = zerov + init;
	    vir::fake_modify(a);
	  }

	return {
	  0.25 * time_median([&] [[gnu::always_inline]] {
		   auto d = b;
		   using std::norm;
		   auto r = norm(d); d_from_r(d, r);
		   r = norm(d); d_from_r(d, r);
		   r = norm(d); d_from_r(d, r);
		   r = norm(d); d_from_r(b, r);
		 }),
	  1./6. * time_median([&] [[gnu::always_inline]] {
		    auto d0 = data[0]; vir::fake_modify(d0); auto r0 = std::norm(d0);
		    auto d1 = data[1]; vir::fake_modify(d1); auto r1 = std::norm(d1);
		    auto d2 = data[2]; vir::fake_modify(d2); auto r2 = std::norm(d2);
		    auto d3 = data[3]; vir::fake_modify(d3); auto r3 = std::norm(d3);
		    auto d4 = data[4]; vir::fake_modify(d4); auto r4 = std::norm(d4);
		    auto d5 = data[5]; vir::fake_modify(d5); auto r5 = std::norm(d5);
		    vir::fake_read(r0, r1, r2, r3, r4, r5);
		  })
	};
      }
  };

void
bench_main()
{
  bench_all<std::complex<std::float16_t>>();
  bench_all<std::complex<float>>();
  bench_all<std::complex<double>>();
}
