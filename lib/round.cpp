/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2026      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#define VIR_EXTENSIONS 1
#include "support.h"

namespace std::simd
{
  /* pure bit-manipulation, derived from the scalar glibc round implementation
   * 1. issue: fails to raise FE_INVALID on SNaN
   * 2. issue: it's not the most efficient implementation on my system
   *
   * x86-64-v3 / znver5: 10 cycles latency
   * x86-64-v3 / alderlake: 16 cycles latency

            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
 float, 4                 11.9           7.86           11.7
 float, 8                 13.6           8.54           12.8
 float, 16                  20           15.7           18.1
 float, 32                  34           33.4           32.5
 float, 64                68.1           68.5             64
------------------------------------------------------------
            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
double, 2                 16.8           10.2           16.1
double, 4                 18.1           10.3           15.5
double, 8                 23.4           18.8           21.9
double, 16                38.3           34.5             35
double, 32                70.7             79           71.7
------------------------------------------------------------
   */
#if 0
  template <_TargetTraits _Traits = _TargetTraits()._M_math_abi(), typename TV>
    [[gnu::flatten]]
    TV
    __round(TV x)
    {
      using T = __vec_value_type<TV>;
      using U = _UInt<sizeof(T)>;
      using I = __integer_from<sizeof(T)>;
      using Lf = numeric_limits<T>;
      using Vf = vec<T, __width_of<TV>>;
      using Vu = rebind_t<U, Vf>;
      using Vi = rebind_t<I, Vf>;

      constexpr int mant_width = Lf::digits - 1;
      constexpr U inf_bits = __builtin_bit_cast(U, Lf::infinity());
      constexpr U exp_mask = inf_bits >> mant_width;
      constexpr U one_bits = __builtin_bit_cast(U, T(1));
      constexpr U bias = one_bits >> mant_width;
      constexpr U mant_mask = (1ull << mant_width) - 1u;
      constexpr U half_bit = 1ull << (mant_width - 1);
      constexpr U sign_bit = __builtin_bit_cast(U, _S_signmask<TV>[0]);

      Vu i0   = bit_cast<Vu>(x);
      Vu j0   = ((i0 >> mant_width) & exp_mask) - bias;
      Vu jsat = Vu(j0) % U(numeric_limits<U>::digits);
      Vu frac = mant_mask >> jsat;
      Vu half = half_bit >> jsat;

      Vu normal = (i0 + half) & ~frac; // trunc(x + .5)
      Vu small  = i0 & sign_bit;
      Vu one    = small | one_bits;

      Vu r = select(Vi(j0) <= I(-1),
		       select(Vi(j0) == I(-1), one, // 0.5 <= |x| < 1 → ±1.0
			      small),    // |x| < 0.5 → ±0.0
		       select(Vi(j0) >= I(mant_width), i0, // input is already rounded
			      normal));
      return bit_cast<TV>(r);
    }
#endif

  template <_TargetTraits _Traits = _TargetTraits()._M_math_abi(), typename TV>
    [[gnu::flatten]]
    TV
    __round_alt(TV x)
    {
      // Alternative: copysign(trunc(|x|) + fixup, x), with fixup = 0 or 1
      //
      // x86-64-v3 / znver5: 7 cycles latency
      // x86-64-v3 / alderlake: 18 cycles latency
      using T = __vec_value_type<TV>;
      if constexpr (__width_of<TV> == 2)
	return TV {T(__builtin_round(x[0])), T(__builtin_round(x[1]))};

      using I = __integer_from<sizeof(T)>;
      const auto abs_x = __fabs<_Traits>(x);
#if 0 // slow in benchmarks
      const auto t_abs = __trunc<_Traits>(abs_x);
      using Vf = vec<T, __width_of<TV>>;
      if (!Vf(x)._M_isinf()._M_any_of()) [[__likely__]]
	{ // spurious FE_INVALID on inf inputs
	  /*
            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
 float, 4                 22.6           4.23           19.5
 float, 8                 21.3           4.77           22.5
 float, 16                23.9           8.85           23.4
 float, 32                26.8           17.3           25.4
 float, 64                36.3           34.8           33.6
------------------------------------------------------------
            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
double, 2                   24           4.49           19.8
double, 4                 23.6            4.5           20.7
double, 8                 23.6           8.65           25.3
double, 16                27.6           18.1           29.9
double, 32                38.5           37.2           32.5
------------------------------------------------------------
	   */
	  const auto r_abs // round(abs(x)) =
	    = t_abs + (abs_x - t_abs >= T(.5) ? T(1) : T(0));
	  return __vec_or(__vec_xor(abs_x, x), r_abs);
	}
      else
#endif
	{
	  constexpr T hugeval = 1ull << (numeric_limits<T>::digits - 1);
	  const auto large = __vec_bit_cast<I>(abs_x) >= __builtin_bit_cast(I, hugeval);
#if 1
	  /*
            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
 float, 4                 24.1            4.3           17.2
 float, 8                 21.8           5.58           16.8
 float, 16                  24           10.8           17.1
 float, 32                  27           21.8           21.8
 float, 64                  44           43.1           38.1
------------------------------------------------------------
            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
double, 2                 22.4           5.43           22.7
double, 4                   27           5.54             22
double, 8                 24.1           10.7           21.8
double, 16                29.7             22             22
double, 32                45.9           44.8           41.7
------------------------------------------------------------
	   */
	  const auto small = __vec_bit_cast<I>(abs_x) < __builtin_bit_cast(I, T(.5));
	  const auto y = (large | small) ? T() : abs_x;
	  const auto t = __trunc<_Traits>(y);
	  const auto r = t + (y - T(.5) < t ? T(0) : T(1));
#else
	  /*
            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
 float, 4                 25.3           4.96           17.4
 float, 8                   24           4.85           20.2
 float, 16                20.3           9.63           21.4
 float, 32                25.5           19.1           22.2
 float, 64                39.3           37.8           26.7
------------------------------------------------------------
            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
double, 2                 26.5           4.85           20.5
double, 4                 23.7           4.42           20.1
double, 8                 25.6           8.52           23.9
double, 16                25.6           17.6             20
double, 32                41.8           38.4           37.8
------------------------------------------------------------
	   */
	  const auto y = large ? T() : abs_x;
	  const auto t = __trunc<_Traits>(y);
	  const auto r = t + (y - t < T(.5) ? T(0) : T(1));
#endif
	  return large ? x : __vec_or(__vec_xor(abs_x, x), r);
	}
    }

