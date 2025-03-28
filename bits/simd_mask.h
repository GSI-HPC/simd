/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_MASK2_H_
#define PROTOTYPE_SIMD_MASK2_H_

#include "detail.h"
#include "simd_abi.h"
#include "simd_iterator.h"

#include <concepts>
#include <climits>

namespace std::__detail
{
  // Deducing the right ABI tag for basic_simd_mask -> basic_simd is tricky for the AVX w/o AVX2
  // case, where basic_simd<T, Abi> might be unusable and we therefore need to deduce another ABI
  // tag.
  template <size_t _Bytes, typename _Abi>
    struct __simd_abi_for_mask
    { using type = __deduce_t<__mask_integer_from<_Bytes>, _Abi::_S_size>; };

  template <size_t _Bytes, typename _Abi>
    requires (std::destructible<std::datapar::basic_simd<__mask_integer_from<_Bytes>, _Abi>>
                or not __simd_abi_tag<_Abi>)
    struct __simd_abi_for_mask<_Bytes, _Abi>
    { using type = _Abi; };

  template <size_t _Bytes, typename _Abi>
    using __simd_abi_for_mask_t = typename __simd_abi_for_mask<_Bytes, _Abi>::type;
}

namespace std::datapar
{
  // not supported:
  // - deleted: dctor, dtor, cctor, cassign
  // - no members except value_type and abi_type
  template <size_t _Bytes, typename _Abi>
    requires (__detail::_SimdMaskTraits<_Bytes, _Abi>::_S_size == 0)
    class basic_simd_mask<_Bytes, _Abi>
    {
    public:
      using value_type = bool;

      using abi_type = _Abi;

#define _GLIBCXX_DELETE_SIMD_MASK                                              \
  delete("This specialization is disabled because of an invalid combination "  \
         "of template arguments to basic_simd_mask.")

      basic_simd_mask() = _GLIBCXX_DELETE_SIMD_MASK;

      ~basic_simd_mask() = _GLIBCXX_DELETE_SIMD_MASK;

      basic_simd_mask(const basic_simd_mask&) = _GLIBCXX_DELETE_SIMD_MASK;

      basic_simd_mask& operator=(const basic_simd_mask&) = _GLIBCXX_DELETE_SIMD_MASK;

#undef _GLIBCXX_DELETE_SIMD_MASK
    };

  template <size_t _Bytes, typename _Abi>
    class basic_simd_mask
    {
      using _Tp = __detail::__mask_integer_from<_Bytes>;

      using _Traits = __detail::_SimdMaskTraits<_Bytes, _Abi>;

      using _MemberType = typename _Traits::_MaskMember;

      using _SimdType = basic_simd<_Tp, __detail::__simd_abi_for_mask_t<_Bytes, _Abi>>;

    public:
      // the only non-static data member
      alignas(_Traits::_S_mask_align) _MemberType _M_data;

      using _Impl = typename _Traits::_MaskImpl;

      static constexpr bool _S_is_bitmask = sizeof(_MemberType) < _Traits::_S_size;

      // really public:

      using value_type = bool;

      using abi_type = _Abi;

      static constexpr auto size = __detail::__ic<_Traits::_S_size>;

#if SIMD_IS_A_RANGE
      using iterator = __iterator<basic_simd_mask>;
      using const_iterator = __iterator<const basic_simd_mask>;

      static_assert(std::random_access_iterator<iterator>);
      static_assert(std::sentinel_for<std::default_sentinel_t, iterator>);

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
      basic_simd_mask() noexcept = default;

      // suggested extension [simd.mask.overview] p4
      // ABI-specific conversions
      template <typename _Up>
        requires requires { _Traits::template _S_mask_conversion<_Up>(_M_data); }
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        operator _Up() const
        { return _Traits::template _S_mask_conversion<_Up>(_M_data); }

      template <typename _Up>
        requires (_Traits::template _S_is_mask_ctor_arg<_Up>)
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        basic_simd_mask(_Up __x)
        : _M_data(_Traits::_S_mask_construction(__x))
        {}

      // private init (implementation detail)
      _GLIBCXX_SIMD_INTRINSIC constexpr
      basic_simd_mask(__detail::_PrivateInit, const _MemberType& __init) noexcept
      : _M_data(__init)
      {}

