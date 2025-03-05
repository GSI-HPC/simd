/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_DETAIL_BITMASK_H_
#define PROTOTYPE_DETAIL_BITMASK_H_

#include "detail.h"

#include <type_traits>
#include <bitset>

namespace std::__detail
{
  inline constexpr size_t
  __div_roundup(size_t __a, size_t __b)
  { return (__a + __b - 1) / __b; }

  template <size_t _Np, bool _Sanitized>
    struct _BitMask
    {
      static_assert(_Np > 0);

      static constexpr size_t _NBytes = __div_roundup(_Np, __CHAR_BIT__);

      using _Tp = conditional_t<_Np == 1, bool,
                                make_unsigned_t<__mask_integer_from<
                                                  std::min(sizeof(0ULL), std::__bit_ceil(_NBytes))
                               >>>;

      static constexpr int _S_array_size = __div_roundup(_NBytes, sizeof(_Tp));

      _Tp _M_bits[_S_array_size];

      static constexpr int _S_unused_bits
        = _Np == 1 ? 0 : _S_array_size * sizeof(_Tp) * __CHAR_BIT__ - _Np;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbool-operation"
      static constexpr _Tp _S_bitmask = +_Tp(~_Tp()) >> _S_unused_bits;
#pragma GCC diagnostic pop

      constexpr _BitMask() noexcept = default;

      constexpr
      _BitMask(const _BitMask&) noexcept = default;

      constexpr _BitMask&
      operator=(const _BitMask&) noexcept = default;

      template <std::unsigned_integral _Up>
        constexpr _BitMask(_Up __x) noexcept
        : _M_bits{_Sanitized ? static_cast<_Tp>(_S_bitmask & __x) : static_cast<_Tp>(__x)} {}

      template <std::integral _Up>
        requires (_Sanitized)
        _GLIBCXX_SIMD_ALWAYS_INLINE static constexpr _BitMask
        __create_unchecked(_Up __x)
        { return {{static_cast<_Tp>(__x)}}; }

      constexpr
      _BitMask(bitset<_Np> __x) noexcept
      : _M_bits(static_cast<_Tp>(__x.to_ullong()))
      {}

      template <bool _RhsSanitized>
        requires (_RhsSanitized == false and _Sanitized == true)
        constexpr
        _BitMask(const _BitMask<_Np, _RhsSanitized>& __rhs) noexcept
        : _BitMask(__rhs._M_sanitized()) {}

      constexpr _Tp
      _M_to_bits() const noexcept
      {
        static_assert(_Sanitized);
        static_assert(_S_array_size == 1);
        return _M_bits[0];
      }

      constexpr _Tp
      _M_to_unsanitized_bits() const noexcept
      {
        static_assert(_S_array_size == 1);
        return _M_bits[0];
      }

      constexpr bitset<_Np>
      _M_to_bitset() const noexcept
      {
        static_assert(_S_array_size == 1);
        return _M_bits[0];
      }

      constexpr decltype(auto)
      _M_sanitized() const noexcept
      {
        if constexpr (_Sanitized)
          return *this;
        else if constexpr (_Np == 1)
          return _SanitizedBitMask<1>::__create_unchecked(_M_bits[0]);
        else
          {
            _SanitizedBitMask<_Np> __r = {};
            for (int __i = 0; __i < _S_array_size; ++__i)
              __r._M_bits[__i] = _M_bits[__i];
            if constexpr (_S_unused_bits > 0)
              __r._M_bits[_S_array_size - 1] &= _S_bitmask;
            return __r;
          }
      }

      template <size_t _Mp, bool _LSanitized>
        constexpr _BitMask<_Np + _Mp, _Sanitized>
        _M_prepend(_BitMask<_Mp, _LSanitized> __lsb) const noexcept
        {
          constexpr size_t _RN = _Np + _Mp;
          using _Rp = _BitMask<_RN, _Sanitized>;
          if constexpr (_Rp::_S_array_size == 1)
            {
              _Rp __r{{_M_bits[0]}};
              __r._M_bits[0] <<= _Mp;
              __r._M_bits[0] |= __lsb._M_sanitized()._M_bits[0];
              return __r;
            }
          else
            __assert_unreachable<_Rp>();
        }

