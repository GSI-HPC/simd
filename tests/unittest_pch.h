/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef TESTS_UNITTEST_PCH_H_
#define TESTS_UNITTEST_PCH_H_

#include "../simd"
#include "../constexpr_wrapper.h"

#include <source_location>
#include <iostream>
#include <concepts>
#include <cfenv>

using run_function = void(*)();

// global objects
static std::vector<run_function> run_functions = {};

static std::int64_t passed_tests = 0;

static std::int64_t failed_tests = 0;

static std::string_view test_name = "unknown";

// ------------------------------------------------

namespace dp = std::datapar;

template <typename T>
  consteval std::basic_string_view<char>
  type_to_string(T* = nullptr)
  {
    std::string_view fun = std::source_location::current().function_name();
    const auto offset = fun.find("with T = ");
    if (offset != std::string_view::npos)
      {
        fun = fun.substr(offset + 9);
        return fun.substr(0, fun.size() - 1);
      }
    return fun;
  }

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

std::ostream& operator<<(std::ostream& s, std::byte b)
{ return s << std::hex << static_cast<unsigned>(b) << std::dec; }

template <typename T, typename Abi>
std::ostream& operator<<(std::ostream& s, std::datapar::basic_simd<T, Abi> const& v)
{
  using U = std::conditional_t<
              sizeof(T) == 1, int, std::conditional_t<
                                     is_character_type_v<T>,
                                     std::__detail::__make_unsigned_int_t<T>, T>>;
  s << '[' << U(v[0]);
  for (int i = 1; i < v.size(); ++i)
    s << ", " << U(v[i]);
  return s << ']';
}

template <std::size_t B, typename Abi>
std::ostream& operator<<(std::ostream& s, std::datapar::basic_simd_mask<B, Abi> const& v)
{
  s << '<';
  for (int i = 0; i < v.size(); ++i)
    s << int(v[i]);
  return s << '>';
}

template <std::__detail::__vec_builtin V>
  std::ostream& operator<<(std::ostream& s, V v)
  {
    return s << std::datapar::simd<std::__detail::__value_type_of<V>,
                                   std::__detail::__width_of<V>>(v);
  }

template <typename T, typename U>
  std::ostream& operator<<(std::ostream& s, const std::pair<T, U>& x)
  { return s << '{' << x.first << ", " << x.second << '}'; }

struct additional_info
{
  const bool failed = false;

  additional_info
  operator()(auto const& value0, auto const&... more)
  {
    if (failed)
      [&] {
        std::cout << "  " << value0;
        ((std::cout << ' ' << more), ...);
        std::cout << std::endl;
      }();
    return *this;
  }
};

struct log_novalue {};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

#pragma GCC diagnostic pop

template <typename T, typename R = typename T::value_type>
  R
  value_type_impl(int);

template <typename T>
  T
  value_type_impl(float);

template <typename T>
  using value_type_t = decltype(value_type_impl<T>(int()));

template <typename T>
  struct as_unsigned;

template <typename T>
  using as_unsigned_t = typename as_unsigned<T>::type;

template <typename T>
  requires (sizeof(T) == sizeof(unsigned char))
  struct as_unsigned<T>
  { using type = unsigned char; };

template <typename T>
  requires (sizeof(T) == sizeof(unsigned short))
  struct as_unsigned<T>
  { using type = unsigned short; };

template <typename T>
  requires (sizeof(T) == sizeof(unsigned int))
  struct as_unsigned<T>
  { using type = unsigned int; };

template <typename T>
  requires (sizeof(T) == sizeof(unsigned long long))
  struct as_unsigned<T>
  { using type = unsigned long long; };

template <typename T, typename Abi>
  struct as_unsigned<std::datapar::basic_simd<T, Abi>>
  { using type = std::datapar::rebind_t<as_unsigned_t<T>, std::datapar::basic_simd<T, Abi>>; };

