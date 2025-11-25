/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef BENCH_H_
#define BENCH_H_

#include "../include/simd"
#include "simd_benchmarking.h"
#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <complex>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdfloat>
#include <x86gprintrin.h>

namespace simd = std::simd;

template <class T>
  consteval auto
  value_type_t_impl()
  {
    if constexpr (requires { typename T::value_type; })
      return typename T::value_type();
    else if constexpr (requires(T x) { x[0]; })
      return T()[0];
    else
      return T();
  }

template <class T>
  using value_type_t = decltype(value_type_t_impl<T>());

/**
 * Alias for a vector builtin with given value type and total sizeof.
 */
template <typename T, size_t Bytes>
  requires (std::has_single_bit(Bytes))
  using vec_builtin_type_bytes [[gnu::vector_size(Bytes)]] = T;

/**
 * Constrain to any vector builtin with given value type and optional width.
 */
template <typename T, typename ValueType, int W = sizeof(T) / sizeof(ValueType)>
  concept vec_builtin_of
    = not std::is_scalar_v<T> and W >= 1 and sizeof(T) == W * sizeof(ValueType)
        and std::same_as<vec_builtin_type_bytes<ValueType, sizeof(T)>, T>
        and requires(T& v, ValueType x) { v[0] = x; };

/**
 * Constrain to any vector builtin.
 */
template <typename T>
  concept vec_builtin = not std::is_class_v<T> and vec_builtin_of<T, value_type_t<T>>;

/**
 * The width (number of value_type elements) of the given vector builtin.
 */
template <vec_builtin T>
  inline constexpr int width_of = sizeof(T) / sizeof(value_type_t<T>);

template <class T>
  inline constexpr int size_v = [] {
    if constexpr (requires { { T::size() } -> std::integral; })
      return T::size();
    else if constexpr(vec_builtin<T>)
      return width_of<T>;
    else
      return 1;
  }();

template <typename T>
  inline constexpr int speedup_size_v = [] {
    if constexpr (size_v<T> == 1)
      return 1;
    else
      {
        using Native = simd::vec<value_type_t<T>>;
        if constexpr (sizeof(T) > sizeof(Native))
          return Native::size();
        else
          return size_v<T>;
      }
  }();

template <typename T>
  struct is_character_type
  : std::bool_constant<false>
  {};

template <typename T>
  inline constexpr bool is_character_type_v = is_character_type<T>::value;

template <typename T>
  struct is_character_type<const T>
  : is_character_type<T>
  {};

template <typename T>
  struct is_character_type<T&>
  : is_character_type<T>
  {};

template <> struct is_character_type<char> : std::bool_constant<true> {};
template <> struct is_character_type<wchar_t> : std::bool_constant<true> {};
template <> struct is_character_type<char8_t> : std::bool_constant<true> {};
template <> struct is_character_type<char16_t> : std::bool_constant<true> {};
template <> struct is_character_type<char32_t> : std::bool_constant<true> {};

template <typename T, typename Abi>
std::ostream& operator<<(std::ostream& s, std::simd::basic_vec<T, Abi> const& v)
{
  using U = std::conditional_t<sizeof(T) == 1, int,
                               std::conditional_t<is_character_type_v<T>,
                                                  std::simd::_UInt<sizeof(T)>, T>>;
  s << '[' << U(v[0]);
  for (int i = 1; i < v.size(); ++i)
    s << ", " << U(v[i]);
  return s << ']';
}

template <std::size_t B, typename Abi>
std::ostream& operator<<(std::ostream& s, std::simd::basic_mask<B, Abi> const& v)
{
  s << '<';
  for (int i = 0; i < v.size(); ++i)
    s << int(v[i]);
  return s << '>';
}

template <vec_builtin V>
  std::ostream& operator<<(std::ostream& s, V v)
  { return s << simd::vec<value_type_t<V>, width_of<V>>(v); }

template <int N>
  using Info = std::array<const char*, N>;

template <int N>
  struct Times
  {
    std::array<double, N> cycles_per_call;
    int size;

    using DoubleN = double[N];

    constexpr
    Times(std::floating_point auto... init)
    : cycles_per_call{init...}, size(-1)
    { static_assert(sizeof...(init) == N); }

    constexpr
    Times(const std::array<double, N>& init, int init_size)
    : cycles_per_call(init), size(init_size)
    {}

    constexpr double
    operator[](int i) const
    { return cycles_per_call[i]; }
  };