      // TODO bitset conversions

      // broadcast ctor
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr explicit
      basic_simd_mask(value_type __x) noexcept
      : _M_data(_Impl::template _S_mask_broadcast<_Tp>(__x))
      {}

      // conversion ctor
      template <size_t _UBytes, class _UAbi>
        requires(size() == basic_simd_mask<_UBytes, _UAbi>::size.value)
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        // TODO: new paper? implicit conversion if *only* the ABI tag differs:
        // equal size and equal sizeof (_Bytes), but different ABI tag
        // implicit conversion in one direction, but not the other (avoid interconvertible types)
        explicit(_UBytes != _Bytes or _Traits::template _S_explicit_mask_conversion<_UAbi>)
        basic_simd_mask(const basic_simd_mask<_UBytes, _UAbi>& __x) noexcept
        : basic_simd_mask(__detail::__private_init, _Impl::template _S_convert<_Tp>(__x))
        {}

      // generator ctor
      template <__detail::__simd_generator_invokable<value_type, size.value> _Fp>
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr explicit
        basic_simd_mask(_Fp&& __gen)
        : _M_data([&]<__detail::_SimdSizeType... _Is>(__detail::_SimdIndexSequence<_Is...>) {
            if constexpr (requires {_Impl::template _S_mask_generator<_Tp>(__gen);})
              return _Impl::template _S_mask_generator<_Tp>(__gen);
            else if constexpr (size.value == 1)
              return bool(__gen(__detail::__ic<0>));
            else
              {
                static_assert(not _S_is_bitmask);
                return _MemberType { _Tp(-bool(__gen(__detail::__ic<_Is>)))... };
              }
          }(__detail::_MakeSimdIndexSequence<size()>()))
        {}

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr value_type
      operator[](__detail::_SimdSizeType __i) const
      {
        __glibcxx_simd_precondition(__i >= 0, "error: negative index");
        __glibcxx_simd_precondition(__i < size.value, "error: index out of bounds");
        return _Impl::_S_get(_M_data, __i);
      }

#if SIMD_HAS_SUBSCRIPT_GATHER
      template <std::integral _Up, typename _Ap>
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
        resize_t<__detail::__simd_size_v<_Up, _Ap>, basic_simd_mask>
        operator[](basic_simd<_Up, _Ap> const& __idx) const
        {
          __glibcxx_simd_precondition(is_unsigned_v<_Up> or all_of(__idx >= 0), "out-of-bounds");
          __glibcxx_simd_precondition(all_of(__idx < _Up(size)), "out-of-bounds");
          using _Rp = resize_t<__detail::__simd_size_v<_Up, _Ap>, basic_simd_mask>;
          return _Rp([&](int __i) {
                   return _Impl::_S_get(_M_data, __idx[__i]);
                 }));
        }
#endif

