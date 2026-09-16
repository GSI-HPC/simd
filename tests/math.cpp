/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2024–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */
// requires float

#include "unittest.h"

#define MAKE_ADRESSABLE(fun)                                                                       \
  static constexpr struct fun##Wrap                                                                \
  {                                                                                                \
    static constexpr auto                                                                          \
    operator()(const auto& x) -> decltype(std::fun(x))                                             \
    { return std::fun(x); }                                                                        \
                                                                                                   \
    static constexpr auto                                                                          \
    operator()(const auto& x, const auto& y) -> decltype(std::fun(x, y))                           \
    { return std::fun(x, y); }                                                                     \
  } fun##Obj

MAKE_ADRESSABLE(fabs);
MAKE_ADRESSABLE(trunc);
MAKE_ADRESSABLE(ceil);
MAKE_ADRESSABLE(floor);
MAKE_ADRESSABLE(nearbyint);
MAKE_ADRESSABLE(rint);
MAKE_ADRESSABLE(nextafter);

#if VIR_PATCH_MATH
template <typename V>
  struct Tests
  {
    using T = typename V::value_type;
    using M = typename V::mask_type;

    static_assert(std::floating_point<T>);

    using L = std::numeric_limits<T>;

    static constexpr T min = L::lowest();
    static constexpr T norm_min = L::min();
    static constexpr T max = L::max();
#if !_GLIBCXX_FAST_MATH
    static constexpr T denorm_min = L::denorm_min();
#endif
#if !__FINITE_MATH_ONLY__
    static constexpr T inf = L::infinity();
    static constexpr T nan = L::quiet_NaN();
#endif
    static constexpr T zero = 0;

    static constexpr T after_one = 1 + L::epsilon();
    static constexpr T before_one = (2 - L::epsilon()) / 2;

    ADD_TEST(Roundings) {
      make_packed_array<V>(+0.,
#if !__NO_SIGNED_ZEROS__
			   -0.,
#endif
			   0.5, -0.5, 1, 1.5, -1.5, 2, 2.5, -2.5, 3, -3, 9, -9,
			   before_one, -before_one, after_one, -after_one,
			   2 * before_one, -2 * before_one, 2 * after_one, -2 * after_one,
#if !__FINITE_MATH_ONLY__
			   inf, -inf, nan,
#endif
#if !_GLIBCXX_FAST_MATH
			   denorm_min, norm_min / 3,
#endif
			   norm_min, max,
			   0x1.fffffffffffffp52, -0x1.fffffffffffffp52,
			   0x1.ffffffffffffep52, -0x1.ffffffffffffep52,
			   0x1.ffffffffffffdp52, -0x1.ffffffffffffdp52,
			   0x1.fffffep21, -0x1.fffffep21,
			   0x1.fffffcp21, -0x1.fffffcp21,
			   0x1.fffffep22, -0x1.fffffep22,
			   0x1.fffffcp22, -0x1.fffffcp22,
			   0x1.fffffep23, -0x1.fffffep23,
			   0x1.fffffcp23, -0x1.fffffcp23,
			   0x1.8p23, -0x1.8p23),
      [](auto& t, V x) {
	t.verify_equal_fun(x, fabsObj);
	t.verify_equal(copysign(x, x), x);
	t.verify_equal(copysign(-x, x), x);
	t.verify_equal(copysign(fabs(x), x), x);
	t.verify_equal_fun(x, truncObj);
	t.verify_equal_fun(x, ceilObj);
	t.verify_equal_fun(x, floorObj);
	if !consteval
	{
	  t.verify_equal_fun(x, nearbyintObj);
	}
	t.verify_equal(nextafter(x, x), x);
	if consteval
	  {
	    // constexpr nextafter is ill-formed when returning subnormal or zero
	    x = select(fabs(x) <= norm_min, before_one, x);
	  }
	t.verify_equal_fun(x,    V(), nextafterObj);
	t.verify_equal_fun(x, V(max), nextafterObj);
	t.verify_equal_fun(x, V(min), nextafterObj);
      }
    };

    static constexpr auto round_special_value = make_math_test {
      std::array {T(+0.),
#if !__NO_SIGNED_ZEROS__
		  T(-0.),
#endif
		  T(0.45), T(0.5), T(0.55),
		  T(-0.45), T(-0.5), T(-0.55),
		  T(1.45), T(1.5), T(1.55),
		  T(-1.45), T(-1.5), T(-1.55),
		  T(2), T(2.5), T(-2.5),
		  T(3), T(-3), T(9), T(-9),
		  before_one, -before_one, after_one, -after_one,
		  2 * before_one, -2 * before_one, 2 * after_one, -2 * after_one,
#if !__FINITE_MATH_ONLY__
		  inf, -inf, nan,
#endif
#if !_GLIBCXX_FAST_MATH
		  denorm_min, norm_min / 3,
#endif
		  norm_min, max,
		  T(0x1.fffffffffffffp52), T(-0x1.fffffffffffffp52),
		  T(0x1.ffffffffffffep52), T(-0x1.ffffffffffffep52),
		  T(0x1.ffffffffffffdp52), T(-0x1.ffffffffffffdp52),
		  T(0x1.fffffep21), T(-0x1.fffffep21),
		  T(0x1.fffffcp21), T(-0x1.fffffcp21),
		  T(0x1.fffffep22), T(-0x1.fffffep22),
		  T(0x1.fffffcp22), T(-0x1.fffffcp22),
		  T(0x1.fffffep23), T(-0x1.fffffep23),
		  T(0x1.fffffcp23), T(-0x1.fffffcp23),
		  T(0x1.8p23), T(-0x1.8p23)},
      10000,
      [](const auto& x) { return std::round(x); },
      require_exact_fpexcept
    };

    ADD_TEST(hypot) {
      std::tuple {(test_iota<V> + std::cw<21>) / std::cw<3>},
      [](auto& t, V x) {
	t.verify_equal_to_ulp(hypot(x, x),
			      V([&](int i) -> T {
				return std::hypot(x[i], x[i]);
			      }), std::cw<1>)(
	  "input: {}", x);
      }
    };

    ADD_TEST(cos) {
      std::tuple {(test_iota<V> + std::cw<21>) / std::cw<3>},
      [](auto& t, V x) {
	t.verify_equal_to_ulp(cos(x), V([&](int i) -> T { return std::cos(x[i]); }), std::cw<1>)
	  ("input: {}", x);
      }
    };

    static constexpr auto cos_special_values = make_math_test {
      std::array{
#ifdef __STDC_IEC_559__
	nan, nan, inf, -inf, -zero, denorm_min, norm_min / 3,
#endif
	zero, norm_min, T(1), T(2), max / 5, max / 3, max / 2, max
      },
      10000,
      [](const auto& x) { return std::cos(x); }
    };

    ADD_TEST(sin) {
      std::tuple {(test_iota<V> + std::cw<21>) / std::cw<3>},
      [](auto& t, V x) {
	t.verify_equal_to_ulp(sin(x), V([&](int i) -> T { return std::sin(x[i]); }), std::cw<1>)
	  ("input: {}", x);
      }
    };

    static constexpr auto sin_special_values = make_math_test {
      std::array{
#ifdef __STDC_IEC_559__
	nan, nan, inf, -inf, -zero, denorm_min, norm_min / 3,
#endif
	zero, norm_min, T(1), T(2), max / 5, max / 3, max / 2, max
      },
      10000,
      [](const auto& x) { return std::sin(x); }
    };

    static constexpr auto hypot_special_values = make_math_test {
      std::array{
#ifdef __STDC_IEC_559__
	nan, nan, inf, -inf, -zero, denorm_min, norm_min / 3,
#endif
	zero, norm_min, T(1), T(2), max / 5, max / 3, max / 2,
#if!_GLIBCXX_FAST_MATH
	max // fast-math hypot is imprecise for the max exponent
#endif
      },
      10000,
      [](const auto& x, const auto& y) { return std::hypot(x, y); }
    };
  };
#endif
