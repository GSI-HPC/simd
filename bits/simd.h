/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023-2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD2_H_
#define PROTOTYPE_SIMD2_H_

#include "detail.h"
#include "simd_mask.h"
#include "flags.h"
#include "simd_iterator.h"
#include "loadstore.h"
#include <span>

namespace std::datapar
{
  // not supported:
  // - deleted: dctor, dtor, cctor, cassign
  // - no members except value_type, abi_type, and mask_type
  template <typename _Tp, typename _Abi>
    requires (not __detail::__vectorizable<_Tp> or
              __detail::_SimdTraits<__detail::__canonical_vec_type_t<_Tp>, _Abi>::_S_size == 0)
    class basic_simd<_Tp, _Abi>
    {
    public:
      using value_type = _Tp;

      using abi_type = _Abi;

      using mask_type = basic_simd_mask<sizeof(conditional_t<is_void_v<_Tp>, int, _Tp>), _Abi>;

      basic_simd() = delete;

      ~basic_simd() = delete;

      basic_simd(const basic_simd&) = delete;

      basic_simd& operator=(const basic_simd&) = delete;
    };

  // --------------------------------------------------------------
  // supported
  template <typename _Tp, typename _Abi>
    class basic_simd
    {
      static_assert(__detail::__vectorizable<_Tp> and __detail::__valid_abi_tag<_Abi, _Tp>);

      using _Tcanon = __detail::__canonical_vec_type_t<_Tp>;
      using _Traits = __detail::_SimdTraits<_Tcanon, _Abi>;

      static constexpr __detail::__canonical_vec_type_t<_Tp>* _S_type_tag = nullptr;

    public:
      using _MemberType = typename _Traits::_SimdMember;

      alignas(_Traits::_S_simd_align) _MemberType _M_data;

      using _Impl = typename _Traits::_SimdImpl;

      using value_type = _Tp;

      using abi_type = _Abi;

      using mask_type = basic_simd_mask<sizeof(_Tp), _Abi>;

      static constexpr auto size = __detail::__ic<_Traits::_S_size>;

#if SIMD_IS_A_RANGE
      using iterator = __iterator<basic_simd>;
      using const_iterator = __iterator<const basic_simd>;

      //static_assert(std::random_access_iterator<iterator>);
      //static_assert(std::sentinel_for<std::default_sentinel_t, iterator>);

      constexpr iterator
      begin()
      { return iterator(*this, 0); }

      constexpr const_iterator
      begin() const
      { return const_iterator(*this, 0); }

      constexpr const_iterator
      cbegin() const
      { return const_iterator(*this, 0); }

      constexpr std::default_sentinel_t
      end() const
      { return {}; }

      constexpr std::default_sentinel_t
      cend() const
      { return {}; }
#endif

      constexpr
      basic_simd() = default;

      // ABI-specific conversions
      template <typename _Up>
        requires requires { _Traits::template _S_simd_conversion<_Up>(_M_data); }
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        operator _Up() const
        { return _Traits::template _S_simd_conversion<_Up>(_M_data); }

      template <typename _Up>
        requires (_Traits::template _S_is_simd_ctor_arg<_Up>)
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        basic_simd(_Up __x)
        : _M_data(_Traits::_S_simd_construction(__x))
        {}

      // implicit broadcast constructor
      template <typename _Up>
        requires constructible_from<value_type, _Up>
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        explicit(not __detail::__broadcast_constructible<_Up, value_type>)
        basic_simd(_Up&& __x) noexcept
        : _M_data(_Impl::_S_broadcast(_Tcanon(static_cast<_Up&&>(__x))))
        {}

      template <__detail::__value_preserving_convertible_to<value_type> _U0,
                same_as<_U0> _U1, same_as<_U0>... _Us>
        requires (size.value == 2 + sizeof...(_Us))
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        basic_simd(_U0 __x0, _U1 __x1, _Us... __xs)
        : basic_simd(array<value_type, size.value>{__x0, __x1, __xs...})
        {}

