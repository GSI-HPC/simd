/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

struct VecInit
{
  template <class T>
    static constexpr T
    rhs_init()
    {
      if constexpr (vec_builtin<T>)
        return []<int... Is>(std::integer_sequence<int, Is...>) {
          constexpr T r{value_type_t<T>((Is % 31) + 1)...};
          return r;
        }(std::make_integer_sequence<int, size_v<T>>());
      else if constexpr (requires { typename T::abi_type; })
        {
#if USE_STD_SIMD
          constexpr
#endif
          T r([](int i) -> T::value_type { return (i % 31) + 1; });
          return r;
        }
      else
        return 6;
    }
};

struct IntInit
{
  template <class>
    static constexpr int
    rhs_init()
    { return 6; }
};

template <bool X>
  struct Left
  { static constexpr bool left = X; };

template <bool X>
  struct Const
  { static constexpr bool rhs_const = X; };

struct LeftVec
: Left<true>, Const<false>, VecInit
{ static constexpr char name[9] = "`x << v`"; };

struct LeftInt
: Left<true>, Const<false>, IntInit
{ static constexpr char name[9] = "`x << i`"; };

struct LeftVecConst
: Left<true>, Const<true>, VecInit
{ static constexpr char name[15] = "`x << const v`"; };

struct LeftIntConst
: Left<true>, Const<true>, IntInit
{ static constexpr char name[9] = "`x << 6`"; };

struct RightVec
: Left<false>, Const<false>, VecInit
{ static constexpr char name[9] = "`x >> v`"; };

struct RightInt
: Left<false>, Const<false>, IntInit
{ static constexpr char name[9] = "`x >> i`"; };

struct RightVecConst
: Left<false>, Const<true>, VecInit
{ static constexpr char name[15] = "`x >> const v`"; };

struct RightIntConst
: Left<false>, Const<true>, IntInit
{ static constexpr char name[9] = "`x >> 6`"; };

template <int Special, class What>
  struct Benchmark<Special, What>
  {
    static constexpr Info<2> info = {"Latency", "Throughput"};

    template <bool Latency, class T>
      [[gnu::noinline]]
      static double
      do_benchmark()
      {
        using value_type = value_type_t<T>;
        T a_init = T() + value_type(23);
        if constexpr (Latency)
          return 0.25 * time_mean2<4'000'000, 10>([&](auto& need_more)
          {
            auto a = a_init;
            auto b = What::template rhs_init<T>();
            while (need_more)
              {
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                a = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                a = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                a = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                a = What::left ? a << b : a >> b;
              }
            fake_read(a);
          });
        else
          return 0.125 * time_mean2<3'000'000, 10>([&](auto& need_more)
          {
            while (need_more)
              {
                auto a = a_init;
                auto b = What::template rhs_init<T>();
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r0 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r1 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r2 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r3 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r4 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r5 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r6 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r7 = What::left ? a << b : a >> b;
                fake_read(r0, r1, r2, r3, r4, r5, r6, r7);
              }
          });
      }

    template <class T>
      [[gnu::flatten]]
      static Times<2>
      run()
      { return {do_benchmark<true, T>(), do_benchmark<false, T>()}; }
  };

template <class T>
  void
  all_shifts()
  {
    bench_all<T, LeftVec>();
    bench_all<T, LeftVecConst>();
    bench_all<T, LeftInt>();
    bench_all<T, LeftIntConst>();
    bench_all<T, RightVec>();
    bench_all<T, RightVecConst>();
    bench_all<T, RightInt>();
    bench_all<T, RightIntConst>();
  }

template <class What>
  void
  all_integers()
  {
    bench_all<signed char, What>();
    bench_all<unsigned char, What>();
    bench_all<signed short, What>();
    bench_all<unsigned short, What>();
    bench_all<signed int, What>();
    bench_all<unsigned int, What>();
    bench_all<signed long, What>();
    bench_all<unsigned long, What>();
  }

int
main()
{
#if defined ONLY
  bench_all<ONLY>();
#else
  all_integers<LeftVec>();
  all_integers<LeftVecConst>();
  all_integers<LeftInt>();
  all_integers<LeftIntConst>();
  all_integers<RightVec>();
  all_integers<RightVecConst>();
  all_integers<RightInt>();
  all_integers<RightIntConst>();
#endif
}
