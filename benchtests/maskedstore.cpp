/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"
#include <random>
#include <bit>
#include <climits>

alignas(64) char mem[64 * 64] = {};

template <typename T>
  static T value = {};

template <typename T>
  inline void
  store(T& x, typename T::mask_type k, size_t offset = 0)
  {
    using Mem = value_type_t<T>;
    Mem* ptr = reinterpret_cast<Mem*>(mem + offset);
    asm("":"+m,g,v,x"(x), "+g,v,x,m"(k));
    simd::unchecked_store(x, ptr, x.size(), k);
    asm(""::"m"(*mem));
  }

std::random_device rd;
std::mt19937 gen(rd());

constexpr auto aligned = std::simd::flag_aligned;

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr Info<4> info
      = {"Random Mask", "Epilogue Mask", "Transform", "read-modify-write"};

    template <typename T>
      static constexpr bool accept = std::default_initializable<T>
                                       and requires { typename T::abi_type; }
                                       and size_v<T> <= simd::vec<value_type_t<T>>::size();

    template <class V>
      static Times<4>
      run()
      {
        using T = value_type_t<V>;
        using U = simd::_UInt<sizeof(T)>;
        using UV = simd::rebind_t<U, V>;
        constexpr unsigned nbits = sizeof(U) * CHAR_BIT;
        static_assert(std::has_single_bit(nbits));

        //constexpr int N = 1024 / sizeof(T);

        //constexpr int NRandom = 64 * 64;

        constexpr auto mem_alignment = simd::alignment_v<V>;

        alignas(mem_alignment) std::array<T, 1024 + V::size()> masks = {};
        std::uniform_int_distribution<unsigned> dist_unsigned;
        for (auto& b : masks)
          b = (dist_unsigned(gen) & 0x100) == 0 ? T(1) : T(0);

        alignas(mem_alignment) std::array<T, 1024 + V::size()> input = {};
        alignas(mem_alignment) std::array<T, 1024 + V::size()> output = {};

        std::conditional_t<
          std::is_floating_point_v<T>, std::uniform_real_distribution<T>,
          std::uniform_int_distribution<std::conditional_t<sizeof(T) == 1, short, T>>> dist;

        for (auto& x : input)
          x = dist(gen);

        return {
          time_mean2<800'000>([&](auto& need_more)
          {
            V obj = {};
            while (need_more)
              {
                asm ("":"+m"(masks));
                auto&& make_mask = [&] [[gnu::always_inline]] -> typename V::mask_type {
                  size_t i = (need_more.it * V::size()) % 1024;
                  return simd::unchecked_load<V>(masks.begin() + i, masks.end()) == T();
                };
                store<V>(obj, make_mask(), 0 * sizeof(V));
              }
          }),
          time_mean2<800'000>([&](auto& need_more)
          {
            V obj = {};
            UV iota([](U i) { return i; });
            vir::fake_modify(iota);
            while (need_more)
              {
                auto&& make_mask = [&] [[gnu::always_inline]] -> typename V::mask_type {
                  return iota < U(need_more.it % V::size());
                };
                store<V>(obj, make_mask(), 0 * sizeof(V));
              }
          }),
          time_mean2<1'000'000>([&](auto& need_more)
          {
            while (need_more)
              {
                asm ("":"+m"(output), "+m"(input));
                size_t i = (need_more.it * V::size()) % 1024;
                V obj = simd::unchecked_load<V>(input.begin() + i, input.end(), aligned);
                if constexpr (std::is_floating_point_v<T>)
                  simd::unchecked_store(obj, output.begin() + i, output.end(), obj > T(0.5), aligned);
                else if constexpr (std::is_signed_v<T>)
                  simd::unchecked_store(obj, output.begin() + i, output.end(), obj >= T(), aligned);
                else
                  simd::unchecked_store(obj, output.begin() + i, output.end(), obj > T(std::numeric_limits<T>::max() / 2), aligned);
              }
          }),
          time_mean2<1'000'000>([&](auto& need_more)
          {
            while (need_more)
              {
                asm ("":"+m"(output), "+m"(input));
                size_t i = (need_more.it * V::size()) % 1024;
                V obj = simd::unchecked_load<V>(input.begin() + i, input.end(), aligned);
                V out = simd::unchecked_load<V>(output.begin() + i, output.end(), aligned);
                if constexpr (std::is_floating_point_v<T>)
                  out = simd::select(obj > T(0.5), obj, out);
                else if constexpr (std::is_signed_v<T>)
                  out = simd::select(obj >= T(), obj, out);
                else
                  out = simd::select(obj > T(std::numeric_limits<T>::max() / 2), obj, out);
                simd::unchecked_store(out, output.begin() + i, output.end(), aligned);
              }
          })
        };
      }
  };

//static_assert(accept_type_for_benchmark<simd::vec<float, 1>, Benchmark<0>>);

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
