/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"
#include <climits>

namespace my
{
  using std::simd::reduce;

  template <typename T>
    requires(std::integral<T> or std::floating_point<T>)
    T
    reduce(T x)
    { return x; }

  template <vec_builtin V>
    auto
    reduce(const V v)
    {
      auto acc = v[0];
      for (unsigned i = 1; i < sizeof(v) / sizeof(acc); ++i)
        acc += v[i];
      return acc;
    }
}

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr std::array info = {
      "Latency", "Throughput"
    };

    template <typename T>
      static constexpr bool accept = size_v<T> >= 2;

    template <class T, class Stat>
      static TimeResults
      do_latency()
      {
        T a = {};
        vir::fake_modify(a);
        using my::reduce;
        return (time_generic<Stat>([=]() mutable {
                  T r = a;
                  r = broadcast<T>(reduce(vir::make_unknown(r)));
                  r = broadcast<T>(reduce(vir::make_unknown(r)));
                  r = broadcast<T>(reduce(vir::make_unknown(r)));
                  r = broadcast<T>(reduce(vir::make_unknown(r)));
                  vir::fake_read(r);
                  a = r;
                }) - time_minimum([=]() mutable {
                       T r = a;
                       r = broadcast<T>(vir::make_unknown(r)[0]);
                       r = broadcast<T>(vir::make_unknown(r)[0]);
                       r = broadcast<T>(vir::make_unknown(r)[0]);
                       r = broadcast<T>(vir::make_unknown(r)[0]);
                       vir::fake_read(r);
                       a = r;
                     })
               ) * 0.25;
      }

    template <class T, class Stat>
      static TimeResults
      do_throughput()
      {
        T a = {};
        vir::fake_modify(a);
        using my::reduce;
        return time_generic<Stat>([=]() {
                 vir::fake_read(reduce(vir::make_unknown(a)));
                 vir::fake_read(reduce(vir::make_unknown(a)));
                 vir::fake_read(reduce(vir::make_unknown(a)));
                 vir::fake_read(reduce(vir::make_unknown(a)));
                 vir::fake_read(reduce(vir::make_unknown(a)));
                 vir::fake_read(reduce(vir::make_unknown(a)));
                 vir::fake_read(reduce(vir::make_unknown(a)));
                 vir::fake_read(reduce(vir::make_unknown(a)));
               }) * 0.125;
      }

    template <class V>
      [[gnu::flatten]]
      static Times<info.size()>
      run()
      {
        return {
          do_latency<V, AccMeanLowHalf>(),
          do_throughput<V, AccMeanLowHalf>()
        };
      }
  };

void
bench_main()
{
  bench_all<signed char>();
  bench_all<unsigned char>();
  bench_all<signed short>();
  bench_all<unsigned short>();
  bench_all<signed int>();
  bench_all<unsigned int>();
  bench_all<signed long>();
  bench_all<unsigned long>();
  bench_all<std::float16_t>();
  bench_all<float>();
  bench_all<double>();
  //bench_all<std::complex<std::float16_t>>();
  //bench_all<std::complex<float>>();
  //bench_all<std::complex<double>>();
}
