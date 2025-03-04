/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_SPLIT_H_
#define PROTOTYPE_SIMD_SPLIT_H_

#include "simd.h"
#include <array>
#include <tuple>

namespace std::__detail
{
  template <typename _Tp, size_t>
    using __repeat_type = _Tp;

  template <typename _T0, typename... _Ts>
    _GLIBCXX_SIMD_INTRINSIC constexpr typename _T0::value_type
    __get_simd_element_from_pack(auto __i, const _T0& __x, const _Ts&... __pack)
    {
      if constexpr (__i.value < _T0::size.value)
        return __x[__i.value];
      else
        return __get_simd_element_from_pack(__ic<__i.value - _T0::size.value>, __pack...);
    }
}

namespace std::datapar
{
  template <typename _V, typename _Tp, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    chunk(const basic_simd<_Tp, _Abi>& __x) noexcept
    {
      constexpr int __in = __detail::__simd_size_v<_Tp, _Abi>;
      constexpr int __out = _V::size();
      constexpr int __rem = __in % __out;
      if constexpr (__rem == 0)
        { // -> array
          using _Rp = std::array<_V, __in / __out>;
          if constexpr (sizeof(_Rp) == sizeof(__x))
            return __builtin_bit_cast(_Rp, __x);
          else
            return [&]<size_t... _Is>(std::index_sequence<_Is...>) {
              return _Rp {_V([&](auto __i) { return __x[__i + __out * _Is]; })...};
            }(std::make_index_sequence<__in / __out>());
        }
      else // -> tuple
        return [&]<size_t... _Is>(std::index_sequence<_Is...>) {
          using _Rem = resize_t<__rem, _V>;
          using _Rp = std::tuple<__detail::__repeat_type<_V, _Is>..., _Rem>;
          return _Rp {_V([&](auto __i) {
                        return __x[__i + __out * _Is];
                      })..., _Rem([&](auto __i) {
                               return __x[__i + __in - __rem];
                             })};
        }(std::make_index_sequence<__in / __out>());
    }

  template <typename _M, size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    chunk(const basic_simd_mask<_Bs, _Abi>& __x) noexcept
    {
      constexpr int __in = basic_simd_mask<_Bs, _Abi>::size();
      constexpr int __out = _M::size();
      constexpr int __rem = __in % __out;
      if constexpr (__rem == 0)
        { // -> array
          using _Rp = std::array<_M, __in / __out>;
          if constexpr (sizeof(_Rp) == sizeof(__x))
            return __builtin_bit_cast(_Rp, __x);
          else
            return [&]<size_t... _Is>(std::index_sequence<_Is...>) {
              return _Rp {_M([&](auto __i) { return __x[__i + __out * _Is]; })...};
            }(std::make_index_sequence<__in / __out>());
        }
      else // -> tuple
        return [&]<size_t... _Is>(std::index_sequence<_Is...>) {
          using _Rem = resize_t<__rem, _M>;
          using _Rp = std::tuple<__detail::__repeat_type<_M, _Is>..., _Rem>;
          return _Rp {_M([&](auto __i) {
                        return __x[__i + __out * _Is];
                      })..., _Rem([&](auto __i) {
                               return __x[__i + __in - __rem];
                             })};
        }(std::make_index_sequence<__in / __out>());
    }

  template <size_t _Np, typename _Tp, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    chunk(const basic_simd<_Tp, _Abi>& __x) noexcept
    { return chunk<resize_t<_Np, basic_simd<_Tp, _Abi>>>(__x); }

  template <size_t _Np, size_t _Bs, typename _Abi>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr auto
    chunk(const basic_simd_mask<_Bs, _Abi>& __x) noexcept
    { return chunk<resize_t<_Np, basic_simd_mask<_Bs, _Abi>>>(__x); }
}

