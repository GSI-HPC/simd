/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

MAKE_VECTORMATH_OVERLOAD(hypot)

// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77776
template <typename T>
  constexpr T
  hypot3_exp(T x, T y, T z) noexcept
  {
    using limits = std::numeric_limits<T>;

    constexpr T
    zero = 0;

    x = std::abs(x);
    y = std::abs(y);
    z = std::abs(z);

    if (std::isinf(x) | std::isinf(y) | std::isinf(z))  [[unlikely]]
      return limits::infinity();
    if (std::isnan(x) | std::isnan(y) | std::isnan(z))	[[unlikely]]
      return limits::quiet_NaN();
    if ((x==zero) & (y==zero) & (z==zero))	[[unlikely]]
      return zero;
    if ((y==zero) & (z==zero))	[[unlikely]]
      return x;
    if ((x==zero) & (z==zero))	[[unlikely]]
      return y;
    if ((x==zero) & (y==zero))	[[unlikely]]
      return z;

    auto sort = [](T& a, T& b, T& c)	constexpr noexcept -> void
    {
      if (a > b) std::swap(a, b);
      if (b > c) std::swap(b, c);
      if (a > b) std::swap(a, b);
    };

    sort(x, y, z);	//	x <= y <= z

    int
      exp = 0;

    z = std::frexp(z, &exp);
    y = std::ldexp(y, -exp);
    x = std::ldexp(x, -exp);

    T
    sum = x*x + y*y;

    sum += z*z;
    return std::ldexp(std::sqrt(sum), exp);
  }

// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77776
template <typename T>
  constexpr T
  hypot3_scale(T x, T y, T z) noexcept
  {
    using limits = std::numeric_limits<T>;

    auto prev_power2 = [](const T value)	constexpr noexcept -> T
    {
      return std::exp2(std::floor(std::log2(value)));
    };

    constexpr T sqrtmax = std::sqrt(limits::max());
    constexpr T scale_up = prev_power2(sqrtmax);
    constexpr T scale_down = T(1) / scale_up;
    constexpr T zero = 0;

    x = std::abs(x);
    y = std::abs(y);
    z = std::abs(z);

    if (std::isinf(x) | std::isinf(y) | std::isinf(z))  [[unlikely]]
      return limits::infinity();
    if (std::isnan(x) | std::isnan(y) | std::isnan(z))	[[unlikely]]
      return limits::quiet_NaN();
    if ((x==zero) & (y==zero) & (z==zero))	[[unlikely]]
      return zero;
    if ((y==zero) & (z==zero))	[[unlikely]]
      return x;
    if ((x==zero) & (z==zero))	[[unlikely]]
      return y;
    if ((x==zero) & (y==zero))	[[unlikely]]
      return z;

    auto sort = [](T& a, T& b, T& c)	constexpr noexcept -> void
    {
      if (a > b) std::swap(a, b);
      if (b > c) std::swap(b, c);
      if (a > b) std::swap(a, b);
    };

    sort(x, y, z);	//	x <= y <= z

    const T
    scale = (z > sqrtmax) ? scale_down : (z < 1) ? scale_up : 1;

    x *= scale;
    y *= scale;
    z *= scale;

    T
    sum = x*x + y*y;

    sum += z*z;
    return std::sqrt(sum) / scale;
  }