template <int Special, class...>
  struct Benchmark
  { static_assert("The benchmark must specialize this type."); };

struct NoRef
{ static constexpr int size = -1; };

template <typename T, typename B>
  concept accept_type_for_benchmark
    = std::default_initializable<T>
        and (not requires { { auto(not B::template accept<T>) } -> std::convertible_to<bool>; }
               or B::template accept<T>);

template <class T, class B, class Ref = NoRef>
  requires (not accept_type_for_benchmark<T, B>)
  Ref
  bench_lat_thr(const char*, const Ref& ref = {})
  { return ref; }

template <class T, class B, class Ref = NoRef>
  requires accept_type_for_benchmark<T, B>
  Times<B::info.size()>
  bench_lat_thr(const char* id, const Ref& ref = {})
  {
    constexpr int N = B::info.size();
#if NOCOLOR
    static constexpr char red[] = "";
    static constexpr char green[] = "";
    static constexpr char dgreen[] = "";
    static constexpr char normal[] = "";
#else
    static constexpr char red[] = "\033[1;40;31m";
    static constexpr char green[] = "\033[1;40;32m";
    static constexpr char dgreen[] = "\033[0;40;32m";
    static constexpr char normal[] = "\033[0m";
#endif

    const Times<N> results = { B::template run<T>().cycles_per_call, size_v<T> };
    std::cout << id;
    for (int i = 0; i < N; ++i)
      {
        double speedup = 1;
        if constexpr (!std::is_same_v<Ref, NoRef>)
          speedup = ref[i] * size_v<T> / (results[i] * ref.size);

        std::cout << std::setprecision(3) << std::setw(15) << results[i];
        if (speedup_size_v<T> <= ref.size)
          {
            if (speedup >= 1.1)
              std::cout << green;
            else if (speedup > 0.995)
              std::cout << dgreen;
            else
              std::cout << red;
          }
        else if (speedup >= speedup_size_v<T> * 0.90 / ref.size && speedup >= 1.5)
          std::cout << green;
        else if (speedup > 1.1)
          std::cout << dgreen;
        else if (speedup < 0.95)
          std::cout << red;
        std::cout << std::setw(12) << speedup << normal;
      }
    std::cout << std::endl;
    if constexpr (std::same_as<Ref, NoRef>)
      return results;
    else
      return ref;
  }

template <std::size_t N>
  using cstr = char[N];

template <class B, std::size_t N>
  void
  print_header(const cstr<N> &id_name)
  {
    std::cout << id_name;
    for (unsigned i = 0; i < B::info.size(); ++i)
      std::cout << ' ' << std::setw(14) << B::info[i] << std::setw(12) << "Speedup";
    std::cout << '\n';

    char pad[N] = {};
    std::memset(pad, ' ', N - 1);
    pad[N - 1] = '\0';
    std::cout << pad;
    for (unsigned i = 0; i < B::info.size(); ++i)
      std::cout << std::setw(15) << "[cycles/call]" << std::setw(12) << "[per value]";
    std::cout << '\n';
  }

