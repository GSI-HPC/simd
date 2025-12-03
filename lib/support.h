/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef LIB_SUPPORT_H_
#define LIB_SUPPORT_H_

#define VIR_NEXT_PATCH 1
#define VIR_EXTENSIONS 1
#include <bits/simd_math.h>
#include <bits/simd_alg.h>
#include <bits/simd_mask_reductions.h>
#include <limits>

namespace std::simd
{
  template <__simd_floating_point V>
    inline constexpr V inf_v = numeric_limits<typename V::value_type>::infinity();

  template <__simd_floating_point V>
    inline constexpr V finite_max_v = numeric_limits<typename V::value_type>::max();

  template <__simd_floating_point V>
    inline constexpr V norm_min_v = V() + numeric_limits<typename V::value_type>::min();

  template <__simd_floating_point V>
    inline constexpr V denorm_min_v = V() + numeric_limits<typename V::value_type>::denorm_min();

  template <__simd_floating_point V>
    inline constexpr V mantissa_mask_v = norm_min_v<V> - denorm_min_v<V>;

  template <typename T>
    struct IncreasePrecisionImpl
    { using type = void; };

  template <>
    struct IncreasePrecisionImpl<_Float16>
    { using type = float; };

  template <>
    struct IncreasePrecisionImpl<float>
    { using type = double; };

  template <typename T>
    using IncreasePrecision = typename IncreasePrecisionImpl<T>::type;

  template <typename V>
    [[gnu::always_inline]]
    inline V
    operator&(V x, V y) noexcept
    { return __vec_and(x._M_get(), y._M_get()); }

  template <typename V>
    [[gnu::always_inline]]
    inline V
    operator&(V x, type_identity_t<V> y) noexcept
    { return __vec_and(x._M_get(), y._M_get()); }

  template <typename V>
    [[gnu::always_inline]]
    inline V
    operator&(type_identity_t<V> x, V y) noexcept
    { return __vec_and(x._M_get(), y._M_get()); }

  template <typename V>
    [[gnu::always_inline]]
    inline V
    operator|(V x, V y) noexcept
    { return __vec_or(x._M_get(), y._M_get()); }

  template <typename V>
    [[gnu::always_inline]]
    inline V
    operator|(V x, type_identity_t<V> y) noexcept
    { return __vec_or(x._M_get(), y._M_get()); }

  template <typename V>
    [[gnu::always_inline]]
    inline V
    operator|(type_identity_t<V> x, V y) noexcept
    { return __vec_or(x._M_get(), y._M_get()); }

  template <typename V>
    [[gnu::always_inline]]
    inline V
    operator^(V x, V y) noexcept
    { return __vec_xor(x._M_get(), y._M_get()); }

  template <typename V>
    [[gnu::always_inline]]
    inline V
    operator^(V x, type_identity_t<V> y) noexcept
    { return __vec_xor(x._M_get(), y._M_get()); }

  template <typename V>
    [[gnu::always_inline]]
    inline V
    operator^(type_identity_t<V> x, V y) noexcept
    { return __vec_xor(x._M_get(), y._M_get()); }

  template <typename T, typename V>
    [[gnu::always_inline]]
    constexpr rebind_t<T, V>
    value_bit_cast(const V& x) noexcept
    { return bit_cast<rebind_t<T, V>>(x); }

  template <typename V>
    [[gnu::always_inline]]
    constexpr auto
    int_bit_cast(const V& x) noexcept
    { return value_bit_cast<__integer_from<sizeof(typename V::value_type)>>(x); }

