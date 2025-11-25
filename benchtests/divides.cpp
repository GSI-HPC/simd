/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"
#include <climits>

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr Info<4> info = {"Latency", "Throughput", "ΔLatency", "ΔThroughput"};

    template <class T>
      [[gnu::flatten]]
      static Times<info.size()>
      run()
      {
        using TT = value_type_t<T>;

        constexpr TT init = [] {
          //constexpr int nbits = CHAR_BIT * sizeof(TT);
          TT init = std::numeric_limits<TT>::max();
          /*          if constexpr (std::is_integral_v<TT>)
            init <<= nbits/2;*/
          return init;
        }();

        T zerov = T();
        T b = zerov + TT(7);

        vir::fake_modify(zerov, b);

        T data[6];
        for (T& a : data)
          a = zerov + init;

        return {
          0.25 * time_mean([&] [[gnu::always_inline]] {
                   T r = data[0] / b;
                   r = data[0] / r;
                   r = data[0] / r;
                   r = data[0] / r;
                   b = r;
                 }),
          1./6. * time_mean([&] [[gnu::always_inline]] {
                    vir::fake_modify(b);
                    T r0 = data[0] / b;
                    T r1 = data[1] / b;
                    T r2 = data[2] / b;
                    T r3 = data[3] / b;
                    T r4 = data[4] / b;
                    T r5 = data[5] / b;
                    vir::fake_read(r0, r1, r2, r3, r4, r5);
                  }),
          0.25 * time_latency(data, [&] [[gnu::always_inline]] (auto fake, T a) {
                   T x = a - TT(1);
                   if constexpr (fake)
                     vir::fake_modify(x);
                   else
                     {
                       x = x / b;
                       vir::fake_read(x);
                       x = x / b;
                       vir::fake_read(x);
                       x = x / b;
                       vir::fake_read(x);
                       x = x / b;
                     }
                   return x * zerov + init;
                 }),
          time_throughput(data, [=] [[gnu::always_inline]] (auto fake, T a) {
            if constexpr (fake)
              vir::fake_modify(a);
            else
              a = a / b;
            return a;
          })
        };
      }
  };

int
main()
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
}