      // Return a new _BitMask with size _NewSize while dropping _DropLsb least
      // significant bits. If the operation implicitly produces a sanitized bitmask,
      // the result type will have _Sanitized set.
      template <size_t _DropLsb, size_t _NewSize = _Np - _DropLsb>
        constexpr auto
        _M_extract() const noexcept
        {
          static_assert(_Np > _DropLsb);
          static_assert(_DropLsb + _NewSize <= sizeof(0ULL) * __CHAR_BIT__,
                        "not implemented for bitmasks larger than one ullong");
          if constexpr (_NewSize == 1)
            return _SanitizedBitMask<1>::__create_unchecked(_M_bits[0] >> _DropLsb);
          else
            return _BitMask<_NewSize,
                            ((_NewSize + _DropLsb == sizeof(_Tp) * __CHAR_BIT__
                                && _NewSize + _DropLsb <= _Np)
                               || ((_Sanitized || _Np == sizeof(_Tp) * __CHAR_BIT__)
                                     && _NewSize + _DropLsb >= _Np))>(_M_bits[0]
                     >> _DropLsb);
        }

      // True if all bits are set. Implicitly sanitizes if _Sanitized == false.
      constexpr bool
      all() const noexcept
      {
        if constexpr (_Np == 1)
          return _M_bits[0];
        else if constexpr (!_Sanitized)
          return _M_sanitized().all();
        else
          {
            constexpr _Tp __allbits = ~_Tp();
            for (int __i = 0; __i < _S_array_size - 1; ++__i)
              if (_M_bits[__i] != __allbits)
                return false;
            return _M_bits[_S_array_size - 1] == _S_bitmask;
          }
      }

      // True if at least one bit is set. Implicitly sanitizes if _Sanitized ==
      // false.
      constexpr bool
      any() const noexcept
      {
        if constexpr (_Np == 1)
          return _M_bits[0];
        else if constexpr (!_Sanitized)
          return _M_sanitized().any();
        else
          {
            for (int __i = 0; __i < _S_array_size - 1; ++__i)
              if (_M_bits[__i] != 0)
                return true;
            return _M_bits[_S_array_size - 1] != 0;
          }
      }

      // True if no bit is set. Implicitly sanitizes if _Sanitized == false.
      constexpr bool
      none() const noexcept
      {
        if constexpr (_Np == 1)
          return !_M_bits[0];
        else if constexpr (!_Sanitized)
          return _M_sanitized().none();
        else
          {
            for (int __i = 0; __i < _S_array_size - 1; ++__i)
              if (_M_bits[__i] != 0)
                return false;
            return _M_bits[_S_array_size - 1] == 0;
          }
      }

      // Returns the number of set bits. Implicitly sanitizes if _Sanitized ==
      // false.
      constexpr int
      count() const noexcept
      {
        if constexpr (_Np == 1)
          return _M_bits[0];
        else if constexpr (!_Sanitized)
          return _M_sanitized().none();
        else
          {
            int __result = __builtin_popcountll(_M_bits[0]);
            for (int __i = 1; __i < _S_array_size; ++__i)
              __result += __builtin_popcountll(_M_bits[__i]);
            return __result;
          }
      }

      // Returns the bit at offset __i as bool.
      constexpr bool
      operator[](size_t __i) const noexcept
      {
        if constexpr (_Np == 1)
          return _M_bits[0];
        else if constexpr (_S_array_size == 1)
          return (_M_bits[0] >> __i) & 1;
        else
          {
            const size_t __j = __i / (sizeof(_Tp) * __CHAR_BIT__);
            const size_t __shift = __i % (sizeof(_Tp) * __CHAR_BIT__);
            return (_M_bits[__j] >> __shift) & 1;
          }
      }

      constexpr bool
      operator[](__constexpr_wrapper_like auto __i) const noexcept
      {
        static_assert(__i < _Np);
        constexpr size_t __j = __i / (sizeof(_Tp) * __CHAR_BIT__);
        constexpr size_t __shift = __i % (sizeof(_Tp) * __CHAR_BIT__);
        return static_cast<bool>(_M_bits[__j] & (_Tp(1) << __shift));
      }