template <class T, class... ExtraFlags>
  void
  bench_all()
  {
    static_assert((std::same_as<decltype(ExtraFlags::name[0]), const char&> and ...));
    using B = Benchmark<0, ExtraFlags...>;
    constexpr std::size_t value_type_field = 6;
    constexpr std::size_t abi_field = 24;
    constexpr std::size_t type_field = value_type_field + 2 + abi_field;
    constexpr std::size_t id_size = type_field + (1 + ... + (1 + sizeof(ExtraFlags::name)));
    char id[id_size];
    std::memset(id, ' ', id_size - 1);
    id[id_size - 1] = '\0';
    std::memcpy(id + id_size/2 - 2, "TYPE", 4);
    print_header<B>(id);
    std::memcpy(id + id_size/2 - 2, "    ", 4);
    char* const typestr = id;
    char* const abistr  = id + value_type_field + 2;
    id[value_type_field] = ',';

    if constexpr (std::is_same_v<T, float>)
      std::memcpy(typestr, " float", value_type_field);
    else if constexpr (std::is_same_v<T, double>)
      std::memcpy(typestr, "double", value_type_field);
    else if constexpr (std::is_same_v<T, long double>)
      std::memcpy(typestr, "ldoubl", value_type_field);
#ifdef __STDCPP_FLOAT16_T__
    else if constexpr (std::is_same_v<T, std::float16_t>)
      std::memcpy(typestr, " flt16", value_type_field);
#endif
#ifdef __STDCPP_FLOAT32_T__
    else if constexpr (std::is_same_v<T, std::float32_t>)
      std::memcpy(typestr, " flt32", value_type_field);
#endif
#ifdef __STDCPP_FLOAT64_T__
    else if constexpr (std::is_same_v<T, std::float64_t>)
      std::memcpy(typestr, " flt64", value_type_field);
#endif
    else if constexpr (std::is_same_v<T, long long>)
      std::memcpy(typestr, " llong", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned long long>)
      std::memcpy(typestr, "ullong", value_type_field);
    else if constexpr (std::is_same_v<T, long>)
      std::memcpy(typestr, "  long", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned long>)
      std::memcpy(typestr, " ulong", value_type_field);
    else if constexpr (std::is_same_v<T, int>)
      std::memcpy(typestr, "   int", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned>)
      std::memcpy(typestr, "  uint", value_type_field);
    else if constexpr (std::is_same_v<T, short>)
      std::memcpy(typestr, " short", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned short>)
      std::memcpy(typestr, "ushort", value_type_field);
    else if constexpr (std::is_same_v<T, char>)
      std::memcpy(typestr, "  char", value_type_field);
    else if constexpr (std::is_same_v<T, signed char>)
      std::memcpy(typestr, " schar", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned char>)
      std::memcpy(typestr, " uchar", value_type_field);
    else if constexpr (std::is_same_v<T, char8_t>)
      std::memcpy(typestr, " char8", value_type_field);
    else if constexpr (std::is_same_v<T, char16_t>)
      std::memcpy(typestr, "char16", value_type_field);
    else if constexpr (std::is_same_v<T, char32_t>)
      std::memcpy(typestr, "char32", value_type_field);
    else if constexpr (std::is_same_v<T, wchar_t>)
      std::memcpy(typestr, " wchar", value_type_field);
#if VIR_NEXT_PATCH
    else if constexpr (std::is_same_v<T, std::complex<std::float16_t>>)
      std::memcpy(typestr, "cxfp16", value_type_field);
    else if constexpr (std::is_same_v<T, std::complex<std::float32_t>>)
      std::memcpy(typestr, "cxfp32", value_type_field);
    else if constexpr (std::is_same_v<T, std::complex<std::float64_t>>)
      std::memcpy(typestr, "cxfp64", value_type_field);
    else if constexpr (std::is_same_v<T, std::complex<float>>)
      std::memcpy(typestr, "cx-flt", value_type_field);
    else if constexpr (std::is_same_v<T, std::complex<double>>)
      std::memcpy(typestr, "cx-dbl", value_type_field);
#endif
    else
      std::memcpy(typestr, "??????", value_type_field);

    if constexpr (sizeof...(ExtraFlags) > 0)
      {
        char* extraflags = id + type_field + 1;
        ([&] {
          std::memcpy(extraflags, ExtraFlags::name, sizeof(ExtraFlags::name) - 1);
          extraflags += sizeof(ExtraFlags::name) + 1;
        }(), ...);
      }

    auto set_abistr = [&](const char* str) {
      std::size_t len = std::strlen(str);
      if (len > abi_field)
        {
          str += len - abi_field;
          len = abi_field;
        }
      const std::size_t ncpy = std::min(abi_field, len);
      std::memcpy(abistr, str, ncpy);
      if (len < abi_field)
        std::memset(abistr + ncpy, ' ', abi_field - ncpy);
    };

    constexpr bool can_use_gnu_reference = std::is_arithmetic_v<T>;
    auto bench_gnu_vectors = [&](const auto ref0) {
      const auto ref16 = [&] {
        if constexpr (can_use_gnu_reference)
          {
            using V16 [[gnu::vector_size(16)]] = T;
            if constexpr (alignof(V16) == sizeof(V16))
              {
                set_abistr(("[[" + std::to_string(16 / sizeof(T)) + "]]").c_str());
                return bench_lat_thr<V16, B>(id, ref0);
              }
            else
              return ref0;
          }
        else
          return ref0;
      }();
      const auto ref32 = [&] {
#ifdef __AVX__
        if constexpr (can_use_gnu_reference)
          {
            using V32 [[gnu::vector_size(32)]] = T;
            if constexpr (alignof(V32) == sizeof(V32))
              {
                set_abistr(("[[" + std::to_string(32 / sizeof(T)) + "]]").c_str());
                return bench_lat_thr<V32, B>(id, ref16);
              }
          }
#endif
        return ref16;
      }();
      return [&] {
#ifdef __AVX512F__
        if constexpr (can_use_gnu_reference)
          {
            using V64 [[gnu::vector_size(64)]] = T;
            if constexpr (alignof(V64) == sizeof(V64))
              {
                set_abistr(("[[" + std::to_string(64 / sizeof(T)) + "]]").c_str());
                return bench_lat_thr<V64, B>(id, ref32);
              }
          }
#endif
        return ref32;
      }();
    };

    set_abistr("");
    const auto ref0 = bench_lat_thr<T, B>(id);
    set_abistr("1");
    const auto ref1 = bench_lat_thr<simd::vec<T, 1>, B>(id, ref0);

    constexpr bool use_gnu_reference
      = can_use_gnu_reference and std::is_same_v<decltype(ref1), const NoRef>;
    const auto ref = [&] {
      if constexpr (use_gnu_reference)
        return bench_gnu_vectors(ref1);
      else
        return ref1;
    }();

    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ([&] {
        constexpr int N = 2 << Is;
        if constexpr (std::constructible_from<simd::vec<T, N>> and N / simd::vec<T>::size() <= 8)
          {
            set_abistr(std::to_string(N).c_str());
            bench_lat_thr<simd::vec<T, N>, B>(id, ref);
          }
      }(), ...);
    }(std::make_integer_sequence<int, 8>());

    if constexpr (requires { { B::more_types[0] } -> std::same_as<const char* const&>; })
      {
        constexpr int N = B::more_types.size();
        [&]<int... Is>(std::integer_sequence<int, Is...>) {
          ([&] {
            set_abistr(B::more_types[Is]);
            bench_lat_thr<T, Benchmark<Is + 1, ExtraFlags...>>(id, ref);
          }(), ...);
        }(std::make_integer_sequence<int, N>());
      }

    if constexpr (not use_gnu_reference)
      bench_gnu_vectors(ref);

    char sep[id_size + 2 * 15 + 2 * 12];
    std::memset(sep, '-', sizeof(sep) - 1);
    sep[sizeof(sep) - 1] = '\0';
    std::cout << sep << std::endl;
  }

