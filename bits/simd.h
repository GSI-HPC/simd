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
    class basic_simd
    {
    public:
      using value_type = _Tp;

      using abi_type = _Abi;

      using mask_type = basic_simd_mask<sizeof(conditional_t<is_void_v<_Tp>, int, _Tp>), _Abi>;

#define _GLIBCXX_DELETE_SIMD                                                                    \
      _GLIBCXX_DELETE_MSG("This specialization is disabled because of an invalid combination "  \
          "of template arguments to basic_simd.")

      basic_simd() = _GLIBCXX_DELETE_SIMD;

      ~basic_simd() = _GLIBCXX_DELETE_SIMD;

      basic_simd(const basic_simd&) = _GLIBCXX_DELETE_SIMD;

      basic_simd& operator=(const basic_simd&) = _GLIBCXX_DELETE_SIMD;

#undef _GLIBCXX_DELETE_SIMD
    };

  // --------------------------------------------------------------
  // supported, but not complex
  template <__detail::__vectorizable _Tp, typename _Abi>
    requires (not __detail::__complex_like<_Tp>)
      and (__detail::_SimdTraits<__detail::__canonical_vec_type_t<_Tp>, _Abi>::_S_size > 0)
      and (not is_void_v<typename __detail::_SimdTraits<__detail::__canonical_vec_type_t<_Tp>,
                                                        _Abi>::_SimdMember>)
      and (not is_same_v<_Tp, __detail::_MaskUInt128>) // ABI tags allow it for masks
    class basic_simd<_Tp, _Abi>
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

      _GLIBCXX_SIMD_ALWAYS_INLINE static constexpr basic_simd
      _S_load(const auto* __ptr)
      { return {__detail::__private_init, _Impl::_S_load(__ptr, _S_type_tag)}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE static constexpr basic_simd
      _S_partial_load(const auto* __ptr, auto __rg_size)
      { return {__detail::__private_init, _Impl::_S_partial_load(__ptr, __rg_size, _S_type_tag)}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr void
      _M_store(auto* __ptr) const
      { _Impl::_S_store(_M_data, __ptr, _S_type_tag); }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr void
      _M_partial_store(auto* __ptr, auto __rg_size) const
      { _Impl::_S_partial_store(_M_data, __ptr, __rg_size, _S_type_tag); }

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

      _GLIBCXX_SIMD_INTRINSIC constexpr basic_simd
      _M_swap_neighbors() const noexcept
      {
        static_assert((size.value & 1) == 0);
        return {__detail::__private_init, _Impl::_S_swap_neighbors(_M_data)};
      }

      template <typename _Up = value_type>
        _GLIBCXX_SIMD_INTRINSIC constexpr
        basic_simd<_Up, typename abi_type::template _Rebind<_Up, size.value / 2>>
        _M_hadd() const noexcept
        {
          using namespace std::__detail;
          static_assert((size.value & 1) == 0);
          if constexpr (size.value == 2)
            return _M_data[0] + _M_data[1];
          else if constexpr (__vec_builtin<_MemberType>)
            return {__private_init,
                    __vec_extract_part<0, 2>(_VecOps<_MemberType>::_S_hadd(_M_data, _M_data))};
          else if constexpr (__array_type<_MemberType, 2>)
            return {__private_init,
                    _VecOps<typename _MemberType::value_type>::_S_hadd(_M_data[0], _M_data[1])};
          else if constexpr (__array_type<_MemberType, 4>)
            return {__private_init, array {
              _VecOps<typename _MemberType::value_type>::_S_hadd(_M_data[0], _M_data[1]),
              _VecOps<typename _MemberType::value_type>::_S_hadd(_M_data[2], _M_data[3])
            }};
        }
    };

  template <__detail::__complex_like _Tp, typename _Abi>
    requires __detail::__simd_type<basic_simd<typename _Tp::value_type,
                                              typename _Abi::template _Rebind<
                                                typename _Tp::value_type, _Abi::_S_size * 2>>>
      and (_Abi::_S_size > 0)
    class basic_simd<_Tp, _Abi>
    {
      using _T0 = typename _Tp::value_type;

    public:
      using _TSimd = basic_simd<_T0, typename _Abi::template _Rebind<_T0, _Abi::_S_size * 2>>;

      using _Impl = typename _TSimd::_Impl;

      using _RealSimd = basic_simd<_T0, typename _Abi::template _Rebind<_T0, _Abi::_S_size>>;

      _TSimd _M_data = {};

      using value_type = _Tp;

      using abi_type = _Abi;

      using mask_type = basic_simd_mask<sizeof(value_type), _Abi>;

      static constexpr auto size = __detail::__ic<_Abi::_S_size>;

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

      // TODO: implementation-defined conversions?
      // whatever _TSimd provides …

      // broadcast constructor
      template <typename _Up>
        requires constructible_from<value_type, _Up>
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        explicit(not __detail::__broadcast_constructible<_Up, value_type>)
        basic_simd(_Up&& __x) noexcept // _Up is always real (complex specialization is below)
        : _M_data([&](int __i) { return (__i & 1) == 0 ? __x : _T0(); })
        {}

      template <__detail::__complex_like _Up>
        requires constructible_from<value_type, _Up>
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        explicit(not __detail::__broadcast_constructible<_Up, value_type>)
        basic_simd(_Up&& __x) noexcept
        : basic_simd([&](int) { return static_cast<value_type>(__x); })
        {}

      // complex init
      // FIXME: should be _RealSimd instead of deduced _Vp (LWG4230)
      template <__detail::__simd_floating_point<size.value> _Vp>
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        explicit(not __detail::__higher_floating_point_rank_than<_T0, typename _Vp::value_type>)
        basic_simd(const _Vp& __re, const _Vp& __im = {}) noexcept
        : _M_data([&](int __i) { return ((__i & 1) == 0 ? __re : __im)[__i / 2]; })
        {}

      // type conversion constructor
      template <__detail::__complex_like _Up, typename _UAbi>
        requires(__detail::__simd_size_v<_Up, _UAbi> == size.value
                   and std::constructible_from<value_type, _Up>)
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        explicit(not convertible_to<_Up, value_type>)
        basic_simd(const basic_simd<_Up, _UAbi>& __x) noexcept
        : _M_data(__x._M_data)
        {}

      template <typename _Up, typename _UAbi> // _Up is not complex!
        requires (__detail::__simd_size_v<_Up, _UAbi> == size())
          and std::constructible_from<value_type, _Up>
          and (not is_same_v<_T0, _Up>)
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        explicit(not convertible_to<_Up, value_type>)
        basic_simd(const basic_simd<_Up, _UAbi>& __x) noexcept
        : basic_simd(basic_simd<_T0, typename _UAbi::template _Rebind<_T0>>(__x))
        {}

      // generator constructor
      template <__detail::__simd_generator_invokable<value_type, size()> _Fp>
        constexpr explicit
        basic_simd(_Fp&& __gen)
        : _M_data([&] {
            using _Arr = std::array<value_type, sizeof(_TSimd) / sizeof(value_type)>;
            const _Arr __tmp = _GLIBCXX_SIMD_INT_PACK(size.value, _Is, -> _Arr {
                                 return {static_cast<value_type>(__gen(__detail::__ic<_Is>))...};
                               });
            return __builtin_bit_cast(_TSimd, __tmp);
          }())
        {}

      template <__detail::__almost_simd_generator_invokable<value_type, size()> _Fp>
        constexpr explicit
        basic_simd(_Fp&& )
          = _GLIBCXX_DELETE_MSG("Invalid return type of the generator function: "
                                "Requires value-preserving conversion or implicitly "
                                "convertible user-defined type.");

      // conversion from statically sized range
      template <__detail::__static_sized_range<size.value> _Rg, typename... _Flags>
        constexpr // implicit!
        basic_simd(_Rg&& __range, flags<_Flags...> __flags = {})
        : _M_data(span<const ranges::range_value_t<_Rg>, size.value * 2>(
                    reinterpret_cast<const ranges::range_value_t<_Rg>*>(__range.data()),
                    size.value * 2), __flags)
        {
          static_assert(__detail::__loadstore_convertible_to<std::ranges::range_value_t<_Rg>,
                                                             value_type, _Flags...>);
        }

      template <__detail::__static_sized_range<size.value> _Rg, typename... _Flags>
        constexpr // implicit!
        basic_simd(_Rg&& __range, const mask_type& __k, flags<_Flags...> __flags = {})
        : _M_data(span<const ranges::range_value_t<_Rg>, size.value * 2>(
                    reinterpret_cast<const ranges::range_value_t<_Rg>*>(__range.data()),
                    size.value * 2), __builtin_bit_cast(typename _TSimd::mask_type, __k), __flags)
        {
          static_assert(__detail::__loadstore_convertible_to<std::ranges::range_value_t<_Rg>,
                                                             value_type, _Flags...>);
        }

      template <typename _Up>
        _GLIBCXX_SIMD_ALWAYS_INLINE static constexpr basic_simd
        _S_load(const _Up* __ptr)
        {
          if constexpr (__detail::__complex_like<_Up>)
            return {__detail::__private_init,
                    _TSimd::_S_load(reinterpret_cast<const typename _Up::value_type*>(__ptr))};
          else
            return basic_simd(_RealSimd::_S_load(__ptr));
        }

      template <typename _Up>
        _GLIBCXX_SIMD_ALWAYS_INLINE static constexpr basic_simd
        _S_partial_load(const _Up* __ptr, auto __rg_size)
        {
          if constexpr (__detail::__complex_like<_Up>)
            return {
              __detail::__private_init,
              _TSimd::_S_partial_load(reinterpret_cast<const typename _Up::value_type*>(__ptr),
                                      __rg_size * 2)};
          else
            return basic_simd(_RealSimd::_S_partial_load(__ptr, __rg_size));
        }

      template <typename _Up>
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr void
      _M_store(_Up* __ptr) const
      {
        static_assert(__detail::__complex_like<_Up>);
        _M_data._M_store(reinterpret_cast<typename _Up::value_type*>(__ptr));
      }

      template <typename _Up>
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr void
      _M_partial_store(_Up* __ptr, auto __rg_size) const
      {
        static_assert(__detail::__complex_like<_Up>);
        _M_data._M_partial_store(reinterpret_cast<typename _Up::value_type*>(__ptr), __rg_size);
      }

#if RANGES_TO_SIMD
      // optimize the contiguous_range case
      template <std::ranges::contiguous_range _Rg, typename... _Flags>
        requires std::ranges::sized_range<_Rg>
        constexpr explicit
        basic_simd(std::from_range_t, _Rg&& __range, flags<_Flags...> __flags = {})
        : _M_data(std::load<basic_simd>(__range, __flags)._M_data)
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
      basic_simd(__detail::_PrivateInit, const _TSimd& __init)
      : _M_data(__init)
      {}

      _GLIBCXX_SIMD_INTRINSIC constexpr
      basic_simd(__detail::_PrivateInit, const typename _TSimd::_MemberType& __init)
      : _M_data(__detail::__private_init, __init)
      {}

      // unary operators (for any _Tp)
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd
      operator+() const
      { return *this; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd
      operator-() const
      { return {__detail::__private_init, -_M_data}; }

      // compound assignment [basic_simd.cassign]
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator+=(basic_simd& __lhs, const basic_simd& __x)
      { return __lhs = __lhs + __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator-=(basic_simd& __lhs, const basic_simd& __x)
      { return __lhs = __lhs - __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator*=(basic_simd& __lhs, const basic_simd& __x)
      { return __lhs = __lhs * __x; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd&
      operator/=(basic_simd& __lhs, const basic_simd& __x)
      { return __lhs = __lhs / __x; }

      // binary operators [basic_simd.binary]
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator+(const basic_simd& __x, const basic_simd& __y)
      { return {__detail::__private_init, __x._M_data + __y._M_data}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator-(const basic_simd& __x, const basic_simd& __y)
      { return {__detail::__private_init, __x._M_data - __y._M_data}; }

      template <__detail::_BuildFlags _Fl = {}>
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
        operator*(const basic_simd& __x, const basic_simd& __y)
        {
          return {__detail::__private_init,
                  _Impl::_S_complex_multiplies(__x._M_data._M_data, __y._M_data._M_data)};
        }

      template <typename _VV, __detail::_BuildFlags _Fl = {}>
        static constexpr basic_simd
        _S_div(_VV __x, _VV __y)
        {
          using namespace std::__detail;
          if constexpr (_Fl._M_finite_math_only() or not _Fl._M_conforming_to_STDC_annex_G())
            {
              _TSimd __ynorm = __y._M_data * __y._M_data;
              __ynorm += __ynorm._M_swap_neighbors();
              return {__private_init, (__x * __y._M_conj())._M_data / __ynorm};
            }
          else
            {
              // scale each real/imag pair of __y by ...
              using _Up = __make_unsigned_int_t<_T0>;
              using _Ip = make_signed_t<_Up>;
              using _USimd = rebind_t<_Up, _TSimd>;
              using _ISimd = rebind_t<_Ip, _TSimd>;

              const _USimd __xbits = __builtin_bit_cast(_USimd, __x._M_data);
              const _USimd __ybits = __builtin_bit_cast(_USimd, __y._M_data);

              const _USimd __xexp = __xbits & __fp_exponent_mask<_T0>;
              const _USimd __yexp = __ybits & __fp_exponent_mask<_T0>;

              const _USimd __max_xexp = max(__xexp, __xexp._M_swap_neighbors());
              const _USimd __max_yexp = max(__yexp, __yexp._M_swap_neighbors());
              const _USimd __avg_max_exp = (__max_xexp + __max_yexp) >> 1;
              const _USimd __scale = __fp_exponent_bias<_T0> - __avg_max_exp;

              // Now scale all inputs by pow(2, __scale >> dig), effectively cancelling a common
              // 2^n factor from the fraction.
              //
              // Scaling the inputs can make their absolute values both greater and smaller:
              // overflow is not possible, but (gradual) underflow is possible
              //
              // Underflow detection: the resulting exponent bits (including sign bit) are <= 0.
              //
              // FIXME: if one of the inputs is +/-inf or NaN then exponent reduction will turn it
              // into a finite number.

              _USimd __x_scaled_bits = __xbits + __fp_exponent_bias<_T0> - __avg_max_exp;
              _USimd __y_scaled_bits = __ybits + __fp_exponent_bias<_T0> - __avg_max_exp;
              const _USimd __x_adj_exp = __xexp + __fp_exponent_bias<_T0> - __avg_max_exp;
              const _USimd __y_adj_exp = __yexp + __fp_exponent_bias<_T0> - __avg_max_exp;
              const auto __xunderflow = _ISimd(__x_adj_exp) <= _Ip();
              const auto __yunderflow = _ISimd(__y_adj_exp) <= _Ip();
              if (any_of(__xunderflow or __yunderflow)) [[unlikely]] // maybe not? happens for +/-0
                {
                  // turn underflow into +/-0
                  // TODO: implement gradual underflow
                  // first flip the sign back
                  _USimd __x_mask0 = __x_adj_exp & __fp_sign_mask<_T0>;
                  _USimd __y_mask0 = __y_adj_exp & __fp_sign_mask<_T0>;
                  __x_scaled_bits ^= __x_mask0;
                  __y_scaled_bits ^= __y_mask0;
                  // then null non-sign bits
                  constexpr int __n = sizeof(_Ip) * __CHAR_BIT__ - 1;
                  __x_scaled_bits &= ~_USimd(_ISimd(__x_mask0) >> __n) | __fp_sign_mask<_T0>;
                  __y_scaled_bits &= ~_USimd(_ISimd(__y_mask0) >> __n) | __fp_sign_mask<_T0>;

                  // TODO: support bitmasks
                  /*
                  __xscaled._M_data = select(__xunderflow, copysign(_TSimd(), __x._M_data),
                                             __xscaled._M_data);
                  __yscaled._M_data = select(__yunderflow, copysign(_TSimd(), __y._M_data),
                                             __yscaled._M_data);
                   */
                }
              basic_simd __xscaled = __builtin_bit_cast(basic_simd, __x_scaled_bits);
              basic_simd __yscaled = __builtin_bit_cast(basic_simd, __y_scaled_bits);
              if (not all_of(isfinite(__x._M_data) and isfinite(__y._M_data))) [[unlikely]]
                [&] [[gnu::cold]] () {
                  __xscaled._M_data = select(isfinite(__x._M_data), __xscaled._M_data, __x._M_data);
                  __yscaled._M_data = select(isfinite(__y._M_data), __yscaled._M_data, __y._M_data);
                }();

              _TSimd __ynorm = __yscaled._M_data * __yscaled._M_data;
              __ynorm += __ynorm._M_swap_neighbors();

              return {__private_init, (__xscaled * __yscaled._M_conj())._M_data / __ynorm};
            }
        }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend basic_simd
      operator/(const basic_simd& __x, const basic_simd& __y)
      {
        return basic_simd::_S_div<conditional_t<_Abi::_S_pass_by_reference,
                                                const basic_simd&, basic_simd>>(__x, __y);
      }

      // FIXME: should return _RealSimd instead of deduced type (LWG4230)
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
      real() const noexcept
      { return permute<size.value>(_M_data, [](int __i) { return __i * 2; }); }

      // FIXME: should return _RealSimd instead of deduced type (LWG4230)
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
      imag() const noexcept
      { return permute<size.value>(_M_data, [](int __i) { return __i * 2 + 1; }); }

      // FIXME: should be _RealSimd instead of deduced _Vp (LWG4230)
      template <__detail::__simd_floating_point _Vp>
        requires same_as<typename _Vp::value_type, _T0> and (_Vp::size.value == size.value)
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr void
        real(const _Vp& __x) noexcept
        {
          if (_M_is_constprop() and __x._M_is_constprop())
            _M_data = _TSimd([&](int __i) {
                        return (__i & 1) == 0 ? _T0(__x[__i / 2]) : _M_data[__i];
                      });
          else
            _Impl::_S_complex_set_real(_M_data._M_data, __x._M_data);
        }

      // FIXME: should be _RealSimd instead of deduced _Vp (LWG4230)
      template <__detail::__simd_floating_point _Vp>
        requires same_as<typename _Vp::value_type, _T0> and (_Vp::size.value == size.value)
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr void
        imag(const _Vp& __x) noexcept
        {
          if (_M_is_constprop() and __x._M_is_constprop())
            _M_data = _TSimd([&](int __i) {
                        return (__i & 1) == 1 ? _T0(__x[__i / 2]) : _M_data[__i];
                      });
          else
            _Impl::_S_complex_set_imag(_M_data._M_data, __x._M_data);
        }

      // associated functions
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd
      _M_conj() const
      { return {__detail::__private_init, _Impl::_S_complex_conj(_M_data._M_data)}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr _RealSimd
      _M_norm() const
      {
#if 0
        return (_M_data * _M_data)._M_hadd();
#elif 0
        const auto __squared = _M_data * _M_data;
        return permute<size.value>(
                 __squared + __squared._M_swap_neighbors(),
                 [](unsigned __i) { return __i * 2; });
#elif 0
        const auto __squared = _M_data * _M_data;
        return permute<size.value>(__squared, [](int __i) { return __i * 2; })
                 + permute<size.value>(__squared, [](int __i) { return __i * 2 + 1; });
#elif 1
        auto __re = real();
        auto __im = imag();
        return __re * __re + __im * __im;
#endif
      }

      // compares [basic_simd.comparison]
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend mask_type
      operator==(const basic_simd& __x, const basic_simd& __y)
      { return (__x._M_data == __y._M_data).template _M_and_neighbors<mask_type>(); }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr friend mask_type
      operator!=(const basic_simd& __x, const basic_simd& __y)
      { return (__x._M_data != __y._M_data).template _M_or_neighbors<mask_type>(); }

      constexpr std::array<value_type, size.value>
      _M_to_array() const noexcept
      {
        std::array<value_type, size.value> __r = {};
        std::datapar::unchecked_store(*this, __r);
        return __r;
      }

        //explicit
      operator std::array<value_type, size()>() const noexcept
      { return _M_to_array(); }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd
      __select_impl(const mask_type& __k, const basic_simd& __t, const basic_simd& __f)
      {
        // FIXME: the bit_cast is wrong for bit-masks
        if constexpr (sizeof(__k) == sizeof(basic_simd)) // vector masks
          return {__detail::__private_init,
                  __select_impl(__builtin_bit_cast(typename _TSimd::mask_type, __k),
                                __t._M_data, __f._M_data)};
        else
          return {__detail::__private_init,
                  __select_impl(typename _TSimd::mask_type(__detail::__private_init,
                                                           mask_type::_Impl::_S_dup_every_element(
                                                             __k._M_data)),
                                __t._M_data, __f._M_data)};
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr value_type
      operator[](__detail::_SimdSizeType __i) const
      {
        __glibcxx_simd_precondition(__i >= 0, "error: negative index");
        __glibcxx_simd_precondition(__i < size.value, "error: index out of bounds");

        // this struct has the correct sizeof without turning padding bits into objects
        struct _Arr
        { alignas(_TSimd) value_type __data[size.value]; };

        return __builtin_bit_cast(_Arr, _M_data).__data[__i];
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

      _GLIBCXX_SIMD_INTRINSIC constexpr bool
      _M_is_constprop() const
      { return _M_data._M_is_constprop(); }
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