template <typename T0, typename T1>
  constexpr T0
  ulp_distance_signed(T0 val0, const T1& ref1)
  {
    if constexpr (std::is_floating_point_v<T1>)
      return ulp_distance_signed(val0, std::datapar::rebind_t<T1, T0>(ref1));
    else if constexpr (std::is_floating_point_v<value_type_t<T0>>)
      {
        int fp_exceptions = 0;
        if not consteval
          {
            fp_exceptions = std::fetestexcept(FE_ALL_EXCEPT);
          }
        using std::isnan;
        using std::abs;
        using T = value_type_t<T0>;
        using L = std::numeric_limits<T>;
        constexpr T0 signexp_mask = -L::infinity();
        T0 ref0(ref1);
        T1 val1(val0);
        const auto subnormal = fabs(ref1) < L::min();
        using I = as_unsigned_t<T1>;
        const T1 eps1 = select(subnormal, L::denorm_min(),
                               L::epsilon() * std::bit_cast<T0>(
                                                std::bit_cast<I>(ref1)
                                                  & std::bit_cast<I>(signexp_mask)));
        const T0 ulp = select(val0 == ref0 || (isnan(val0) && isnan(ref0)),
                              T0(), T0((ref1 - val1) / eps1));
        if not consteval
          {
            std::feclearexcept(FE_ALL_EXCEPT ^ fp_exceptions);
          }
        return ulp;
      }
    else
      return ref1 - val0;
  }

template <typename T0, typename T1>
  constexpr T0
  ulp_distance(const T0& val, const T1& ref)
  {
    auto ulp = ulp_distance_signed(val, ref);
    using T = value_type_t<decltype(ulp)>;
    if constexpr (std::is_unsigned_v<T>)
      return ulp;
    else
      {
        using std::abs;
        return fabs(ulp);
      }
  }

struct constexpr_verifier
{
  struct ignore_the_rest
  {
    constexpr ignore_the_rest
    operator()(auto const&, auto const&...)
    { return *this; }
  };

  bool okay = true;

  constexpr ignore_the_rest
  verify(const auto& k) &
  {
    okay = okay and std::datapar::all_of(k);
    return {};
  }

  constexpr ignore_the_rest
  verify_equal(const auto& v, const auto& ref) &
  {
    okay = okay and std::datapar::all_of(v == ref);
    return {};
  }

  template <typename T, typename U>
    constexpr ignore_the_rest
    verify_equal(const std::pair<T, U>& x, const std::pair<T, U>& y) &
    {
      verify_equal(x.first, y.first);
      verify_equal(x.second, y.second);
      return {};
    }

  constexpr ignore_the_rest
  verify_not_equal(const auto& v, const auto& ref) &
  {
    okay = okay and std::datapar::all_of(v != ref);
    return {};
  }

  constexpr ignore_the_rest
  verify_equal_to_ulp(const auto& x, const auto& y, float allowed_distance) &
  {
    okay = okay and std::datapar::all_of(ulp_distance(x, y) <= allowed_distance);
    return {};
  }

  constexpr_verifier() = default;

  constexpr_verifier(const constexpr_verifier&) = delete;

  constexpr_verifier(constexpr_verifier&&) = delete;
};

template <auto... Init>
  [[nodiscard]]
  consteval bool
  constexpr_test(auto&& fun, auto&&... args)
  {
    constexpr_verifier t;
    if constexpr (sizeof...(Init) == 0)
      fun(t, args...);
    else
      (fun(t, Init, args...), ...);
    return t.okay;
  }

template <typename T>
  T
  make_value_unknown(const T& x)
  {
    T y = x;
    asm("" : "+m"(y));
    return y;
  }

struct runtime_verifier
{
  const std::string_view test_kind;

  template <typename X, typename Y>
    additional_info
    log_failure(const X& x, const Y& y, std::source_location loc, std::size_t ip,
                std::string_view s)
    {
      ++failed_tests;
      std::cout << loc.file_name() << ':' << loc.line() << ':' << loc.column() << ": ("
                << std::hex << ip << std::dec << ") in "
                << test_kind << " test of '" << test_name
                << "' " << s << " failed";
      if constexpr (not std::is_same_v<X, log_novalue>)
        {
          std::cout << ":\n   result: " << std::boolalpha;
          if constexpr (is_character_type_v<X>)
            std::cout << int(x);
          else
            std::cout << x;
          if constexpr (not std::is_same_v<decltype(y), const log_novalue&>)
            {
              std::cout << "\n expected: ";
              if constexpr (is_character_type_v<Y>)
                std::cout << int(y);
              else
                std::cout << y;
            }
        }
      std::cout << std::endl;
      return additional_info {true};
    }

  [[gnu::always_inline]] static inline
  size_t
  determine_ip()
  {
    size_t _ip = 0;
#ifdef __x86_64__
    asm volatile("lea 0(%%rip),%0" : "=r"(_ip));
#elif defined __i386__
    asm volatile("1: movl $1b,%0" : "=r"(_ip));
#elif defined __arm__
    asm volatile("mov %0,pc" : "=r"(_ip));
#elif defined __aarch64__
    asm volatile("adr %0,." : "=r"(_ip));
#endif
    return _ip;
  }

