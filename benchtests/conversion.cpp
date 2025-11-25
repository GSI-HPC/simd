/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#define USE_MEAN 1
#include "bench.h"

template <int Special, class What>
  struct Benchmark<Special, What>
  {
    using To = typename What::type;

    static constexpr Info<2> info = {"Latency", "Throughput"};

    template <typename T>
      static constexpr bool accept = sizeof(To) * size_v<T> <= sizeof(simd::vec<To>)
                                       and not std::is_same_v<To, value_type_t<T>>;

    template <class T, class From>
      [[gnu::always_inline]]
      static auto
      cvt(From in)
      {
          if constexpr (std::is_scalar_v<From>)
            return static_cast<T>(in);
          else if constexpr (vec_builtin<From>)
            {
              using ToV [[gnu::vector_size(sizeof(T) * width_of<From>)]] = T;
              return __builtin_convertvector(in, ToV);
            }
          else
            return static_cast<simd::rebind_t<T, From>>(in);
      }

    template <class ToV, class From>
      [[gnu::always_inline]]
      static ToV
      fake_cvt(const From in)
      {
        ToV r {};
        constexpr auto bytes = std::min(sizeof(From), sizeof(ToV));
        std::memcpy(&r, &in, bytes);
        return r;
      }

    template <class From>
      static Times<info.size()>
      run()
      {
        using ToV = decltype(cvt<To>(std::declval<From>()));
        ToV zero = {};
        ToV one = zero + To(1);
        vir::fake_modify(zero, one);

        auto process_one = [&] [[gnu::always_inline]] (auto fake, From in) {
          ToV cvted = zero;
          if (fake)
            cvted = fake_cvt<ToV>(in);
          else
            cvted = cvt<To>(in);
          //vir::fake_modify(cvted);
          return fake_cvt<From>(cvted);
        };

        From inputs[8] = {};
        return { time_latency(inputs, process_one),
                 time_throughput(inputs, process_one) };
      }
  };

struct Int8
{
  using type = signed char;
  static constexpr char name[] = "  int8";
};

struct UInt8
{
  using type = unsigned char;
  static constexpr char name[] = " uint8";
};

struct Int16
{
  using type = short;
  static constexpr char name[] = " int16";
};

struct UInt16
{
  using type = unsigned short;
  static constexpr char name[] = "uint16";
};

struct Int32
{
  using type = int;
  static constexpr char name[] = " int32";
};

struct UInt32
{
  using type = unsigned int;
  static constexpr char name[] = "uint32";
};

struct Int64
{
  using type = std::int64_t;
  static constexpr char name[] = " int64";
};

struct UInt64
{
  using type = std::uint64_t;
  static constexpr char name[] = "uint64";
};

struct Float
{
  using type = float;
  static constexpr char name[] = " float";
};

struct Double
{
  using type = double;
  static constexpr char name[] = "double";
};

int
main()
{
  bench_all<double, Double>();
  bench_all<double,  Float>();
  bench_all<double,  Int64>();
  bench_all<double, UInt64>();
  bench_all<double,  Int32>();
  bench_all<double, UInt32>();
  bench_all<double,  Int16>();
  bench_all<double, UInt16>();
  bench_all<double,   Int8>();
  bench_all<double,  UInt8>();

  bench_all<float, Double>();
  bench_all<float,  Float>();
  bench_all<float,  Int64>();
  bench_all<float, UInt64>();
  bench_all<float,  Int32>();
  bench_all<float, UInt32>();
  bench_all<float,  Int16>();
  bench_all<float, UInt16>();
  bench_all<float,   Int8>();
  bench_all<float,  UInt8>();

  bench_all<int, Double>();
  bench_all<int,  Float>();
  bench_all<int,  Int64>();
  bench_all<int, UInt64>();
  bench_all<int,  Int32>();
  bench_all<int, UInt32>();
  bench_all<int,  Int16>();
  bench_all<int, UInt16>();
  bench_all<int,   Int8>();
  bench_all<int,  UInt8>();
}