      // type conversion constructor
      template <typename _Up, typename _UAbi>
        requires(__detail::__simd_size_v<_Up, _UAbi> == size()
#if SIMD_STD_BYTE
                   and (std::constructible_from<_Tp, _Up>
                          or ((std::is_enum_v<_Tp> or std::is_enum_v<_Up>)
                                 and requires(_Tp __x, _Up __y) { __x = _Tp(__y); }))
#else
                   and std::constructible_from<_Tp, _Up>
#endif
                )
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        explicit(not __detail::__value_preserving_convertible_to<_Up, value_type>
                   || __detail::__higher_rank_than<_Up, value_type>)
        basic_simd(const basic_simd<_Up, _UAbi>& __x) noexcept
        : _M_data(__detail::_SimdConverter<__detail::__canonical_vec_type_t<_Up>, _UAbi,
                                           _Tcanon, _Abi>()(__data(__x)))
        {}

      // generator constructor
      template <__detail::__simd_generator_invokable<value_type, size()> _Fp>
        constexpr explicit
        basic_simd(_Fp&& __gen)
        : _M_data(_Impl::template _S_generator<_Tcanon>(static_cast<_Fp&&>(__gen)))
        {}

      template <__detail::__almost_simd_generator_invokable<value_type, size()> _Fp>
        constexpr explicit
        basic_simd(_Fp&& )
          = _GLIBCXX_DELETE_MSG("Invalid return type of the generator function: "
                                "Requires value-preserving conversion or implicitly "
                                "convertible user-defined type.");

      // ranges typically don't have a static size
      // but if one does, this ctor is useful (std::array, C-array, span of static extent)
      //
      // Note that simd::simd is not a match, because it's not a contiguous range. Thus, if the
      // constraint were to be relaxed to a random-access range, I'd expect ambiguities with the
      // conversion constructor.
      template <__detail::__static_sized_range<size.value> _Rg, typename... _Flags>
        constexpr // implicit!
        basic_simd(_Rg&& __range, flags<_Flags...> __flags = {})
        : _M_data(_Impl::_S_load(__flags.template _S_adjust_pointer<basic_simd>(
                                   std::ranges::data(__range)), _S_type_tag))
        {
          static_assert(__detail::__loadstore_convertible_to<std::ranges::range_value_t<_Rg>,
                                                             value_type, _Flags...>);
        }

#if RANGES_TO_SIMD
      // optimize the contiguous_range case
      template <std::ranges::contiguous_range _Rg, typename... _Flags>
        requires std::ranges::sized_range<_Rg>
        constexpr explicit
        basic_simd(std::from_range_t, _Rg&& __range, flags<_Flags...> __flags = {})
        : _M_data(__data(std::load<basic_simd>(__range, __flags)))
        {
          static_assert(__detail::__loadstore_convertible_to<std::ranges::range_value_t<_Rg>,
                                                             value_type, _Flags...>);
          __glibcxx_simd_precondition(std::ranges::size(__range) <= unsigned(size),
                                      "Input range is too large. "
                                      "Consider using std::views::take(N) or something similar "
                                      "for reducing the size of the input.");
        }

      // support non-contiguous_range as well
      template <std::ranges::input_range _Rg, typename... _Flags>
        requires __detail::__loadstore_convertible_to<std::ranges::range_value_t<_Rg>,
                                                      value_type, _Flags...>
        constexpr explicit
        basic_simd(std::from_range_t, _Rg&& __range, flags<_Flags...> __flags = {})
        : _M_data(_Impl::template _S_generator<_Tcanon>(
                    [&__range, __it = std::ranges::begin(__range)] (int __i) mutable {
                      __glibcxx_simd_precondition(__it != std::ranges::end(__range),
                                                  "Input range is too small.");
                      auto __r = static_cast<value_type>(*__it++);
                      __glibcxx_simd_precondition(__i + 1 < size
                                                    or __it == std::ranges::end(__range),
                                      "Input range is too large. "
                                      "Consider using std::views::take(N) or something similar "
                                      "for reducing the size of the input.");
                      return __r;
                    }))
        {}

      // and give a better error message when the user might have expected `ranges::to` to work
      template <std::ranges::range _Rg, typename... _Flags>
        basic_simd(std::from_range_t, _Rg&&, flags<_Flags...> = {})
        : _M_data{}
        {
          static_assert(false, "'ranges::to<basic_simd>()' requires a value-preserving conversion. "
                               "Call 'ranges::to<basic_simd>(flag_convert)' to allow all "
                               "implicit conversions.");
        }
#endif