  /** @internal
   * @brief Determines if x*x + y*y can be safely shortened to x*x under IEEE-754 rounding.
   *
   * The condition checks if the bit pattern difference between x and y is large enough such that
   * y^2 is negligible compared to x^2 (i.e., y^2 < 0.5 * ulp(x^2)), ensuring x² + y² rounds to x².
   * The threshold is derived from:
   *   (digits + 2) << (digits - 2)
   * where 'digits' is the floating-point precision (24 for float, 53 for double).
   *
   * Why this works:
   * - For normalized values, bit patterns increase monotonically with magnitude
   * - The threshold guarantees exponent difference >= (digits/2 + 1) (13 for float, 27 for double)
   * - This ensures y < x * 2^(-digits/2), making y² < 0.5 * ulp(x^2)
   *
   * Example for float (24-bit precision) at threshold:
   *   Threshold = (24+2) << (24-2) = 26 << 22
   *   Let x = 2^100 (bit pattern: (100+127) << 23 = 227 << 23)
   *   Let y = 2^87  (bit pattern: (87+127) << 23 = 214 << 23)
   *   Bit diff = (227 - 214) << 23 = 26 << 22 (exactly threshold)
   *   Then:
   *     x² = 2^200, y² = 2^174
   *     ulp(x²) = 2^(200-23) = 2^177
   *     0.5 * ulp(x²) = 2^176
   *     Since 2^174 < 2^176, x² + y² rounds to x²
   *
   * Example for double (53-bit precision) at threshold:
   *   Threshold = (53+2) << (53-2) = 55 << 51
   *   Let x = 2^1000 (bit pattern: (1000+1023) << 52 = 2023 << 52)
   *   Let y = 1.5 * 2^972 (bit pattern: (972+1023) << 52 + (1 << 51) = 1995 << 52 + (1 << 51))
   *   Bit diff = (2023 << 52) - (1995 << 52 + (1 << 51))
   *            = (28 << 52) - (1 << 51)
   *            = (56 << 51) - (1 << 51)
   *            = 55 << 51 (exactly threshold)
   *   Then:
   *     x² = 2^2000, y² = (1.5)² * 2^1944 = 2.25 * 2^1944
   *     ulp(x²) = 2^(2000-52) = 2^1948
   *     0.5 * ulp(x²) = 2^1947
   *     Since 2.25 * 2^1944 = 0.28125 * 2^1947 < 0.5 * 2^1947,
   *     x² + y² rounds to x²
   *
   *
   * @pre
   * 1. x >= y >= 0
   * 2. Only valid for normalized numbers (subnormals are irrelevant in this context)
   *
   * @note The condition is sufficient but not necessary (values below threshold might still round
   * correctly).
   */
  template <typename V>
    [[gnu::always_inline]]
    constexpr typename V::mask_type
    is_large_diff(const V& x, const V& y)
    {
      using T = typename V::value_type;
      using L = numeric_limits<T>;
      __integer_from<sizeof(T)> delta_exp = L::digits + 2;
      delta_exp <<= L::digits - 2; // -1 for implicit 1; another -1 for division by 2
      return int_bit_cast(x) - int_bit_cast(y) >= delta_exp;
    }

  /** @internal
   * Adjusts all given arguments by @f$2^n@f$ and returns @f$2^-n@f$.
   * @f$n = 1-\floor log_2 \mathtt{hi} \rfloor@f$
   */
  template <typename V>
    [[gnu::always_inline]]
    inline V
    rescale_factors(V& hi, auto&... to_scale)
    {
      constexpr V two = 2;
      constexpr V half = 1 / two;
      const V hi_exp = hi & inf_v<V>; // round down to next power-of-2 = 2^(1-n) = 2*2^-n
      const V scale = hi_exp ^ inf_v<V>; // = 2/hi_exp = 2^n
      hi = (hi & mantissa_mask_v<V>) | two; // = hi * scale
      ((to_scale *= scale), ...);
      return half * hi_exp; // = hi_exp/2 = 1/scale = 2^-n
#if 0
      using T = typename V::value_type;
      using I = __integer_from<sizeof(T)>;
      using IV = rebind_t<I, V>;
      // 
      const IV exponent_bits = bit_cast<IV>(inf_v<V>);
      const IV exponent_lsb = bit_cast<IV>(norm_min_v<V>);
      const IV hi_exp = bit_cast<IV>(hi) & exponent_bits; // e.g. 0x5380'0000
      const V scale = bit_cast<V>((((hi_exp * 2) & exponent_bits) ^ exponent_bits) + exponent_lsb);
      const V scale_back = bit_cast<V>(hi_exp - exponent_lsb);
      hi = (hi & mantissa_mask_v<V>) | two;
      ((to_scale *= scale), ...);
      return scale_back;
#endif
    }
}
#endif  // LIB_SUPPORT_H_
