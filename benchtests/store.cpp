/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2023–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

alignas(64) char mem[64 * 64] = {};

template <typename T>
  static T value = {};

template <typename T>
  inline void
  store(T& x, size_t offset = 0)
  {
    using Mem = value_type_t<T>;
    Mem* ptr = reinterpret_cast<Mem*>(mem + offset);
    vir::fake_modify(x);
    if constexpr (std::is_arithmetic_v<T>)
      ptr[0] = x;
    else if constexpr (simd::__simd_vec_type<T>)
      simd::unchecked_store(x, ptr, x.size());
    else
      std::memcpy(ptr, &x, sizeof(x));
    asm(""::"m"(*mem));
  }

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr Info<1> info = {"Throughput"};

    template <class V>
      static Times<1>
      run()
      {
        return {
          0.0625 * time_mean2<400'000>([](auto& need_more) {
                     V obj = {};
                     while (need_more)
                       {
                         template for (const int i : std::_IotaArray<16>)
                           store<V>(obj, i * sizeof(V));
                       }
                   })
        };
      }
  };

int
main()
{
  bench_all<char>();
  bench_all<short>();
  bench_all<int>();
  bench_all<long long>();
  bench_all<float>();
  bench_all<double>();
}
