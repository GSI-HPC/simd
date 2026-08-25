/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

FUN1(trunc) Trunc;
FUN1(floor) Floor;
FUN1(ceil) Ceil;
FUN1(round) Round;
FUN1(rint) Rint;
FUN1(nearbyint) Nearbyint;

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
    static constexpr std::array info = {
      "Latency", "Throughput",
      "Sweep"
    };

    template <typename T>
      static constexpr bool accept = std::has_single_bit(unsigned(size_v<T>))
				       && size_v<T> * sizeof(value_type_t<T>) >= 16
				       && std::is_class_v<T>;

    template <bool Latency, class T>
      static auto
      do_benchmark()
      {
	T z = T();
	z = vir::make_unknown(z);
        T a = T();
	T b = T() + 3.5f;
	b = vir::make_unknown(b);
        return time_minimum<200'000>([=] [[gnu::always_inline]] () mutable {
		 a = a * z + b;
                 if constexpr (Latency)
		   {
		     T r = a;
		     r = What::apply(vir::make_unknown(r));
		     r = What::apply(vir::make_unknown(r));
		     r = What::apply(vir::make_unknown(r));
		     r = What::apply(vir::make_unknown(r));
		     vir::fake_read(r);
		     a = r;
		   }
                 else
                   {
		     vir::fake_read(What::apply(vir::make_unknown(a)));
		     vir::fake_read(What::apply(vir::make_unknown(a)));
		     vir::fake_read(What::apply(vir::make_unknown(a)));
		     vir::fake_read(What::apply(vir::make_unknown(a)));
                   }
	       }) * 0.25;
      }

    template <class T, bool WithRound>
      static auto
      sweep()
      {
        using TT = value_type_t<T>;
	alignas(4096) std::array<TT, 4 * 4096> inputs;
	for (auto& x : inputs)
	  x = random<TT>();
        return time_minimum<50, 300>([&] [[gnu::always_inline]] {
		 T a = T();
		 for (unsigned j = 1; j < inputs.size(); j += size_v<T>)
		   {
		     a += load<T>(inputs, j);
		     if constexpr (WithRound)
		       {
			 const T r = What::apply(nop_fun(a));
			 a -= r;
			 a = nop_fun(a);
			 a += r;
		       }
		     else
		       {
			 const T z = nop_fun(a);
			 a -= z;
			 a = nop_fun(a);
			 a += z;
		       }
		     store<T>(a, inputs, j);
		   }
	       }) / (inputs.size() / size_v<T>);
      }

    template <class T>
      //[[gnu::flatten]]
      static Times<info.size()>
      run()
      {
	T inputs[8] = {random<T>(), random<T>(), random<T>(), random<T>(),
		       random<T>(), random<T>(), random<T>(), random<T>()};
	return {
	  time_latency(inputs, ProcessOne<What, T>()) * 0.5, //do_benchmark<true, T>(),
	  time_throughput(inputs, ProcessOne<What, T>()) * 0.5, //do_benchmark<false, T>(),
	  sweep<T, true>() - sweep<T, false>()
	};
      }
  };

void
bench_main()
{
  bench_all<float, Trunc    >();
  bench_all<float, Floor    >();
  bench_all<float, Ceil     >();
  bench_all<float, Round    >();
  bench_all<float, Rint     >();
  bench_all<float, Nearbyint>();
  bench_all<double, Trunc    >();
  bench_all<double, Floor    >();
  bench_all<double, Ceil     >();
  bench_all<double, Round    >();
  bench_all<double, Rint     >();
  bench_all<double, Nearbyint>();
}
