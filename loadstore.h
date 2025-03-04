/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef SIMD_LOADSTORE_H_
#define SIMD_LOADSTORE_H_

#include "simd_mask.h"
#include "flags.h"

#include <bits/iterator_concepts.h>
#include <bits/ranges_base.h>

namespace std::__detail
{
  template <typename _Vp, typename _Tp>
    struct __simd_load_return;

  template <typename _Tp>
    struct __simd_load_return<void, _Tp>
    { using type = std::datapar::basic_simd<_Tp>; };

  template <typename _Up, typename _Abi, typename _Tp>
    struct __simd_load_return<std::datapar::basic_simd<_Up, _Abi>, _Tp>
    { using type = std::datapar::basic_simd<_Up, _Abi>; };

  template <typename _Vp, typename _Tp>
    using __simd_load_return_t = typename __simd_load_return<_Vp, _Tp>::type;

  template <typename _Tp>
    concept __sized_contiguous_range
      = ranges::contiguous_range<_Tp> and ranges::sized_range<_Tp>;

  template <typename _Vp, typename _Tp>
    using __load_mask_type_t = typename __simd_load_return_t<_Vp, _Tp>::mask_type;
}

namespace std::datapar
{
  template <class _Vp = void, ranges::range _Rg, typename... _Flags>
    requires (not __detail::__sized_contiguous_range<_Rg>)
    constexpr void
    unchecked_load(_Rg&&, flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "unchecked_load(range, flags = {}) requires a contiguous and sized range.");

  template <class _Vp = void, input_or_output_iterator _It, typename... _Flags>
    requires (not contiguous_iterator<_It>)
    constexpr void
    unchecked_load(_It, iter_difference_t<_It>, flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "unchecked_load(first, size, flags = {}) requires a contiguous iterator.");

  template <class _Vp = void, input_or_output_iterator _It, sentinel_for<_It> _Sp,
            typename... _Flags>
    requires (not contiguous_iterator<_It> or not sized_sentinel_for<_It, _Sp>)
    constexpr void
    unchecked_load(_It, _Sp, flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "unchecked_load(first, last, flags = {}) requires a contiguous iterator and a"
          " sized sentinel.");

  template <class _Vp = void, ranges::range _Rg, typename... _Flags>
    requires (not __detail::__sized_contiguous_range<_Rg>)
    constexpr void
    partial_load(_Rg&&, flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "partial_load(range, flags = {}) requires a contiguous and sized range.");

  template <class _Vp = void, input_or_output_iterator _It, typename... _Flags>
    requires (not contiguous_iterator<_It>)
    constexpr void
    partial_load(_It, iter_difference_t<_It>, flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "partial_load(first, size, flags = {}) requires a contiguous iterator.");

  template <class _Vp = void, input_or_output_iterator _It, sentinel_for<_It> _Sp,
            typename... _Flags>
    requires (not contiguous_iterator<_It> or not sized_sentinel_for<_It, _Sp>)
    constexpr void
    partial_load(_It, _Sp, flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "partial_load(first, last, flags = {}) requires a contiguous iterator and a"
          " sized sentinel.");