      // [simd.mask.unary]
      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr basic_simd_mask
      operator!() const noexcept
      { return {__detail::__private_init, _Impl::_S_bit_not(_M_data)}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr _SimdType
      operator+() const noexcept
      { return operator _SimdType(); }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr _SimdType
      operator-() const noexcept
      {
        if constexpr (__detail::__vec_builtin<_MemberType> and sizeof(_M_data) == sizeof(_SimdType))
          return std::bit_cast<_SimdType>(_M_data);
        else
          {
            _SimdType __r = {};
            _SimdType::_Impl::_S_masked_assign(_M_data, __data(__r), _Tp(-1));
            return __r;
          }
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE constexpr _SimdType
      operator~() const noexcept
      {
        if constexpr (sizeof(basic_simd_mask) == sizeof(_SimdType))
          return std::bit_cast<_SimdType>(*this) - _Tp(1);
        else
          {
            _SimdType __r = _Tp(-1);
            _SimdType::_Impl::_S_masked_assign(_M_data, __data(__r), _Tp(-2));
            return __r;
          }
      }

      // [simd.mask.conv]
      template <typename _Up, typename _UAbi>
        requires (__detail::__simd_size_v<_Up, _UAbi> == size.value)
        _GLIBCXX_SIMD_ALWAYS_INLINE constexpr explicit(sizeof(_Up) != _Bytes)
        operator basic_simd<_Up, _UAbi>() const noexcept
        {
          using _Rp = basic_simd<_Up, _UAbi>;
          _Rp __r {};
          _Rp::_Impl::_S_masked_assign(__data(*this), __data(__r), 1);
          return __r;
        }

      // [simd.mask.binary]
      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator&&(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return {__detail::__private_init, _Impl::_S_logical_and(__x._M_data, __y._M_data)}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator||(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return {__detail::__private_init, _Impl::_S_logical_or(__x._M_data, __y._M_data)}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator&(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return {__detail::__private_init, _Impl::_S_bit_and(__x._M_data, __y._M_data)}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator|(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return {__detail::__private_init, _Impl::_S_bit_or(__x._M_data, __y._M_data)}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator^(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return {__detail::__private_init, _Impl::_S_bit_xor(__x._M_data, __y._M_data)}; }

      // [simd.mask.cassign]
      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask&
      operator&=(basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      {
        __x._M_data = _Impl::_S_bit_and(__x._M_data, __y._M_data);
        return __x;
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask&
      operator|=(basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      {
        __x._M_data = _Impl::_S_bit_or(__x._M_data, __y._M_data);
        return __x;
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask&
      operator^=(basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      {
        __x._M_data = _Impl::_S_bit_xor(__x._M_data, __y._M_data);
        return __x;
      }

      // [simd.mask.comparison]
      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator==(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      {
        return {__detail::__private_init,
                _Impl::_S_bit_not(_Impl::_S_bit_xor(__x._M_data, __y._M_data))};
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator!=(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return {__detail::__private_init, _Impl::_S_bit_xor(__x._M_data, __y._M_data)}; }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator>=(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return __x || !__y; }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator<=(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return !__x || __y; }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator>(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return __x && !__y; }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      operator<(const basic_simd_mask& __x, const basic_simd_mask& __y) noexcept
      { return !__x && __y; }

      // [simd.mask.cond]
      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      __select_impl(const basic_simd_mask& __k, const basic_simd_mask& __t,
                       const basic_simd_mask& __f) noexcept
      {
        basic_simd_mask __ret = __f;
        _Impl::_S_masked_assign(__k._M_data, __ret._M_data, __t._M_data);
        return __ret;
      }

      _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr basic_simd_mask
      __select_impl(const basic_simd_mask& __k, same_as<bool> auto __t,
                       same_as<bool> auto __f) noexcept
      {
        if (__t == __f)
          return basic_simd_mask(__t);
        else if (__t)
          return __k;
        else
          return !__k;
      }

      template <typename _T0, typename _T1>
        requires (__detail::__vectorizable<__detail::__nopromot_common_type_t<_T0, _T1>>
                    and sizeof(__detail::__nopromot_common_type_t<_T0, _T1>) == _Bytes
                    and convertible_to<_T0, simd<__detail::__nopromot_common_type_t<_T0, _T1>,
                                                size.value>>
                    and convertible_to<_T1, simd<__detail::__nopromot_common_type_t<_T0, _T1>,
                                                size.value>>)
        _GLIBCXX_SIMD_ALWAYS_INLINE friend constexpr
        simd<__detail::__nopromot_common_type_t<_T0, _T1>, size.value>
        __select_impl(const basic_simd_mask& __k, const _T0& __t, const _T1& __f) noexcept
        {
          using _Rp = __detail::__nopromot_common_type_t<_T0, _T1>;
          using _RV = simd<_Rp, size.value>;
          _RV __ret = __f;
          _RV::_Impl::_S_masked_assign(__data(__k), __data(__ret), _Rp(__t));
          return __ret;
        }

      _GLIBCXX_SIMD_INTRINSIC friend constexpr const auto&
      __data(const basic_simd_mask& __x)
      { return __x._M_data; }

      _GLIBCXX_SIMD_INTRINSIC friend constexpr auto&
      __data(basic_simd_mask& __x)
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
}

namespace std::__detail
{
  template <size_t _Bs, typename _Abi>
    struct __is_mask<std::datapar::basic_simd_mask<_Bs, _Abi>>
    : is_default_constructible<std::datapar::basic_simd_mask<_Bs, _Abi>>
    {};
}

#endif  // PROTOTYPE_SIMD_MASK2_H_
// vim: et ts=8 sw=2 tw=100 cc=101