      // Set the bit at offset __i to __x.
      constexpr void
      set(size_t __i, bool __x) noexcept
      {
        if constexpr (_Np == 1)
          _M_bits[0] = __x;
        else if constexpr (_S_array_size == 1)
          {
            _M_bits[0] &= ~_Tp(_Tp(1) << __i);
            _M_bits[0] |= _Tp(_Tp(__x) << __i);
          }
        else
          {
            const size_t __j = __i / (sizeof(_Tp) * __CHAR_BIT__);
            const size_t __shift = __i % (sizeof(_Tp) * __CHAR_BIT__);
            _M_bits[__j] &= ~_Tp(_Tp(1) << __shift);
            _M_bits[__j] |= _Tp(_Tp(__x) << __shift);
          }
      }

      constexpr void
      set(__constexpr_wrapper_like auto __i, bool __x) noexcept
      {
        static_assert(__i < _Np);
        if constexpr (_Np == 1)
          _M_bits[0] = __x;
        else
          {
            constexpr size_t __j = __i / (sizeof(_Tp) * __CHAR_BIT__);
            constexpr size_t __shift = __i % (sizeof(_Tp) * __CHAR_BIT__);
            constexpr _Tp __mask = ~_Tp(_Tp(1) << __shift);
            _M_bits[__j] &= __mask;
            _M_bits[__j] |= _Tp(_Tp(__x) << __shift);
          }
      }

      // Inverts all bits. Sanitized input leads to sanitized output.
      constexpr _BitMask
      operator~() const noexcept
      {
        if constexpr (_Np == 1)
          return !_M_bits[0];
        else
          {
            _BitMask __result{};
            for (int __i = 0; __i < _S_array_size - 1; ++__i)
              __result._M_bits[__i] = ~_M_bits[__i];
            if constexpr (_Sanitized)
              __result._M_bits[_S_array_size - 1]
                      = _M_bits[_S_array_size - 1] ^ _S_bitmask;
            else
              __result._M_bits[_S_array_size - 1] = ~_M_bits[_S_array_size - 1];
            return __result;
          }
      }

      constexpr _BitMask&
      operator^=(const _BitMask& __b) & noexcept
      {
        _GLIBCXX_SIMD_INT_PACK(_S_array_size, _Is, {
          ((_M_bits[_Is] ^= __b._M_bits[_Is]), ...);
        });
        return *this;
      }

      constexpr _BitMask&
      operator|=(const _BitMask& __b) & noexcept
      {
        _GLIBCXX_SIMD_INT_PACK(_S_array_size, _Is, {
          ((_M_bits[_Is] |= __b._M_bits[_Is]), ...);
        });
        return *this;
      }

      constexpr _BitMask&
      operator&=(const _BitMask& __b) & noexcept
      {
        _GLIBCXX_SIMD_INT_PACK(_S_array_size, _Is, {
          ((_M_bits[_Is] &= __b._M_bits[_Is]), ...);
        });
        return *this;
      }

      friend constexpr _BitMask
      operator^(const _BitMask& __a, const _BitMask& __b) noexcept
      {
        _BitMask __r = __a;
        __r ^= __b;
        return __r;
      }

      friend constexpr _BitMask
      operator|(const _BitMask& __a, const _BitMask& __b) noexcept
      {
        _BitMask __r = __a;
        __r |= __b;
        return __r;
      }

      friend constexpr _BitMask
      operator&(const _BitMask& __a, const _BitMask& __b) noexcept
      {
        _BitMask __r = __a;
        __r &= __b;
        return __r;
      }

      _GLIBCXX_SIMD_INTRINSIC
      constexpr bool
      _M_is_constprop() const
      {
        if constexpr (_S_array_size == 0)
          return __builtin_constant_p(_M_bits[0]);
        else
          {
            for (int __i = 0; __i < _S_array_size; ++__i)
              if (!__builtin_constant_p(_M_bits[__i]))
                return false;
            return true;
          }
      }
    };

  template <std::integral _Tp>
    _BitMask(_Tp)
    -> _BitMask<__CHAR_BIT__ * sizeof(_Tp)>;
}

#endif  // PROTOTYPE_DETAIL_BITMASK_H_
