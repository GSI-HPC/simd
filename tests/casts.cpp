/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2020–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "unittest.h"
#include "complex_init.h"

template <typename From, typename To>
  consteval From
  finite_cast(To x)
  {
    const From max = std::numeric_limits<From>::max();
    const From min = std::numeric_limits<From>::lowest();
    const From r = static_cast<From>(x);
    const int digits = std::numeric_limits<To>::digits - std::numeric_limits<From>::digits;
    if (digits > 0 && r == static_cast<From>(x + (x < 0 ? 1 : -1)))
      // handle float(INT_MAX) because int(float(INT_MAX)) is UB
      return finite_cast<From>(x << digits);
    else if (r > max)
      return max;
    else if (r < min)
      return min;
    else if (static_cast<To>(r) == x) // guard against UB on conversion
      return r;
    // unreachable
  }

template <typename V, typename To>
  consteval auto
  make_input_array()
  {
    using From = typename V::value_type;
    using LF = std::numeric_limits<From>;
    using LT = std::numeric_limits<To>;
    if constexpr (std::is_floating_point_v<From> && std::is_floating_point_v<To>)
      { // everything goes
	return make_packed_array<V>(
		 0xc0000080U, 0xc0000081U, 0xc0000082U, 0xc0000084U, 0xc0000088U,
		 0xc0000090U, 0xc00000A0U, 0xc00000C0U, 0xc000017fU, 0xc0000180U,
		 0x100000001LL, 0x100000011LL, 0x100000111LL, 0x100001111LL,
		 0x100011111LL, 0x100111111LL, 0x101111111LL, -0x100000001LL,
		 -0x100000011LL, -0x100000111LL, -0x100001111LL, -0x100011111LL,
		 -0x100111111LL, -0x101111111LL,
 LF::min(), LF::min() + 1, LF::lowest(), LF::lowest() + 1,
 LF::max() - 1, LF::max(), LF::max() - 0xff, LF::max() - 0xff, LF::max() - 0x55,
 0, 1, -1, -10, -100, -1000, -10000);
      }
    else if constexpr (std::is_floating_point_v<From> && std::signed_integral<To>)
      { // out of range is UB
	return make_packed_array<V>(
		 finite_cast<From>(LT::max()), finite_cast<From>(LT::max() / 2 + 1),
		 finite_cast<From>(LT::min()), finite_cast<From>(LT::min() / 2),
		 0, 1, 0x11, -0x11, LF::min(), -LF::min(), LF::denorm_min(), -LF::denorm_min());
      }
    else if constexpr (std::is_floating_point_v<From> && std::unsigned_integral<To>)
      { // out of range is UB
	return make_packed_array<V>(
		 finite_cast<From>(LT::max()), finite_cast<From>(LT::max() / 2 + 1),
		 0, 1, 0x11, 0x5c, 0xc5, LF::min(), LF::denorm_min());
      }
    else if constexpr (std::is_integral_v<From>)
      { // only integral inputs
	return make_packed_array<V>(
		 1, LF::min(), LF::min() + 1, 0, 0x1c, 0x52,
		 LF::max() - 1, LF::max(), LF::max() - 0xff, LF::max() - 0x55);
      }
    else if constexpr (complex_like<From>)
      {
	using From = typename From::value_type;
	using LF = std::numeric_limits<From>;
	static_assert(std::is_floating_point_v<From>);
	static_assert(complex_like<To>);
	static_assert(std::is_floating_point_v<typename To::value_type>);
	using Cx = C<From>;
	constexpr From one = 1;
	return make_packed_array<V>(
		 Cx(0xc0000080U, 0xc0000081U), Cx(0xc0000082U, 0xc0000084U),
		 Cx(0xc0000088U, 0xc0000090U), Cx(0xc00000A0U, 0xc00000C0U),
		 Cx(0xc000017fU, 0xc0000180U), Cx(0x100000001LL, 0x100000011LL),
		 Cx(0x100000111LL, 0x100001111LL), Cx(0x100011111LL, 0x100111111LL),
		 Cx(0x101111111LL, -0x100000001LL), Cx(-0x100000011LL, -0x100000111LL),
		 Cx(-0x100001111LL, -0x100011111LL), Cx(-0x100111111LL, -0x101111111LL),
		 Cx{LF::min(), LF::min() + one}, Cx{LF::lowest(), LF::lowest() + one},
		 Cx{LF::max() - one, LF::max()}, Cx{LF::max() - From(0xff), LF::max() - From(0xff)},
		 Cx{LF::max() - From(0x55), 0}, Cx{one, -one}, Cx{From(-10), From(-100)},
		 Cx{From(-1000), From(-10000)});
      }
    else if constexpr (complex_like<To>)
      return make_input_array<V, typename To::value_type>();
    else
      static_assert(false);
  }

template <typename V, typename T>
  struct CvtTest
  { };

template <typename V, typename T>
  requires requires(typename V::value_type x) { static_cast<T>(x); }
  struct CvtTest<V, T>
  {
    using W = simd::rebind_t<T, V>;
    ADD_TEST(cvt, std::destructible<W> && (!std::is_same_v<V, W>)) {
      make_input_array<V, T>(),
      [](auto& t, V input) {
	const W expected([&](int i) { return static_cast<T>(input[i]); });
	const auto converted = static_cast<W>(input);
	t.verify_equal(converted, expected);
      }
    };
  };

template <typename V>
  struct Tests
  :
#ifdef __STDCPP_FLOAT64_T__
    CvtTest<V, std::float64_t>,
#endif
#ifdef __STDCPP_FLOAT32_T__
    CvtTest<V, std::float32_t>,
#endif
#ifdef __STDCPP_FLOAT16_T__
    CvtTest<V, std::float16_t>,
#endif
    CvtTest<V, std::int64_t>,
    CvtTest<V, std::int32_t>,
    CvtTest<V, std::int16_t>,
    CvtTest<V, std::int8_t>,
    CvtTest<V, std::uint64_t>,
    CvtTest<V, std::uint32_t>,
    CvtTest<V, std::uint16_t>,
    CvtTest<V, std::uint8_t>
#ifdef __STDCPP_FLOAT64_T__
    , CvtTest<V, std::complex<std::float64_t>>
#endif
#ifdef __STDCPP_FLOAT32_T__
    , CvtTest<V, std::complex<std::float32_t>>
#endif
#ifdef __STDCPP_FLOAT16_T__
    , CvtTest<V, std::complex<std::float16_t>>
#endif
  {};
