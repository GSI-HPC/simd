/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

MAKE_VECTORMATH_OVERLOAD(hypot)

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr std::array info = {
#if DELTA
      "ΔLatency", "ΔThroughput",
#endif
      "Latency", "Throughput", "Sweep"};

    template <bool Latency, class T>
      static double
      do_benchmark()
      {
        T a = T() + 0x1.fe8222p-10f;
        T b = T() + 0x1.82a4bcp-9f;
        // b = std::numeric_limits<T>::quiet_NaN();
        using ::hypot;
        using std::hypot;
        if constexpr (Latency)
          return time_mean<200'000>([=]() mutable {
                   T r = a;
                   r = hypot(vir::make_unknown(r), vir::make_unknown(b));
                   r = hypot(vir::make_unknown(r), vir::make_unknown(b));
                   r = hypot(vir::make_unknown(r), vir::make_unknown(b));
                   r = hypot(vir::make_unknown(r), vir::make_unknown(b));
                   vir::fake_read(r);
                   a = r;
                 }) * 0.25;
        else
          return time_mean<100'000>([=]() {
                   vir::fake_read(hypot(vir::make_unknown(a), vir::make_unknown(b)));
                   vir::fake_read(hypot(vir::make_unknown(a), vir::make_unknown(b)));
                   vir::fake_read(hypot(vir::make_unknown(a), vir::make_unknown(b)));
                   vir::fake_read(hypot(vir::make_unknown(a), vir::make_unknown(b)));
                   vir::fake_read(hypot(vir::make_unknown(a), vir::make_unknown(b)));
                   vir::fake_read(hypot(vir::make_unknown(a), vir::make_unknown(b)));
                   vir::fake_read(hypot(vir::make_unknown(a), vir::make_unknown(b)));
                   vir::fake_read(hypot(vir::make_unknown(a), vir::make_unknown(b)));
                 }) * 0.125;
      }

    template <class T>
      [[gnu::always_inline]]
      static inline T
      load(std::span<value_type_t<T>> mem, int offset)
      {
        if constexpr (simd::__simd_vec_type<T>)
          return simd::unchecked_load<T>(mem.subspan(offset, T::size()));
        else if constexpr (std::is_scalar_v<T>)
          return mem[offset];
        else
          {
            T r;
            std::memcpy(&r, mem.data() + offset, sizeof(r));
            return r;
          }
      }

    template <class T>
      static double
      sweep()
      {
        using TT = value_type_t<T>;
        alignas(4096) std::array<TT, 4096> inputs;
        for (int i = 0; i < 1024; ++i)
          {
            constexpr TT mid = sqrt(std::numeric_limits<TT>::max());
            inputs[i * 4 + 0] = 1/mid * (1 + i / TT(17));
            inputs[i * 4 + 1] = std::numeric_limits<TT>::epsilon() + i / TT(1023);
            inputs[i * 4 + 2] = mid * ((1024 - i) / TT(1025));
            inputs[i * 4 + 3] = mid * (i / TT(1024));
          }
        // finally, sprinkle one subnormal and one huge number into the mix
        inputs[3011] = std::numeric_limits<TT>::min() * TT(0.12);
        inputs[3903] = std::numeric_limits<TT>::max() * TT(0.93);
        using ::hypot;
        using std::hypot;
        return time_mean<1>([&] {
                 for (int i = 0; i < 4096; ++i)
                   {
                     T a = T() + inputs[i];
                     for (int j = 0; j < 4096; j += size_v<T>)
                       {
                         T b = load<T>(inputs, j);
                         vir::fake_read(hypot(a, b));
                       }
                   }
               }) / (4096 * 4096 / size_v<T>);
      }

    template <class T>
      [[gnu::flatten]]
      static Times<info.size()>
      run()
      {
#if DELTA
        T zero = T();
        vir::fake_modify(zero);
        auto process_one = [=] [[gnu::always_inline]] (auto fake, auto in) {
          T a = in[0];
          T b = in[1];
          if constexpr (fake)
            {
              vir::fake_read(a, b);
              a -= zero;
              b -= zero;
            }
          else
            {
              using ::hypot;
              using std::hypot;
              T r = hypot(a, b);
              a -= r;
              b -= r;
            }
          return std::array {a, b};
        };

        std::array<T, 2> data[8] = {};
        for (auto& d : data)
          {
            d[0] += 0x1.fe8222p-10f;
            d[1] += 0x1.82a4bcp-9f;
          }
#endif
        return {
#if DELTA
          time_latency(data, process_one),
          time_throughput(data, process_one),
#endif
          do_benchmark<true, T>(),
          do_benchmark<false, T>(),
          sweep<T>()
        };
      }
  };

int
main()
{
  bench_all<float>();
  bench_all<double>();
}