  /*
            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
 float, 4                   18           6.81           12.1
 float, 8                 14.1           6.57           12.3
 float, 16                20.9           13.4           14.2
 float, 32                29.4           28.2           29.4
 float, 64                60.5           63.3           61.7
------------------------------------------------------------
            TYPE       Latency     Throughput          Sweep
                 [cycles/call]  [cycles/call]  [cycles/call]
double, 2                 22.2           8.78             17
double, 4                 16.4           8.42           19.1
double, 8                 26.9           16.5           19.3
double, 16                39.6           35.4           28.8
double, 32                76.2           73.1           74.6
------------------------------------------------------------
   */
  template <_TargetTraits _Traits = _TargetTraits()._M_math_abi(), typename TV>
    [[gnu::flatten]]
    TV
    __round(TV x)
    {
#if __has_builtin(__builtin_elementwise_round)
      return __builtin_elementwise_round(x);
#else
      using T = __vec_value_type<TV>;
      if constexpr (__width_of<TV> == 2)
	return TV {T(__builtin_round(x[0])), T(__builtin_round(x[1]))};

      using U = _UInt<sizeof(T)>;
      using I = __integer_from<sizeof(T)>;
      const auto abs_x = __fabs<_Traits>(x);
      const auto t_abs = __trunc<_Traits>(abs_x);
      const auto int_abs_x  = __vec_bit_cast<I>(abs_x);
      const auto int_t_abs  = __vec_bit_cast<I>(t_abs);

      // a) for |x| without fractional mantissa bits: diff = 0
      // b) for |x| >= 1: XOR cancels exponent bits and mantissa bits signifying values >= 1
      //                  If the MSB is at the .5 position we need to add 1 to the result
      // c) for |x| < 1: trunc(|x|) is 0 => so diff = bit-pattern of |x|
      // => diff needs to be compared against
      // a) any positive non-zero integer
      // b) integer with 1 bit at .5 position
      // c) bit-pattern of .5
      const auto diff = int_abs_x ^ int_t_abs;

      constexpr int mant_width = numeric_limits<T>::digits - 1;
      constexpr I one_bits = __builtin_bit_cast(I, T(1));
      constexpr I one_half_bits = __builtin_bit_cast(I, T(.5));
      constexpr I bias = one_bits >> mant_width;

      const auto biased_exp = int_abs_x >> mant_width;
      const auto exponent = __vec_bit_cast<I>(__vec_bit_cast<U>(biased_exp - bias)
						    % numeric_limits<U>::digits);
      const auto threshold
	= ((biased_exp > (mant_width + bias)) | (int_abs_x < one_bits))
	    ? one_half_bits // a) + c)
	    : (I(1) << (mant_width - 1)) >> exponent; // b)

      const TV sign_bit = __vec_xor(abs_x, x);
      const TV r_abs = t_abs + __builtin_bit_cast(
				     TV, diff < threshold ? I() : one_bits);
      return __vec_or(sign_bit, r_abs);
#endif
    }

  template <_TargetTraits _Traits = _TargetTraits()._M_math_abi(), typename V0, typename V1>
    [[gnu::flatten]]
    _GLIBCXX_SIMD_MATH_RET_TYPE(V0, V1)
    __2x_round(V0 x, V1 y)
    {
#if __has_builtin(__builtin_elementwise_round)
      V0 lo = __builtin_elementwise_round(x);
      V1 hi = __builtin_elementwise_round(y);
#else
      // __round_alt has better throughput and is therefore more efficient in the 2x case
      V0 lo = __round_alt<_Traits, V0>(x);
      V1 hi = __round_alt<_Traits, V1>(y);
#endif
      _GLIBCXX_SIMD_MATH_RETURN(lo, hi);
    }

#define FN round
#define NO_FAST 1
#include "instantiate_1arg.h"
}
