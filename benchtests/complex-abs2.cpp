/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"
#include <complex>
#include <random>

static std::mt19937 rng = std::mt19937(123);

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr std::array info = {"Sweep"};

    template <typename T>
      static constexpr T
      random_scalar(unsigned offset)
      {
	// sprinkle one subnormal and one huge number into the mix
	if (offset == 3011)
	  return std::numeric_limits<T>::min() * T(0.12);
	else if (offset == 3902)
	  return std::numeric_limits<T>::max() * T(0.93);
	static constexpr T mid = std::sqrt(std::numeric_limits<T>::max());
	unsigned r = rng() - rng.min();
	const unsigned mode = r % 4; r /= 4;
	const unsigned i = r % 1024;
	switch (mode)
	{
	  case 0: return 1/mid * (1 + i / T(17));
	  case 1: return std::numeric_limits<T>::epsilon() + i / T(1023);
	  case 2: return mid * ((1024 - i) / T(1025));
	  case 3: return mid * (i / T(1024));
	}
	std::unreachable();
      }

    template <class T>
      [[gnu::flatten]]
      static Times<info.size()>
      run()
      {
	std::array<T, 512 * 1024 / sizeof(T)> inputs;
	//constexpr int total_values = size_v<T> * inputs.size();
	using C = value_type_t<T>;
	using TT = typename value_type_t<T>::value_type;
	for (unsigned i = 0; i < inputs.size(); ++i)
	  {
	    if constexpr (simd_vec_type<T>)
	      inputs[i] = T([=](int j) { return C{random_scalar<TT>(i * T::size() + 2 * j), random_scalar<TT>(i * T::size() + 2 * j + 1)}; });
	    else
	      inputs[i] = C{random_scalar<TT>(i), random_scalar<TT>(i)};
	  }
	using std::abs;
	return {
	  time_median<20>([&] {
	    for (const T& a : inputs)
	      {
		vir::fake_read(abs(a));
	      }
	  }) / inputs.size()
	};
      }
  };


void
bench_main()
{
  //bench_all<std::complex<std::float16_t>>();
  bench_all<std::complex<float>>();
  bench_all<std::complex<double>>();
}