namespace std::__detail
{
  template <typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC constexpr auto
    __as_simd_builtin(_Tp __x) noexcept
    {
      if constexpr (_Tp::size.value == 1)
        {
          using _Vp [[__gnu__::__vector_size__(sizeof(_Tp))]] = typename _Tp::value_type;
          return _Vp {__data(__x)};
        }
      else if constexpr (__vec_builtin<__remove_cvref_t<decltype(__data(__x))>>)
        return __data(__x);
      else
        return __data(__x)._M_data;
    }

  template <typename _T0, typename _T1, typename... _Ts>
    _GLIBCXX_SIMD_INTRINSIC constexpr auto
    __cat_recursive(const _T0& __x0, const _T1& __x1, const _Ts&... __xs) noexcept
    {
      using _Tp = typename _T0::value_type;
      constexpr int __size = _T0::size.value + _T1::size.value;
      const std::datapar::resize_t<__size, _T0>
        __x01{__private_init,
              [&]<_SimdSizeType... _Is, _SimdSizeType... _Js, _SimdSizeType... _Ks>
                [[__gnu__::__always_inline__]]
                (_SimdIndexSequence<_Is...>, _SimdIndexSequence<_Js...>,
                 _SimdIndexSequence<_Ks...>) {
#ifdef __clang__
                constexpr int __simd_bytes = std::__bit_ceil(__size) * sizeof(_Tp);
                using _Rp [[__gnu__::__vector_size__(__simd_bytes)]] = _Tp;
                return _Rp {
                  __vec_get(__as_simd_builtin(__x0), _Is)...,
                  __vec_get(__as_simd_builtin(__x1), _Js)...,
                  ((void)_Ks, 0)...
                };
#else
                constexpr int __x0_size = sizeof(__x0) / sizeof(_Tp);
                return __builtin_shufflevector(__as_simd_builtin(__x0), __as_simd_builtin(__x1),
                                               _Is..., __x0_size + _Js...,
                                               ((void)_Ks, -1)...);
#endif
              }(_MakeSimdIndexSequence<_T0::size.value>(),
                _MakeSimdIndexSequence<_T1::size.value>(),
                _MakeSimdIndexSequence<std::__bit_ceil(__size) - __size>())};
      if constexpr (sizeof...(_Ts) == 0)
        return __x01;
      else
        return __cat_recursive(__x01, __xs...);
    }
}

namespace std::datapar
{
  template <typename _Tp, typename... _Abis>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
    simd<_Tp, (__detail::__simd_size_v<_Tp, _Abis> + ...)>
    cat(const basic_simd<_Tp, _Abis>&... __xs) noexcept
    {
      constexpr int __size = (__detail::__simd_size_v<_Tp, _Abis> + ...);
      if constexpr (sizeof...(_Abis) == 1)
        return simd<_Tp, __size>(__xs...);
      else if constexpr (__size <= __detail::__simd_size_v<_Tp, __detail::_NativeAbi<_Tp>>)
        return __detail::__cat_recursive(__xs...);
      else
        return simd<_Tp, (__detail::__simd_size_v<_Tp, _Abis> + ...)>(
                 [&] [[__gnu__::__always_inline__]] (auto __i) {
                   return __detail::__get_simd_element_from_pack(__i, __xs...);
                 });
    }

  template <size_t _Bs, typename... _Abis>
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
    simd_mask<__detail::__mask_integer_from<_Bs>, (basic_simd_mask<_Bs, _Abis>::size.value + ...)>
    cat(const basic_simd_mask<_Bs, _Abis>&... __xs) noexcept
    {
      return simd_mask<__detail::__mask_integer_from<_Bs>,
                       (basic_simd_mask<_Bs, _Abis>::size.value + ...)>(
               [&] [[__gnu__::__always_inline__]] (auto __i) {
                 return __detail::__get_simd_element_from_pack(__i, __xs...);
               });
    }
}

#endif  // PROTOTYPE_SIMD_SPLIT_H_
