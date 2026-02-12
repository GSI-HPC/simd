/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef TESTS_UNITTEST_PCH_H_
#define TESTS_UNITTEST_PCH_H_

#include "../include/bits/simd_details.h"
#include <string_view>
#include <climits>

namespace test
{
  struct precondition_failure
  {
    std::string_view file;
    int line;
    std::string_view expr;
    std::string_view msg;
  };

#undef __glibcxx_simd_precondition

#define __glibcxx_simd_precondition(expr, msg, ...) \
  do {                                              \
    if (__builtin_expect(!bool(expr), false))       \
      throw test::precondition_failure{__FILE__, __LINE__, #expr, msg}; \
  } while(false)
}

#undef _GLIBCXX_SIMD_NOEXCEPT
#define _GLIBCXX_SIMD_NOEXCEPT noexcept(false)

#include "../include/simd"

#include <source_location>
#include <print>
#include <concepts>
#include <cfenv>
#include <meta>
#include <ranges>

using run_function = void(*)();

// global objects
static std::int64_t passed_tests = 0;

static std::int64_t failed_tests = 0;

static bool first_fail = true;
// ------------------------------------------------

namespace simd = std::simd;

template <typename _Tp>
  struct canonical_vec_type
  { using type = _Tp; };

template <typename _Tp>
  using canonical_vec_type_t = typename canonical_vec_type<_Tp>::type;

template <std::same_as<long> _Tp>
  requires (sizeof(_Tp) == sizeof(int))
  struct canonical_vec_type<_Tp>
  { using type = int; };

template <std::same_as<long> _Tp>
  requires (sizeof(_Tp) == sizeof(long long))
  struct canonical_vec_type<_Tp>
  { using type = long long; };

template <std::same_as<unsigned long> _Tp>
  requires (sizeof(_Tp) == sizeof(unsigned int))
  struct canonical_vec_type<_Tp>
  { using type = unsigned int; };

template <std::same_as<unsigned long> _Tp>
  requires (sizeof(_Tp) == sizeof(unsigned long long))
  struct canonical_vec_type<_Tp>
  { using type = unsigned long long; };

/* Not yet
template <typename _Tp>
  requires std::is_enum_v<_Tp>
  struct canonical_vec_type<_Tp>
  { using type = canonical_vec_type<std::underlying_type_t<_Tp>>::type; };
 */

template <>
  struct canonical_vec_type<char>
  { using type = std::conditional_t<std::is_signed_v<char>, signed char, unsigned char>; };

template <>
  struct canonical_vec_type<char8_t>
  { using type = unsigned char; };

template <>
  struct canonical_vec_type<char16_t>
  { using type = uint_least16_t; };

template <>
  struct canonical_vec_type<char32_t>
  { using type = uint_least32_t; };

template <>
  struct canonical_vec_type<wchar_t>
  {
    using type = std::conditional_t<std::is_signed_v<wchar_t>,
				    simd::__integer_from<sizeof(wchar_t)>,
				    simd::_UInt<sizeof(wchar_t)>>;
  };

template <>
  struct canonical_vec_type<_Float64>
  { using type = double; };

template <>
  struct canonical_vec_type<_Float32>
  { using type = float; };

template <std::size_t N, typename CharT>
  struct std::formatter<std::bitset<N>, CharT>
  {
    constexpr typename std::basic_format_parse_context<CharT>::iterator
    parse(std::basic_format_parse_context<CharT>& ctx)
    { return ctx.begin(); }

    template <typename Out>
      typename std::basic_format_context<Out, CharT>::iterator
      format(const std::bitset<N>& bs, std::basic_format_context<Out, CharT>& ctx) const
      { return std::format_to(ctx.out(), "[{}]", bs.to_string()); }
  };

#if VIR_NEXT_PATCH
template <typename T>
  concept complex_like = std::simd::__complex_like<T>;

template <complex_like T, typename CharT>
  struct std::formatter<T, CharT>
  {
    constexpr typename std::basic_format_parse_context<CharT>::iterator
    parse(std::basic_format_parse_context<CharT>& ctx)
    { return ctx.begin(); }