  template <class _Vp = void, __detail::__sized_contiguous_range _Rg, typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, ranges::range_value_t<_Rg>>
    unchecked_load(_Rg&& __r, flags<_Flags...> __f = {})
    {
      using _RV = __detail::__simd_load_return_t<_Vp, ranges::range_value_t<_Rg>>;
      using _Rp = typename _RV::value_type;
      static_assert(__detail::__loadstore_convertible_to<
                      ranges::range_value_t<_Rg>, _Rp, _Flags...>,
                    "The converting load is not value-preserving. "
                    "Pass 'flag_convert' if lossy conversion matches the intent.");

      constexpr bool __throw_on_out_of_bounds = (is_same_v<_Flags, __detail::_Throw> or ...);
      constexpr bool __allow_out_of_bounds
        = (__throw_on_out_of_bounds or ... or is_same_v<_Flags, __detail::_AllowPartialLoadStore>);
      constexpr auto __static_size = __detail::__static_range_size(__r);

      static_assert(__static_size >= _RV::size.value or __allow_out_of_bounds
                      or __static_size == dynamic_extent, "Out-of-bounds simd load");

      const auto* __ptr = __f.template _S_adjust_pointer<_RV>(ranges::data(__r));
      constexpr __detail::__canonical_vec_type_t<_Rp>* __type_tag = nullptr;

      const auto __rg_size = std::ranges::size(__r);
      if constexpr (not __allow_out_of_bounds)
        __glibcxx_simd_precondition(
          std::ranges::size(__r) >= _RV::size(),
          "Input range is too small. Did you mean to use 'partial_load'?");

      if constexpr (__throw_on_out_of_bounds)
        {
          if (__rg_size < _RV::size())
            throw std::out_of_range("unchecked_load: Input range is too small.");
        }

      if consteval
        {
          return _RV([&](size_t __i) {
                   return __i < __rg_size ? static_cast<_Rp>(__r[__i]) : _Rp();
                 });
        }
      else
        {
          if constexpr ((__static_size != dynamic_extent and __static_size >= _RV::size())
                          or not __allow_out_of_bounds)
            return _RV(__detail::__private_init, _RV::_Impl::_S_load(__ptr, __type_tag));
          else
            return _RV(__detail::__private_init, _RV::_Impl::_S_partial_load(__ptr, __rg_size,
                                                                             __type_tag));
        }
    }

  template <class _Vp = void, contiguous_iterator _It, typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, iter_value_t<_It>>
    unchecked_load(_It __first, iter_difference_t<_It> __n, flags<_Flags...> __f = {})
    { return unchecked_load<_Vp>(span<const iter_value_t<_It>>(__first, __n), __f); }

  template <class _Vp = void, contiguous_iterator _It, sized_sentinel_for<_It> _Sp,
            typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, iter_value_t<_It>>
    unchecked_load(_It __first, _Sp __last, flags<_Flags...> __f = {})
    { return unchecked_load<_Vp>(span<const iter_value_t<_It>>(__first, __last), __f); }

  template <class _Vp = void, __detail::__sized_contiguous_range _Rg, typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, ranges::range_value_t<_Rg>>
    partial_load(_Rg&& __r, flags<_Flags...> __f = {})
    { return unchecked_load<_Vp>(__r, __f | __allow_partial_loadstore); }

  template <class _Vp = void, contiguous_iterator _It, typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, iter_value_t<_It>>
    partial_load(_It __first, iter_difference_t<_It> __n, flags<_Flags...> __f = {})
    { return partial_load<_Vp>(span<const iter_value_t<_It>>(__first, __n), __f); }

  template <class _Vp = void, contiguous_iterator _It, sized_sentinel_for<_It> _Sp,
            typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, iter_value_t<_It>>
    partial_load(_It __first, _Sp __last, flags<_Flags...> __f = {})
    { return partial_load<_Vp>(span<const iter_value_t<_It>>(__first, __last), __f); }

  // masked loads
  template <class _Vp = void, ranges::range _Rg, typename... _Flags>
    requires (not __detail::__sized_contiguous_range<_Rg>)
    constexpr void
    unchecked_load(_Rg&&,
                        const __detail::__load_mask_type_t<_Vp, ranges::range_value_t<_Rg>>& __k,
                        flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "unchecked_load(range, flags = {}) requires a contiguous and sized range.");

  template <class _Vp = void, input_or_output_iterator _It, typename... _Flags>
    requires (not contiguous_iterator<_It>)
    constexpr void
    unchecked_load(_It, iter_difference_t<_It>,
                        const __detail::__load_mask_type_t<_Vp, iter_value_t<_It>>& __k,
                        flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "unchecked_load(first, size, flags = {}) requires a contiguous iterator.");

  template <class _Vp = void, input_or_output_iterator _It, sentinel_for<_It> _Sp,
            typename... _Flags>
    requires (not contiguous_iterator<_It> or not sized_sentinel_for<_It, _Sp>)
    constexpr void
    unchecked_load(_It, _Sp,
                        const __detail::__load_mask_type_t<_Vp, iter_value_t<_It>>& __k,
                        flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "unchecked_load(first, last, flags = {}) requires a contiguous iterator and a"
          " sized sentinel.");

