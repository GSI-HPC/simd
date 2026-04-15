/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "include/simd"
#include <complex>
#include <stdfloat>

namespace simd = std::simd;

// vec.math ///////////////////////////////////////

namespace math_tests
{
  using simd::__deduced_vec_t;
  using simd::__math_floating_point;
  using std::is_same_v;

  using vf2 = simd::vec<float, 2>;
  using vf4 = simd::vec<float, 4>;

  template <typename T0, typename T1>
    concept has_common_type = requires { typename std::common_type<T0, T1>::type; };

  template <typename T>
    concept has_deduced_vec = requires { typename simd::__deduced_vec_t<T>; };

  static_assert(!has_common_type<vf2, vf4>);
#if VIR_CONSTEVAL_BROADCAST
  static_assert( has_common_type<int, vf2>);
#else
  static_assert(!has_common_type<int, vf2>);
#endif

  template <typename T, bool Strict = false>
    struct holder
    {
      T value;

      constexpr
      operator const T&() const
      { return value; }

      template <typename U>
	requires (!std::same_as<T, U>) && Strict
	operator U() const = delete;
    };

  // The next always has a common_type because the UDT is convertible_to<float> and is not an
  // arithmetic type:
  static_assert( has_common_type<holder<int>, vf2>);

  // It's up to the UDT to constrain itself better:
  static_assert(!has_common_type<holder<int, true>, vf2>);

  // However, a strict UDT can still work
  static_assert( has_common_type<holder<float, true>, vf2>);

  // Except if it needs any kind of conversion, even if it's value-preserving. Again the semantics
  // are what the UDT defined.
  static_assert(!has_common_type<holder<short, true>, vf2>);

  static_assert(!has_deduced_vec<int>);
  static_assert(!__math_floating_point<int>);
  static_assert(!__math_floating_point<float>);
  static_assert(!__math_floating_point<simd::vec<int>>);
  static_assert( __math_floating_point<simd::vec<float>>);

  template <typename... Ts>
    concept lerp_invocable = requires(Ts... xs) { simd::lerp(xs...); };

  static_assert(lerp_invocable<vf2, float, short>);

#if VIR_CONSTEVAL_BROADCAST
  static_assert( lerp_invocable<vf2, float, int>);
#else
  static_assert(!lerp_invocable<vf2, float, int>);
#endif
  static_assert([] {
    vf2 x = 0.f;
    float y = 0x2'00'00'04;
    return simd::lerp(x, y, 0.5f)[0];
  }() == float(0x1'00'00'02));

  template <typename T0, typename... T1>
    concept not_hypot_invocable
      = !requires(T1... y) { simd::hypot(T0(), y...); }
	  && !requires(T1... y) { simd::hypot(y..., T0()); };

  template <typename R, typename T0, typename... T1>
    concept hypot_invocable_r = requires(T1... y) {
      { simd::hypot(T0(), y...) } -> std::same_as<R>;
      { simd::hypot(y..., T0()) } -> std::same_as<R>;
    };

  static_assert(hypot_invocable_r<vf2, vf2, vf2>);
  static_assert(hypot_invocable_r<vf2, vf2, holder<vf2>>);
  static_assert(hypot_invocable_r<vf2, holder<vf2>, vf2>);

  static_assert(not_hypot_invocable<vf2, vf4>);
#if VIR_CONSTEVAL_BROADCAST
  static_assert(hypot_invocable_r<vf2, int, vf2>);
#else
  static_assert(not_hypot_invocable<int, vf2>);
#endif

  static_assert(hypot_invocable_r<vf2, holder<int>, vf2>);
  static_assert(not_hypot_invocable<holder<int, true>, vf2>);
  static_assert(hypot_invocable_r<vf2, holder<float, true>, vf2>);
  static_assert(not_hypot_invocable<holder<short, true>, vf2>);
  static_assert(hypot_invocable_r<vf2, float, vf2>);

  constexpr simd::vec<float, 1>
    operator""_f1(long double x)
  { return float(x); }

  constexpr simd::vec<float, 4>
    operator""_f4(long double x)
  { return float(x); }

  static_assert(simd::floor(1.1_f1)[0] == std::floor(1.1f));
  static_assert(simd::floor(simd::basic_vec(std::array{1.1f, 1.2f, 2.f, 3.f}))[0] == std::floor(1.1f));
  static_assert(simd::floor(holder {1.1_f1})[0] == std::floor(1.1f));
  static_assert(simd::hypot(1.1_f1, 1.2_f1)[0] == std::hypot(1.1f, 1.2f));
  static_assert(simd::hypot(1.1_f1, 1.2f)[0] == std::hypot(1.1f, 1.2f));
  // the next doesn't work with the P1928 spec, but it can be made to work
  static_assert(simd::hypot(simd::basic_vec(std::array{1.1f}), 1.2f)[0] == std::hypot(1.1f, 1.2f));
  static_assert(simd::hypot(1.1f, 1.2_f1)[0] == std::hypot(1.1f, 1.2f));
#if VIR_CONSTEVAL_BROADCAST
  static_assert(simd::hypot(1, 1.2_f1)[0] == std::hypot(1.f, 1.2f));
  static_assert(simd::hypot(1.2_f1, 1)[0] == std::hypot(1.f, 1.2f));
#endif
  static_assert(simd::hypot(holder {1.f}, 1.2_f1)[0] == std::hypot(1.f, 1.2f));
  // the following must not be valid. if you want vec<double> be explicit about it:
  static_assert(not_hypot_invocable<double, simd::vec<float, 1>>);
  static_assert(not_hypot_invocable<double, simd::vec<float, 1>, simd::vec<float, 1>>);

  static_assert(hypot_invocable_r<simd::vec<float, 1>, holder<float>,
				  std::constant_wrapper<2>, simd::vec<float, 1>>);
  static_assert(hypot_invocable_r<simd::vec<float, 1>, holder<short>,
				  simd::vec<float, 1>, float>);
}
