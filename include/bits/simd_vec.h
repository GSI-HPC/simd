/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef _GLIBCXX_SIMD_VEC_H
#define _GLIBCXX_SIMD_VEC_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include "simd_mask.h"
#include "simd_flags.h"

#include <bits/utility.h>
#include <bits/stl_function.h>
#include <cmath>

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

namespace std::simd
{
  // disabled basic_vec
  template <typename _Tp, typename _Ap>
    class basic_vec
    {
    public:
      using value_type = _Tp;

      using abi_type = _Ap;

      using mask_type = basic_mask<0, void>; // disabled

#define _GLIBCXX_DELETE_SIMD "This specialization is disabled because of an invalid combination "  \
                             "of template arguments to basic_vec."

      basic_vec() = delete(_GLIBCXX_DELETE_SIMD);

      ~basic_vec() = delete(_GLIBCXX_DELETE_SIMD);

      basic_vec(const basic_vec&) = delete(_GLIBCXX_DELETE_SIMD);

      basic_vec& operator=(const basic_vec&) = delete(_GLIBCXX_DELETE_SIMD);

#undef _GLIBCXX_DELETE_SIMD
    };

  template <typename _Tp, typename _Ap>
    class _BinaryOps
    {
      using _Vp = basic_vec<_Tp, _Ap>;

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator+(const _Vp& __x, const _Vp& __y) noexcept
      {
        _Vp __r = __x;
        __r += __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator-(const _Vp& __x, const _Vp& __y) noexcept
      {
        _Vp __r = __x;
        __r -= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator*(const _Vp& __x, const _Vp& __y) noexcept
      {
        _Vp __r = __x;
        __r *= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator/(const _Vp& __x, const _Vp& __y) noexcept
      {
        _Vp __r = __x;
        __r /= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator%(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a % __a; }
      {
        _Vp __r = __x;
        __r %= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator&(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a & __a; }
      {
        _Vp __r = __x;
        __r &= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator|(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a | __a; }
      {
        _Vp __r = __x;
        __r |= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator^(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a ^ __a; }
      {
        _Vp __r = __x;
        __r ^= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator<<(const _Vp& __x, const _Vp& __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires (_Tp __a) { __a << __a; }
      {
        _Vp __r = __x;
        __r <<= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator<<(const _Vp& __x, __simd_size_type __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires (_Tp __a, __simd_size_type __b) { __a << __b; }
      {
        _Vp __r = __x;
        __r <<= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator>>(const _Vp& __x, const _Vp& __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires (_Tp __a) { __a >> __a; }
      {
        _Vp __r = __x;
        __r >>= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator>>(const _Vp& __x, __simd_size_type __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires (_Tp __a, __simd_size_type __b) { __a >> __b; }
      {
        _Vp __r = __x;
        __r >>= __y;
        return __r;
      }
    };

  struct _LoadCtorTag
  {};

  template <typename _Cx, __vec_builtin _TV, _TargetTraits = {}>
    [[__gnu__::__cold__]]
    constexpr _TV
    __cx_redo_mul(_TV __r, const _TV __x, const _TV __y, const auto __nan, int __n)
    {
      // redo multiplication using scalar complex-mul on (NaN, NaN) results
      alignas(_TV) __vec_value_type<_TV> __arr[__width_of<_TV>] = {};
      for (int __i = 0; __i < __n; __i += 2)
        {
          if (__nan[__i] && __nan[__i + 1])
            {
              using _Tc = typename _Cx::value_type;
              const _Cx __cx(_Tc(__x[__i]), _Tc(__x[__i + 1]));
              const _Cx __cy(_Tc(__y[__i]), _Tc(__y[__i + 1]));
              const _Cx __cr = __cx * __cy;
              __arr[__i] = __cr.real();
              __arr[__i + 1] = __cr.imag();
            }
          else
            {
              __arr[__i] = __r[__i];
              __arr[__i + 1] = __r[__i + 1];
            }
        }
      return __builtin_bit_cast(_TV, __arr);
    }

  template <typename _Cx, __vec_builtin _TV, _TargetTraits = {}>
    [[__gnu__::__cold__]]
    constexpr void
    __cxctgus_redo_mul(_TV& __re0, _TV& __im0, const _TV __re1, const _TV __im1,
                       const _TV __re, const _TV __im, const auto __nan, int __n)
    {
      alignas(_TV) __vec_value_type<_TV> __arr_re[__width_of<_TV>] = {};
      alignas(_TV) __vec_value_type<_TV> __arr_im[__width_of<_TV>] = {};
      for (int __i = 0; __i < __n; ++__i)
        {
          if (__nan[__i])
            {
              const _Cx __c0(__re0[__i], __im0[__i]);
              const _Cx __c1(__re1[__i], __im1[__i]);
              const _Cx __cr = __c0 * __c1;
              __arr_re[__i] = __cr.real();
              __arr_im[__i] = __cr.imag();
            }
          else
            {
              __arr_re[__i] = __re[__i];
              __arr_im[__i] = __im[__i];
            }
        }
      __re0 = __builtin_bit_cast(_TV, __arr_re);
      __im0 = __builtin_bit_cast(_TV, __arr_im);
    }

  template <typename _Cx, floating_point _Tp, _TargetTraits = {}>
    [[__gnu__::__always_inline__]]
    constexpr void
    __cxctgus_redo_mul(_Tp& __re0, _Tp& __im0, const _Tp __re1, const _Tp __im1,
                       const _Tp, const _Tp, const auto, int)
    {
      const _Cx __c0(__re0, __im0);
      const _Cx __c1(__re1, __im1);
      const _Cx __cr = __c0 * __c1;
      __re0 = __cr.real();
      __im0 = __cr.imag();
    }

  template <integral _Tp>
    inline constexpr _Tp __max_shift
      = (sizeof(_Tp) < sizeof(int) ? sizeof(int) : sizeof(_Tp)) * __CHAR_BIT__;

  template <__vectorizable _Tp, __abi_tag _Ap>
    requires (_Ap::_S_nreg == 1)
      && (!__complex_like<_Tp>)
    class basic_vec<_Tp, _Ap>
    : _BinaryOps<_Tp, _Ap>
    {
      template <typename, typename>
        friend class basic_vec;

      template <size_t, typename>
        friend class basic_mask;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr int _S_full_size = __bit_ceil(unsigned(_S_size));

      static constexpr bool _S_is_scalar = __scalar_abi_tag<_Ap>;

      static_assert(!_S_is_scalar || _S_size == 1);

      static constexpr bool _S_use_bitmask = _Ap::_S_is_bitmask;

      using _DataType = typename _Ap::template _DataType<_Tp>;

      _DataType _M_data;

      static constexpr bool _S_is_partial = sizeof(_M_data) > sizeof(_Tp) * _S_size;

    public:
      using value_type = _Tp;

      using abi_type = _Ap;

      using mask_type = basic_mask<sizeof(_Tp), abi_type>;

      using iterator = __iterator<basic_vec>;

      using const_iterator = __iterator<const basic_vec>;

      constexpr iterator
      begin() noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      begin() const noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      cbegin() const noexcept
      { return {*this, 0}; }

      constexpr default_sentinel_t
      end() const noexcept
      { return {}; }

      constexpr default_sentinel_t
      cend() const noexcept
      { return {}; }

      static constexpr auto size = __simd_size_c<_S_size>;

      // internal but public API ----------------------------------------------
      [[__gnu__::__always_inline__]]
      static constexpr basic_vec
      _S_init(_DataType __x)
      {
        basic_vec __r;
        __r._M_data = __x;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr const _DataType&
      _M_get() const
      { return _M_data; }

      [[__gnu__::__always_inline__]]
      friend constexpr bool
      __is_const_known(const basic_vec& __x)
      { return __builtin_constant_p(__x._M_data); }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_concat_data() const
      {
        if constexpr (_S_is_scalar)
          return __vec_builtin_type<value_type, 1>{_M_data};
        else
          return _M_data;
      }

      template <int _Size = _S_size, int _Offset = 0, typename _A0, typename _Fp>
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_static_permute(const basic_vec<value_type, _A0>& __x, _Fp&& __idxmap)
        {
          using _Xp = basic_vec<value_type, _A0>;
          basic_vec __r;
          if constexpr (_S_is_scalar)
            {
              constexpr __simd_size_type __j = [&] consteval {
                if constexpr (__index_permutation_function_nosize<_Fp>)
                  return __idxmap(_Offset);
                else
                  return __idxmap(_Offset, _Size);
              }();
              if constexpr (__j == simd::zero_element || __j == simd::uninit_element)
                return basic_vec();
              else
                static_assert(__j >= 0 && __j < _Xp::_S_size);
              __r._M_data = __x[__j];
            }
          else
            {
              auto __idxmap2 = [=](auto __i) consteval {
                if constexpr (int(__i + _Offset) >= _Size) // _S_full_size > _Size
                  return __simd_size_c<simd::uninit_element>;
                else if constexpr (__index_permutation_function_nosize<_Fp>)
                  return __simd_size_c<__idxmap(__i + _Offset)>;
                else
                  return __simd_size_c<__idxmap(__i + _Offset, _Size)>;
              };
              constexpr auto __adj_idx = [](auto __i) {
                constexpr int __j = __i;
                if constexpr (__j == simd::zero_element)
                  return __simd_size_c<__bit_ceil(unsigned(_Xp::_S_size))>;
                else if constexpr (__j == simd::uninit_element)
                  return __simd_size_c<-1>;
                else
                  {
                    static_assert(__j >= 0 && __j < _Xp::_S_size);
                    return __simd_size_c<__j>;
                  }
              };
              constexpr auto [...__is0] = _IotaArray<_S_size>;
              constexpr bool __needs_zero_element
                = ((__idxmap2(__simd_size_c<__is0>).value == simd::zero_element) || ...);
              constexpr auto [...__is_full] = _IotaArray<_S_full_size>;
              if constexpr (_A0::_S_nreg == 2 && !__needs_zero_element)
                {
                  __r._M_data = __builtin_shufflevector(
                                  __x._M_data0._M_data, __x._M_data1._M_data,
                                  __adj_idx(__idxmap2(__simd_size_c<__is_full>)).value...);
                }
              else
                {
                  __r._M_data = __builtin_shufflevector(
                                  __x._M_concat_data(), decltype(__x._M_concat_data())(),
                                  __adj_idx(__idxmap2(__simd_size_c<__is_full>)).value...);
                }
            }
          return __r;
        }

      using _HalfVec = __similar_vec<value_type, _S_size / 2, _Ap>;

      [[__gnu__::__always_inline__]]
      constexpr void
      _M_complex_set_real(const _HalfVec& __x) requires ((_S_size & 1) == 0)
      {
        if (__is_const_known(*this, __x))
          {
            constexpr auto [...__is] = _IotaArray<_S_size>;
            _M_data = _DataType { ((__is & 1) == 0 ? value_type(__x[__is / 2]) : _M_data[__is])...};
          }
        else if constexpr (_S_size == 2)
          _M_data[0] = __x[0];
        else
          _VecOps<_DataType>::_S_overwrite_even_elements(_M_data, __x);
      }

      [[__gnu__::__always_inline__]]
      constexpr void
      _M_complex_set_imag(const _HalfVec& __x) requires ((_S_size & 1) == 0)
      {
        if (__is_const_known(*this, __x))
          {
            constexpr auto [...__is] = _IotaArray<_S_size>;
            _M_data = _DataType { ((__is & 1) == 1 ? value_type(__x[__is / 2]) : _M_data[__is])...};
          }
        else if constexpr (_S_size == 2)
          _M_data[1] = __x[0];
        else
          _VecOps<_DataType>::_S_overwrite_odd_elements(_M_data, __x);
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      _M_complex_conj() const
      {
        static_assert((_S_size & 1) == 0);
        return _VecOps<_DataType>::_S_complex_negate_imag(_M_data);
      }

      template <typename _CxVec, _TargetTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr void
        _M_complex_multiply_with(basic_vec __yvec)
        {
          const _DataType __x = _M_data;
          const _DataType __y = __yvec._M_data;
          static_assert((_S_size & 1) == 0);
          using _VO = _VecOps<_DataType>;
          if constexpr (_Traits.template _M_eval_as_f32<value_type>())
            {
              using _Vf = rebind_t<float, basic_vec>;
              _Vf __xf = _Vf(*this);
              __xf.template _M_complex_multiply_with<_CxVec>(_Vf(__yvec));
              *this = basic_vec(__xf);
              return;
            }
          else if (_VecOps<_DataType, _S_size>::_S_complex_imag_is_const_known_zero(__x))
            {
              if (_VecOps<_DataType, _S_size>::_S_complex_imag_is_const_known_zero(__y))
                _M_data = __x * __y;
              else
                {
                  if (_Traits._M_conforming_to_STDC_annex_G())
                    {
                      auto __a = _VO::_S_dup_even(__x) * __y;
                      auto __b = _DataType() * _VO::_S_swap_neighbors(__y);
#if SIMD_DIAGNOSE_INDETERMINATE_SIGNED_ZERO
                      //if (_SuperImpl::_S_any_of(_SuperImpl::_S_equal_to(__a, 0))) // __b is ±0 by construction
#endif
                      _M_data = _VO::_S_addsub(__a, __b);
                    }
                  else
                    _M_data = _VO::_S_dup_even(__x) * __y;
                }
            }
          else if (_VecOps<_DataType, _S_size>::_S_complex_imag_is_const_known_zero(__y))
            {
              if (_Traits._M_conforming_to_STDC_annex_G())
                _M_data = _VO::_S_addsub(_VO::_S_dup_even(__y) * __x,
                                         _DataType() * _VO::_S_swap_neighbors(__x));
              else
                _M_data = _VO::_S_dup_even(__y) * __x;
            }
          else if (_VecOps<_DataType, _S_size>::_S_complex_real_is_const_known_zero(__y))
            {
              if (_Traits._M_conforming_to_STDC_annex_G())
                _M_data = _VO::_S_addsub(_DataType(), _VO::_S_dup_odd(__y)
                                           * _VO::_S_swap_neighbors(__x));
              else
                _M_data = _VO::_S_dup_odd(__y)
                            * _VO::_S_complex_negate_real(_VO::_S_swap_neighbors(__x));
            }
          else if (_VecOps<_DataType, _S_size>::_S_complex_real_is_const_known_zero(__x))
            {
              if (_Traits._M_conforming_to_STDC_annex_G())
                _M_data = _VO::_S_addsub(_DataType(), _VO::_S_dup_odd(__x)
                                           * _VO::_S_swap_neighbors(__y));
              else
                _M_data = _VO::_S_dup_odd(__x)
                            * _VO::_S_complex_negate_real(_VO::_S_swap_neighbors(__y));
            }
          else
            {
#if _GLIBCXX_X86
              if (_Traits._M_have_fma() && !__is_const_known(__x, __y))
                {
                  if constexpr (_Traits._M_have_fma())
                    _M_data = __x86_complex_multiplies(__x, __y);
                }
              else
#endif
                _M_data = _VO::_S_addsub(_VO::_S_dup_even(__x) * __y,
                                         _VO::_S_dup_odd(__x) * _VO::_S_swap_neighbors(__y));
              mask_type __nan = _M_isnan();
              if (_Traits._M_conforming_to_STDC_annex_G() && __nan._M_any_of()) [[unlikely]]
                _M_data = __cx_redo_mul<typename _CxVec::value_type>(_M_data, __x, __y, __nan,
                                                                     _S_size);
            }
        }

      template <typename _Cx, _TargetTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        static constexpr void
        _S_cxctgus_mul(basic_vec& __re0, basic_vec& __im0, basic_vec __re1, basic_vec __im1)
        {
          if constexpr (_S_is_scalar)
            {
              const _Cx __c0(__re0._M_data, __im0._M_data);
              const _Cx __c1(__re1._M_data, __im1._M_data);
              const _Cx __cr = __c0 * __c1;
              __re0._M_data = __cr.real();
              __im0._M_data = __cr.imag();
            }
          else if constexpr (_Traits.template _M_eval_as_f32<value_type>())
            {
              using _Vf = rebind_t<float, basic_vec>;
              _Vf __re0f = __re0;
              _Vf __im0f = __im0;
              _Vf::template _S_cxctgus_mul<_Cx>(__re0f, __im0f, __re1, __im1);
              __re0 = basic_vec(__re0f);
              __im0 = basic_vec(__im0f);
            }
          else
            {
              basic_vec __re = __re0 * __re1 - __im0 * __im1;
              basic_vec __im = __re0 * __im1 + __im0 * __re1;
              const auto __nan = __re._M_isnan() && __im._M_isnan();
              if (any_of(__nan)) [[unlikely]]
                __cxctgus_redo_mul<_Cx>(__re0._M_data, __im0._M_data, __re1._M_data, __im1._M_data,
                                        __re._M_data, __im._M_data, __nan._M_data, _S_size);
              else
                {
                  __re0 = __re;
                  __im0 = __im;
                }
            }
        }

      template <typename _Vp>
        [[__gnu__::__always_inline__]]
        constexpr auto
        _M_chunk() const noexcept
        {
          constexpr int __n = _S_size / _Vp::_S_size;
          constexpr int __rem = _S_size % _Vp::_S_size;
          constexpr auto [...__is] = _IotaArray<__n>;
          if constexpr (__rem == 0)
            {
              if constexpr (_Vp::_S_is_scalar)
                return array<_Vp, __n> {_Vp::_S_init(_M_data[__is])...};
              else
                return array<_Vp, __n> {
                  _Vp::_S_init(
                    _VecOps<typename _Vp::_DataType>::_S_extract(
                      _M_data, integral_constant<int, __is * _Vp::_S_size>()))...
              };
            }
          else
            {
              using _Rest = resize_t<__rem, _Vp>;
              _Rest __rest;
              if constexpr (_Rest::_S_size > 1)
                __rest = _VecOps<typename _Rest::_DataType>::_S_extract(
                           _M_data, integral_constant<int, __n * _Vp::_S_size>());
              else
                __rest = _M_data[__n * _Vp::_S_size];
              return tuple(_Vp::_S_init(
                             _VecOps<typename _Vp::_DataType>::_S_extract(
                               _M_data, integral_constant<int, __is * _Vp::_S_size>()))...,
                           __rest);
            }
        }

      template <typename _A0, typename... _As>
        [[__gnu__::__always_inline__]]
        constexpr void
        _M_assign_from(auto _Offset, const basic_vec<value_type, _A0>& __x0,
                       const basic_vec<value_type, _As>&... __xs)
        {
          if constexpr (_Offset.value >= _A0::_S_size)
            // make the pack as small as possible
            _M_assign_from(integral_constant<int, _Offset.value - _A0::_S_size>(), __xs...);
          else if constexpr (_A0::_S_size >= _S_size + _Offset.value)
            {
              if constexpr (_S_size == 1)
                _M_data = __x0[_Offset];
              else
                _M_data = _VecOps<_DataType>::_S_extract(__x0._M_concat_data(), _Offset);
            }
          else
            _M_data = _VecOps<_DataType>::_S_extract(
                        __vec_concat_sized<__x0.size(), __xs.size()...>(__x0._M_concat_data(),
                                                                        __xs._M_concat_data()...),
                        _Offset);
        }

      template <typename _A0>
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_concat(const basic_vec<value_type, _A0>& __x0) noexcept
        { return static_cast<const basic_vec&>(__x0); }

      template <typename... _As>
        requires (sizeof...(_As) > 1)
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_concat(const basic_vec<value_type, _As>&... __xs) noexcept
        {
          using _A0 = _As...[0];
          using _A1 = _As...[1];
          if constexpr (!_S_is_partial
                          && ((!basic_vec<value_type, _As>::_S_is_partial
                                  && _As::_S_size * sizeof...(_As) == _S_size) && ...))
            return basic_vec::_S_init(__vec_concat(__xs._M_concat_data()...));

          else
            {
              constexpr bool __simple_inserts
                = sizeof...(_As) == 2 && _A1::_S_size <= 2
                    && is_same_v<_DataType, typename basic_vec<value_type, _A0>::_DataType>;
              if (!__builtin_is_constant_evaluated() && __simple_inserts)
                {
                  if constexpr (__simple_inserts)
                    {
                      const auto& __x0 = __xs...[0];
                      const auto& __x1 = __xs...[1];
                      basic_vec __r;
                      __r._M_data = __x0._M_data;
                      if constexpr (_A1::_S_size == 1)
                        __r._M_data[_S_size - 1] = __x1[0];
                      else
                        {
                          for (int __i = __x0.size.value; __i < _S_size; ++__i)
                            __r._M_data[__i] = __x1._M_data[__i - __x0.size.value];
                        }
                      return __r;
                    }
                }
              else
                return basic_vec::_S_init(__vec_concat_sized<_As::_S_size...>(
                                            __xs._M_concat_data()...));
            }
        }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_reduce_1(auto __binary_op) const
      {
        static_assert(__has_single_bit(unsigned(_S_size)));
        auto [__a, __b] = chunk<_S_size / 2>(*this);
        return __binary_op(__a, __b);
      }

      [[__gnu__::__always_inline__]]
      constexpr value_type
      _M_reduce_tail(const auto& __rest, auto __binary_op) const
      {
        if constexpr (__rest.size() > _S_size)
          {
            auto [__a, __b] = __rest.template _M_chunk<basic_vec>();
            return __binary_op(*this, __a)._M_reduce_tail(__b, __binary_op);
          }
        else if constexpr (_S_is_scalar)
          return __binary_op(*this, __rest)._M_data;
        else if constexpr (__rest.size() == _S_size)
          return __binary_op(*this, __rest)._M_reduce(__binary_op);
        else
          return _M_reduce_1(__binary_op)._M_reduce_tail(__rest, __binary_op);
      }

      template <int _Shift, _ArchTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr basic_vec
        _M_elements_shifted_down() const
        {
          static_assert(_Shift < _S_size && _Shift > 0);
#ifdef __SSE2__
          if (!__is_const_known(*this))
            {
              if constexpr (sizeof(_M_data) == 16)
                return reinterpret_cast<_DataType>(
                         __builtin_ia32_psrldqi128(__vec_bit_cast<long long>(_M_data),
                                                   _Shift * sizeof(value_type) * 8));
              else
                {
                  const auto __x = reinterpret_cast<__vec_builtin_type_bytes<long long, 16>>(
                                     __vec_zero_pad_to_16(_M_data));
                  const auto __shifted = __builtin_ia32_psrldqi128(
                                           __x, _Shift * sizeof(value_type) * 8);
                  return _VecOps<_DataType>::_S_extract(__vec_bit_cast<value_type>(__shifted));
                }
            }
#endif
          return _S_static_permute(*this, [](int __i) {
                   return __i + _Shift >= _S_size ? zero_element : __i + _Shift;
                 });
        }

      template <typename _BinaryOp, _ArchTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr value_type
        _M_reduce(_BinaryOp __binary_op) const
      {
        if constexpr (_S_size == 1)
          return operator[](0);
        else if constexpr (_Traits.template _M_eval_as_f32<value_type>()
                             && (is_same_v<_BinaryOp, plus<>>
                                    || is_same_v<_BinaryOp, multiplies<>>))
          return value_type(rebind_t<float, basic_vec>(*this)._M_reduce(__binary_op));
#ifdef __SSE2__
        else if constexpr (is_integral_v<value_type> && sizeof(value_type) == 1
                             && is_same_v<decltype(__binary_op), multiplies<>>)
          {
            // convert to unsigned short because of missing 8-bit mul instruction
            // we don't need to preserve the order of elements
            using _V16 = resize_t<_S_size / 2, rebind_t<unsigned short, basic_vec>>;
            auto __a = __builtin_bit_cast(_V16, *this);
            return __binary_op(__a, __a >> 8)._M_reduce(__binary_op);
            // alternative: return _V16(*this)._M_reduce(__binary_op);
          }
#endif
        else if constexpr (__has_single_bit(unsigned(_S_size)))
          {
            if constexpr (sizeof(_M_data) > 16)
              return _M_reduce_1(__binary_op)._M_reduce(__binary_op);
            else if constexpr (_S_size == 2)
              return _M_reduce_1(__binary_op)[0];
            else
              {
                static_assert(_S_size <= 16);
                auto __x = *this;
#ifdef __SSE2__
                if constexpr (sizeof(_M_data) <= 16 && is_integral_v<value_type>)
                  {
                    if constexpr (_S_size > 8)
                      __x = __binary_op(__x, __x.template _M_elements_shifted_down<8>());
                    if constexpr (_S_size > 4)
                      __x = __binary_op(__x, __x.template _M_elements_shifted_down<4>());
                    if constexpr (_S_size > 2)
                      __x = __binary_op(__x, __x.template _M_elements_shifted_down<2>());
                    return __binary_op(__x, __x.template _M_elements_shifted_down<1>())[0];
                  }
#endif
                if constexpr (_S_size > 8)
                  __x = __binary_op(__x, _S_static_permute(__x, _SwapNeighbors<8>()));
                if constexpr (_S_size > 4)
                  __x = __binary_op(__x, _S_static_permute(__x, _SwapNeighbors<4>()));
#ifdef __SSE2__
                // avoid pshufb by "promoting" to int
                if constexpr (is_integral_v<value_type> && sizeof(value_type) <= 1)
                  return resize_t<4, rebind_t<int, basic_vec>>(chunk<4>(__x)[0])
                           ._M_reduce(__binary_op);
#endif
                if constexpr (_S_size > 2)
                  __x = __binary_op(__x, _S_static_permute(__x, _SwapNeighbors<2>()));
                if constexpr (is_integral_v<value_type> && sizeof(value_type) == 2)
                  return __binary_op(__x, _S_static_permute(__x, _SwapNeighbors<1>()))[0];
                else
                  return __binary_op(vec<value_type, 1>(__x[0]), vec<value_type, 1>(__x[1]))[0];
              }
          }
        else
          {
            // e.g. _S_size = 16 + 16 + 15 (vec<char, 47>)
            // -> 8 + 8 + 7 -> 4 + 4 + 3 -> 2 + 2 + 1 -> 1
            auto __chunked = chunk<__bit_floor(unsigned(_S_size)) / 2>(*this);
            using _Cp = decltype(__chunked);
            if constexpr (tuple_size_v<_Cp> == 4)
              {
                const auto& [__a, __b, __c, __rest] = __chunked;
                return __binary_op(__binary_op(__a, __b), __c)._M_reduce_tail(__rest, __binary_op);
              }
            else if constexpr (tuple_size_v<_Cp> == 3)
              {
                const auto& [__a, __b, __rest] = __chunked;
                return __binary_op(__a, __b)._M_reduce_tail(__rest, __binary_op);
              }
            else
              static_assert(false);
          }
      }

      // [simd.math] ----------------------------------------------------------
      //
      // ISO/IEC 60559 on the classification operations (5.7.2 General Operations):
      // "They are never exceptional, even for signaling NaNs."
      //
      template <_OptTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr mask_type
        _M_isnan() const requires is_floating_point_v<value_type>
        {
          if constexpr (_Traits._M_finite_math_only())
            return mask_type(false);
          else if constexpr (_S_is_scalar)
            return mask_type(std::isnan(_M_data));
          else if constexpr (_S_use_bitmask)
            return _M_isunordered(*this);
          else if constexpr (!_Traits._M_support_snan())
            return !(*this == *this);
          else if (__is_const_known(_M_data))
            return mask_type([&](int __i) { return std::isnan(_M_data[__i]); });
          else
            {
              // 60559: NaN is represented as Inf + non-zero mantissa bits
              using _Ip = __integer_from<sizeof(value_type)>;
              return __builtin_bit_cast(_Ip, numeric_limits<value_type>::infinity())
                       < __builtin_bit_cast(rebind_t<_Ip, basic_vec>, _M_fabs());
            }
        }

      template <_TargetTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr mask_type
        _M_isinf() const requires is_floating_point_v<value_type>
        {
          if constexpr (_Traits._M_finite_math_only())
            return mask_type(false);
          else if constexpr (_S_is_scalar)
            return mask_type(std::isinf(_M_data));
          else if (__is_const_known(_M_data))
            return mask_type([&](int __i) { return std::isinf(_M_data[__i]); });
#ifdef _GLIBCXX_X86
          else if constexpr (_S_use_bitmask)
            return mask_type::_S_init(__x86_bitmask_isinf(_M_data));
          else if constexpr (_Traits._M_have_avx512dq())
            return __x86_bit_to_vecmask<typename mask_type::_DataType>(
                     __x86_bitmask_isinf(_M_data));
#endif
          else
            {
              using _Ip = __integer_from<sizeof(value_type)>;
              return __vec_bit_cast<_Ip>(_M_fabs()._M_data)
                       == __builtin_bit_cast(_Ip, numeric_limits<value_type>::infinity());
            }
        }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      _M_abs() const
      {
        static_assert(signed_integral<value_type>);
        return _M_data < 0 ? -_M_data : _M_data;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      _M_fabs() const
      {
        static_assert(is_floating_point_v<value_type>);
        if constexpr (_S_is_scalar)
          return std::fabs(_M_data);
        else
          return __vec_and(__vec_not(_S_signmask<_DataType>), _M_data);
      }

      template <_TargetTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr mask_type
        _M_isunordered(basic_vec __y) const requires is_floating_point_v<value_type>
        {
          if constexpr (_Traits._M_finite_math_only())
            return mask_type(false);
          else if constexpr (_S_is_scalar)
            return mask_type(std::isunordered(_M_data, __y._M_data));
#ifdef _GLIBCXX_X86
          else if constexpr (_S_use_bitmask)
            return _M_bitmask_cmp<_X86Cmp::_Unord>(__y._M_data);
#endif
          else
            return mask_type([&](int __i) {
                     return std::isunordered(_M_data[__i], __y._M_data[__i]);
                   });
        }

      /** @internal
       * Implementation of @ref partial_load.
       *
       * @param __mem  A pointer to an array of @p __n values. Can be complex or real.
       * @param __n    Read no more than @p __n values from memory. However, depending on @p __mem
       *               alignment, out of bounds reads are benign.
       */
      template <typename _Up, _ArchTraits _Traits = {}>
        static inline basic_vec
        _S_partial_load(const _Up* __mem, size_t __n)
        {
          if constexpr (_S_is_scalar)
            return __n == 0 ? basic_vec() : basic_vec(static_cast<value_type>(*__mem));
          else if (__is_const_known_equal_to(__n >= size_t(_S_size), true))
            return basic_vec(_LoadCtorTag(), __mem);
          else if constexpr (__converts_trivially<_Up, value_type>)
            {
#if _GLIBCXX_X86
              if constexpr (_Traits._M_have_avx512f()
                              || (_Traits._M_have_avx() && sizeof(_Up) >= 4))
                {
                  const auto __k = __n < _S_size ? mask_type::_S_partial_mask_of_n(int(__n))
                                                 : mask_type(true);
                  return _S_masked_load(__mem, mask_type::_S_partial_mask_of_n(int(__n)));
                }
#endif
              if (__n >= size_t(_S_size)) [[unlikely]]
                return basic_vec(_LoadCtorTag(), __mem);
#if _GLIBCXX_X86 // TODO: where else is this "safe"?
              // allow out-of-bounds read when it cannot lead to a #GP
              else if (__is_const_known_equal_to(
                         __ptr_is_aligned_to(__mem, sizeof(_Up) * _S_full_size), true))
                return __select_impl(mask_type::_S_partial_mask_of_n(int(__n)),
                                     basic_vec(_LoadCtorTag(), __mem), basic_vec());
#endif
              else if constexpr (_S_size > 4)
                {
                  alignas(_DataType) byte __dst[sizeof(_DataType)] = {};
                  const byte* __src = reinterpret_cast<const byte*>(__mem);
                  __memcpy_chunks<sizeof(_Up), sizeof(_DataType)>(__dst, __src, __n);
                  return __builtin_bit_cast(_DataType, __dst);
                }
              else if (__n == 0) [[unlikely]]
                return basic_vec();
              else if constexpr (_S_size == 2)
                return _DataType {static_cast<value_type>(__mem[0]), 0};
              else
                {
                  constexpr auto [...__is] = _IotaArray<_S_size - 2>;
                  return _DataType{
                    static_cast<value_type>(__mem[0]),
                    static_cast<value_type>(__is + 1 < __n ? __mem[__is + 1] : 0)...
                  };
                }
            }
          else
            return static_cast<basic_vec>(rebind_t<_Up, basic_vec>::_S_partial_load(__mem, __n));
        }

      /** @internal
       * Implementation of @ref masked_load.
       */
      template <typename _Up, _ArchTraits _Traits = {}>
        static inline basic_vec
        _S_masked_load(const _Up* __mem, mask_type __k)
        {
#if _GLIBCXX_X86
          if constexpr (_Traits._M_have_avx512f())
            return __x86_masked_load<_DataType>(__mem, __k._M_data);
          else if constexpr (_Traits._M_have_avx() && (sizeof(_Up) == 4 || sizeof(_Up) == 8))
            {
              if constexpr (__converts_trivially<_Up, value_type>)
                return __x86_masked_load<_DataType>(__mem, __k._M_data);
              else
                {
                  using _UV = rebind_t<_Up, basic_vec>;
                  return basic_vec(_UV::_S_masked_load(__mem, typename _UV::mask_type(__k)));
                }
            }
#endif
          if (__k._M_none_of()) [[unlikely]]
            return basic_vec();
          else if constexpr (_S_is_scalar)
            return basic_vec(static_cast<value_type>(*__mem));
          else
            {
              // Use at least 4-byte __bits in __bit_foreach for better code-gen
              _Bitmask<_S_size < 32 ? 32 : _S_size> __bits = __k._M_to_uint();
              [[assume(__bits != 0)]]; // because of '__k._M_none_of()' branch above
              if constexpr (__converts_trivially<_Up, value_type>)
                {
                  _DataType __r = {};
                  __bit_foreach(__bits, [&] [[__gnu__::__always_inline__]] (int __i) {
                    __r[__i] = __mem[__i];
                  });
                  return __r;
                }
              else
                {
                  using _UV = rebind_t<_Up, basic_vec>;
                  alignas(_UV) _Up __tmp[sizeof(_UV) / sizeof(_Up)] = {};
                  __bit_foreach(__bits, [&] [[__gnu__::__always_inline__]] (int __i) {
                    __tmp[__i] = __mem[__i];
                  });
                  return basic_vec(__builtin_bit_cast(_UV, __tmp));
                }
            }
        }

      template <typename _Up>
        [[__gnu__::__always_inline__]]
        inline void
        _M_store(_Up* __mem) const
        {
          if constexpr (__converts_trivially<value_type, _Up>)
            __builtin_memcpy(__mem, &_M_data, sizeof(_Up) * _S_size);
          else
            rebind_t<_Up, basic_vec>(*this)._M_store(__mem);
        }

      /** @internal
       * Implementation of @ref partial_store.
       *
       * @note This is a static function to allow passing @p __v via register in case the function
       * is not inlined.
       *
       * @note The function is not marked @c __always_inline__ since code-gen can become fairly
       * long.
       */
      template <typename _Up, _ArchTraits _Traits = {}>
        static inline void
        _S_partial_store(const basic_vec __v, _Up* __mem, size_t __n)
        {
          if (__is_const_known_equal_to(__n >= _S_size, true))
            __v._M_store(__mem);
#if _GLIBCXX_X86
          else if constexpr (_Traits._M_have_avx512f() && !_S_is_scalar)
            {
              const auto __k = __n < _S_size ? mask_type::_S_partial_mask_of_n(int(__n))
                                             : mask_type(true);
              return _S_masked_store(__v, __mem, __k);
            }
#endif
          else if (__n >= _S_size) [[unlikely]]
            __v._M_store(__mem);
          else if (__n == 0) [[unlikely]]
            return;
          else if constexpr (__converts_trivially<value_type, _Up>)
            {
              byte* __dst = reinterpret_cast<byte*>(__mem);
              const byte* __src = reinterpret_cast<const byte*>(&__v._M_data);
              __memcpy_chunks<sizeof(_Up), sizeof(_M_data)>(__dst, __src, __n);
            }
          else
            {
              using _UV = rebind_t<_Up, basic_vec>;
              _UV::_S_partial_store(_UV(__v), __mem, __n);
            }
        }

      template <typename _Up, _ArchTraits _Traits = {}>
        //[[__gnu__::__always_inline__]]
        static inline void
        _S_masked_store(const basic_vec __v, _Up* __mem, const mask_type __k)
        {
#if _GLIBCXX_X86
          if constexpr (_Traits._M_have_avx512f())
            {
              __x86_masked_store(__v._M_data, __mem, __k._M_data);
              return;
            }
          else if constexpr (_Traits._M_have_avx() && (sizeof(_Up) == 4 || sizeof(_Up) == 8))
            {
              if constexpr (__converts_trivially<value_type, _Up>)
                __x86_masked_store(__v._M_data, __mem, __k._M_data);
              else
                {
                  using _UV = rebind_t<_Up, basic_vec>;
                  _UV::_S_masked_store(_UV(__v), __mem, typename _UV::mask_type(__k));
                }
              return;
            }
#endif
          if (__k._M_none_of()) [[unlikely]]
            return;
          else if constexpr (_S_is_scalar)
            __mem[0] = __v._M_data;
          else
            {
              // Use at least 4-byte __bits in __bit_foreach for better code-gen
              _Bitmask<_S_size < 32 ? 32 : _S_size> __bits = __k._M_to_uint();
              [[assume(__bits != 0)]]; // because of '__k._M_none_of()' branch above
              if constexpr (__converts_trivially<value_type, _Up>)
                {
                  __bit_foreach(__bits, [&] [[__gnu__::__always_inline__]] (int __i) {
                    __mem[__i] = __v[__i];
                  });
                }
              else
                {
                  const rebind_t<_Up, basic_vec> __cvted(__v);
                  __bit_foreach(__bits, [&] [[__gnu__::__always_inline__]] (int __i) {
                    __mem[__i] = __cvted[__i];
                  });
                }
            }
        }

      // [simd.overview] default constructor ----------------------------------
      basic_vec() = default;

      // [simd.overview] impl-def conversions ---------------------------------
      constexpr
      basic_vec(_DataType __x) requires (!_S_is_scalar)
        : _M_data(__x)
      {}

      constexpr
      operator _DataType() const
      requires (!_S_is_scalar)
      { return _M_data; }

#if _GLIBCXX_X86
      template <__vec_builtin _IV>
        requires same_as<__x86_intel_intrin_value_type<value_type>, __vec_value_type<_IV>>
          && (sizeof(_IV) == sizeof(_DataType) && sizeof(_IV) >= 16
                 && !is_same_v<_IV, _DataType>)
        constexpr
        basic_vec(_IV __x)
        : _M_data(reinterpret_cast<_DataType>(__x))
        {}

      template <__vec_builtin _IV>
        requires same_as<__x86_intel_intrin_value_type<value_type>, __vec_value_type<_IV>>
          && (sizeof(_IV) == sizeof(_DataType) && sizeof(_IV) >= 16
                 && !is_same_v<_IV, _DataType>)
        constexpr
        operator _IV() const
        { return reinterpret_cast<_IV>(_M_data); }
#endif

      // [simd.ctor] broadcast constructor ------------------------------------
      template <__explicitly_convertible_to<value_type> _Up>
        [[__gnu__::__always_inline__]]
        constexpr explicit(!__broadcast_constructible<_Up, value_type>)
        basic_vec(_Up&& __x) noexcept
          : _M_data(_DataType() == _DataType() ? static_cast<value_type>(__x) : value_type())
        {}

      template <__simd_vec_bcast_consteval<value_type> _Up>
        consteval
        basic_vec(_Up&& __x)
        : _M_data(_DataType() == _DataType()
                    ? __value_preserving_cast<value_type>(__x) : value_type())
        {}

      // [simd.ctor] conversion constructor -----------------------------------
      template <typename _Up, typename _UAbi>
        requires (__simd_size_v<_Up, _UAbi> == _S_size)
          && __explicitly_convertible_to<_Up, value_type>
        // FIXME(file LWG issue): missing constraint `constructible_from<value_type, _Up>`
        [[__gnu__::__always_inline__]]
        constexpr
        explicit(!__value_preserving_convertible_to<_Up, value_type>
                   || __higher_rank_than<_Up, value_type>)
        basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
          : _M_data([&] [[__gnu__::__always_inline__]]() {
              if constexpr (_S_is_scalar)
                return static_cast<value_type>(__x[0]);
              else
                return __vec_cast<_DataType>(__x._M_concat_data());
            }())
        {}

      // [simd.ctor] generator constructor ------------------------------------
      template <__simd_generator_invokable<value_type, _S_size> _Fp>
        [[__gnu__::__always_inline__]]
        constexpr explicit
        basic_vec(_Fp&& __gen)
        : _M_data([&] [[__gnu__::__always_inline__]] {
            constexpr auto [...__is] = _IotaArray<_S_size>;
            return _DataType{static_cast<value_type>(__gen(__simd_size_c<__is>))...};
          }())
        {}

      // [simd.ctor] load constructor -----------------------------------------
      template <typename _Up>
        [[__gnu__::__always_inline__]]
        constexpr
        basic_vec(_LoadCtorTag, const _Up* __ptr)
          : _M_data()
        {
          if constexpr (_S_is_scalar)
            _M_data = static_cast<value_type>(__ptr[0]);
          else if (__builtin_is_constant_evaluated())
            {
              constexpr auto [...__is] = _IotaArray<_S_size>;
              _M_data = _DataType{static_cast<value_type>(__ptr[__is])...};
            }
          else if constexpr (__converts_trivially<_Up, value_type>)
            {
              // This assumes std::floatN_t to be bitwise equal to float/double
              __builtin_memcpy(&_M_data, __ptr, sizeof(value_type) * _S_size);
            }
          else
            {
              __vec_builtin_type<_Up, _S_full_size> __tmp = {};
              __builtin_memcpy(&__tmp, __ptr, sizeof(_Up) * _S_size);
              _M_data = __vec_cast<_DataType>(__tmp);
            }
        }

      template <__static_sized_range<size.value> _Rg, typename... _Flags>
        requires __vectorizable<ranges::range_value_t<_Rg>>
          && __explicitly_convertible_to<ranges::range_value_t<_Rg>, value_type>
        [[__gnu__::__always_inline__]]
        constexpr
        basic_vec(_Rg&& __range, flags<_Flags...> __flags = {})
          : basic_vec(_LoadCtorTag(), __flags.template _S_adjust_pointer<basic_vec>(
                                        ranges::data(__range)))
        {
          static_assert(__loadstore_convertible_to<ranges::range_value_t<_Rg>, value_type,
                                                   _Flags...>);
        }

      // [simd.subscr] --------------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      {
        if constexpr (_S_is_scalar)
          return _M_data;
        else
          return _M_data[__i];
      }

      // [simd.unary] unary operators -----------------------------------------
      // increment and decrement are implemented in terms of operator+=/-= which avoids UB on
      // padding elements while not breaking UBsan
      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator++() noexcept requires requires(value_type __a) { ++__a; }
      { return *this += value_type(1); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator++(int) noexcept requires requires(value_type __a) { __a++; }
      {
        basic_vec __r = *this;
        *this += value_type(1);
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator--() noexcept requires requires(value_type __a) { --__a; }
      { return *this -= value_type(1); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator--(int) noexcept requires requires(value_type __a) { __a--; }
      {
        basic_vec __r = *this;
        *this -= value_type(1);
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      operator!() const noexcept requires requires(value_type __a) { !__a; }
      { return mask_type::_S_init(!_M_data); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator+() const noexcept requires requires(value_type __a) { +__a; }
      { return *this; }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator-() const noexcept requires requires(value_type __a) { -__a; }
      { return _S_init(-_M_data); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator~() const noexcept requires requires(value_type __a) { ~__a; }
      { return _S_init(~_M_data); }

      // [simd.cassign] binary operators
#define _GLIBCXX_SIMD_DEFINE_OP(sym)                                 \
      [[__gnu__::__always_inline__]]                                 \
      friend constexpr basic_vec&                                    \
      operator sym##=(basic_vec& __x, const basic_vec& __y) noexcept \
      requires requires(value_type __a) { __a sym __a; }             \
      {                                                              \
        __x._M_data sym##= __y._M_data;                              \
        return __x;                                                  \
      }

      _GLIBCXX_SIMD_DEFINE_OP(&)
      _GLIBCXX_SIMD_DEFINE_OP(|)
      _GLIBCXX_SIMD_DEFINE_OP(^)

#undef _GLIBCXX_SIMD_DEFINE_OP

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator+=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a + __a; }
      {
        if constexpr (_S_is_partial && is_integral_v<value_type> && is_signed_v<value_type>)
          { // avoid spurious UB on signed integer overflow of the padding element(s). But don't
            // remove UB of the active elements (so that UBsan can still do its job).
            using _UV = typename _Ap::template _DataType<make_unsigned_t<value_type>>;
            const _DataType __result
              = reinterpret_cast<_DataType>(reinterpret_cast<_UV>(__x._M_data)
                                              + reinterpret_cast<_UV>(__y._M_data));
            const auto __positive = __y > value_type();
            const auto __overflow = __positive != (__result > __x);
            if (__overflow._M_any_of())
              __builtin_unreachable(); // trigger UBsan
            __x._M_data = __result;
          }
        else if constexpr (_TargetTraits()._M_eval_as_f32<value_type>())
          __x = basic_vec(rebind_t<float, basic_vec>(__x) + __y);
        else
          __x._M_data += __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator-=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a - __a; }
      {
        if constexpr (_S_is_partial && is_integral_v<value_type> && is_signed_v<value_type>)
          { // avoid spurious UB on signed integer overflow of the padding element(s). But don't
            // remove UB of the active elements (so that UBsan can still do its job).
            using _UV = typename _Ap::template _DataType<make_unsigned_t<value_type>>;
            const _DataType __result
              = reinterpret_cast<_DataType>(reinterpret_cast<_UV>(__x._M_data)
                                              - reinterpret_cast<_UV>(__y._M_data));
            const auto __positive = __y > value_type();
            const auto __overflow = __positive != (__result < __x);
            if (__overflow._M_any_of())
              __builtin_unreachable(); // trigger UBsan
            __x._M_data = __result;
          }
        else if constexpr (_TargetTraits()._M_eval_as_f32<value_type>())
          __x = basic_vec(rebind_t<float, basic_vec>(__x) - __y);
        else
          __x._M_data -= __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator*=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a * __a; }
      {
        if constexpr (_S_is_partial && is_integral_v<value_type> && is_signed_v<value_type>)
          { // avoid spurious UB on signed integer overflow of the padding element(s). But don't
            // remove UB of the active elements (so that UBsan can still do its job).
            for (int __i = 0; __i < _S_size; ++__i)
              {
                if (__builtin_mul_overflow_p(__x._M_data[__i], __y._M_data[__i], value_type()))
                  __builtin_unreachable();
              }
            using _UV = typename _Ap::template _DataType<make_unsigned_t<value_type>>;
            __x._M_data = reinterpret_cast<_DataType>(reinterpret_cast<_UV>(__x._M_data)
                                                        * reinterpret_cast<_UV>(__y._M_data));
          }

        // 'uint16 * uint16' promotes to int and can therefore lead to UB. The standard does not
        // require to avoid the undefined behavior. It's unnecessary and easy to avoid. It's also
        // unexpected because there's no UB on the vector types (which don't promote).
        else if constexpr (_S_is_scalar && is_unsigned_v<value_type>
                             && is_signed_v<decltype(value_type() * value_type())>)
          __x._M_data = unsigned(__x._M_data) * unsigned(__y._M_data);

        else if constexpr (_TargetTraits()._M_eval_as_f32<value_type>())
          __x = basic_vec(rebind_t<float, basic_vec>(__x) * __y);

        else
          __x._M_data *= __y._M_data;
        return __x;
      }

      template <_TargetTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        friend constexpr basic_vec&
        operator/=(basic_vec& __x, const basic_vec& __y) noexcept
        requires requires(value_type __a) { __a / __a; }
        {
          const basic_vec __result([&](int __i) -> value_type { return __x[__i] / __y[__i]; });
          if (__is_const_known(__result))
            // the optimizer already knows the values of the result
            return __x = __result;

#ifdef __SSE2__
          // x86 doesn't have integral SIMD division instructions
          // While division is faster, the required conversions are still a problem:
          // see PR121274, PR121284, and PR121296 for missed optimizations wrt. conversions
          //
          // With only 1 or 2 divisions, the conversion to and from fp is too expensive.
          if constexpr (is_integral_v<value_type> && _S_size > 2
                          && __value_preserving_convertible_to<value_type, double>)
            {
              // If the denominator (y) is known to the optimizer, don't convert to fp because the
              // integral division can be translated into shifts/multiplications.
              if (!__is_const_known(__y))
                {
                  // With AVX512FP16 use vdivph for 8-bit integers
                  if constexpr (_Traits._M_have_avx512fp16()
                                  && __value_preserving_convertible_to<value_type, _Float16>)
                    return __x = basic_vec(rebind_t<_Float16, basic_vec>(__x) / __y);
                  else if constexpr (__value_preserving_convertible_to<value_type, float>)
                    return __x = basic_vec(rebind_t<float, basic_vec>(__x) / __y);
                  else
                    return __x = basic_vec(rebind_t<double, basic_vec>(__x) / __y);
                }
            }
#endif
          if constexpr (_Traits._M_eval_as_f32<value_type>())
            return __x = basic_vec(rebind_t<float, basic_vec>(__x) / __y);

          basic_vec __y1 = __y;
          if constexpr (_S_is_partial)
            {
              if constexpr (is_integral_v<value_type>)
                {
                  // Assume integral division doesn't have SIMD instructions and must be done per
                  // element anyway. Partial vectors should skip their padding elements.
                  for (int __i = 0; __i < _S_size; ++__i)
                    __x._M_data[__i] /= __y._M_data[__i];
                  return __x;
                }
              else
                __y1 = __select_impl(mask_type::_S_init(mask_type::_S_implicit_mask),
                                     __y, basic_vec(value_type(1)));
            }
          __x._M_data /= __y1._M_data;
          return __x;
        }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator%=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a % __a; }
      {
        static_assert(is_integral_v<value_type>);
        if constexpr (_S_is_partial)
          {
            const basic_vec __y1 = __select_impl(mask_type::_S_init(mask_type::_S_implicit_mask),
                                                 __y, basic_vec(value_type(1)));
            if (__is_const_known(__y1))
              __x._M_data %= __y1._M_data;
            else
              {
                // Assume integral division doesn't have SIMD instructions and must be done per
                // element anyway. Partial vectors should skip their padding elements.
                for (int __i = 0; __i < _S_size; ++__i)
                  __x._M_data[__i] %= __y._M_data[__i];
              }
          }
        else
          __x._M_data %= __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator<<=(basic_vec& __x, const basic_vec& __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires(value_type __a) { __a << __a; }
      {
        __glibcxx_simd_precondition(is_unsigned_v<value_type> || all_of(__y >= value_type()),
                                    "negative shift is undefined behavior");
        __glibcxx_simd_precondition(all_of(__y < __max_shift<value_type>),
                                    "too large shift invokes undefined behavior");
        __x._M_data <<= __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator>>=(basic_vec& __x, const basic_vec& __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires(value_type __a) { __a >> __a; }
      {
        __glibcxx_simd_precondition(is_unsigned_v<value_type> || all_of(__y >= value_type()),
                                    "negative shift is undefined behavior");
        __glibcxx_simd_precondition(all_of(__y < __max_shift<value_type>),
                                    "too large shift invokes undefined behavior");
        __x._M_data >>= __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator<<=(basic_vec& __x, __simd_size_type __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires(value_type __a, __simd_size_type __b) { __a << __b; }
      {
        __glibcxx_simd_precondition(__y >= 0, "negative shift is undefined behavior");
        __glibcxx_simd_precondition(__y < int(__max_shift<value_type>),
                                    "too large shift invokes undefined behavior");
        __x._M_data <<= __y;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator>>=(basic_vec& __x, __simd_size_type __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires(value_type __a, __simd_size_type __b) { __a >> __b; }
      {
        __glibcxx_simd_precondition(__y >= 0, "negative shift is undefined behavior");
        __glibcxx_simd_precondition(__y < int(__max_shift<value_type>),
                                    "too large shift invokes undefined behavior");
        __x._M_data >>= __y;
        return __x;
      }

      // [simd.comparison] ----------------------------------------------------
#if _GLIBCXX_X86
      template <_X86Cmp _Cmp>
        [[__gnu__::__always_inline__]]
        constexpr mask_type
        _M_bitmask_cmp(_DataType __y) const
        {
          static_assert(_S_use_bitmask);
          if (__is_const_known(_M_data, __y))
            {
              constexpr auto [...__is] = _IotaArray<_S_size>;
              constexpr auto __cmp_op = [] [[__gnu__::__always_inline__]]
                                          (value_type __a, value_type __b) {
                if constexpr (_Cmp == _X86Cmp::_Eq)
                  return __a == __b;
                else if constexpr (_Cmp == _X86Cmp::_Lt)
                  return __a < __b;
                else if constexpr (_Cmp == _X86Cmp::_Le)
                  return __a <= __b;
                else if constexpr (_Cmp == _X86Cmp::_Unord)
                  return std::isunordered(__a, __b);
                else if constexpr (_Cmp == _X86Cmp::_Neq)
                  return __a != __b;
                else if constexpr (_Cmp == _X86Cmp::_Nlt)
                  return !(__a < __b);
                else if constexpr (_Cmp == _X86Cmp::_Nle)
                  return !(__a <= __b);
                else
                  static_assert(false);
              };
              return mask_type::_S_init(((__cmp_op(__vec_get(_M_data, __is), __vec_get(__y, __is))
                                            ? (1ULL << __is) : 0) | ...));
            }
          else
            return mask_type::_S_init(__x86_bitmask_cmp<_Cmp>(_M_data, __y));
        }
#endif

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator==(const basic_vec& __x, const basic_vec& __y) noexcept
      {
#if _GLIBCXX_X86
        if constexpr (_S_use_bitmask)
          return __x._M_bitmask_cmp<_X86Cmp::_Eq>(__y._M_data);
        else
#endif
          return mask_type::_S_init(__x._M_data == __y._M_data);
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator!=(const basic_vec& __x, const basic_vec& __y) noexcept
      {
#if _GLIBCXX_X86
        if constexpr (_S_use_bitmask)
          return __x._M_bitmask_cmp<_X86Cmp::_Neq>(__y._M_data);
        else
#endif
          return mask_type::_S_init(__x._M_data != __y._M_data);
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<(const basic_vec& __x, const basic_vec& __y) noexcept
      {
#if _GLIBCXX_X86
        if constexpr (_S_use_bitmask)
          return __x._M_bitmask_cmp<_X86Cmp::_Lt>(__y._M_data);
        else
#endif
          return mask_type::_S_init(__x._M_data < __y._M_data);
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<=(const basic_vec& __x, const basic_vec& __y) noexcept
      {
#if _GLIBCXX_X86
        if constexpr (_S_use_bitmask)
          return __x._M_bitmask_cmp<_X86Cmp::_Le>(__y._M_data);
        else
#endif
          return mask_type::_S_init(__x._M_data <= __y._M_data);
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>(const basic_vec& __x, const basic_vec& __y) noexcept
      { return __y < __x; }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return __y <= __x; }

      // [simd.cond] ---------------------------------------------------------
      template <_TargetTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        friend constexpr basic_vec
        __select_impl(const mask_type& __k, const basic_vec& __t, const basic_vec& __f) noexcept
        {
          if constexpr (_S_size == 1)
            return __k[0] ? __t : __f;
          else if constexpr (_S_use_bitmask)
            {
#if _GLIBCXX_X86
              if (__is_const_known(__k, __t, __f))
                return basic_vec([&](int __i) { return __k[__i] ? __t[__i] : __f[__i]; });
              else
                return __x86_bitmask_blend(__k._M_data, __t._M_data, __f._M_data);
#else
              static_assert(false, "TODO");
#endif
            }
          else if (__builtin_is_constant_evaluated())
            return __k._M_data ? __t._M_data : __f._M_data;
          else
            {
              using _VO = _VecOps<_DataType>;
              if (_VO::_S_is_const_known_equal_to(__f._M_data, 0))
                {
                  if (is_integral_v<value_type> && sizeof(_M_data) >= 8
                        && _VO::_S_is_const_known_equal_to(__t._M_data, 1))
                    // basic_mask::operator+ arrives here; '+__k' would be recursive
                    return basic_vec((-__k)._M_abs());
                  /*                  else if (is_integral_v<value_type> && sizeof(_M_data) >= 8
                             && _VO::_S_is_const_known_equal_to(__t._M_data, value_type(-1)))
                    return basic_vec(-__k);*/
                  else
                    return __vec_and(reinterpret_cast<_DataType>(__k._M_data), __t._M_data);
                }
              else if (_VecOps<_DataType>::_S_is_const_known_equal_to(__t._M_data, 0))
                {
                  if (is_integral_v<value_type> && sizeof(_M_data) >= 8
                        && _VO::_S_is_const_known_equal_to(__f._M_data, 1))
                    return value_type(1) + basic_vec(-__k);
                  else
                    return __vec_and(reinterpret_cast<_DataType>(__vec_not(__k._M_data)), __f._M_data);
                }
              else
                {
#if _GLIBCXX_X86
                  // this works around bad code-gen when the compiler can't see that __k is a vector-mask.
                  // This pattern, is recognized to match the x86 blend instructions, which only consider
                  // the sign bit of the mask register. Also, without SSE4, if the compiler knows that __k
                  // is a vector-mask, then the '< 0' is elided.
                  return __k._M_data < 0 ? __t._M_data : __f._M_data;
#endif
                  return __k._M_data ? __t._M_data : __f._M_data;
                }
            }
        }
    };

  template <__vectorizable _Tp, __abi_tag _Ap>
    requires (_Ap::_S_nreg > 1)
      && (!__complex_like<_Tp>)
    class basic_vec<_Tp, _Ap>
    : _BinaryOps<_Tp, _Ap>
    {
      template <typename, typename>
        friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr int _N0 = __bit_ceil(unsigned(_S_size)) / 2;

      static constexpr int _N1 = _S_size - _N0;

      using _DataType0 = __similar_vec<_Tp, _N0, _Ap>;

      using _DataType1 = __similar_vec<_Tp, _N1, _Ap>;

      static_assert(_DataType0::abi_type::_S_nreg + _DataType1::abi_type::_S_nreg == _Ap::_S_nreg);

      static constexpr bool _S_is_scalar = _DataType0::_S_is_scalar;

      _DataType0 _M_data0;

      _DataType1 _M_data1;

    public:
      using value_type = _Tp;

      using abi_type = _Ap;

      using mask_type = basic_mask<sizeof(_Tp), abi_type>;

      using iterator = __iterator<basic_vec>;

      using const_iterator = __iterator<const basic_vec>;

      constexpr iterator
      begin() noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      begin() const noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      cbegin() const noexcept
      { return {*this, 0}; }

      constexpr default_sentinel_t
      end() const noexcept
      { return {}; }

      constexpr default_sentinel_t
      cend() const noexcept
      { return {}; }

      static constexpr auto size = __simd_size_c<_S_size>;

      [[__gnu__::__always_inline__]]
      static constexpr basic_vec
      _S_init(const _DataType0& __x, const _DataType1& __y)
      {
        basic_vec __r;
        __r._M_data0 = __x;
        __r._M_data1 = __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr const _DataType0&
      _M_get_low() const
      { return _M_data0; }

      [[__gnu__::__always_inline__]]
      constexpr const _DataType1&
      _M_get_high() const
      { return _M_data1; }

      [[__gnu__::__always_inline__]]
      friend constexpr bool
      __is_const_known(const basic_vec& __x)
      { return __is_const_known(__x._M_data0) && __is_const_known(__x._M_data1); }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_concat_data() const
      {
        return __vec_concat(_M_data0._M_concat_data(),
                            __vec_zero_pad_to<sizeof(_M_data0)>(_M_data1._M_concat_data()));
      }

      template <int _Size = _S_size, int _Offset = 0, typename _A0, typename _Fp>
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_static_permute(const basic_vec<value_type, _A0>& __x, _Fp&& __idxmap)
        {
          return _S_init(
                   _DataType0::template _S_static_permute<_Size, _Offset>(__x, __idxmap),
                   _DataType1::template _S_static_permute<_Size, _Offset + _N0>(__x, __idxmap));
        }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      _M_complex_conj() const
      { return _S_init(_M_data0._M_complex_conj(), _M_data1._M_complex_conj()); }

      template <typename _CxVec, _TargetTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr void
        _M_complex_multiply_with(const basic_vec& __yvec)
        {
          _M_data0.template _M_complex_multiply_with<_CxVec>(__yvec._M_data0);
          _M_data1.template _M_complex_multiply_with<_CxVec>(__yvec._M_data1);
        }

      template <typename _Cx>
        [[__gnu__::__always_inline__]]
        static constexpr void
        _S_cxctgus_mul(basic_vec& __re0, basic_vec& __im0,
                       const basic_vec& __re1, const basic_vec& __im1)
        {
          _DataType0::template _S_cxctgus_mul<_Cx>(__re0._M_data0, __im0._M_data0,
                                                   __re1._M_data0, __im1._M_data0);
          _DataType1::template _S_cxctgus_mul<_Cx>(__re0._M_data1, __im0._M_data1,
                                                   __re1._M_data1, __im1._M_data1);
        }

      template <typename _Vp>
        [[__gnu__::__always_inline__]]
        constexpr auto
        _M_chunk() const noexcept
        {
          constexpr int __n = _S_size / _Vp::_S_size;
          constexpr int __rem = _S_size % _Vp::_S_size;
          if constexpr (_N0 == _Vp::_S_size)
            {
              if constexpr (__rem == 0)
                return array<_Vp, __n> {_M_data0, _M_data1};
              else
                return tuple<_Vp, resize_t<__rem, _Vp>> {_M_data0, _M_data1};
            }
          else if constexpr (__rem == 0)
            {
              using _Rp = array<_Vp, __n>;
              if constexpr (sizeof(_Rp) == sizeof(*this))
                {
                  static_assert(!_Vp::_S_is_partial);
                  return __builtin_bit_cast(_Rp, *this);
                }
              else
                {
                  constexpr auto [...__is] = _IotaArray<__n>;
                  return _Rp {_Vp([&](int __i) { return (*this)[__i + __is * _Vp::_S_size]; })...};
                }
            }
          else
            {
              constexpr auto [...__is] = _IotaArray<__n>;
              using _Rest = resize_t<__rem, _Vp>;
              // can't bit-cast because the member order of tuple is reversed
              return tuple(_Vp  ([&](int __i) { return (*this)[__i + __is * _Vp::_S_size]; })...,
                           _Rest([&](int __i) { return (*this)[__i + __n * _Vp::_S_size]; }));
            }
        }

      template <typename _A0, typename... _As>
        [[__gnu__::__always_inline__]]
        constexpr void
        _M_assign_from(auto _Offset, const basic_vec<value_type, _A0>& __x0,
                       const basic_vec<value_type, _As>&... __xs)
        {
          if constexpr (_Offset.value >= _A0::_S_size)
            // make the pack as small as possible
            _M_assign_from(integral_constant<int, _Offset.value - _A0::_S_size>(), __xs...);
          else
            {
              _M_data0._M_assign_from(_Offset, __x0, __xs...);
              _M_data1._M_assign_from(integral_constant<int, _Offset + _DataType0::size>(),
                                      __x0, __xs...);
            }
        }

      template <typename _A0>
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_concat(const basic_vec<value_type, _A0>& __x0) noexcept
        { return basic_vec(__x0); }

      template <typename _A0, typename... _As>
        requires (sizeof...(_As) >= 1)
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_concat(const basic_vec<value_type, _A0>& __x0,
                  const basic_vec<value_type, _As>&... __xs) noexcept
        {
          if constexpr (_A0::_S_size == _N0)
            {
              if constexpr (sizeof...(_As) == 1)
                return _S_init(__x0, __xs...);
              else
                return _S_init(__x0, _DataType1::_S_concat(__xs...));
            }
          else if (__is_const_known(__x0, __xs...))
            {
              basic_vec __r;
              __r._M_data0.template _M_assign_from(integral_constant<int, 0>(), __x0, __xs...);
              __r._M_data1.template _M_assign_from(_DataType0::size, __x0, __xs...);
              return __r;
            }
          else
            {
              basic_vec __r = {};
              byte* __dst = reinterpret_cast<byte*>(&__r);
              constexpr size_t __nbytes0 = sizeof(value_type) * _A0::_S_size;
              __builtin_memcpy(__dst, &__x0, _A0::_S_nreg == 1 ? sizeof(__x0) : __nbytes0);
              __dst += sizeof(value_type) * _A0::_S_size;
              template for (const auto& __x : {__xs...})
                {
                  constexpr size_t __nbytes = sizeof(value_type) * __x.size.value;
                  __builtin_memcpy(__dst, &__x, __nbytes);
                  __dst += __nbytes;
                }
              return __r;
            }
        }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_reduce_1(auto __binary_op) const
      {
        static_assert(_N0 == _N1);
        return __binary_op(_M_data0, _M_data1);
      }

      [[__gnu__::__always_inline__]]
      constexpr value_type
      _M_reduce_tail(const auto& __rest, auto __binary_op) const
      {
        if constexpr (__rest.size() > _S_size)
          {
            auto [__a, __b] = __rest.template _M_chunk<basic_vec>();
            return __binary_op(*this, __a)._M_reduce_tail(__b, __binary_op);
          }
        else if constexpr (__rest.size() == _S_size)
          return __binary_op(*this, __rest)._M_reduce(__binary_op);
        else
          return _M_reduce_1(__binary_op)._M_reduce_tail(__rest, __binary_op);
      }

      template <typename _BinaryOp, _TargetTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr value_type
        _M_reduce(_BinaryOp __binary_op) const
        {
          if constexpr (_Traits.template _M_eval_as_f32<value_type>()
                          && (is_same_v<_BinaryOp, plus<>>
                                 || is_same_v<_BinaryOp, multiplies<>>))
            return value_type(rebind_t<float, basic_vec>(*this)._M_reduce(__binary_op));
#ifdef __SSE2__
          else if constexpr (is_integral_v<value_type> && sizeof(value_type) == 1
                               && is_same_v<decltype(__binary_op), multiplies<>>)
            {
              // convert to unsigned short because of missing 8-bit mul instruction
              // we don't need to preserve the order of elements
#if 1
              using _V16 = resize_t<_S_size / 2, rebind_t<unsigned short, basic_vec>>;
              auto __a = __builtin_bit_cast(_V16, *this);
              return __binary_op(__a, __a >> 8)._M_reduce(__binary_op);
#else
              // alternative:
              using _V16 = rebind_t<unsigned short, basic_vec>;
              return _V16(*this)._M_reduce(__binary_op);
#endif
            }
#endif
          else if constexpr (_N0 == _N1)
            return _M_reduce_1(__binary_op)._M_reduce(__binary_op);
#if 0 // needs benchmarking before we do this
          else if constexpr (sizeof(_M_data0) == sizeof(_M_data1)
                               && requires {
                                 __default_identity_element<value_type, decltype(__binary_op)>();
                               })
            { // extend to power-of-2 with identity element for more parallelism
              _DataType0 __v1 = __builtin_bit_cast(_DataType0, _M_data1);
              constexpr _DataType0 __id
                = __default_identity_element<value_type, decltype(__binary_op)>();
              constexpr auto __k = _DataType0::mask_type::_S_partial_mask_of_n(_N1);
              __v1 = __select_impl(__k, __v1, __id);
              return __binary_op(_M_data0, __v1)._M_reduce(__binary_op);
            }
#endif
          else
            return _M_data0._M_reduce_1(__binary_op)._M_reduce_tail(_M_data1, __binary_op);
        }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      _M_isnan() const requires is_floating_point_v<value_type>
      { return mask_type::_S_init(_M_data0._M_isnan(), _M_data1._M_isnan()); }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      _M_isinf() const requires is_floating_point_v<value_type>
      { return mask_type::_S_init(_M_data0._M_isinf(), _M_data1._M_isinf()); }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      _M_isunordered(basic_vec __y) const requires is_floating_point_v<value_type>
      {
        return mask_type::_S_init(_M_data0._M_isunordered(__y._M_data0),
                                  _M_data1._M_isunordered(__y._M_data1));
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      _M_abs() const
      { return _S_init(_M_data0._M_abs(), _M_data1._M_abs()); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      _M_fabs() const
      { return _S_init(_M_data0._M_fabs(), _M_data1._M_fabs()); }

      template <typename _Up>
        [[__gnu__::__always_inline__]]
        static inline basic_vec
        _S_partial_load(const _Up* __mem, size_t __n)
        {
          if (__n >= _N0)
            return _S_init(_DataType0(_LoadCtorTag(), __mem),
                           _DataType1::_S_partial_load(__mem + _N0, __n - _N0));
          else
            return _S_init(_DataType0::_S_partial_load(__mem, __n),
                           _DataType1());
        }

      template <typename _Up, _ArchTraits _Traits = {}>
        static inline basic_vec
        _S_masked_load(const _Up* __mem, mask_type __k)
        {
          return _S_init(_DataType0::_S_masked_load(__mem, __k._M_data0),
                         _DataType1::_S_masked_load(__mem + _N0, __k._M_data1));
        }

      template <typename _Up>
        [[__gnu__::__always_inline__]]
        inline void
        _M_store(_Up* __mem) const
        {
          _M_data0._M_store(__mem);
          _M_data1._M_store(__mem + _N0);
        }

      template <typename _Up>
        [[__gnu__::__always_inline__]]
        static inline void
        _S_partial_store(const basic_vec& __v, _Up* __mem, size_t __n)
        {
          if (__n >= _N0)
            {
              __v._M_data0._M_store(__mem);
              _DataType1::_S_partial_store(__v._M_data1, __mem + _N0, __n - _N0);
            }
          else
            {
              _DataType0::_S_partial_store(__v._M_data0, __mem, __n);
            }
        }

      template <typename _Up>
        [[__gnu__::__always_inline__]]
        static inline void
        _S_masked_store(const basic_vec& __v, _Up* __mem, const mask_type& __k)
        {
          _DataType0::_S_masked_store(__v._M_data0, __mem, __k._M_data0);
          _DataType1::_S_masked_store(__v._M_data1, __mem + _N0, __k._M_data1);
        }

      basic_vec() = default;

      // [simd.overview] impl-def conversions ---------------------------------
      using _NativeVecType
        = decltype([] {
            if constexpr (_S_is_scalar)
              return _InvalidInteger();
            else
              return __vec_builtin_type<value_type, __bit_ceil(unsigned(_S_size))>();
          }());

      [[__gnu__::__always_inline__]]
      constexpr
      basic_vec(const _NativeVecType& __x) requires (!_S_is_scalar)
      : _M_data0(_VecOps<__vec_builtin_type<value_type, _N0>>::_S_extract(__x)),
        _M_data1(_VecOps<__vec_builtin_type<value_type, __bit_ceil(unsigned(_N1))>>
                   ::_S_extract(__x, integral_constant<int, _N0>()))
      {}

      [[__gnu__::__always_inline__]]
      constexpr
      operator _NativeVecType() const requires (!_S_is_scalar)
      { return _M_concat_data(); }

      // [simd.ctor] broadcast constructor ------------------------------------
      template <__explicitly_convertible_to<value_type> _Up>
        [[__gnu__::__always_inline__]]
        constexpr explicit(!__broadcast_constructible<_Up, value_type>)
        basic_vec(_Up&& __x) noexcept
          : _M_data0(static_cast<value_type>(__x)), _M_data1(static_cast<value_type>(__x))
        {}

      template <__simd_vec_bcast_consteval<value_type> _Up>
        consteval
        basic_vec(_Up&& __x)
        : _M_data0(__value_preserving_cast<value_type>(__x)),
          _M_data1(__value_preserving_cast<value_type>(__x))
        {}

      // [simd.ctor] conversion constructor -----------------------------------
      template <typename _Up, typename _UAbi>
        requires (__simd_size_v<_Up, _UAbi> == _S_size)
          && __explicitly_convertible_to<_Up, value_type>
        [[__gnu__::__always_inline__]]
        constexpr
        explicit(!__value_preserving_convertible_to<_Up, value_type>
                   || __higher_rank_than<_Up, value_type>)
        basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
          : _M_data0(get<0>(chunk<_N0>(__x))),
            _M_data1(get<1>(chunk<_N0>(__x)))
        {}

      // [simd.ctor] generator constructor ------------------------------------
      template <__simd_generator_invokable<value_type, _S_size> _Fp>
        [[__gnu__::__always_inline__]]
        constexpr explicit
        basic_vec(_Fp&& __gen)
          : _M_data0(__gen), _M_data1([&] [[__gnu__::__always_inline__]] (auto __i) {
                               return __gen(__simd_size_c<__i + _N0>);
                             })
        {}

      // [simd.ctor] load constructor -----------------------------------------
      template <typename _Up>
        [[__gnu__::__always_inline__]]
        constexpr
        basic_vec(_LoadCtorTag, const _Up* __ptr)
          : _M_data0(_LoadCtorTag(), __ptr),
            _M_data1(_LoadCtorTag(), __ptr + _N0)
        {}

      template <__static_sized_range<size.value> _Rg, typename... _Flags>
        requires __vectorizable<ranges::range_value_t<_Rg>>
          && __explicitly_convertible_to<ranges::range_value_t<_Rg>, value_type>
        constexpr
        basic_vec(_Rg&& __range, flags<_Flags...> __flags = {})
        : basic_vec(_LoadCtorTag(),
                    __flags.template _S_adjust_pointer<basic_vec>(ranges::data(__range)))
        {
          static_assert(__loadstore_convertible_to<ranges::range_value_t<_Rg>, value_type,
                                                   _Flags...>);
        }

      // [simd.subscr] --------------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      {
        struct _Tmp
        { alignas(basic_vec) const value_type _M_values[_S_size]; };
        return __builtin_bit_cast(_Tmp, *this)._M_values[__i];
      }

      // [simd.unary] unary operators -----------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator++() noexcept requires requires(value_type __a) { ++__a; }
      {
        ++_M_data0;
        ++_M_data1;
        return *this;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator++(int) noexcept requires requires(value_type __a) { __a++; }
      {
        basic_vec __r = *this;
        ++_M_data0;
        ++_M_data1;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator--() noexcept requires requires(value_type __a) { --__a; }
      {
        --_M_data0;
        --_M_data1;
        return *this;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator--(int) noexcept requires requires(value_type __a) { __a--; }
      {
        basic_vec __r = *this;
        --_M_data0;
        --_M_data1;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      operator!() const noexcept requires requires(value_type __a) { !__a; }
      { return mask_type::_S_init(!_M_data0, !_M_data1); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator+() const noexcept requires requires(value_type __a) { +__a; }
      { return *this; }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator-() const noexcept requires requires(value_type __a) { -__a; }
      { return _S_init(-_M_data0, -_M_data1); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator~() const noexcept requires requires(value_type __a) { ~__a; }
      { return _S_init(~_M_data0, ~_M_data1); }

      // [simd.cassign] -------------------------------------------------------
#define _GLIBCXX_SIMD_DEFINE_OP(sym)                                 \
      [[__gnu__::__always_inline__]]                                 \
      friend constexpr basic_vec&                                    \
      operator sym##=(basic_vec& __x, const basic_vec& __y) _GLIBCXX_SIMD_NOEXCEPT \
      {                                                              \
        __x._M_data0 sym##= __y._M_data0;                            \
        __x._M_data1 sym##= __y._M_data1;                            \
        return __x;                                                  \
      }

      _GLIBCXX_SIMD_DEFINE_OP(+)
      _GLIBCXX_SIMD_DEFINE_OP(-)
      _GLIBCXX_SIMD_DEFINE_OP(*)
      _GLIBCXX_SIMD_DEFINE_OP(/)
      _GLIBCXX_SIMD_DEFINE_OP(%)
      _GLIBCXX_SIMD_DEFINE_OP(&)
      _GLIBCXX_SIMD_DEFINE_OP(|)
      _GLIBCXX_SIMD_DEFINE_OP(^)
      _GLIBCXX_SIMD_DEFINE_OP(<<)
      _GLIBCXX_SIMD_DEFINE_OP(>>)

#undef _GLIBCXX_SIMD_DEFINE_OP

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator<<=(basic_vec& __x, __simd_size_type __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires(value_type __a, __simd_size_type __b) { __a << __b; }
      {
        __x._M_data0 <<= __y;
        __x._M_data1 <<= __y;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator>>=(basic_vec& __x, __simd_size_type __y) _GLIBCXX_SIMD_NOEXCEPT
      requires requires(value_type __a, __simd_size_type __b) { __a >> __b; }
      {
        __x._M_data0 >>= __y;
        __x._M_data1 >>= __y;
        return __x;
      }

      // [simd.comparison] ----------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator==(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 == __y._M_data0, __x._M_data1 == __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator!=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 != __y._M_data0, __x._M_data1 != __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 < __y._M_data0, __x._M_data1 < __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 <= __y._M_data0, __x._M_data1 <= __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 > __y._M_data0, __x._M_data1 > __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 >= __y._M_data0, __x._M_data1 >= __y._M_data1); }

      // [simd.cond] ---------------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec
      __select_impl(const mask_type& __k, const basic_vec& __t, const basic_vec& __f) noexcept
      {
        return _S_init(__select_impl(__k._M_data0, __t._M_data0, __f._M_data0),
                       __select_impl(__k._M_data1, __t._M_data1, __f._M_data1));
      }
    };

  // [simd.overview] deduction guide ------------------------------------------
  template <__static_sized_range _Rg, typename... _Ts>
    basic_vec(_Rg&& __r, _Ts...)
    -> basic_vec<ranges::range_value_t<_Rg>,
                 __deduce_abi_t<ranges::range_value_t<_Rg>,
#if 0 // PR117849
                                ranges::size(__r)>>;
#else
                                decltype(std::span(__r))::extent>>;
#endif

#if 1
  // FIXME: file new LWG issue about this missing deduction guide
  template <size_t _Bytes, typename _Ap>
    basic_vec(basic_mask<_Bytes, _Ap>)
    -> basic_vec<__integer_from<_Bytes>,
                 decltype(__abi_rebind<__integer_from<_Bytes>, basic_mask<_Bytes, _Ap>::size.value,
                                       _Ap>())>;
#endif

  // [P3319R5] ----------------------------------------------------------------
  template <__vectorizable _Tp>
    requires is_arithmetic_v<_Tp>
    inline constexpr _Tp
    __iota<_Tp> = _Tp();

  template <typename _Tp, typename _Ap>
    inline constexpr basic_vec<_Tp, _Ap>
    __iota<basic_vec<_Tp, _Ap>> = basic_vec<_Tp, _Ap>([](_Tp __i) -> _Tp {
      static_assert(__simd_size_v<_Tp, _Ap> - 1 <= numeric_limits<_Tp>::max(),
                    "iota object would overflow");
      return __i;
    });
}

#pragma GCC diagnostic pop
#endif // C++26
#endif // _GLIBCXX_SIMD_VEC_H
