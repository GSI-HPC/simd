/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_MASK_REDUCTIONS_H_
#define PROTOTYPE_MASK_REDUCTIONS_H_

#include "simd_mask.h"
#include "simd_reductions.h"
#include "x86_detail.h"

namespace std::datapar
{
  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
    all_of(const basic_simd_mask<_Bs, _Abi>& __k) noexcept
    {
      using _Kp = basic_simd_mask<_Bs, _Abi>;
      constexpr __detail::_SimdSizeType __size = _Kp::size.value;
      if constexpr (__size == 1)
        return __k[0];

      else if (__builtin_is_constant_evaluated() or __k._M_is_constprop())
        return _GLIBCXX_SIMD_INT_PACK(__size, _Is, { return (... and __k[_Is]); });

      else if constexpr (requires {_Abi::_MaskImpl::_S_all_of(__k._M_data);})
        return _Abi::_MaskImpl::_S_all_of(__k._M_data);

      else if constexpr (not _Kp::_S_is_bitmask and sizeof(__k) <= sizeof(int64_t))
        {
          const auto __as_int
            = __builtin_bit_cast(__detail::__mask_integer_from<sizeof(__k)>, __data(__k));
          if constexpr (__detail::__signed_has_single_bit(__size))
            return __as_int == -1;
          else
            return (__as_int & ((1ll << (__size * _Bs * __CHAR_BIT__)) - 1))
                     == (1ll << (__size * _Bs * __CHAR_BIT__)) - 1;
        }

#if _GLIBCXX_SIMD_HAVE_NEON
      else if constexpr (__pv2::__is_neon_abi<_Abi>())
        {
          using _Tp = __detail::__mask_integer_from<_Bs>;
          static_assert(sizeof(__k) == 16);
          const auto __kk
            = __pv2::__vector_bitcast<char>(__data(__k))
                | ~__pv2::__vector_bitcast<char>(_Abi::template _S_implicit_mask<_Tp>());
          const auto __x = __pv2::__vector_bitcast<int64_t>(__kk);
          return __x[0] + __x[1] == -2;
        }

#endif
      else
        return _GLIBCXX_SIMD_INT_PACK(__size, _Is, { return (... and __k[_Is]); });
    }

  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
    any_of(const basic_simd_mask<_Bs, _Abi>& __k) noexcept
    {
      using _Kp = basic_simd_mask<_Bs, _Abi>;
      constexpr __detail::_SimdSizeType __size = _Kp::size.value;
      if constexpr (__size == 1)
        return __k[0];

      else if (__builtin_is_constant_evaluated() or __k._M_is_constprop())
        return _GLIBCXX_SIMD_INT_PACK(__size, _Is, { return (... or __k[_Is]); });

      else if constexpr (requires {_Abi::_MaskImpl::_S_any_of(__k._M_data);})
        return _Abi::_MaskImpl::_S_any_of(__k._M_data);

      else if constexpr (not _Kp::_S_is_bitmask and sizeof(__k) <= sizeof(int64_t))
        return __builtin_bit_cast(__detail::__mask_integer_from<sizeof(__k)>, __data(__k)) != 0;

#if _GLIBCXX_SIMD_HAVE_NEON
      else if constexpr (__pv2::__is_neon_abi<_Abi>())
        {
          static_assert(sizeof(__k) == 16);
          const auto __kk = _Abi::_S_masked(__data(__k));
          const auto __x = __pv2::__vector_bitcast<int64_t>(__kk);
          return (__x[0] | __x[1]) != 0;
        }

#endif
      else
        return _GLIBCXX_SIMD_INT_PACK(__size, _Is, { return (... or __k[_Is]); });
    }

  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
    none_of(const basic_simd_mask<_Bs, _Abi>& __k) noexcept
    {
      using _Kp = basic_simd_mask<_Bs, _Abi>;
      constexpr __detail::_SimdSizeType __size = _Kp::size.value;
      if constexpr (__size == 1)
        return !__k[0];

      else if (__builtin_is_constant_evaluated() or __k._M_is_constprop())
        return _GLIBCXX_SIMD_INT_PACK(__size, _Is, { return (... and not __k[_Is]); });

      else if constexpr (requires {_Abi::_MaskImpl::_S_none_of(__k._M_data);})
        return _Abi::_MaskImpl::_S_none_of(__k._M_data);

      else if constexpr (not _Kp::_S_is_bitmask and sizeof(__k) <= sizeof(int64_t))
        return __builtin_bit_cast(__detail::__mask_integer_from<sizeof(__k)>,
                                  _Abi::_S_masked(__data(__k))) == 0;

#if _GLIBCXX_SIMD_HAVE_NEON
      else if constexpr (__pv2::__is_neon_abi<_Abi>())
        {
          static_assert(sizeof(__k) == 16);
          const auto __kk = _Abi::_S_masked(__data(__k));
          const auto __x = __vector_bitcast<int64_t>(__kk);
          return (__x[0] | __x[1]) == 0;
        }

#endif
      else
        return _GLIBCXX_SIMD_INT_PACK(__size, _Is, { return (... and not __k[_Is]); });
    }

  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
    reduce_count(const basic_simd_mask<_Bs, _Abi>& __k) noexcept
    {
      using _Kp = basic_simd_mask<_Bs, _Abi>;
      constexpr __detail::_SimdSizeType __size = _Kp::size.value;
      if constexpr (__size == 1)
        return +__k[0];

      else if (__builtin_is_constant_evaluated() || __k._M_is_constprop())
        {
          int __r = _GLIBCXX_SIMD_INT_PACK(__size, _Is, { return (... + int(__k[_Is])); });
          if (__builtin_is_constant_evaluated() || __builtin_constant_p(__r))
            return __r;
        }

      else if constexpr (requires {_Abi::_MaskImpl::_S_popcount(__k);})
        return _Abi::_MaskImpl::_S_popcount(__k);

      else
        static_assert(-__size >= __finite_min_v<__detail::__mask_integer_from<_Bs>>);

      return -reduce(-__k);
    }