  [[gnu::always_inline]]
  additional_info
  verify(auto&& k, std::source_location loc = std::source_location::current())
  {
    const auto ip = determine_ip();
    if (std::datapar::all_of(k))
      {
        ++passed_tests;
        return {};
      }
    else
      return log_failure(log_novalue(), log_novalue(), loc, ip, "verify");
  }

  [[gnu::always_inline]]
  additional_info
  verify_equal(auto&& x, auto&& y,
               std::source_location loc = std::source_location::current())
    requires requires { {std::datapar::all_of(x == y)} -> std::same_as<bool>; }
  {
    const auto ip = determine_ip();
    if (std::datapar::all_of(x == y))
      {
        ++passed_tests;
        return {};
      }
    else
      return log_failure(x, y, loc, ip, "verify_equal");
  }

  template <typename T, typename U>
    [[gnu::always_inline]]
    additional_info
    verify_equal(const std::pair<T, U>& x, const std::pair<T, U>& y,
                 std::source_location loc = std::source_location::current())
    {
      const auto ip = determine_ip();
      if (std::datapar::all_of(x.first == y.first) and std::datapar::all_of(x.second == y.second))
        {
          ++passed_tests;
          return {};
        }
      else
        return log_failure(x, y, loc, ip, "verify_equal");
    }

  [[gnu::always_inline]]
  additional_info
  verify_not_equal(auto&& x, auto&& y,
                   std::source_location loc = std::source_location::current())
  {
    const auto ip = determine_ip();
    if (std::datapar::all_of(x != y))
      {
        ++passed_tests;
        return {};
      }
    else
      return log_failure(x, y, loc, ip, "verify_not_equal");
  }

  // ulp_distance_signed can raise FP exceptions and thus must be conditionally executed
  [[gnu::always_inline]]
  additional_info
  verify_equal_to_ulp(auto&& x, auto&& y, float allowed_distance,
                      std::source_location loc = std::source_location::current())
  {
    const auto ip = determine_ip();
    const bool success = std::datapar::all_of(ulp_distance(x, y) <= allowed_distance);
    if (success)
      {
        ++passed_tests;
        return {};
      }
    else
      return log_failure(x, y, loc, ip, "verify_equal_to_ulp")
               ("distance:", ulp_distance_signed(x, y),
                "\n allowed:", allowed_distance);
  }
};

[[gnu::noinline, gnu::noipa]]
void
runtime_test(auto&& fun, auto&&... args)
{
  runtime_verifier t {"runtime"};
  fun(t, make_value_unknown(args)...);
}

template <typename T>
  [[gnu::always_inline]] inline bool
  is_constprop(const T& x)
  { return vir::constexpr_value<T> or __builtin_constant_p(x); }

template <typename T, typename Abi>
  [[gnu::always_inline]] inline bool
  is_constprop(const std::datapar::basic_simd<T, Abi>& x)
  { return x._M_is_constprop(); }

template <std::size_t B, typename Abi>
  [[gnu::always_inline]] inline bool
  is_constprop(const std::datapar::basic_simd_mask<B, Abi>& x)
  { return x._M_is_constprop(); }

template <typename T, std::size_t N>
  [[gnu::always_inline]] inline bool
  is_constprop(const std::array<T, N>& arr)
  {
    return _GLIBCXX_SIMD_INT_PACK(N, Is, {
      return (is_constprop(arr[Is]) and ...);
    });
  }

[[gnu::always_inline, gnu::flatten]]
void
constprop_test(auto&& fun, auto... args)
{
  runtime_verifier t{"constprop"};
  t.verify((is_constprop(args) and ...))
    ("=> At least one argument failed to constant-propagate:", is_constprop(args)...,
     type_to_string<decltype(args)>()...);
  fun(t, args...);
}