template <long Iterations = 50'000, int Retries = 20, class F>
  [[gnu::noinline]]
  double
  time_mean2(F&& fun)
  {
    struct {
#if USE_MEAN
      double mean = 0;
#else
      double minimum = std::numeric_limits<double>::max();
#endif
      unsigned long tsc_start = 0;
      int todo = Retries;
      long it = 1;

      [[gnu::always_inline]]
      operator bool() &
      {
        if (--it > 0) [[likely]]
          return true;

        unsigned int tmp = 0;
        const auto tsc_end = __rdtscp(&tmp);
        if (todo < Retries) [[likely]]
          {
            const double elapsed = tsc_end - tsc_start;
            const double one_mean = elapsed / Iterations;
#if USE_MEAN
            mean += one_mean;
#else
            if (one_mean < minimum)
              {
                minimum = one_mean;
                todo = Retries;
              }
#endif
          }
        if (todo) [[likely]]
          {
            --todo;
            it = Iterations + 1;
            tsc_start = __rdtscp(&tmp);
            return true;
          }
        return false;
      }
    } collector;
    fun(collector);
#if USE_MEAN
    return collector.mean / Retries;
#else
    return collector.minimum;
#endif
  }

template <long Iterations = 50'000, int Retries = 20, class F, class... Args>
  double
  time_mean(F&& fun, Args&&... args)
  {
#if USE_MEAN
    double mean = 0;
#else
    double minimum = std::numeric_limits<double>::max();
#endif
    for (int tries = 0; tries < Retries; ++tries)
      {
        unsigned int tmp;
        long i = Iterations;
        const auto start = __rdtscp(&tmp);
        for (; i; --i)
          fun(std::forward<Args>(args)...);
        const auto end = __rdtscp(&tmp);
        const double elapsed = end - start;
#if USE_MEAN
        mean += elapsed;
#else
        if (elapsed < minimum)
          {
            minimum = elapsed;
            tries = -1;
          }
#endif
      }
#if USE_MEAN
    return mean / (Retries * Iterations);
#else
    return minimum / Iterations;
#endif
  }

