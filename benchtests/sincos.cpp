/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

FUN1(sin) Sine;
FUN1(cos) Cosine;

template <int Special, class What>
  struct Benchmark<Special, What>
  {
    static constexpr Info<2> info = {"Latency", "Throughput"};

    template <class T>
      static Times<2>
      run()
      {
        T a0 = T() + 2;
        T a1 = T() + 3;
        T a2 = T() + 4;
        T a3 = T() + 5;
        return {
          0.25 * time_mean<5'000'000>([&] {
            a0 = What::apply(a0);
            a0 = What::apply(a0);
            a0 = What::apply(a0);
            a0 = What::apply(a0);
            fake_read(a0);
          }),
          0.25 * time_mean<5'000'000>([&]() {
            fake_modify(a0, a1, a2, a3);
            T r0 = What::apply(a0);
            T r1 = What::apply(a1);
            T r2 = What::apply(a2);
            T r3 = What::apply(a3);
            fake_read(r0, r1, r2, r3);
          }),
        };
      }
  };

int
main()
{
  bench_all<float, Sine>();
  bench_all<float, Cosine>();
  bench_all<double, Sine>();
  bench_all<double, Cosine>();
}