bool
check_cpu_support()
{
#if defined __x86_64__ or defined __i386__
    __builtin_cpu_init();
#ifdef __SSE3__
    if (not __builtin_cpu_supports("sse3")) return false;
#endif
#ifdef __SSSE3__
    if (not __builtin_cpu_supports("ssse3")) return false;
#endif
#ifdef __SSE4_1__
    if (not __builtin_cpu_supports("sse4.1")) return false;
#endif
#ifdef __SSE4_2__
    if (not __builtin_cpu_supports("sse4.2")) return false;
#endif
#ifdef __SSE4A__
    if (not __builtin_cpu_supports("sse4a")) return false;
#endif
#ifdef __XOP__
    if (not __builtin_cpu_supports("xop")) return false;
#endif
#ifdef __FMA__
    if (not __builtin_cpu_supports("fma")) return false;
#endif
#ifdef __FMA4__
    if (not __builtin_cpu_supports("fma4")) return false;
#endif
#ifdef __AVX__
    if (not __builtin_cpu_supports("avx")) return false;
#endif
#ifdef __AVX2__
    if (not __builtin_cpu_supports("avx2")) return false;
#endif
#ifdef __BMI__
    if (not __builtin_cpu_supports("bmi")) return false;
#endif
#ifdef __BMI2__
    if (not __builtin_cpu_supports("bmi2")) return false;
#endif
#if defined __LZCNT__ and not defined __clang__
    if (not __builtin_cpu_supports("lzcnt")) return false;
#endif
#ifdef __F16C__
    if (not __builtin_cpu_supports("f16c")) return false;
#endif
#ifdef __POPCNT__
    if (not __builtin_cpu_supports("popcnt")) return false;
#endif
#ifdef __AVX512F__
    if (not __builtin_cpu_supports("avx512f")) return false;
#endif
#ifdef __AVX512DQ__
    if (not __builtin_cpu_supports("avx512dq")) return false;
#endif
#ifdef __AVX512BW__
    if (not __builtin_cpu_supports("avx512bw")) return false;
#endif
#ifdef __AVX512VL__
    if (not __builtin_cpu_supports("avx512vl")) return false;
#endif
#ifdef __AVX512BITALG__
    if (not __builtin_cpu_supports("avx512bitalg")) return false;
#endif
#ifdef __AVX512VBMI__
    if (not __builtin_cpu_supports("avx512vbmi")) return false;
#endif
#ifdef __AVX512VBMI2__
    if (not __builtin_cpu_supports("avx512vbmi2")) return false;
#endif
#ifdef __AVX512IFMA__
    if (not __builtin_cpu_supports("avx512ifma")) return false;
#endif
#ifdef __AVX512CD__
    if (not __builtin_cpu_supports("avx512cd")) return false;
#endif
#ifdef __AVX512VNNI__
    if (not __builtin_cpu_supports("avx512vnni")) return false;
#endif
#ifdef __AVX512VPOPCNTDQ__
    if (not __builtin_cpu_supports("avx512vpopcntdq")) return false;
#endif
#ifdef __AVX512VP2INTERSECT__
    if (not __builtin_cpu_supports("avx512vp2intersect")) return false;
#endif
#ifdef __AVX512FP16__
    if (not __builtin_cpu_supports("avx512fp16")) return false;
#endif
#endif
    return true;
}

int run_check_cpu_support = [] {
  if (not check_cpu_support())
    {
      std::cerr << "Incompatible CPU.\n";
      std::exit(EXIT_SUCCESS);
    }
  return 0;
}();

/**
 * The value of the largest element in test_iota<V, Init>.
 */
template <typename V, int Init = 0, int Max = V::size() + Init - 1>
  constexpr typename V::value_type test_iota_max
    = std::min(Max, sizeof(typename V::value_type) < sizeof(int)
                 ? std::min(V::size() + Init - 1,
                            int(std::numeric_limits<typename V::value_type>::max()))
                 : V::size() + Init - 1);

/**
 * Starts iota sequence at Init.
 *
 * With `Max <= 0`: Wrap-around on overflow
 * Otherwise: [Init..Max, Init..Max, ...] (inclusive)
 *
 * Use dp::iota if a non-monotonic sequence is a bug.
 */
template <typename V, int Init = 0, int Max = int(test_iota_max<V, Init>)>
  constexpr V test_iota = V([](int i) {
              i += Init;
              static_assert(Max <= 0 or Max > Init or V::size() == 1);
              if constexpr (Max > Init)
                {
                  while (i > Max)
                    i -= Max - Init + 1;
                }
              return static_cast<typename V::value_type>(i);
            });

/**
 * A data-parallel object initialized with {values..., values..., ...}
 */