  template <class _Vp = void, ranges::range _Rg, typename... _Flags>
    requires (not __detail::__sized_contiguous_range<_Rg>)
    constexpr void
    partial_load(_Rg&&,
                      const __detail::__load_mask_type_t<_Vp, ranges::range_value_t<_Rg>>& __k,
                      flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "partial_load(range, flags = {}) requires a contiguous and sized range.");

  template <class _Vp = void, input_or_output_iterator _It, typename... _Flags>
    requires (not contiguous_iterator<_It>)
    constexpr void
    partial_load(_It, iter_difference_t<_It>,
                      const __detail::__load_mask_type_t<_Vp, iter_value_t<_It>>& __k,
                      flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "partial_load(first, size, flags = {}) requires a contiguous iterator.");

  template <class _Vp = void, input_or_output_iterator _It, sentinel_for<_It> _Sp,
            typename... _Flags>
    requires (not contiguous_iterator<_It> or not sized_sentinel_for<_It, _Sp>)
    constexpr void
    partial_load(_It, _Sp,
                      const __detail::__load_mask_type_t<_Vp, iter_value_t<_It>>& __k,
                      flags<_Flags...> = {})
      = _GLIBCXX_DELETE_MSG(
          "partial_load(first, last, flags = {}) requires a contiguous iterator and a"
          " sized sentinel.");

  template <class _Vp = void, __detail::__sized_contiguous_range _Rg, typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, ranges::range_value_t<_Rg>>
    unchecked_load(_Rg&& __r,
                        const __detail::__load_mask_type_t<_Vp, ranges::range_value_t<_Rg>>& __k,
                        flags<_Flags...> __f = {})
    {
      using _RV = __detail::__simd_load_return_t<_Vp, ranges::range_value_t<_Rg>>;
      using _Rp = typename _RV::value_type;
      static_assert(__detail::__loadstore_convertible_to<
                      ranges::range_value_t<_Rg>, _Rp, _Flags...>,
                    "The converting load is not value-preserving. "
                    "Pass 'flag_convert' if lossy conversion matches the intent.");

      constexpr bool __throw_on_out_of_bounds = (is_same_v<_Flags, __detail::_Throw> or ...);
      constexpr bool __allow_out_of_bounds
        = (__throw_on_out_of_bounds or ... or is_same_v<_Flags, __detail::_AllowPartialLoadStore>);
      constexpr auto __static_size = __detail::__static_range_size(__r);

      static_assert(__static_size >= _RV::size.value or __allow_out_of_bounds
                      or __static_size == dynamic_extent, "Out-of-bounds simd load");

      const auto* __ptr = __f.template _S_adjust_pointer<_RV>(ranges::data(__r));
      constexpr __detail::__canonical_vec_type_t<_Rp>* __type_tag = nullptr;

      if constexpr (not __allow_out_of_bounds)
        __glibcxx_simd_precondition(
          ranges::size(__r) >= _RV::size(),
          "Input range is too small. Did you mean to use 'partial_load'?");

      const auto __rg_size = ranges::size(__r);
      if constexpr (__throw_on_out_of_bounds)
        {
          if (__rg_size < _RV::size())
            throw out_of_range("unchecked_load: Input range is too small.");
        }

      if (__builtin_is_constant_evaluated())
        {
          if constexpr (__allow_out_of_bounds)
            return _RV([&](size_t __i) { return __i < __rg_size and __k[__i] ? __r[__i] : _Rp(); });
          else
            return _RV([&](size_t __i) { return __k[__i] ? __r[__i] : _Rp(); });
        }
      else if constexpr ((__static_size != dynamic_extent and __static_size >= _RV::size())
                      or not __allow_out_of_bounds)
        return _RV::_Impl::_S_masked_load(__k._M_data, __ptr, __type_tag);
      else if (__rg_size >= _RV::size())
        return _RV::_Impl::_S_masked_load(__k._M_data, __ptr, __type_tag);
      else
        {
          using _Ip = __detail::__mask_integer_from<sizeof(_Rp)>;
          auto __k2 = _RV::mask_type::_Impl::template _S_mask_with_n_true<_Ip>(__rg_size);
          __k2 = _RV::mask_type::_Impl::_S_logical_and(__k._M_data, __k2);
          return _RV::_Impl::_S_masked_load(__k2, __ptr, __type_tag);
        }
    }

