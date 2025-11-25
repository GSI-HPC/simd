/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

FUN2(scalbn) Scalbn;

template <int Special, class What>
  struct Benchmark<Special, What>
  {
    static constexpr Info<2> info = {"Latency", "Throughput"};

    template <class T>
      [[gnu::flatten]]
      static Times<2>
      run()
      {
        T inputs[64] = {};
        for (int i = 0; i < 64; ++i)
          inputs[i] = T() + i * .83f - 16;
        int n = 8;
        fake_modify(n);
        T& a0 = inputs[0];
        T& a1 = inputs[1];
        T& a2 = inputs[2];
        T& a3 = inputs[3];
        return {
          time_mean<5'000'000>([&]() {
            T r = What::apply(a0, n);
            fake_read(r);
            if constexpr (stdx::is_simd_v<T>)
              {
                a0 = -1.e38f;
                where(r < 1.e38f, a0) = r;
              }
            else
              a0 = r < 1.e38f ? r : -1.e38f;
          }),
          0.25 * time_mean<5'000'000>([&]() {
                   int n0 = 8;
                   int n1 = -9;
                   int n2 = 100;
                   int n3 = 2000;
                   fake_modify(n0, n1, n2, n3);
                   T r0 = What::apply(a0, n0);
                   T r1 = What::apply(a1, n1);
                   T r2 = What::apply(a2, n2);
                   T r3 = What::apply(a3, n3);
                   fake_read(r0, r1, r2, r3);
                   fake_modify(a0, a1, a2, a3);
                 })
        };
      }
  };

int
main()
{
  bench_all<float, Scalbn>();
  bench_all<double, Scalbn>();
}