template <typename V, auto... values>
  constexpr V vec = [] {
    using T = typename V::value_type;
    constexpr std::array<T, sizeof...(values)> arr = {values...};
    return V([&](size_t i) { return arr[i % arr.size()]; });
  }();

template <typename V>
  struct Tests;

template <typename T>
  concept array_specialization
    = requires {
      typename T::value_type;
      std::tuple_size<T>::value;
    } and std::same_as<T, std::array<typename T::value_type, std::tuple_size_v<T>>>;

template <typename Args = void, typename Fun = void>
  struct add_test
  {
    alignas(std::bit_floor(sizeof(Args))) Args args;
    Fun fun;
  };

struct dummy_test
{
  static constexpr std::array<int, 0> args = {};
  static constexpr auto fun = [](auto&) {};
};

template <auto test_ref>
  constexpr void
  invoke_test(std::string_view name, auto... is)
  {
    test_name = name;
    static constexpr auto t = *test_ref;
    [[maybe_unused]] static constexpr auto args = t.args;
    [[maybe_unused]] static constexpr auto fun = t.fun;
    using A = std::remove_const_t<decltype(args)>;
    if constexpr (array_specialization<A>)
      { /* call for each element */
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
          ([&] {
            std::string tmp_name = std::string(name) + std::to_string(Is);
            test_name = tmp_name;
            ((std::cout << "Testing '" << test_name) << ... << (' ' + std::to_string(is)))
              << ' ' << args[Is] << "'\n";
            constexpr bool passed
              = constexpr_test(fun, is..., args[Is]);
            if (passed)
              ++passed_tests;
            else
              {
                ++failed_tests;
                std::cout << "=> constexpr test of '" << test_name << "' failed.\n";
              }
            constprop_test(fun, is..., args[Is]);
            runtime_test(fun, is..., args[Is]);
          }(), ...);
        }(std::make_index_sequence<std::tuple_size_v<A>>());
      }
    else
      {
        ((std::cout << "Testing '" << test_name) << ... << (' ' + std::to_string(is))) << "'\n";
        constexpr bool passed
          = std::apply([&](auto... xs) -> bool {
              return constexpr_test(fun, is..., xs...);
            }, args);
        if (passed)
          ++passed_tests;
        else
          {
            ++failed_tests;
            std::cout << "=> constexpr test of '" << test_name << "' failed.\n";
          }
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
          constprop_test(fun, is..., std::get<Is>(args)...);
          runtime_test(fun, is..., std::get<Is>(args)...);
        }(std::make_index_sequence<std::tuple_size_v<A>>());
      }
  }

#define ADD_TEST(name, ...)                                                                        \
    template <int>                                                                                 \
      static constexpr auto name##_tmpl = dummy_test {};                                           \
                                                                                                   \
    static constexpr void                                                                          \
    name()                                                                                         \
    { invoke_test<&name##_tmpl<0>>(#name); }                                                       \
                                                                                                   \
    const int init_##name = [] {                                                                   \
      run_functions.push_back(name);                                                               \
      return 0;                                                                                    \
    }();                                                                                           \
                                                                                                   \
    template <int Tmp>                                                                             \
      requires (Tmp == 0) __VA_OPT__(and (__VA_ARGS__))                                            \
      static constexpr auto name##_tmpl<Tmp> = add_test

#define ADD_TEST_N(name, N, ...)                                                                   \
    template <int>                                                                                 \
      static constexpr auto name##_tmpl = dummy_test {};                                           \
                                                                                                   \
    static constexpr void                                                                          \
    name()                                                                                         \
    {                                                                                              \
      []<int... Is>(std::integer_sequence<int, Is...>) {                                           \
        (invoke_test<&name##_tmpl<0>>(#name, vir::cw<Is>), ...);                                   \
      }(std::make_integer_sequence<int, N>());                                                     \
    }                                                                                              \
                                                                                                   \
    const int init_##name = [] {                                                                   \
      run_functions.push_back(name);                                                               \
      return 0;                                                                                    \
    }();                                                                                           \
                                                                                                   \
    template <int Tmp>                                                                             \
      requires (Tmp == 0) __VA_OPT__(and (__VA_ARGS__))                                            \
      static constexpr auto name##_tmpl<Tmp> = add_test

template <typename = void>
void test_runner();

int main()
{
  test_runner();
  std::cout << "Passed tests: " << passed_tests << "\nFailed tests: " << failed_tests << '\n';
  return failed_tests != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

#endif  // TESTS_UNITTEST_PCH_H_