  template <class _Vp = void, contiguous_iterator _It, typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, iter_value_t<_It>>
    unchecked_load(_It __first, iter_difference_t<_It> __n,
                        const __detail::__load_mask_type_t<_Vp, iter_value_t<_It>>& __k,
                        flags<_Flags...> __f = {})
    { return unchecked_load<_Vp>(span<const iter_value_t<_It>>(__first, __n), __k, __f); }

  template <class _Vp = void, contiguous_iterator _It, sized_sentinel_for<_It> _Sp,
            typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, iter_value_t<_It>>
    unchecked_load(_It __first, _Sp __last,
                        const __detail::__load_mask_type_t<_Vp, iter_value_t<_It>>& __k,
                        flags<_Flags...> __f = {})
    { return unchecked_load<_Vp>(span<const iter_value_t<_It>>(__first, __last), __k, __f); }

  template <class _Vp = void, __detail::__sized_contiguous_range _Rg, typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, ranges::range_value_t<_Rg>>
    partial_load(_Rg&& __r,
                      const __detail::__load_mask_type_t<_Vp, ranges::range_value_t<_Rg>>& __k,
                      flags<_Flags...> __f = {})
    { return unchecked_load<_Vp>(__r, __k, __f | __allow_partial_loadstore); }

  template <class _Vp = void, contiguous_iterator _It, typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, iter_value_t<_It>>
    partial_load(_It __first, iter_difference_t<_It> __n,
                      const __detail::__load_mask_type_t<_Vp, iter_value_t<_It>>& __k,
                      flags<_Flags...> __f = {})
    { return partial_load<_Vp>(span<const iter_value_t<_It>>(__first, __n), __k, __f); }

  template <class _Vp = void, contiguous_iterator _It, sized_sentinel_for<_It> _Sp,
            typename... _Flags>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr __detail::__simd_load_return_t<_Vp, iter_value_t<_It>>
    partial_load(_It __first, _Sp __last,
                      const __detail::__load_mask_type_t<_Vp, iter_value_t<_It>>& __k,
                      flags<_Flags...> __f = {})
    { return partial_load<_Vp>(span<const iter_value_t<_It>>(__first, __last), __k, __f); }

  // stores ///////////////////////////////////////////////////////////////////////////////////////

  template <typename _Tp, typename _Abi, __detail::__sized_contiguous_range _Rg, typename... _Flags>
    requires indirectly_writable<ranges::iterator_t<_Rg>, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    unchecked_store(const basic_simd<_Tp, _Abi>& __v, _Rg&& __r,
                         flags<_Flags...> __f = {})
    {
      using _TV = basic_simd<_Tp, _Abi>;
      static_assert(destructible<_TV>);
      static_assert(__detail::__loadstore_convertible_to<
                      _Tp, ranges::range_value_t<_Rg>, _Flags...>,
                    "The converting store is not value-preserving. "
                    "Pass 'flag_convert' if lossy conversion matches the intent.");

      constexpr bool __throw_on_out_of_bounds = (is_same_v<_Flags, __detail::_Throw> or ...);
      constexpr bool __allow_out_of_bounds
        = (__throw_on_out_of_bounds or ... or is_same_v<_Flags, __detail::_AllowPartialLoadStore>);
      constexpr auto __static_size = __detail::__static_range_size(__r);

      static_assert(__static_size >= _TV::size.value or __allow_out_of_bounds
                      or __static_size == dynamic_extent, "Out-of-bounds simd store");

      auto* __ptr = __f.template _S_adjust_pointer<_TV>(ranges::data(__r));
      constexpr __detail::__canonical_vec_type_t<_Tp>* __type_tag = nullptr;

      const auto __rg_size = ranges::size(__r);
      if constexpr (not __allow_out_of_bounds)
        __glibcxx_simd_precondition(
          ranges::size(__r) >= _TV::size(),
          "output range is too small. Did you mean to use 'partial_store'?");

      if constexpr (__throw_on_out_of_bounds)
        {
          if (__rg_size < _TV::size())
            throw std::out_of_range("unchecked_store: Output range is too small.");
        }

      if (__builtin_is_constant_evaluated())
        {
          for (unsigned __i = 0; __i < (__allow_out_of_bounds ? __rg_size : _TV::size()); ++__i)
            __ptr[__i] = static_cast<std::ranges::range_value_t<_Rg>>(__v[__i]);
        }
#ifdef __AVX512F__
      else if constexpr (__allow_out_of_bounds)
        {
          const typename _TV::mask_type __k([&](unsigned __i) { return __i < __rg_size; });
          _TV::_Impl::_S_masked_store(__v._M_data, __ptr, __k._M_data);
        }
#endif
      else if constexpr ((__static_size != dynamic_extent and __static_size >= _TV::size())
                        or not __allow_out_of_bounds)
        _TV::_Impl::_S_store(__v._M_data, __ptr, __type_tag);
      else if (__rg_size >= _TV::size())
        _TV::_Impl::_S_store(__v._M_data, __ptr, __type_tag);
      else if (__builtin_constant_p(__rg_size))
        {
          for (unsigned __i = 0; __i < __rg_size; ++__i)
            __ptr[__i] = static_cast<std::ranges::range_value_t<_Rg>>(__v[__i]);
        }
      else
        _TV::_Impl::_S_partial_store(__v._M_data, __ptr, __rg_size, __type_tag);
    }

