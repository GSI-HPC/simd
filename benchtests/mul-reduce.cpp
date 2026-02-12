/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"
#include <climits>
#include <functional>

namespace my
{
  using std::simd::reduce;

  template <typename T>
    requires(std::integral<T> or std::floating_point<T>)
    T
    reduce(T x, std::multiplies<>)
    { return x; }

  template <vec_builtin V>
    auto
    reduce(const V v, std::multiplies<>)
    {
      auto acc = v[0];
      for (unsigned i = 1; i < sizeof(v) / sizeof(acc); ++i)
        acc *= v[i];
      return acc;
    }
}

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr Info<2> info = {"Latency", "Throughput"};

    template <typename T>
      static constexpr bool accept = size_v<T> >= 2;

    template <class V>
      [[gnu::flatten]]
      static Times<2>
      run()
      {
        using TT = value_type_t<V>;

        auto lat_process_one = [=] [[gnu::always_inline]] (auto fake, V in) {
          V x = [] [[gnu::noipa]] (auto& tmp) { return tmp; }(in);
          if constexpr (std::convertible_to<TT, V>)
            {
              if constexpr (fake)
                x = x[0];
              else
                x = my::reduce(x, std::multiplies<>());
            }
          else
            {
              if constexpr (fake)
                x = V() + x[0];
              else
                x = V() + my::reduce(x, std::multiplies<>());
            }
          x = [] [[gnu::noipa]] (auto& tmp) { return tmp; }(x);
          return x;
        };

        auto thr_process_one = [=] [[gnu::always_inline]] (auto fake, V in) {
          V x = in;
          vir::fake_modify_one(x);
          if constexpr (std::convertible_to<TT, V>)
            {
              if constexpr (fake)
                x = x[0];
              else
                x = my::reduce(x, std::multiplies<>());
            }
          else
            {
              if constexpr (fake)
                x = V() + x[0];
              else
                x = V() + my::reduce(x, std::multiplies<>());
            }
          return x;
        };

        V a[8] = {};
        return { time_latency(a, lat_process_one),
                 time_throughput(a, thr_process_one) };
      }
  };

int
main()
{
  bench_all<signed char>();
  /*  bench_all<unsigned char>();
  bench_all<signed short>();
  bench_all<unsigned short>();
  bench_all<signed int>();
  bench_all<unsigned int>();
  bench_all<signed long>();
  bench_all<unsigned long>();
  bench_all<std::float16_t>();
  bench_all<float>();
  bench_all<double>();*/
  //bench_all<std::complex<std::float16_t>>();
  //bench_all<std::complex<float>>();
  //bench_all<std::complex<double>>();
}