  /**
   * Precondition: any_of(__k) is true.
   * on failure: ill-formed (constant expression) or returns unspecified value
   */
  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
    reduce_min_index(const basic_simd_mask<_Bs, _Abi>& __k)
    {
      __glibcxx_simd_precondition(any_of(__k), "any_of(k) must be true");

      constexpr int __size = basic_simd_mask<_Bs, _Abi>::size.value;
      if constexpr (__size == 1)
        return 0;

      else if (__builtin_is_constant_evaluated() or __k._M_is_constprop())
        {
          const int __r = [&] {
            for (int __i = 0; __i < __size; ++__i)
              if (__k[__i])
                return __i;
            return -1;
          }();
          if (__builtin_is_constant_evaluated() or __builtin_constant_p(__r))
            return __r;
        }

      if constexpr (__size == 2)
        return __k[0] ? 0 : 1;

      else if constexpr (requires {_Abi::_MaskImpl::_S_reduce_min_index(__k);})
        return _Abi::_MaskImpl::_S_reduce_min_index(__k);

      else
        return __detail::__lowest_bit(_Abi::_MaskImpl::_S_to_bits(__data(__k))._M_to_bits());
    }

  /**
   * Precondition: any_of(__k) is true.
   * on failure: ill-formed (constant expression) or returns unspecified value
   */
  template <size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
    reduce_max_index(const basic_simd_mask<_Bs, _Abi>& __k)
    {
      __glibcxx_simd_precondition(any_of(__k), "any_of(k) must be true");

      constexpr int __size = basic_simd_mask<_Bs, _Abi>::size.value;
      if constexpr (__size == 1)
        return 0;

      else if (__builtin_is_constant_evaluated() or __k._M_is_constprop())
        {
          const int __r = [&] {
            for (int __i = __size - 1; __i >= 0; --__i)
              if (__k[__i])
                return __i;
            return -1;
          }();
          if (__builtin_is_constant_evaluated() or __builtin_constant_p(__r))
            return __r;
        }

      if constexpr (__size == 2)
        return __k[1] ? 1 : 0;

      else if constexpr (requires {_Abi::_MaskImpl::_S_reduce_max_index(__k);})
        return _Abi::_MaskImpl::_S_reduce_max_index(__k);

      else
        return __detail::__highest_bit(
                 _Abi::_MaskImpl::_S_to_bits(__data(__k))._M_sanitized()._M_to_bits());
    }

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
  all_of(same_as<bool> auto __x) noexcept
  { return __x; }

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
  any_of(same_as<bool> auto __x) noexcept
  { return __x; }

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
  none_of(same_as<bool> auto __x) noexcept
  { return not __x; }

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
  reduce_count(same_as<bool> auto __x) noexcept
  { return static_cast<__detail::_SimdSizeType>(__x); }

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
  reduce_min_index(same_as<bool> auto __x) noexcept
  {
    __glibcxx_simd_precondition(__x, "any_of(x) must be true");
    return 0;
  }

  _GLIBCXX_SIMD_ALWAYS_INLINE constexpr __detail::_SimdSizeType
  reduce_max_index(same_as<bool> auto __x) noexcept
  {
    __glibcxx_simd_precondition(__x, "any_of(x) must be true");
    return 0;
  }
}
#endif  // PROTOTYPE_MASK_REDUCTIONS_H_