template <typename T>
  constexpr T
  hypot3_mkretz(T x, T y, T z)
  {
    using limits = std::numeric_limits<T>;

    auto prev_power2 = [](const T value) constexpr noexcept -> T
    {
      return std::exp2(std::floor(std::log2(value)));
    };

    constexpr T sqrtmax = std::sqrt(limits::max());
    constexpr T sqrtmin = std::sqrt(limits::min());
    constexpr T scale_up = prev_power2(sqrtmax);
    constexpr T scale_down = T(1) / scale_up;
    constexpr T zero = 0;

    if (not (std::isnormal(x) && std::isnormal(y) && std::isnormal(z))) [[unlikely]]
      {
        if (std::isinf(x) | std::isinf(y) | std::isinf(z))
          return limits::infinity();
        else if (std::isnan(x) | std::isnan(y) | std::isnan(z))
          return limits::quiet_NaN();
        const bool xz = x == zero;
        const bool yz = y == zero;
        const bool zz = z == zero;
        if (xz)
          {
            if (yz)
              return zz ? zero : z;
            else if (zz)
              return y;
          }
        else if (yz && zz)
          return x;
      }

    x = std::abs(x);
    y = std::abs(y);
    z = std::abs(z);

    T a = std::max(std::max(x, y), z);
    T b = std::min(std::max(x, y), z);
    T c = std::min(x, y);

    if (a >= sqrtmin && a <= sqrtmax) [[likely]]
      return std::sqrt(__builtin_assoc_barrier(c * c + b * b) + a * a);

    const T scale = a >= sqrtmin ? scale_down : scale_up;

    a *= scale;
    b *= scale;
    c *= scale;

    return std::sqrt(__builtin_assoc_barrier(c * c + b * b) + a * a) / scale;
  }

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr std::array info = {
#if DELTA
      "ΔLatency", "ΔThroughput",
#endif
      "Latency", "Throughput", "Sweep"};

    static constexpr std::array more_types = {"hypot3_exp", "hypot3_scale", "hypot3_mkretz"};

    template <class T>
      [[gnu::always_inline]]
      static T call_hypot(T a, T b, T c)
      {
        using ::hypot;
        using std::hypot;
        if constexpr (Special == 0)
          return hypot(a, b, c);
        else if constexpr (Special == 1)
          return hypot3_exp(a, b, c);
        else if constexpr (Special == 2)
          return hypot3_scale(a, b, c);
        else if constexpr (Special == 3)
          return hypot3_mkretz(a, b, c);
      }

    template <bool Latency, class T>
      static double
      do_benchmark()
      {
        T a = T() + 0x1.fe8222p-10f;
        T b = T() + 0x1.82a4bcp-9f;
        T c = T() + 0x1.f323e6p2f;
        if constexpr (Latency)
          return time_mean<200'000>([=]() mutable {
                   T r = a;
                   r = call_hypot(vir::make_unknown(r), vir::make_unknown(b), vir::make_unknown(c));
                   r = call_hypot(vir::make_unknown(r), vir::make_unknown(b), vir::make_unknown(c));
                   r = call_hypot(vir::make_unknown(r), vir::make_unknown(b), vir::make_unknown(c));
                   r = call_hypot(vir::make_unknown(r), vir::make_unknown(b), vir::make_unknown(c));
                   vir::fake_read(r);
                   a = r;
                 }) * 0.25;
        else
          return time_mean<100'000>([=]() {
                   vir::fake_read(call_hypot(vir::make_unknown(a), vir::make_unknown(b), vir::make_unknown(c)));
                   vir::fake_read(call_hypot(vir::make_unknown(a), vir::make_unknown(b), vir::make_unknown(c)));
                   vir::fake_read(call_hypot(vir::make_unknown(a), vir::make_unknown(b), vir::make_unknown(c)));
                   vir::fake_read(call_hypot(vir::make_unknown(a), vir::make_unknown(b), vir::make_unknown(c)));
                   vir::fake_read(call_hypot(vir::make_unknown(a), vir::make_unknown(b), vir::make_unknown(c)));
                   vir::fake_read(call_hypot(vir::make_unknown(a), vir::make_unknown(b), vir::make_unknown(c)));
                   vir::fake_read(call_hypot(vir::make_unknown(a), vir::make_unknown(b), vir::make_unknown(c)));
                   vir::fake_read(call_hypot(vir::make_unknown(a), vir::make_unknown(b), vir::make_unknown(c)));
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
        inputs[1013] = std::numeric_limits<TT>::min() * TT(0.12);
        inputs[3906] = std::numeric_limits<TT>::max() * TT(0.93);
        return time_mean<1>([&] {
                 for (int i = 0; i < 4096; i += 819)
                   {
                     T a = T() + inputs[i];
                     for (int j = 0; j < 4096; j += 5)
                       {
                         T b = T() + inputs[j];
                         for (int k = 0; k < 4096; k += size_v<T>)
                           {
                             T c = load<T>(inputs, k);
                             vir::fake_read(call_hypot(a, b, c));
                           }
                       }
                   }
               }) / (819 * 5 * 4096 / size_v<T>);
      }

    template <class T>
      static Times<info.size()>
      run()
      {
#if DELTA
        T zero = T();
        T half = T() + value_type_t<T>(.5);
        vir::fake_modify(zero, half);
        auto process_one = [=] [[gnu::always_inline]] (auto fake, auto in) {
          T a = in[0] + zero;
          T b = in[1] + zero;
          T c = in[2] + zero;
          if constexpr (fake)
            {
              T x = a;
              T y = b;
              T z = c;
              vir::fake_modify(x, y, z);
              return std::array{
                a + x * zero,
                b + y * zero,
                c + z * zero
              };
            }
          else
            {
              T x = call_hypot(a, b, c);
              return std::array{
                a + x * zero,
                b + x * zero,
                c + x * zero
              };
            }
        };

        std::array<T, 3> data[8] = {};
        for (auto& d : data)
          {
            d[0] += 0x1.fe8222p-10f;
            d[1] += 0x1.82a4bcp-9f;
            d[2] += 0x1.f323e6p-2f;
          }
#endif
        const Times<info.size()> r = {
#if DELTA
          time_latency(data, process_one),
          time_throughput(data, process_one),
#endif
          do_benchmark<true, T>(),
          do_benchmark<false, T>(),
          sweep<T>()
        };
        //std::cout << data[0][0] << ' ' << data[0][1] << ' ' << data[0][2] << std::endl;
        return r;
      }
  };

int
main()
{
  bench_all<float>();
  bench_all<double>();
}
