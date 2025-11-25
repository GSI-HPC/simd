/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

FUN1(exp) Exp;
FUN1(expm1) Expm1;

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
          inputs[i] = T() + i * .83f - value_type_t<T>(16);
        T a0 = inputs[0];
        T a1 = inputs[1];
        T a2 = inputs[2];
        T a3 = inputs[3];
        return {
          0.25 * time_mean<5'000'000>([&] {
                   a0 = What::apply(a0);
                   a0 = What::apply(-a0);
                   a0 = What::apply(a0);
                   a0 = What::apply(-a0);
                   fake_read(a0);
                 }),
          0.25 * time_mean<5'000'000>([&] {
                   T r0 = What::apply(a0);
                   T r1 = What::apply(a1);
                   T r2 = What::apply(a2);
                   T r3 = What::apply(a3);
                   fake_read(r0, r1, r2, r3);
                   fake_modify(a0, a1, a2, a3);
                 })
        };
      }
  };

int
main()
{
  bench_all<float, Exp>();
  bench_all<double, Exp>();
  bench_all<float, Expm1>();
  bench_all<double, Expm1>();
}