      // private init
      _GLIBCXX_SIMD_INTRINSIC constexpr
      basic_simd(__detail::_PrivateInit, const _MemberType& __init)
      : _M_data(__init)
      {}

      // unary operators (for any _Tp)
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr mask_type
      operator!() const
      requires requires(value_type __a) { {!__a} -> same_as<bool>; }
      { return {__detail::__private_init, _Impl::_S_negate(__data(*this))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd
      operator+() const
      requires requires(value_type __a) { +__a; }
      { return *this; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd
      operator-() const
      requires requires(value_type __a) { -__a; }
      { return {__detail::__private_init, _Impl::_S_unary_minus(__data(*this))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd
      operator~() const
      requires requires(value_type __a) { ~__a; }
      { return {__detail::__private_init, _Impl::_S_complement(__data(*this))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd&
      operator++()
      requires requires(value_type __a) { ++__a; }
      {
        _Impl::_S_increment(_M_data);
        return *this;
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd
      operator++(int)
      requires requires(value_type __a) { __a++; }
      {
        basic_simd __r = *this;
        _Impl::_S_increment(_M_data);
        return __r;
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd&
      operator--()
      requires requires(value_type __a) { --__a; }
      {
        _Impl::_S_decrement(_M_data);
        return *this;
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd
      operator--(int)
      requires requires(value_type __a) { __a--; }
      {
        basic_simd __r = *this;
        _Impl::_S_decrement(_M_data);
        return __r;
      }

      // compound assignment [basic_simd.cassign]
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator+=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a + __b; }
      { return __lhs = __lhs + __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator-=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a - __b; }
      { return __lhs = __lhs - __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator*=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a * __b; }
      { return __lhs = __lhs * __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator/=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a / __b; }
      { return __lhs = __lhs / __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator%=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a % __b; }
      { return __lhs = __lhs % __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator&=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a & __b; }
      { return __lhs = __lhs & __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator|=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a | __b; }
      { return __lhs = __lhs | __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator^=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a ^ __b; }
      { return __lhs = __lhs ^ __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator<<=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a << __b; }
      { return __lhs = __lhs << __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator>>=(basic_simd& __lhs, const basic_simd& __x)
      requires requires(value_type __a, value_type __b) { __a >> __b; }
      { return __lhs = __lhs >> __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator<<=(basic_simd& __lhs, int __x)
      requires requires(value_type __a, int __b) { __a << __b; }
      { return __lhs = __lhs << __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator>>=(basic_simd& __lhs, int __x)
      requires requires(value_type __a, int __b) { __a >> __b; }
      { return __lhs = __lhs >> __x; }

      // binary operators [basic_simd.binary]
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator+(const basic_simd& __x, const basic_simd& __y)
      requires requires(value_type __a) { __a + __a; }
      { return {__detail::__private_init, _Impl::_S_plus(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator-(const basic_simd& __x, const basic_simd& __y)
      requires requires(value_type __a) { __a - __a; }
      { return {__detail::__private_init, _Impl::_S_minus(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator*(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a * __a; }
      { return {__detail::__private_init, _Impl::_S_multiplies(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator/(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a / __a; }
      { return {__detail::__private_init, _Impl::_S_divides(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator%(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a % __a; }
      { return {__detail::__private_init, _Impl::_S_modulus(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator&(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a & __a; }
      { return {__detail::__private_init, _Impl::_S_bit_and(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator|(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a | __a; }
      { return {__detail::__private_init, _Impl::_S_bit_or(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator^(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a ^ __a; }
      { return {__detail::__private_init, _Impl::_S_bit_xor(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator>>(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a >> __a; }
      {
        __glibcxx_simd_precondition(is_unsigned_v<value_type> or all_of(__y >= value_type()),
                                    "negative shift is undefined behavior");
        __glibcxx_simd_precondition(
          all_of(__y < value_type(std::max(sizeof(int), sizeof(value_type)) * __CHAR_BIT__)),
          "too large shift invokes undefined behavior");
        return {__detail::__private_init, _Impl::_S_bit_shift_right(__data(__x), __data(__y))};
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator<<(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a << __a; }
      {
        __glibcxx_simd_precondition(is_unsigned_v<value_type> or all_of(__y >= value_type()),
                                    "negative shift is undefined behavior");
        __glibcxx_simd_precondition(
          all_of(__y < value_type(std::max(sizeof(int), sizeof(value_type)) * __CHAR_BIT__)),
          "too large shift invokes undefined behavior");
        return {__detail::__private_init, _Impl::_S_bit_shift_left(__data(__x), __data(__y))};
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator>>(const basic_simd& __x, int __y)
      requires requires (value_type __a, int __b) { __a >> __b; }
      {
        __glibcxx_simd_precondition(__y >= 0, "negative shift is undefined behavior");
        __glibcxx_simd_precondition(
          __y < int(std::max(sizeof(int), sizeof(value_type)) * __CHAR_BIT__),
          "too large shift invokes undefined behavior");
        return {__detail::__private_init, _Impl::_S_bit_shift_right(__data(__x), __y)};
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator<<(const basic_simd& __x, int __y)
      requires requires (value_type __a, int __b) { __a << __b; }
      {
        __glibcxx_simd_precondition(__y >= 0, "negative shift is undefined behavior");
        __glibcxx_simd_precondition(
          __y < int(std::max(sizeof(int), sizeof(value_type)) * __CHAR_BIT__),
          "too large shift invokes undefined behavior");
        return {__detail::__private_init, _Impl::_S_bit_shift_left(__data(__x), __y)};
      }

      // compares [basic_simd.comparison]
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend mask_type
      operator==(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a == __a; }
      { return {__detail::__private_init, _Impl::_S_equal_to(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend mask_type
      operator!=(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a != __a; }
      { return {__detail::__private_init, _Impl::_S_not_equal_to(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend mask_type
      operator<(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a < __a; }
      { return {__detail::__private_init, _Impl::_S_less(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend mask_type
      operator<=(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a <= __a; }
      { return {__detail::__private_init, _Impl::_S_less_equal(__data(__x), __data(__y))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend mask_type
      operator>(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a > __a; }
      { return {__detail::__private_init, _Impl::_S_less(__data(__y), __data(__x))}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend mask_type
      operator>=(const basic_simd& __x, const basic_simd& __y)
      requires requires (value_type __a) { __a >= __a; }
      { return {__detail::__private_init, _Impl::_S_less_equal(__data(__y), __data(__x))}; }

      constexpr std::array<_Tp, size()>
      _M_to_array() const noexcept
      {
        std::array<_Tp, size()> __r = {};
        std::datapar::unchecked_store(*this, __r);
        return __r;
      }

        //explicit
      operator std::array<_Tp, size()>() const noexcept
      { return _M_to_array(); }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd
      __select_impl(const mask_type& __k, const basic_simd& __t, const basic_simd& __f)
      {
        auto __ret = __f;
        _Impl::_S_masked_assign(__data(__k), __ret._M_data, __t._M_data);
        return __ret;
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr value_type
      operator[](__detail::_SimdSizeType __i) const
      {
        __glibcxx_simd_precondition(__i >= 0, "error: negative index");
        __glibcxx_simd_precondition(__i < size.value, "error: index out of bounds");
        return static_cast<value_type>(_Impl::_S_get(_M_data, __i));
      }

#if SIMD_HAS_SUBSCRIPT_GATHER
      template <std::integral _Up, typename _Ap>
        constexpr
        resize_t<__detail::__simd_size_v<_Up, _Ap>, basic_simd>
        operator[](basic_simd<_Up, _Ap> const& __idx) const
        {
          __glibcxx_simd_precondition(is_unsigned_v<_Up> or all_of(__idx >= 0), "out-of-bounds");
          __glibcxx_simd_precondition(all_of(__idx < _Up(size)), "out-of-bounds");
          using _Rp = resize_t<__detail::__simd_size_v<_Up, _Ap>, basic_simd>;
          return _Rp(__detail::__private_init,
                     _Rp::_Impl::template _S_generator<_Tcanon>([&](int __i) {
                       return _Impl::_S_get(_M_data, __idx[__i]);
                     }));
        }
#endif

      friend constexpr const auto& __data(const basic_simd& __x)
      { return __x._M_data; }

      friend constexpr auto& __data(basic_simd& __x)
      { return __x._M_data; }

      _GLIBCXX_SIMD_INTRINSIC constexpr bool
      _M_is_constprop() const
      {
        if constexpr (requires {_Impl::_S_is_constprop(_M_data);})
          return _Impl::_S_is_constprop(_M_data);
        else if constexpr (requires {_M_data._M_is_constprop();})
          return _M_data._M_is_constprop();
        else
          return __builtin_constant_p(_M_data);
      }
    };

  template <__detail::__static_sized_range _Rg, typename... _Other>
    basic_simd(_Rg&& __r, _Other...)
      -> basic_simd<ranges::range_value_t<_Rg>,
                   __detail::__deduce_t<ranges::range_value_t<_Rg>,
#if 0 // PR117849
                                        ranges::size(__r)>>;
#else
                                        decltype(std::span(__r))::extent>>;
#endif

#if RANGES_TO_SIMD
    template <std::ranges::input_range _Rg>
    basic_simd(std::from_range_t, _Rg&& x)
    -> basic_simd<std::ranges::range_value_t<_Rg>>;

  template <std::ranges::input_range _Rg, typename... _Flags>
    basic_simd(std::from_range_t, _Rg&& x, flags<_Flags...>)
    -> basic_simd<std::ranges::range_value_t<_Rg>>;
#endif

  template <size_t _Bs, typename _Abi>
    basic_simd(basic_simd_mask<_Bs, _Abi>)
    -> basic_simd<__detail::__mask_integer_from<_Bs>,
                  __detail::__simd_abi_for_mask_t<_Bs, _Abi>>;

  template <__detail::__vectorizable _Tp, __detail::__vectorizable _Up,
            __detail::__valid_abi_tag<_Up> _Abi>
    struct rebind<_Tp, basic_simd<_Up, _Abi>>
    {
      using _Abi1 = typename _Abi::template _Rebind<__detail::__canonical_vec_type_t<_Tp>>;
      using type = basic_simd<_Tp, _Abi1>;
    };

  template <__detail::__vectorizable _Tp, size_t _Bytes,
            __detail::__valid_abi_tag<__detail::__mask_integer_from<_Bytes>> _Abi>
    struct rebind<_Tp, basic_simd_mask<_Bytes, _Abi>>
    {
      using _Abi1 = typename _Abi::template _Rebind<__detail::__canonical_vec_type_t<_Tp>>;
      using type = basic_simd_mask<_Bytes, _Abi1>;
    };

  template <__detail::_SimdSizeType _Np, __detail::__vectorizable _Up,
            __detail::__valid_abi_tag<_Up> _Abi>
    struct resize<_Np, basic_simd<_Up, _Abi>>
    {
      using _Abi1 = typename _Abi::template _Rebind<_Up, _Np>;
      using type = basic_simd<_Up, _Abi1>;
    };

  template <__detail::_SimdSizeType _Np, size_t _Bytes,
            __detail::__valid_abi_tag<__detail::__mask_integer_from<_Bytes>> _Abi>
    struct resize<_Np, basic_simd_mask<_Bytes, _Abi>>
    {
      using _Up = __detail::__mask_integer_from<_Bytes>;
      using _Abi1 = typename _Abi::template _Rebind<_Up, _Np>;
      using type = basic_simd_mask<_Bytes, _Abi1>;
    };

  template <typename _Tp, typename _Abi, __detail::__vectorizable _Up>
    struct alignment<basic_simd<_Tp, _Abi>, _Up>
    : std::integral_constant<size_t, alignof(rebind_t<_Up, basic_simd<_Tp, _Abi>>)>
    {};

  template <size_t _Bs, typename _Abi>
    struct alignment<basic_simd_mask<_Bs, _Abi>, bool>
    : std::integral_constant<size_t, alignof(simd<__detail::__make_unsigned_int_t<bool>,
                                                  basic_simd_mask<_Bs, _Abi>::size()>)>
    {};
}

namespace std::__detail
{
  template <typename _Tp, typename _Abi>
    struct __is_simd<std::datapar::basic_simd<_Tp, _Abi>>
    : is_default_constructible<std::datapar::basic_simd<_Tp, _Abi>>
    {};
}


#endif  // PROTOTYPE_SIMD2_H_
// vim: et ts=8 sw=2 tw=100 cc=101