template <typename T, std::size_t N>
  using carray = T[N];

template <long Iterations = 50'000, int Retries = 20, typename T, std::size_t N>
  [[gnu::noinline]]
  double
  time_latency(carray<T, N>& data, auto&& process_one)
  {
    for (auto& x : data)
      vir::fake_modify_one(x);
    double dt = 0.;
    do
      {
        auto&& d0 = data[0];
        dt = time_mean<Iterations, Retries>([=] [[gnu::always_inline]] mutable {
               d0 = process_one(std::false_type(), d0);
               vir::fake_read_one(d0);
             }) - time_mean<Iterations, Retries>([=] [[gnu::always_inline]] mutable {
                    d0 = process_one(std::true_type(), d0);
                    vir::fake_read_one(d0);
                  });
      }
    while (dt < 0.98);
    return dt;
  }

template <long Iterations = 50'000, int Retries = 20, typename T, std::size_t N>
  [[gnu::noinline]]
  double
  time_throughput(carray<T, N>& init_data, auto&& process_one)
  {
    for (auto& x : init_data)
      vir::fake_modify_one(x);

    double dt;
    do
      {
        dt = (time_mean2<Iterations, Retries>([&](auto& need_more) {
                T data[N];
                std::ranges::copy(init_data, data);
                while (need_more)
                  {
                    template for (std::size_t i : std::_IotaArray<N>)
                      data[i] = process_one(std::false_type(), data[i]);
                  }
                template for (std::size_t i : std::_IotaArray<N>)
                  vir::fake_read_one(data[i]);
              }) - time_mean2<Iterations, Retries>([&](auto& need_more) {
                     T data[N];
                     std::ranges::copy(init_data, data);
                     while (need_more)
                       {
                         template for (std::size_t i : std::_IotaArray<N>)
                           data[i] = process_one(std::true_type(), data[i]);
                       }
                     template for (std::size_t i : std::_IotaArray<N>)
                       vir::fake_read_one(data[i]);
                   }))
               / N;
      }
    while (dt < 0.2);

    return dt;
  }

template <typename T>
  [[gnu::always_inline]] inline auto
  subscript_or_value(T x, int i)
  {
    if constexpr(std::is_arithmetic_v<T>)
      return x;
    else
      return x[i];
  }

#define MAKE_VECTORMATH_OVERLOAD(name)                              \
template <vec_builtin T, class... More>                             \
  T name(T a, More... more)                                         \
  {                                                                 \
    constexpr auto [...is] = std::_IotaArray<width_of<T>>;  \
    return T {[&](int i) {                                          \
      return std::name(a[i], subscript_or_value(more, i)...);       \
    }(is)...};                                                      \
  }

#define FUN1(fun)                                                 \
MAKE_VECTORMATH_OVERLOAD(fun)                                     \
typedef struct F_##fun                                            \
{                                                                 \
  static constexpr char name[] = #fun "(x)";                      \
                                                                  \
  template <class T>                                              \
    [[gnu::always_inline]]                                        \
    static auto                                                   \
    apply(const T& x)                                             \
    {                                                             \
      using ::fun;                                                \
      using std::fun;                                             \
      return fun(x);                                              \
    }                                                             \
}

#define FUN2(fun)                                                 \
MAKE_VECTORMATH_OVERLOAD(fun)                                     \
typedef struct F_##fun                                            \
{                                                                 \
  static constexpr char name[] = #fun "(x, y)";                   \
                                                                  \
  template <class T, class U>                                     \
    [[gnu::always_inline]]                                        \
    static auto                                                   \
    apply(const T& x, const U& y)                                 \
    {                                                             \
      using ::fun;                                                \
      using std::fun;                                             \
      return fun(x, y);                                           \
    }                                                             \
}

#endif  // BENCH_H_