  template <typename _Tp, typename _Abi, contiguous_iterator _It, typename... _Flags>
    requires indirectly_writable<_It, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    unchecked_store(const basic_simd<_Tp, _Abi>& __v, _It __first, iter_difference_t<_It> __n,
                         flags<_Flags...> __f = {})
    { unchecked_store(__v, std::span<iter_value_t<_It>>(__first, __n), __f); }

  template <typename _Tp, typename _Abi, contiguous_iterator _It, sized_sentinel_for<_It> _Sp,
            typename... _Flags>
    requires indirectly_writable<_It, _Tp>
    constexpr void
    unchecked_store(const basic_simd<_Tp, _Abi>& __v, _It __first, _Sp __last,
                         flags<_Flags...> __f = {})
    { unchecked_store(__v, std::span<iter_value_t<_It>>(__first, __last), __f); }

  template <typename _Tp, typename _Abi, __detail::__sized_contiguous_range _Rg, typename... _Flags>
    requires indirectly_writable<ranges::iterator_t<_Rg>, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    partial_store(const basic_simd<_Tp, _Abi>& __v, _Rg&& __r, flags<_Flags...> __f = {})
    { unchecked_store(__v, __r, __f | __allow_partial_loadstore); }

  template <typename _Tp, typename _Abi, contiguous_iterator _It, typename... _Flags>
    requires indirectly_writable<_It, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    partial_store(const basic_simd<_Tp, _Abi>& __v, _It __first, iter_difference_t<_It> __n,
                       flags<_Flags...> __f = {})
    { partial_store(__v, std::span<iter_value_t<_It>>(__first, __n), __f); }

  template <typename _Tp, typename _Abi, contiguous_iterator _It, sized_sentinel_for<_It> _Sp,
            typename... _Flags>
    requires indirectly_writable<_It, _Tp>
    constexpr void
    partial_store(const basic_simd<_Tp, _Abi>& __v, _It __first, _Sp __last,
                       flags<_Flags...> __f = {})
    { partial_store(__v, std::span<iter_value_t<_It>>(__first, __last), __f); }

  // masked stores ////////////////////////////////////////////////////////////////////////////////

