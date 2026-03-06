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
	  0.25 * time_mean([&] [[gnu::always_inline]] {
		   auto d = data[0];
		   vir::fake_modify(d); T r = d * b;
		   vir::fake_modify(d);   r = d * r;
		   vir::fake_modify(d);   r = d * r;
		   vir::fake_modify(d);   r = d * r;
		   b = r;
		 }),
	  1./6. * time_mean([&] [[gnu::always_inline]] {
		    auto d0 = data[0]; vir::fake_modify(b, d0); T r0 = d0 * b;
		    auto d1 = data[1]; vir::fake_modify(b, d1); T r1 = d1 * b;
		    auto d2 = data[2]; vir::fake_modify(b, d2); T r2 = d2 * b;
		    auto d3 = data[3]; vir::fake_modify(b, d3); T r3 = d3 * b;
		    auto d4 = data[4]; vir::fake_modify(b, d4); T r4 = d4 * b;
		    auto d5 = data[5]; vir::fake_modify(b, d5); T r5 = d5 * b;
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