    template <typename Out>
      typename std::basic_format_context<Out, CharT>::iterator
      format(const T& x, std::basic_format_context<Out, CharT>& ctx) const
      { return std::format_to(ctx.out(), "({}+{}i)", x.real(), x.imag()); }
  };
#endif

template <typename T>
  requires std::is_same_v<T, wchar_t>
    || std::is_same_v<T, char8_t>
    || std::is_same_v<T, char16_t>
    || std::is_same_v<T, char32_t>
  struct std::formatter<T, char>
  {
    constexpr std::basic_format_parse_context<char>::iterator
    parse(std::basic_format_parse_context<char>& ctx)
    { return f_.parse(ctx); }

    template <typename Out>
      typename std::basic_format_context<Out, char>::iterator
      format(T x, std::basic_format_context<Out, char>& ctx) const
      { return f_.format(U(x), ctx); }

  private:
    using U = std::make_unsigned_t<T>;

    std::formatter<U, char> f_ = {};
  };

struct additional_info
{
  const bool failed = false;

  template <typename... Args>
    constexpr void
    operator()(std::string_view fmt, const Args&... args)
    {
      if (failed)
	std::println("| {}", std::format(std::runtime_format(fmt), args...));
    }

  constexpr additional_info
  operator()(const auto& value)
  {
    if (failed)
      std::println("|{:>9} | {}", "", value);
    return *this;
  }
};

struct log_novalue {};

template <typename T>
  struct unwrap_value_types
  { using type = T; };

template <typename T>
  requires requires { typename T::value_type; }
  struct unwrap_value_types<T>
  { using type = typename unwrap_value_types<typename T::value_type>::type; };

template <typename T>
  using value_type_t = typename unwrap_value_types<std::remove_cvref_t<T>>::type;

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
  struct as_unsigned<std::simd::basic_vec<T, Abi>>
  { using type = std::simd::rebind_t<as_unsigned_t<T>, std::simd::basic_vec<T, Abi>>; };

template <typename T0, typename T1>
  constexpr T0
  ulp_distance_signed(T0 val0, const T1& ref1)
  {
    if constexpr (std::is_floating_point_v<T1>)
      return ulp_distance_signed(val0, std::simd::rebind_t<T1, T0>(ref1));
    else if constexpr (std::is_floating_point_v<value_type_t<T0>>)
      {
	int fp_exceptions = 0;
	if !consteval
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
	if !consteval
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

template <typename T>
  constexpr bool
  bit_equal(const T& a, const T& b)
  {
    using std::simd::_UInt;
    if constexpr (sizeof(T) <= sizeof(0ull))
      return std::bit_cast<_UInt<sizeof(T)>>(a) == std::bit_cast<_UInt<sizeof(T)>>(b);
    else if constexpr (std::simd::__simd_vec_or_mask_type<T>)
      {
	using TT = typename T::value_type;
	if constexpr (std::is_integral_v<TT>)
	  return all_of(a == b);
	else if constexpr (T::abi_type::_S_nreg > 1)
	  {
	    return bit_equal(a._M_get_low(), b._M_get_low())
		     && bit_equal(a._M_get_high(), b._M_get_high());
	  }
	else
	  {
	    // float, 4 -> unsigned, 4 (uint_size = 4)
	    // double, 4 -> ullong, 4 (uint_size = 8)
	    // complex<double>, 4 -> ullong, 8 (uint_size = 8)
	    constexpr size_t uint_size = std::min(size_t(8), sizeof(TT));
	    struct B
	    {
	      alignas(T) simd::rebind_t<_UInt<uint_size>,
					simd::resize_t<T::size() * sizeof(TT) / uint_size, T>> data;
	    };
	    if constexpr (sizeof(B) == sizeof(a))
	      return all_of(std::bit_cast<B>(a).data == std::bit_cast<B>(b).data);
	    else
	      {
		auto [a0, a1] = chunk<std::bit_ceil(unsigned(T::size())) / 2>(a);
		auto [b0, b1] = chunk<std::bit_ceil(unsigned(T::size())) / 2>(b);
		return bit_equal(a0, b0) && bit_equal(a1, b1);
	      }
	  }
      }
#if VIR_NEXT_PATCH
    else if constexpr (complex_like<T>)
      return bit_equal(a.real(), b.real()) && bit_equal(a.imag(), b.imag());
#endif
    else
      static_assert(false);
  }

#if VIR_NEXT_PATCH
// true iff real or imag parts of x are +/-inf. This matches the C23 Annex G interpretation.
template <complex_like T, typename Abi>
  constexpr typename simd::basic_vec<T, Abi>::mask_type
  my_isinf(const simd::basic_vec<T, Abi>& x)
  {
    using M = typename simd::basic_vec<T, Abi>::mask_type;
    return M(isinf(x.real()) || isinf(x.imag()));
  }
#endif

// treat as equal if either:
// - operator== yields true
// - or for floats, a and b are NaNs
// - or for complex, a and b are any infinity (see my_isinf)
// - or for complex, a and b are NaNs in real *and* imag components
template <typename V>
  constexpr bool
  equal_with_nan_and_inf_fixup(const V& a, const V& b)
  {
    auto eq = a == b;
    if (std::simd::all_of(eq))
      return true;
    else if constexpr (std::simd::__simd_vec_type<V>)
      {
	using T = typename V::value_type;
#if VIR_NEXT_PATCH
	using M = typename V::mask_type;
	if constexpr (complex_like<T>)
	  { // fix up nan == nan and (inf,nan) == (inf,?)
	    eq |= M(isnan(a.real()) && isnan(a.imag()) && isnan(b.real()) && isnan(a.imag()))
#if 0
		    || (isinf(a.real()) && isunordered(a.imag(), b.imag())
			    && a.real() == b.real())
		    || (isinf(a.imag()) && isunordered(a.real(), b.real())
			    && a.imag() == b.imag()));
#else
		  // a and b are "an infinity" according to C23 Annex G.3
		    || (my_isinf(a) && my_isinf(b));
#endif
	  }
	else
#endif
	if constexpr (std::is_floating_point_v<T>)
	  { // fix up nan == nan results
#if VIR_NEXT_PATCH
	    eq |= isnan(a) && isnan(b);
#else
	    eq |= a._M_isnan() && b._M_isnan();
#endif
	  }
	else
	  return false;
	return std::simd::all_of(eq);
      }
    else if constexpr (std::is_floating_point_v<V>)
      return std::isnan(a) && std::isnan(b);
    else
      return false;
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
  verify_precondition_failure(std::string_view expected_msg, auto&& f) &
  {
    try
      {
	f();
	okay = false;
      }
    catch (const test::precondition_failure& failure)
      {
	okay = okay && failure.msg == expected_msg;
      }
    catch (...)
      {
	okay = false;
      }
    return {};
  }

  constexpr ignore_the_rest
  verify(const auto& k) &
  {
    okay = okay && std::simd::all_of(k);
    return {};
  }

  template <typename V, typename Ref>
    constexpr ignore_the_rest
    verify_equal(const V& v, const Ref& ref) &
    {
      if constexpr (std::is_convertible_v<V, Ref> && std::is_convertible_v<Ref, V>)
	{
	  okay = okay && equal_with_nan_and_inf_fixup<V>(v, ref)
		   && equal_with_nan_and_inf_fixup<Ref>(v, ref);
	}
      else
	{
	  using Common = decltype(std::simd::select(v == ref, v, ref));
	  okay = okay && equal_with_nan_and_inf_fixup<Common>(v, ref);
	}
      return {};
    }

  template <typename V, typename Ref>
    constexpr ignore_the_rest
    verify_bit_equal(const V& v, const Ref& ref) &
    {
      if constexpr (std::is_convertible_v<V, Ref> && std::is_convertible_v<Ref, V>)
	{
	  okay = okay && bit_equal<V>(v, ref) && bit_equal<Ref>(v, ref);
	}
      else
	{
	  using Common = decltype(std::simd::select(v == ref, v, ref));
	  okay = okay && bit_equal<Common>(v, ref);
	}
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
    okay = okay && std::simd::all_of(v != ref);
    return {};
  }

  constexpr ignore_the_rest
  verify_equal_to_ulp(const auto& x, const auto& y, float allowed_distance) &
  {
    okay = okay && std::simd::all_of(ulp_distance(x, y) <= allowed_distance);
    return {};
  }

  constexpr_verifier() = default;

  constexpr_verifier(const constexpr_verifier&) = delete;

  constexpr_verifier(constexpr_verifier&&) = delete;
};

template <int... is>
  [[nodiscard]]
  consteval bool
  constexpr_test(auto&& fun, auto&&... args)
  {
    constexpr_verifier t;
    try
      {
	fun.template operator()<is...>(t, args...);
      }
    catch(const test::precondition_failure& fail)
      {
	return false;
      }
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

template <typename T>
  concept pair_specialization
    = std::same_as<std::remove_cvref_t<T>, std::pair<typename std::remove_cvref_t<T>::first_type,
						     typename std::remove_cvref_t<T>::second_type>>;

struct runtime_verifier
{
  const std::string_view test_kind;

  template <typename T>
    constexpr void
    print_value(const char* what, const T& val)
    {
      if constexpr (std::is_same_v<T, log_novalue>)
	;
      else if constexpr (std::is_floating_point_v<T>)
	std::println(std::runtime_format("|{:>9} | {:a}"), what, val);
      else if constexpr (std::is_integral_v<T>)
	std::println(std::runtime_format("|{:>9} | {:d}"), what, val);
      else if constexpr (std::ranges::range<T>)
	{
	  if constexpr (std::is_floating_point_v<std::ranges::range_value_t<T>>)
	    std::println(std::runtime_format("|{:>9} | {::a}"), what, val);
	  else if constexpr (display_string_of(^^T).contains("string"))
	    std::println(std::runtime_format("|{:>9} | {}"), what, val);
	  else if constexpr (std::is_integral_v<std::ranges::range_value_t<T>>)
	    std::println(std::runtime_format("|{:>9} | {::d}"), what, val);
	  else
	    std::println(std::runtime_format("|{:>9} | {}"), what, val);
	}
      else
	std::println(std::runtime_format("|{:>9} | {}"), what, val);
    }

  template <typename X, typename Y>
    additional_info
    log_failure(const X& x, const Y& y, std::source_location loc, std::size_t ip,
		std::string_view s)
    {
      if (first_fail)
	{
	  first_fail = false;
	  std::println("{}", " ❌ FAIL");
	}
      ++failed_tests;
      std::println(std::runtime_format("{}:{}:{}: ({:x}) in {} test: {} failed"),
		   loc.file_name(), loc.line(), loc.column(), ip, test_kind, s);
      print_value("result", x);
      print_value("expected", y);
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
  verify_precondition_failure(std::string_view expected_msg, auto&& f,
			      std::source_location loc = std::source_location::current()) &
  {
    const auto ip = determine_ip();
    try
      {
	f();
	++failed_tests;
	return log_failure(log_novalue(), log_novalue(), loc, ip,
			   "precondition failure not detected");
      }
    catch (const test::precondition_failure& failure)
      {
	if (failure.msg != expected_msg)
	  {
	    ++failed_tests;
	    return log_failure(failure.msg, expected_msg, loc, ip, "unexpected exception");
	  }
	else
	  {
	    ++passed_tests;
	    return {};
	  }
      }
    catch (...)
      {
	++failed_tests;
	return log_failure(log_novalue(), log_novalue(), loc, ip, "unexpected exception");
      }
  }

  [[gnu::always_inline]]
  additional_info
  verify(auto&& k, std::source_location loc = std::source_location::current())
  {
    const auto ip = determine_ip();
    if (std::simd::all_of(k))
      {
	++passed_tests;
	return {};
      }
    else
      return log_failure(log_novalue(), log_novalue(), loc, ip, "verify");
  }

  template <typename V, typename Ref>
    [[gnu::always_inline]]
    additional_info
    verify_equal(const V& v, const Ref& ref,
		 std::source_location loc = std::source_location::current())
    {
      const auto ip = determine_ip();
      bool ok;
      if constexpr (pair_specialization<V> && pair_specialization<Ref>)
	ok = std::simd::all_of(v.first == ref.first) && std::simd::all_of(v.second == ref.second);
      else if constexpr (std::is_convertible_v<V, Ref> && std::is_convertible_v<Ref, V>)
	ok = equal_with_nan_and_inf_fixup<V>(v, ref) && equal_with_nan_and_inf_fixup<Ref>(v, ref);
      else
	ok = equal_with_nan_and_inf_fixup<decltype(std::simd::select(v == ref, v, ref))>(v, ref);
      if (ok)
	{
	  ++passed_tests;
	  return {};
	}
      else
	return log_failure(v, ref, loc, ip, "verify_equal");
    }

  template <typename V, typename Ref>
    [[gnu::always_inline]]
    additional_info
    verify_bit_equal(const V& v, const Ref& ref,
		     std::source_location loc = std::source_location::current())
    {
      const auto ip = determine_ip();
      bool ok = false;
      if constexpr (std::is_convertible_v<V, Ref> && std::is_convertible_v<Ref, V>)
	ok = bit_equal<V>(v, ref) && bit_equal<Ref>(v, ref);
      else
	ok = bit_equal<decltype(std::simd::select(v == ref, v, ref))>(v, ref);
      if (ok)
	{
	  ++passed_tests;
	  return {};
	}
      else
	return log_failure(v, ref, loc, ip, "verify_bit_equal");
    }

  [[gnu::always_inline]]
  additional_info
  verify_not_equal(auto&& x, auto&& y,
		   std::source_location loc = std::source_location::current())
  {
    const auto ip = determine_ip();
    if (std::simd::all_of(x != y))
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
    const bool success = std::simd::all_of(ulp_distance(x, y) <= allowed_distance);
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

template <int... is>
  [[gnu::noinline, gnu::noipa]]
  void
  runtime_test(auto&& fun, auto&&... args)
  {
    runtime_verifier t {"runtime"};
    fun.template operator()<is...>(t, make_value_unknown(args)...);
  }

template <typename T>
  concept constant_value = requires {
    typename std::integral_constant<std::remove_cvref_t<decltype(T::value)>, T::value>;
  };

template <typename T>
  [[gnu::always_inline]] inline bool
  is_const_known(const T& x)
  { return constant_value<T> || __builtin_constant_p(x); }

template <typename T, typename Abi>
  [[gnu::always_inline]] inline bool
  is_const_known(const std::simd::basic_vec<T, Abi>& x)
  { return __is_const_known(x); }

template <std::size_t B, typename Abi>
  [[gnu::always_inline]] inline bool
  is_const_known(const std::simd::basic_mask<B, Abi>& x)
  { return __is_const_known(x); }

#if VIR_NEXT_PATCH
template <typename T>
  [[gnu::always_inline]] inline bool
  is_const_known(const std::complex<T>& x)
  { return is_const_known(x.real()) && is_const_known(x.imag()); }
#endif

template <std::ranges::sized_range R>
  [[gnu::always_inline]] inline bool
  is_const_known(const R& arr)
  {
    constexpr std::size_t N = std::ranges::size(arr);
    constexpr auto [...is] = std::_IotaArray<N>;
    return (is_const_known(arr[is]) && ...);
  }

template <int... is>
  [[gnu::always_inline, gnu::flatten]]
  inline void
  constprop_test(auto&& fun, auto... args)
  {
    runtime_verifier t{"constprop"};
#ifndef __clang__
    t.verify((is_const_known(args) && ...))
      ("=> The following argument(s) failed to constant-propagate:",
       (is_const_known(args) ? "" : display_string_of(^^decltype(args)))...);//, args...);
#endif
    fun.template operator()<is...>(t, args...);
  }

bool
check_cpu_support()
{
#if defined __x86_64__ || defined __i386__
    __builtin_cpu_init();
#ifdef __SSE3__
    if (!__builtin_cpu_supports("sse3")) return false;
#endif
#ifdef __SSSE3__
    if (!__builtin_cpu_supports("ssse3")) return false;
#endif
#ifdef __SSE4_1__
    if (!__builtin_cpu_supports("sse4.1")) return false;
#endif
#ifdef __SSE4_2__
    if (!__builtin_cpu_supports("sse4.2")) return false;
#endif
#ifdef __SSE4A__
    if (!__builtin_cpu_supports("sse4a")) return false;
#endif
#ifdef __XOP__
    if (!__builtin_cpu_supports("xop")) return false;
#endif
#ifdef __FMA__
    if (!__builtin_cpu_supports("fma")) return false;
#endif
#ifdef __FMA4__
    if (!__builtin_cpu_supports("fma4")) return false;
#endif
#ifdef __AVX__
    if (!__builtin_cpu_supports("avx")) return false;
#endif
#ifdef __AVX2__
    if (!__builtin_cpu_supports("avx2")) return false;
#endif
#ifdef __BMI__
    if (!__builtin_cpu_supports("bmi")) return false;
#endif
#ifdef __BMI2__
    if (!__builtin_cpu_supports("bmi2")) return false;
#endif
#if defined __LZCNT__ && !defined __clang__
    if (!__builtin_cpu_supports("lzcnt")) return false;
#endif
#ifdef __F16C__
    if (!__builtin_cpu_supports("f16c")) return false;
#endif
#ifdef __POPCNT__
    if (!__builtin_cpu_supports("popcnt")) return false;
#endif
#ifdef __AVX512F__
    if (!__builtin_cpu_supports("avx512f")) return false;
#endif
#ifdef __AVX512DQ__
    if (!__builtin_cpu_supports("avx512dq")) return false;
#endif
#ifdef __AVX512BW__
    if (!__builtin_cpu_supports("avx512bw")) return false;
#endif
#ifdef __AVX512VL__
    if (!__builtin_cpu_supports("avx512vl")) return false;
#endif
#ifdef __AVX512BITALG__
    if (!__builtin_cpu_supports("avx512bitalg")) return false;
#endif
#ifdef __AVX512VBMI__
    if (!__builtin_cpu_supports("avx512vbmi")) return false;
#endif
#ifdef __AVX512VBMI2__
    if (!__builtin_cpu_supports("avx512vbmi2")) return false;
#endif
#ifdef __AVX512IFMA__
    if (!__builtin_cpu_supports("avx512ifma")) return false;
#endif
#ifdef __AVX512CD__
    if (!__builtin_cpu_supports("avx512cd")) return false;
#endif
#ifdef __AVX512VNNI__
    if (!__builtin_cpu_supports("avx512vnni")) return false;
#endif
#ifdef __AVX512VPOPCNTDQ__
    if (!__builtin_cpu_supports("avx512vpopcntdq")) return false;
#endif
#ifdef __AVX512VP2INTERSECT__
    if (!__builtin_cpu_supports("avx512vp2intersect")) return false;
#endif
#ifdef __AVX512FP16__
    if (!__builtin_cpu_supports("avx512fp16")) return false;
#endif
#endif
    return true;
}

const int run_check_cpu_support = [] {
  if (!check_cpu_support())
    {
      (void)std::fputs("Incompatible CPU.", stderr);
      std::exit(EXIT_SUCCESS);
    }
  return 0;
}();

/**
 * The value of the largest element in test_iota<V, Init>.
 */
template <typename V, int Init = 0, int Max = V::size() + Init - 1>
  constexpr value_type_t<V> test_iota_max
    = sizeof(value_type_t<V>) < sizeof(int)
	? std::min(int(std::numeric_limits<value_type_t<V>>::max()),
		   Max < 0 ? std::min(V::size() + Init - 1,
				      int(std::numeric_limits<value_type_t<V>>::max()) + Max)
			   : Max)
	: V::size() + Init - 1;

template <typename T, typename Abi, int Init, int Max>
  requires std::is_enum_v<T>
  constexpr T test_iota_max<simd::basic_vec<T, Abi>, Init, Max>
    = static_cast<T>(test_iota_max<simd::basic_vec<std::underlying_type_t<T>, Abi>, Init, Max>);

/**
 * Starts iota sequence at Init.
 *
 * With `Max == 0`: Wrap-around on overflow
 * With `Max < 0`: Subtract from numeric_limits::max (to leave room for arithmetic ops)
 * Otherwise: [Init..Max, Init..Max, ...] (inclusive)
 *
 * Use simd::__iota if a non-monotonic sequence is a bug.
 */
template <typename V, int Init = 0, int MaxArg = int(test_iota_max<V, Init>)>
  constexpr V test_iota = V([](int i) {
	      constexpr int Max = MaxArg < 0 ? int(test_iota_max<V, Init, MaxArg>) : MaxArg;
	      static_assert(Max == 0 || Max > Init || V::size() == 1);
	      i += Init;
	      if constexpr (Max > Init)
		{
		  while (i > Max)
		    i -= Max - Init + 1;
		}
	      using T = value_type_t<V>;
#if VIR_NEXT_PATCH
	      if constexpr (std::simd::__simd_complex<V>)
		return std::complex<T>(T(i), T());
	      else
#endif
		return static_cast<T>(i);
	    });

/**
 * A data-parallel object initialized with {values..., values..., ...}
 */
template <typename V, auto... values>
  constexpr V init_vec = [] {
    using T = typename V::value_type;
    constexpr std::array<T, sizeof...(values)> arr = {T(values)...};
    return V([&](size_t i) { return arr[i % arr.size()]; });
  }();

template <typename V>
  struct Tests;

template <typename T>
  concept array_specialization
    = requires {
      typename T::value_type;
      std::tuple_size<T>::value;
    } && std::same_as<T, std::array<typename T::value_type, std::tuple_size_v<T>>>;

template <int MaxN = -1, typename Args = void, typename Fun = void>
  struct add_test
  {
    alignas(std::bit_floor(sizeof(Args))) Args args;
    Fun fun;
    static constexpr int max_n = MaxN;
  };

template <int MaxN = -1>
  struct repeat_n_times
  {
    template <typename Args = void, typename Fun = void>
      struct add_test
      {
	alignas(std::bit_floor(sizeof(Args))) Args args;
	Fun fun;
	static constexpr int max_n = MaxN;
      };
  };

struct dummy_test
{
  static constexpr std::array<int, 0> args = {};
  static constexpr auto fun = [](auto&, auto...) {};
};

template <auto test_ref, int... is, std::size_t... arg_idx>
  void
  invoke_test_impl(std::index_sequence<arg_idx...>)
  {
    first_fail = true;
    const auto before = failed_tests;
    constexpr auto fun = test_ref->fun;
    [[maybe_unused]] constexpr auto args = test_ref->args;
    constprop_test<is...>(fun, std::get<arg_idx>(args)...);
    runtime_test<is...>(fun, std::get<arg_idx>(args)...);
    constexpr bool passed = constexpr_test<is...>(fun, std::get<arg_idx>(args)...);
    if (passed)
      ++passed_tests;
    else
      {
	++failed_tests;
	if (first_fail)
	  std::println("{}", " ❌ FAIL");
	std::println("{}", "=> constexpr test failed.");
      }
    if (before == failed_tests)
      std::println("{}", " ✅ PASS");
  }

#define ADD_TEST(name, ...)                                                                        \
    template <int>                                                                                 \
      static constexpr dummy_test test_obj_##name = {};                                            \
												   \
    template <int Tmp>                                                                             \
      requires (Tmp == 0) __VA_OPT__(&& (__VA_ARGS__))                                             \
      static constexpr add_test test_obj_##name<Tmp> =

#define ADD_TEST_N(name, N, ...)                                                                   \
    template <int>                                                                                 \
      static constexpr dummy_test test_n_obj_##name = {};                                          \
												   \
    template <int Tmp>                                                                             \
      requires (Tmp == 0) __VA_OPT__(&& (__VA_ARGS__))                                             \
      static constexpr repeat_n_times<N>::add_test test_n_obj_##name<Tmp> =

template <typename T, int N = sizeof(0ll) * CHAR_BIT * 4>
  struct trivial_vector
  {
    int size_ = 0;

    T data_[N];

    consteval void
    push_back(const T& x)
    { data_[size_++] = x; }

    constexpr decltype(auto)
    operator[](this auto&& t, int i)
    { return t.data_[i]; }

    using iterator = T*;

    using const_iterator = const T*;

    constexpr decltype(auto)
    begin(this auto&& t)
    { return &t.data_[0]; }

    constexpr decltype(auto)
    end(this auto&& t)
    { return &t.data_[t.size_]; }

    [[nodiscard]]
    constexpr int
    size() const
    { return size_; }
  };

struct test_info
{
  std::meta::info obj;
  std::string_view name;
};

template <typename T>
  consteval auto
  list_test_members()
  {
    trivial_vector<test_info> r = {};
    auto ctx = std::meta::access_context::current();
    for (std::meta::info ifo : members_of(^^T, ctx))
      {
	if (is_variable_template(ifo) && has_identifier(ifo)
	      && identifier_of(ifo).starts_with("test_"))
	{
	  std::meta::info obj = substitute(ifo, {std::meta::reflect_constant(0)});
	  const int off = identifier_of(ifo).starts_with("test_n_obj_") ? 11 : 9;
	  if (is_same_type(remove_const(type_of(obj)), ^^dummy_test))
	    obj = std::meta::info();
	  r.push_back(test_info {obj, identifier_of(ifo).substr(off)});
	}
      }
    return r;
  }

template <typename V>
  void
  invoke_test_members()
  {
    using std::operator""sv;
    std::string type_name = std::views::split(display_string_of(^^V), "std::simd::"sv)
			      | std::views::join
			      | std::ranges::to<std::string>();
    constexpr auto m = list_test_members<::Tests<V>>();
    std::println("+{:-^78}+", type_name);
    template for (constexpr int i : std::_IotaArray<m.size()>)
      {
	constexpr std::string_view test_name = m[i].name;
	constexpr std::meta::info ifo = m[i].obj;
	if constexpr (ifo == std::meta::info())
	  std::println("|{:>15} |  - |{}", test_name, " 🟡 not applicable");
	else
	  {
	    constexpr auto test_ref = &[:ifo:];
	    constexpr auto args = test_ref->args;
	    using A = std::remove_const_t<decltype(args)>;
	    if constexpr (test_ref->max_n >= 0)
	      {
		static_assert(!array_specialization<A>, "this would be too expensive to compile");
		template for (constexpr int n : std::_IotaArray<test_ref->max_n>)
		  {
		    std::print("|{:>15} |{:>3} |", test_name, n);
		    invoke_test_impl<test_ref, n>(std::make_index_sequence<std::tuple_size_v<A>>());
		  }
	      }
	    else if constexpr (array_specialization<A>)
	      { // call for each element
		template for (constexpr std::size_t I : std::_IotaArray<args.size()>)
		  {
		    std::print("|{:>15} |{:>3}: {} |", test_name, I, args[I]);
		    invoke_test_impl<test_ref>(std::index_sequence<I>());
		  }
	      }
	    else
	      {
		std::print("|{:>15} |  - |", test_name);
		invoke_test_impl<test_ref>(std::make_index_sequence<std::tuple_size_v<A>>());
	      }
	  }
      }
  }

template <typename = void>
void test_runner();

int main()
{
  try
    {
      test_runner();
    }
  catch(const test::precondition_failure& fail)
    {
      std::println(std::runtime_format("{}:{}: Error: precondition '{}' does not hold: {}"),
		   fail.file, fail.line, fail.expr, fail.msg);
      return EXIT_FAILURE;
    }
  std::println(std::runtime_format("Passed tests: {}\nFailed tests: {}"),
	       passed_tests, failed_tests);
  return failed_tests != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

template <typename V, typename... Ts>
  consteval std::array<V, simd::__div_ceil(int(sizeof...(Ts)), V::size())>
  make_packed_array(Ts... values)
  {
    using T = typename V::value_type;
    const std::array<T, sizeof...(Ts)> inputs = {static_cast<T>(values)...};
    std::array<V, simd::__div_ceil(int(sizeof...(Ts)), V::size())> r = {};
    if constexpr (r.size() == 1)
      {
	constexpr int ndups = simd::__div_ceil(V::size(), int(sizeof...(Ts)));
	simd::basic_vec tmp = inputs;
	constexpr auto [...is] = std::_IotaArray<ndups>;
	r[0] = std::get<0>(simd::chunk<V>(simd::cat(((void)is, tmp)...)));
      }
    else
      {
	auto it = inputs.begin();
	for (std::size_t i = 0; i < r.size() - 1; ++i, it += V::size())
	  r[i] = simd::unchecked_load<V>(it, inputs.end());
	r.back() = simd::partial_load<V>(it, inputs.end());
      }
    return r;
  }

#endif  // TESTS_UNITTEST_PCH_H_