  template <typename _Tp, typename _Abi, __detail::__sized_contiguous_range _Rg, typename... _Flags>
    requires indirectly_writable<ranges::iterator_t<_Rg>, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    unchecked_store(const basic_simd<_Tp, _Abi>& __v, _Rg&& __r,
                         const typename basic_simd<_Tp, _Abi>::mask_type& __k,
                         flags<_Flags...> __f = {})
    {
      using _TV = basic_simd<_Tp, _Abi>;
      static_assert(__detail::__loadstore_convertible_to<
                      _Tp, ranges::range_value_t<_Rg>, _Flags...>,
                    "The converting store is not value-preserving. "
                    "Pass 'flag_convert' if lossy conversion matches the intent.");

      constexpr bool __throw_on_out_of_bounds = (is_same_v<_Flags, __detail::_Throw> or ...);
      constexpr bool __allow_out_of_bounds
        = (__throw_on_out_of_bounds or ... or is_same_v<_Flags, __detail::_AllowPartialLoadStore>);
      constexpr auto __static_size = __detail::__static_range_size(__r);

      static_assert(__static_size >= _TV::size.value or __allow_out_of_bounds
                      or __static_size == dynamic_extent, "Out-of-bounds simd store");

      auto* __ptr = __f.template _S_adjust_pointer<_TV>(ranges::data(__r));

      if constexpr (not __allow_out_of_bounds)
        __glibcxx_simd_precondition(
          ranges::size(__r) >= _TV::size(),
          "output range is too small. Did you mean to use 'partial_store'?");

      const auto __rg_size = ranges::size(__r);
      if constexpr (__throw_on_out_of_bounds)
        {
          if (__rg_size < _TV::size())
            throw out_of_range("unchecked_store: Output range is too small.");
        }

      if (__builtin_is_constant_evaluated())
        {
          for (int __i = 0; __i < _TV::size(); ++__i)
            {
              if (__k[__i] and (not __allow_out_of_bounds or size_t(__i) < __rg_size))
                __ptr[__i] = static_cast<ranges::range_value_t<_Rg>>(__v[__i]);
            }
        }
      else if (__allow_out_of_bounds && __rg_size < _TV::size())
        {
          using _Ip = __detail::__mask_integer_from<sizeof(_Tp)>;
          auto __k2 = _TV::mask_type::_Impl::template _S_mask_with_n_true<_Ip>(__rg_size);
          __k2 = _TV::mask_type::_Impl::_S_logical_and(__k._M_data, __k2);
          _TV::_Impl::_S_masked_store(__v._M_data, __ptr, __k2);
        }
      else
        _TV::_Impl::_S_masked_store(__v._M_data, __ptr, __k._M_data);
    }

  template <typename _Tp, typename _Abi, contiguous_iterator _It, typename... _Flags>
    requires indirectly_writable<_It, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    unchecked_store(const basic_simd<_Tp, _Abi>& __v, _It __first, iter_difference_t<_It> __n,
                         const typename basic_simd<_Tp, _Abi>::mask_type& __k,
                         flags<_Flags...> __f = {})
    { unchecked_store(__v, span(__first, __n), __k, __f); }

  template <typename _Tp, typename _Abi, contiguous_iterator _It, sized_sentinel_for<_It> _Sp,
            typename... _Flags>
    requires indirectly_writable<_It, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    unchecked_store(const basic_simd<_Tp, _Abi>& __v, _It __first, _Sp __last,
                         const typename basic_simd<_Tp, _Abi>::mask_type& __k,
                         flags<_Flags...> __f = {})
    { unchecked_store(__v, span(__first, __last), __k, __f); }


  template <typename _Tp, typename _Abi, __detail::__sized_contiguous_range _Rg, typename... _Flags>
    requires indirectly_writable<ranges::iterator_t<_Rg>, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    partial_store(const basic_simd<_Tp, _Abi>& __v, _Rg&& __r,
                         const typename basic_simd<_Tp, _Abi>::mask_type& __k,
                         flags<_Flags...> __f = {})
    { return unchecked_store(__v, __r, __k, __f | __allow_partial_loadstore); }

  template <typename _Tp, typename _Abi, contiguous_iterator _It, typename... _Flags>
    requires indirectly_writable<_It, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    partial_store(const basic_simd<_Tp, _Abi>& __v, _It __first, iter_difference_t<_It> __n,
                         const typename basic_simd<_Tp, _Abi>::mask_type& __k,
                       flags<_Flags...> __f = {})
    { partial_store(__v, span(__first, __n), __k, __f); }

  template <typename _Tp, typename _Abi, contiguous_iterator _It, sized_sentinel_for<_It> _Sp,
            typename... _Flags>
    requires indirectly_writable<_It, _Tp>
    _GLIBCXX_SIMD_ALWAYS_INLINE
    constexpr void
    partial_store(const basic_simd<_Tp, _Abi>& __v, _It __first, _Sp __last,
                         const typename basic_simd<_Tp, _Abi>::mask_type& __k,
                       flags<_Flags...> __f = {})
    { partial_store(__v, span(__first, __last), __k, __f); }
}
#endif  // SIMD_LOADSTORE_H_
