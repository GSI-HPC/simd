/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

FUN1(sin) Sine;
FUN1(cos) Cosine;

template <class What, class T>
  struct ProcessOne
  {
    [[gnu::always_inline]]
    static T operator()(auto fake, T x)
    {
      if (fake)
	x += x;
      else
	x += What::apply(What::apply(x));
      return x;
    }
  };

template <int Special, class What>
  struct Benchmark<Special, What>
  {
    static constexpr Info<2> info = {"Latency", "Throughput"};

    template <typename T>
      static constexpr bool accept = std::has_single_bit(unsigned(size_v<T>))
				       && (size_v<T> * sizeof(value_type_t<T>) >= 16 || size_v<T> == 1);

    template <class T>
      static Times<2>
      run()
      {
#if 0
        using TT = value_type_t<T>;
	T a0 = T() + TT(2);
	T a1 = T() + TT(3);
	T a2 = T() + TT(4);
	T a3 = T() + TT(5);
	return {
	  0.25 * time_median([&] {
		   a0 = What::apply(a0);
		   a0 = What::apply(a0);
		   a0 = What::apply(a0);
		   a0 = What::apply(a0);
		   vir::fake_read(a0);
		 }),
	  0.25 * time_median([&]() {
		   vir::fake_modify(a0, a1, a2, a3);
		   T r0 = What::apply(a0);
		   T r1 = What::apply(a1);
		   T r2 = What::apply(a2);
		   T r3 = What::apply(a3);
		   vir::fake_read(r0, r1, r2, r3);
		 }),
	};
#else
	T inputs[8] = {random<T>(), random<T>(), random<T>(), random<T>(),
		       random<T>(), random<T>(), random<T>(), random<T>()};
	return {
	  time_latency(inputs, ProcessOne<What, T>()) * .5,
	  time_throughput(inputs, ProcessOne<What, T>()) * .5,
	};
#endif
      }
  };

void
bench_main()
{
  bench_all<float, Sine>();
  bench_all<float, Cosine>();
  //bench_all<double, Sine>();
  //bench_all<double, Cosine>();
}
