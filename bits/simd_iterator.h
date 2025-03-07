/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_RANGES_INTEGRATION_ITERATOR_H_
#define PROTOTYPE_RANGES_INTEGRATION_ITERATOR_H_

#include "fwddecl.h"

#if SIMD_IS_A_RANGE
namespace std::datapar
{
  template <typename _Vp>
    class __iterator
    {
      friend class __iterator<const _Vp>;
      _Vp* _M_data = nullptr;
      int _M_offset = 0;

    public:
      using value_type = typename _Vp::value_type;
      using iterator_category = std::random_access_iterator_tag;
      using difference_type = int;

      constexpr __iterator() = default;

      constexpr
      __iterator(_Vp& __d, int __x)
      : _M_data(&__d), _M_offset(__x)
      {}

      constexpr
      __iterator(const __iterator &) = default;

      constexpr
      __iterator(const __iterator<remove_const_t<_Vp>> &__i) requires is_const_v<_Vp>
      : _M_data(__i._M_data), _M_offset(__i._M_offset)
      {}

      constexpr __iterator&
      operator=(const __iterator &) = default;

      constexpr value_type
      operator*() const
      { return (*_M_data)[_M_offset]; } // checked in operator[]

      constexpr __iterator&
      operator++()
      {
        ++_M_offset;
        return *this;
      }

      constexpr __iterator
      operator++(int)
      {
        __iterator r = *this;
        ++_M_offset;
        return r;
      }

      constexpr __iterator&
      operator--()
      {
        --_M_offset;
        return *this;
      }

      constexpr __iterator
      operator--(int)
      {
        __iterator r = *this;
        --_M_offset;
        return r;
      }

      constexpr friend difference_type
      operator-(__iterator __a, __iterator __b)
      { return __a._M_offset - __b._M_offset; }

      constexpr friend difference_type
      operator-(__iterator __it, std::default_sentinel_t)
      { return __it._M_offset - difference_type(_Vp::size.value); }

      constexpr friend difference_type
      operator-(std::default_sentinel_t, __iterator __it)
      { return difference_type(_Vp::size.value) - __it._M_offset; }

      constexpr friend __iterator
      operator+(difference_type __x, const __iterator& __it)
      { return __iterator(*__it._M_data, __it._M_offset + __x); }

      constexpr friend __iterator
      operator+(const __iterator& __it, difference_type __x)
      { return __iterator(*__it._M_data, __it._M_offset + __x); }

      constexpr friend __iterator
      operator-(const __iterator& __it, difference_type __x)
      { return __iterator(*__it._M_data, __it._M_offset - __x); }

      constexpr __iterator&
      operator+=(difference_type __x)
      {
        _M_offset += __x;
        return *this;
      }

      constexpr __iterator&
      operator-=(difference_type __x)
      {
        _M_offset -= __x;
        return *this;
      }

      constexpr value_type
      operator[](difference_type __i) const
      { return (*_M_data)[_M_offset + __i]; } // checked in operator[]

      constexpr friend auto operator<=>(__iterator __a, __iterator __b)
      { return __a._M_offset <=> __b._M_offset; }

      constexpr friend auto operator<=>(__iterator __a, default_sentinel_t)
      { return __a._M_offset <=> _Vp::size.value; }

      constexpr friend bool operator==(__iterator __a, __iterator __b) = default;

      constexpr friend bool operator==(__iterator __a, std::default_sentinel_t)
      { return __a._M_offset == _Vp::size.value; }
    };
}
#endif

#endif  // PROTOTYPE_RANGES_INTEGRATION_ITERATOR_H_
