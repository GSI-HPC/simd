/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_BUILTIN_H_
#define PROTOTYPE_SIMD_BUILTIN_H_

#include "detail.h"
#include "arm_detail.h"
#include "power_detail.h"
#include "x86_detail.h"
#include "simd_converter.h"
#include "detail_bitmask.h"

#include <experimental/bits/numeric_traits.h>

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

namespace std::__detail
{
  template <typename _Abi, _BuildFlags = {}>
    struct _ImplBuiltinBase;

  template <typename _Abi, _BuildFlags = {}>
    struct _ImplBuiltin;

  // mask element type for simd_mask<complex<double>>
  template <int _Np>
    struct _MaskVec128
    {
      using value_type = _MaskUInt128;

      static constexpr int _S_size = _Np;

      using _VecType
        = __vec_builtin_type<__mask_integer_from<sizeof(double)>, 2 * __bit_ceil(unsigned(_Np))>;

      using _HalfVecType = __half_vec_builtin_t<_VecType>;

      template <typename _Abi>
        using _UnderlyingImpl = _ImplBuiltin<typename _Abi::template _Rebind<double, 2 * _Np>>;

      _VecType _M_data;

      _GLIBCXX_SIMD_INTRINSIC constexpr bool
      _M_is_constprop() const
      { return __builtin_constant_p(_M_data); }

      _GLIBCXX_SIMD_INTRINSIC constexpr _HalfVecType
      _M_compress_even() const
      { return _VecOps<_HalfVecType>::_S_compress_even(_M_data); }

      _GLIBCXX_SIMD_INTRINSIC constexpr friend _MaskVec128
      __vec_and(const _MaskVec128& __x, const _MaskVec128& __y)
      { return {__vec_and(__x._M_data, __y._M_data)}; }

      _GLIBCXX_SIMD_INTRINSIC constexpr friend _MaskVec128
      __vec_or(const _MaskVec128& __x, const _MaskVec128& __y)
      { return {__vec_or(__x._M_data, __y._M_data)}; }

      _GLIBCXX_SIMD_INTRINSIC constexpr friend _MaskVec128
      __vec_xor(const _MaskVec128& __x, const _MaskVec128& __y)
      { return {__vec_xor(__x._M_data, __y._M_data)}; }

      _GLIBCXX_SIMD_INTRINSIC constexpr friend _MaskVec128
      __vec_andnot(const _MaskVec128& __x, const _MaskVec128& __y)
      { return {__vec_andnot(__x._M_data, __y._M_data)}; }

      _GLIBCXX_SIMD_INTRINSIC constexpr friend _MaskVec128
      __vec_not(const _MaskVec128& __x)
      { return {__vec_not(__x._M_data)}; }
    };
}

namespace std
{
  template <int _Width>
    struct _VecAbi
    {
      using _SimdSizeType = __detail::_SimdSizeType;

      static constexpr _SimdSizeType _S_size = _Width;

      static constexpr bool _S_pass_by_reference = false;

      template <typename _Tp>
        static constexpr bool _S_defer_to_scalar_abi
          = (_Width == 1 and not __detail::__complex_like<_Tp>);

      template <typename _Tp>
        struct _ToVecBuiltin
        { using type = __detail::__vec_builtin_type<_Tp, __detail::__signed_bit_ceil(_Width)>; };

      template <same_as<__detail::_MaskUInt128> _Tp>
        struct _ToVecBuiltin<_Tp>
        { using type = __detail::_MaskVec128<_Width>; };

      template <typename _Tp>
        using _Vp = typename _ToVecBuiltin<_Tp>::type;

      struct _IsValidAbiTag
      : bool_constant<(_Width >= 1)>
      {};

      template <typename _Tp, __detail::_BuildFlags _Flags = {}>
        struct _IsValidSizeFor
        : bool_constant<_Width >= 1
#if not defined __clang__
                          and sizeof(_Vp<_Tp>) == alignof(_Vp<_Tp>)
#else // __clang__
        // Clang unconditionally overaligns vector builtins to their sizeof, so the preceding
        // condition is always true.
#ifdef _GLIBCXX_SIMD_HAVE_SSE
                          and (sizeof(_Vp<_Tp>) <= 16
                                 or (_Flags._M_have_avx() and sizeof(_Vp<_Tp>) <= 32)
                                 or (_Flags._M_have_avx512f() and sizeof(_Vp<_Tp>) <= 64))
#else
                          and sizeof(_Vp<_Tp>) <= 16
#endif
#endif // __clang__
#ifdef _GLIBCXX_SIMD_HAVE_SSE
                          and (not _Flags._M_have_avx() or _Flags._M_have_avx2()
                                 or sizeof(_Vp<_Tp>) <= 16 or is_floating_point_v<_Tp>)
#endif
                       >
        {};

      template <typename _Tp, __detail::_BuildFlags _Flags>
        requires __detail::__complex_like<_Tp>
        struct _IsValidSizeFor<_Tp, _Flags>
        : _VecAbi<_Width * 2>::_IsValidSizeFor<typename _Tp::value_type>
        {};

      template <__detail::_BuildFlags _Flags>
        struct _IsValidSizeFor<__detail::_MaskUInt128, _Flags>
        : _VecAbi<_Width * 2>::_IsValidSizeFor<double>
        {};

      template <typename _Tp>
        struct _IsValid
        : conjunction<_IsValidAbiTag, __detail::__is_vectorizable<_Tp>, _IsValidSizeFor<_Tp>>
        {};

      template <typename _Tp>
        using _MaskValueType = __detail::__make_signed_int_t<_Tp>;

      template <typename _Tp>
        struct __traits
        : __detail::_InvalidTraits
        {};

      template <typename _Tp>
        requires _IsValid<_Tp>::value
        struct __traits<_Tp>
        {
          // conversions to _VecAbi should be implicit unless _FromAbi is derived from _VecAbi
          template <typename _FromAbi>
            static constexpr bool _S_explicit_mask_conversion = is_base_of_v<_VecAbi, _FromAbi>;

          using _Impl = __detail::_ImplBuiltin<_VecAbi>;

          using _SimdImpl = _Impl;

          using _MaskImpl = _Impl;

          using _SimdMember = _Vp<_Tp>;

          using _MaskMember = _Vp<__detail::__mask_integer_from<sizeof(_Tp)>>;

          static constexpr size_t _S_simd_align = alignof(_SimdMember);

          static constexpr size_t _S_mask_align = alignof(_MaskMember);

          static constexpr _SimdSizeType _S_size = _Width;

          static constexpr _SimdSizeType _S_full_size = __detail::__signed_bit_ceil(_Width);

          static constexpr bool _S_is_partial = _S_full_size > _S_size;

          template <typename _Arg>
            static constexpr bool _S_is_simd_ctor_arg
              = std::is_same_v<_Arg, _SimdMember>
#if _GLIBCXX_SIMD_HAVE_SSE
                  or std::is_same_v<_Arg, __detail::__x86_intrin_t<_SimdMember>>
#endif
                ;

          // guard against PR115897
          static_assert(not _S_is_simd_ctor_arg<_Tp>);

          template <typename _Arg>
            static constexpr bool _S_is_mask_ctor_arg
              = std::is_same_v<_Arg, _MaskMember>;

          template <typename _To>
            requires _S_is_simd_ctor_arg<_To>
            _GLIBCXX_SIMD_INTRINSIC static constexpr _To
            _S_simd_conversion(_SimdMember __x)
            { return __builtin_bit_cast(_To, __x); }

          template <typename _From>
            requires _S_is_simd_ctor_arg<_From>
            _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember
            _S_simd_construction(_From __x)
            { return __builtin_bit_cast(_SimdMember, __x); }

          template <typename _To>
            requires _S_is_mask_ctor_arg<_To>
            _GLIBCXX_SIMD_INTRINSIC static constexpr _To
            _S_mask_conversion(_MaskMember __x)
            { return __x; }

          template <typename _From>
            requires _S_is_mask_ctor_arg<_From>
            _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
            _S_mask_construction(_From __x)
            { return __x; }
        };

      using _Impl = __detail::_ImplBuiltin<_VecAbi>;

      using _SimdImpl = _Impl;

      using _MaskImpl = _Impl;

      static constexpr _SimdSizeType _S_full_size = __detail::__signed_bit_ceil(_Width);

      static constexpr bool _S_is_partial = _S_full_size > _S_size;

      template <__detail::__vectorizable_canon _Up, _SimdSizeType _NewW = _S_size>
        using _Rebind = __detail::__deduce_t<_Up, _NewW, _VecAbi>;

      template <__detail::__vectorizable_canon _Tp>
        using _SimdMember = _Vp<_Tp>;

      template <__detail::__vectorizable_canon _Tp>
        using _MaskMember = _Vp<__detail::__mask_integer_from<sizeof(_Tp)>>;

      template <typename _Tp>
        static constexpr _MaskMember<_Tp>
        _S_implicit_mask = _S_is_partial
                             ? []<int... _Is> (integer_sequence<int, _Is...>) {
                               return _MaskMember<_Tp>{ (_Is < _S_size ? -1 : 0)... };
                             }(make_integer_sequence<int, _S_full_size>())
                             : ~_MaskMember<_Tp>();

      template <same_as<__detail::_MaskUInt128> _Tp>
        static constexpr _MaskMember<_Tp> _S_implicit_mask<_Tp>
          = _S_is_partial ? _MaskMember<_Tp>{__detail::_VecOps<typename _MaskMember<_Tp>::_VecType,
                                                               2 * _S_size>::_S_broadcast(-1)}
                          : ~_MaskMember<_Tp>();

      template <__detail::__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_masked(_TV __x)
        {
          using _Tp = __detail::__value_type_of<_TV>;
          if constexpr (not _S_is_partial)
            return __x;
          else
            return __builtin_bit_cast(_TV, __builtin_bit_cast(_MaskMember<_Tp>, __x)
                                        & _S_implicit_mask<_Tp>);
        }

      template <__detail::__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr auto
        __make_padding_nonzero(_TV __x)
        {
          using _Tp = __detail::__value_type_of<_TV>;
          if constexpr (not _S_is_partial)
            return __x;
          else
            {
              constexpr auto __implicit_mask
                = __builtin_bit_cast(_TV, _S_implicit_mask<_Tp>);
              if constexpr (is_floating_point_v<_Tp>)
                {
                  constexpr auto __one
                    = __detail::__vec_andnot(__implicit_mask,
                                             __detail::__vec_broadcast<_S_full_size>(_Tp(1)));
                  // it's not enough to return `x | 1_in_padding` because the
                  // padding in x might be inf or nan (independent of
                  // __FINITE_MATH_ONLY__, because it's about padding bits)
                  return __detail::__vec_or(__detail::__vec_and(__x, __implicit_mask), __one);
                }
              else
                return __x | ~__implicit_mask;
            }
        }
    };
}

namespace std::__detail
{
  template <typename _Abi, _BuildFlags _Flags>
    struct _ImplBuiltinBase
    {
      using abi_type = _Abi;

      template <typename _Tp>
        using _TypeTag = _Tp*;

      static constexpr _SimdSizeType _S_size = _Abi::_S_size;

      static constexpr _SimdSizeType _S_full_size = _Abi::_S_full_size;

      static constexpr bool _S_is_partial = _Abi::_S_is_partial;

      template <typename _Tp>
        using _SimdMember = typename _Abi::template _SimdMember<_Tp>;

      template <typename _Tp>
        using _MaskMember = typename _Abi::template _MaskMember<__value_type_of<_Tp>>;

      using _SuperImpl = typename _Abi::_Impl;

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr
        std::datapar::basic_simd<__value_type_of<_TV>, _Abi>
        _M_make_simd(_TV __x)
        { return __x; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember<_Tp>
        _S_broadcast(_Tp __x) noexcept
        { return __vec_broadcast<_S_full_size>(__x); }

      template <typename _Tp, typename _Fp>
        inline static constexpr _SimdMember<_Tp>
        _S_generator(_Fp&& __gen)
        {
          return _GLIBCXX_SIMD_VEC_GEN(_SimdMember<_Tp>, _S_size, _Is, {
                   static_cast<_Tp>(__gen(__ic<_Is>))...
                 });
        }

      template <typename _Tp, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember<_Tp>
        _S_load(const _Up* __mem, _TypeTag<_Tp> __tag)
        {
          using _Ucanon = __canonical_vec_type_t<_Up>;
          constexpr bool __need_cvt
            = not is_same_v<_Tp, _Ucanon>
                and not (is_integral_v<_Tp> and is_integral_v<_Up> and sizeof(_Tp) == sizeof(_Up));

          if (__builtin_is_constant_evaluated())
            {
              return _GLIBCXX_SIMD_VEC_GEN(_SimdMember<_Tp>, _S_size, __i,
                                           {static_cast<_Tp>(__mem[__i])...});
            }
          else if constexpr (__need_cvt)
            {
              using _UImpl = typename __deduced_traits<_Ucanon, _S_size>::_SimdImpl;
              constexpr _TypeTag<_Ucanon> __utag = nullptr;
              constexpr _SimdConverter<_Ucanon, typename _UImpl::abi_type, _Tp, abi_type> __cvt;
              return __cvt(_UImpl::_S_load(__mem, __utag));
            }
          else if constexpr (not __signed_has_single_bit(_S_size))
            {
              static_assert(_S_size < _S_full_size);
              using _FullImpl = typename __deduced_traits<_Tp, _S_full_size>::_SimdImpl;
              return _FullImpl::_S_partial_load(__mem, _S_size, __tag);
            }
          else
            {
              _SimdMember<_Tp> __r = {};
              __builtin_memcpy(&__r, __mem, sizeof(_Tp) * _S_size);
              return __r;
            }
        }

      template <typename _Tp, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static _SimdMember<_Tp>
        _S_partial_load(const _Up* __mem, size_t __mem_size, _TypeTag<_Tp> __tag)
        {
          using _Ucanon = __canonical_vec_type_t<_Up>;
          constexpr bool __need_cvt
            = not is_same_v<_Tp, _Ucanon>
                and not (is_integral_v<_Tp> and is_integral_v<_Up> and sizeof(_Tp) == sizeof(_Up));

          if (__mem_size >= _S_size) [[unlikely]]
            return _S_load(__mem, __tag);
          else if constexpr (not __need_cvt)
            return [&] {
              _SimdMember<_Tp> __r = {};
              __builtin_memcpy(&__r, __mem, sizeof(_Up) * __mem_size);
              return __r;
            }();
          else
            return [&] {
              __vec_builtin_type<_Ucanon, _S_full_size> __tmp = {};
              __builtin_memcpy(&__tmp, __mem, sizeof(_Up) * __mem_size);
              return __builtin_convertvector(__tmp, _SimdMember<_Tp>);
            }();
        }

      template <typename _Tp, typename _Up>
        static constexpr inline _SimdMember<_Tp>
        _S_masked_load(_MaskMember<_SimdMember<_Tp>> __k, const _Up* __mem, _TypeTag<_Tp> __tag)
        {
          if consteval
            {
              return _GLIBCXX_SIMD_VEC_GEN(_SimdMember<_Tp>, _S_size, __i,
                                           {(__k[__i] ? static_cast<_Tp>(__mem[__i]) : _Tp())...});
            }
          else
            {
              return __vec_and(reinterpret_cast<_SimdMember<_Tp>>(__k), _S_load(__mem, __tag));
            }
        }

      template <__vec_builtin _TV, typename _Up>
        static constexpr inline _TV
        _S_masked_load(_TV __merge, _MaskMember<_TV> __k, const _Up* __mem)
        {
          _S_bit_iteration(
            _SuperImpl::_S_to_bits(__k),
            [&] [[__gnu__::__always_inline__]] (auto __i) {
              __merge._M_set(__i, static_cast<__value_type_of<_TV>>(__mem[__i]));
            });
          return __merge;
        }

      template <__vec_builtin _TV, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_store(_TV __v, _Up* __mem, _TypeTag<__value_type_of<_TV>>) noexcept
        {
          using _Tp = __value_type_of<_TV>;
          using _Ucanon = __canonical_vec_type_t<_Up>;
          constexpr bool __need_cvt
            = not is_same_v<_Tp, _Ucanon>
                and not (is_integral_v<_Tp> and is_integral_v<_Up> and sizeof(_Tp) == sizeof(_Up));
          if constexpr (not __need_cvt)
            __builtin_memcpy(__mem, &__v, sizeof(_Tp) * _S_size);
          else
            {
              const auto __tmp
                = __builtin_convertvector(__v, __vec_builtin_type<_Ucanon, _S_full_size>);
              __builtin_memcpy(__mem, &__tmp, sizeof(_Up) * _S_size);
            }
        }

      template <__vec_builtin _TV, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_partial_store(_TV __v, _Up* __mem, size_t __mem_size,
                         _TypeTag<__value_type_of<_TV>> __tag)
        {
          using _Tp = __value_type_of<_TV>;
          using _Ucanon = __canonical_vec_type_t<_Up>;
          constexpr bool __need_cvt
            = not is_same_v<_Tp, _Ucanon>
                and not (is_integral_v<_Tp> and is_integral_v<_Up> and sizeof(_Tp) == sizeof(_Up));
          if (__mem_size >= _S_size) [[unlikely]]
            _S_store(__v, __mem, __tag);
          else if constexpr (not __need_cvt)
            __builtin_memcpy(__mem, &__v, sizeof(_Tp) * __mem_size);
          else
            {
              const auto __tmp
                = __builtin_convertvector(__v, __vec_builtin_type<_Ucanon, _S_full_size>);
              __builtin_memcpy(__mem, &__tmp, sizeof(_Up) * __mem_size);
            }
        }

      template <__vec_builtin _TV, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_store(const _TV __v, _Up* __mem, const _MaskMember<_TV> __k)
        {
          using _Tp = __value_type_of<_TV>;
          using _Ucanon = __canonical_vec_type_t<_Up>;
          constexpr bool __need_cvt
            = not is_same_v<_Tp, _Ucanon>
                and not (is_integral_v<_Tp> and is_integral_v<_Up> and sizeof(_Tp) == sizeof(_Up));
          if constexpr (not __need_cvt)
            {
              _S_bit_iteration(
                _SuperImpl::_S_to_bits(__k),
                [&] [[__gnu__::__always_inline__]] (auto __i) {
                  __mem[__i] = __v[__i];
                });
            }
          else
            {
              const auto __uv = __vec_convert<_Ucanon>(__v);
              _S_bit_iteration(
                _SuperImpl::_S_to_bits(__k),
                [&] [[__gnu__::__always_inline__]] (auto __i) {
                  __mem[__i] = __uv[__i];
                });
            }
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_complement(_TV __x)
        { return __vec_not(__x); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_unary_minus(_TV __x)
        {
          // GCC doesn't use the psign instructions, but pxor & psub seem to be
          // just as good a choice as pcmpeqd & psign. So meh.
          return -__x;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_plus(_TV __x, _TV __y)
        { return __x + __y; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_add_sat(_TV __x, _TV __y) noexcept
        {
          // TODO
          return __x + __y;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_minus(_TV __x, _TV __y)
        { return __x - __y; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_multiplies(_TV __x, _TV __y)
        { return __x * __y; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_divides(_TV __x, _TV __y)
        { return __x / _Abi::__make_padding_nonzero(__y); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_modulus(_TV __x, _TV __y)
        { return __x % _Abi::__make_padding_nonzero(__y); }

      template <typename _TV>
        requires __vec_builtin<_TV> or same_as<_TV, _MaskVec128<_S_size>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_bit_and(_TV __x, _TV __y)
        { return __vec_and(__x, __y); }

      template <typename _TV>
        requires __vec_builtin<_TV> or same_as<_TV, _MaskVec128<_S_size>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_bit_or(_TV __x, _TV __y)
        { return __vec_or(__x, __y); }

      template <typename _TV>
        requires __vec_builtin<_TV> or same_as<_TV, _MaskVec128<_S_size>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_bit_xor(_TV __x, _TV __y)
        { return __vec_xor(__x, __y); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_bit_shift_left(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          if constexpr (sizeof(_Tp) < sizeof(int))
            return __y >= _Tp(sizeof(_Tp) * __CHAR_BIT__) ? _Tp() : __x << __y;
          else
            return __x << __y;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_bit_shift_left(_TV __x, int __y)
        {
          using _Tp = __value_type_of<_TV>;
          if (sizeof(_Tp) < sizeof(int) and __y >= int(sizeof(_Tp) * __CHAR_BIT__))
            return _TV();
          else
            return __x << __y;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_bit_shift_right(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          if constexpr (sizeof(_Tp) < sizeof(int) and is_unsigned_v<_Tp>)
            return __y >= _Tp(sizeof(_Tp) * __CHAR_BIT__) ? _Tp() : __x >> __y;
          else if constexpr (sizeof(_Tp) < sizeof(int))
            return __y >= _Tp(sizeof(_Tp) * __CHAR_BIT__) ? reinterpret_cast<_TV>(__x < 0)
                                                          : __x >> __y;
          else
            return __x >> __y;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_bit_shift_right(_TV __x, int __y)
        {
          using _Tp = __value_type_of<_TV>;
          if (sizeof(_Tp) < sizeof(int) and __y >= int(sizeof(_Tp) * __CHAR_BIT__))
            return is_unsigned_v<_Tp> ? _TV() : reinterpret_cast<_TV>(__x < 0);
          else
            return __x >> __y;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_equal_to(_TV __x, _TV __y)
        { return __x == __y; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_not_equal_to(_TV __x, _TV __y)
        { return __x != __y; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_less(_TV __x, _TV __y)
        { return __x < __y; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_less_equal(_TV __x, _TV __y)
        { return __x <= __y; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_negate(_TV __x)
        { return !__x; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_complex_conj(_TV __x)
        { return _VecOps<_TV>::_S_complex_negate_imag(__x); }

      template <__vec_builtin _TV>
        [[gnu::cold]] static constexpr _TV
        _S_complex_redo_mul_on_nan(_TV __r, _TV __x, _TV __y, __vec_builtin auto __k)
        {
          // TODO: just implement the suggestion from C23 Annex G
          using _T0 = __value_type_of<_TV>;
          using _Cp = __detail::__complex_of_t<_T0>;
          static constexpr int _Csize = sizeof(_TV) / sizeof(_Cp);
          using _Cparray = array<_Cp, _Csize>;
          auto __tmp = _GLIBCXX_SIMD_INT_PACK(_Csize, _Is, {
                         return _Cparray {(__k[_Is * 2] != 0 and __k[_Is * 2 + 1] != 0
                                             ? (__builtin_bit_cast(_Cparray, __x)[_Is]
                                                  * __builtin_bit_cast(_Cparray, __y)[_Is])
                                             : __builtin_bit_cast(_Cparray, __r)[_Is])...};
                       });
          return __builtin_bit_cast(_TV, __tmp);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_complex_multiplies(_TV __x, _TV __y)
        {
          static_assert((_S_size & 1) == 0);
          using _VO = _VecOps<_TV>;
          if (_VecOps<_TV, _S_size>::_S_complex_imag_is_constprop_zero(__x))
            {
              if (_VecOps<_TV, _S_size>::_S_complex_imag_is_constprop_zero(__y))
                return __x * __y;
              else
                {
                  if (_Flags._M_conforming_to_STDC_annex_G())
                    {
                      auto __a = _VO::_S_dup_even(__x) * __y;
                      auto __b = _TV() * _VO::_S_swap_neighbors(__y);
#if SIMD_DIAGNOSE_INDETERMINATE_SIGNED_ZERO
                      //if (_SuperImpl::_S_any_of(_SuperImpl::_S_equal_to(__a, 0))) // __b is ±0 by construction
#endif
                      return _VO::_S_addsub(__a, __b);
                    }
                  else
                    return _VO::_S_dup_even(__x) * __y;
                }
            }
          else if (_VecOps<_TV, _S_size>::_S_complex_imag_is_constprop_zero(__y))
            {
              if (_Flags._M_conforming_to_STDC_annex_G())
                return _VO::_S_addsub(_VO::_S_dup_even(__y) * __x,
                                      _TV() * _VO::_S_swap_neighbors(__x));
              else
                return _VO::_S_dup_even(__y) * __x;
            }
          else if (_VecOps<_TV, _S_size>::_S_complex_real_is_constprop_zero(__y))
            {
              if (_Flags._M_conforming_to_STDC_annex_G())
                return _VO::_S_addsub(_TV(), _VO::_S_dup_odd(__y) * _VO::_S_swap_neighbors(__x));
              else
                return _VO::_S_dup_odd(__y)
                         * _VO::_S_complex_negate_real(_VO::_S_swap_neighbors(__x));
            }
          else if (_VecOps<_TV, _S_size>::_S_complex_real_is_constprop_zero(__x))
            {
              if (_Flags._M_conforming_to_STDC_annex_G())
                return _VO::_S_addsub(_TV(), _VO::_S_dup_odd(__x) * _VO::_S_swap_neighbors(__y));
              else
                return _VO::_S_dup_odd(__x)
                         * _VO::_S_complex_negate_real(_VO::_S_swap_neighbors(__y));
            }
          else
            {
              const _TV __r = _SuperImpl::_S_complex_multiplies_impl(__x, __y);
              const auto __k = _SuperImpl::_S_isnan(__r);
              if (not _Flags._M_conforming_to_STDC_annex_G()
                    or _SuperImpl::_S_none_of(__k)) [[likely]]
                return __r;
              else
                return _SuperImpl::_S_complex_redo_mul_on_nan(__r, __x, __y, __k);
            }
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_complex_multiplies_impl(_TV __x, _TV __y)
        {
          using _VO = _VecOps<_TV>;
          return _VO::_S_addsub(_VO::_S_dup_even(__x) * __y,
                                _VO::_S_dup_odd(__x) * _VO::_S_swap_neighbors(__y));
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_complex_set_real(_TV& __c, __value_type_of<_TV> __re) noexcept
        { __c[0] = __re; }

      template <__vec_builtin _TV>
        requires (__width_of<_TV> >= 4)
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_complex_set_real(_TV& __c, __half_vec_builtin_t<_TV> __re) noexcept
        { _VecOps<_TV>::_S_overwrite_even_elements(__c, __re); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_complex_set_real(_TV& __cl, _TV& __ch, _TV __re) noexcept
        { _VecOps<_TV>::_S_overwrite_even_elements(__cl, __ch, __re); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_complex_set_imag(_TV& __c, __value_type_of<_TV> __im) noexcept
        { __c[1] = __im; }

      template <__vec_builtin _TV>
        requires (__width_of<_TV> >= 4)
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_complex_set_imag(_TV& __c, __half_vec_builtin_t<_TV> __im) noexcept
        { _VecOps<_TV>::_S_overwrite_odd_elements(__c, __im); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_complex_set_imag(_TV& __cl, _TV& __ch, _TV __im) noexcept
        { _VecOps<_TV>::_S_overwrite_odd_elements(__cl, __ch, __im); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_min(_TV __x, _TV __y)
        { return __x < __y ? __x : __y; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_max(_TV __x, _TV __y)
        { return __x < __y ? __y : __x; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_minmax(_TV& __min, _TV& __max)
        {
          auto __need_swap = __max < __min;
          auto __lo = __need_swap ? __max : __min;
          __max = __need_swap ? __min : __max;
          __min = __lo;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_clamp(_TV __v, _TV __lo, _TV __hi)
        { return __v < __lo ? __lo : __v < __hi ? __v : __hi; }

      template <__vec_builtin _TV>
        static constexpr _MaskMember<_TV>
        _S_isgreater(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          using _Ip = __make_signed_int_t<_Tp>;
          using _IV = __vec_builtin_type<_Ip, _S_full_size>;
          const auto __xn = __builtin_bit_cast(_IV, __x);
          const auto __yn = __builtin_bit_cast(_IV, __y);
          const auto __xp = __xn < 0 ? -(__xn & __finite_max_v<_Ip>) : __xn;
          const auto __yp = __yn < 0 ? -(__yn & __finite_max_v<_Ip>) : __yn;
          return __vec_andnot(_SuperImpl::_S_isunordered(__x, __y), __xp > __yp);
        }

      template <__vec_builtin _TV>
        static constexpr _MaskMember<_TV>
        _S_isgreaterequal(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          using _Ip = __make_signed_int_t<_Tp>;
          using _IV = __vec_builtin_type<_Ip, _S_full_size>;
          const auto __xn = __builtin_bit_cast(_IV, __x);
          const auto __yn = __builtin_bit_cast(_IV, __y);
          const auto __xp = __xn < 0 ? -(__xn & __finite_max_v<_Ip>) : __xn;
          const auto __yp = __yn < 0 ? -(__yn & __finite_max_v<_Ip>) : __yn;
          return __vec_andnot(_SuperImpl::_S_isunordered(__x, __y), __xp >= __yp);
        }

      template <__vec_builtin _TV>
        static constexpr _MaskMember<_TV>
        _S_isless(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          using _Ip = __make_signed_int_t<_Tp>;
          using _IV = __vec_builtin_type<_Ip, _S_full_size>;
          const auto __xn = __builtin_bit_cast(_IV, __x);
          const auto __yn = __builtin_bit_cast(_IV, __y);
          const auto __xp = __xn < 0 ? -(__xn & __finite_max_v<_Ip>) : __xn;
          const auto __yp = __yn < 0 ? -(__yn & __finite_max_v<_Ip>) : __yn;
          return __vec_andnot(_SuperImpl::_S_isunordered(__x, __y), __xp < __yp);
        }

      template <__vec_builtin _TV>
        static constexpr _MaskMember<_TV>
        _S_islessequal(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          using _Ip = __make_signed_int_t<_Tp>;
          using _IV = __vec_builtin_type<_Ip, _S_full_size>;
          const auto __xn = __builtin_bit_cast(_IV, __x);
          const auto __yn = __builtin_bit_cast(_IV, __y);
          const auto __xp = __xn < 0 ? -(__xn & __finite_max_v<_Ip>) : __xn;
          const auto __yp = __yn < 0 ? -(__yn & __finite_max_v<_Ip>) : __yn;
          return __vec_andnot(_SuperImpl::_S_isunordered(__x, __y), __xp <= __yp);
        }

      template <__vec_builtin _TV>
        static constexpr _MaskMember<_TV>
        _S_islessgreater(_TV __x, _TV __y)
        {
          return __vec_andnot(_SuperImpl::_S_isunordered(__x, __y),
                              _SuperImpl::_S_not_equal_to(__x, __y));
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_abs(_TV __x)
        {
          if constexpr (is_floating_point_v<__value_type_of<_TV>>)
            return _S_fabs(__x);
          else
            return __x < 0 ? -__x : __x;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_fabs(_TV __x)
        {
          static_assert(is_floating_point_v<__value_type_of<_TV>>);
          return __vec_and(_S_absmask<_TV>, __x);
        }

      // Returns __x + __y - __y without -fassociative-math optimizing to __x.
      // - _TV must be __vec_builtin_type<floating-point type, N>.
      // - _UV must be _TV or floating-point type.
      template <__vec_builtin _TV, typename _UV>
        requires std::same_as<_UV, _TV> or std::same_as<_UV, __value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_plus_minus(_TV __x, _UV __y)
        { return __builtin_assoc_barrier(__x + __y) - __y; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_nearbyint(const _TV __x)
        {
          using _Tp = __value_type_of<_TV>;
          const _TV __absx = __vec_and(__x, _S_absmask<_TV>);
          static_assert(__CHAR_BIT__ * sizeof(1ull) >= __digits_v<_Tp>);
          constexpr _TV __shifter_abs = _TV() + (1ull << (__digits_v<_Tp> - 1));
          const _TV __shifter = __vec_or(__absx, __shifter_abs);
          const _TV __shifted = _S_plus_minus(__x, __shifter);
          return __absx < __shifter_abs ? __shifted : __x;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_rint(_TV __x)
        { return _SuperImpl::_S_nearbyint(__x); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_trunc(_TV __x)
        {
          using _Tp = __value_type_of<_TV>;
          const _TV __absx = __vec_and(__x, _S_absmask<_TV>);
          static_assert(__CHAR_BIT__ * sizeof(1ull) >= __digits_v<_Tp>);
          constexpr _Tp __shifter = 1ull << (__digits_v<_Tp> - 1);
          _TV __truncated = _S_plus_minus(__absx, __shifter);
          __truncated -= __truncated > __absx ? _TV() + 1 : _TV();
          return __absx < __shifter ? __vec_or(__vec_xor(__absx, __x), __truncated) : __x;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_round(_TV __x)
        {
          using _Tp = __value_type_of<_TV>;
          const auto __abs_x = _SuperImpl::_S_abs(__x);
          const auto __t_abs = _SuperImpl::_S_trunc(__abs_x);
          // round(abs(x))
          const auto __r_abs = __t_abs + (__abs_x - __t_abs >= _Tp(.5) ? _Tp(1) : 0);
          return __vec_or(__vec_xor(__abs_x, __x), __r_abs);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_floor(_TV __x)
        {
          using _Tp = __value_type_of<_TV>;
          const _TV __y = _SuperImpl::_S_trunc(__x);
          const _TV __negative_input = __builtin_bit_cast(_TV, __x < _Tp(0));
          const _TV __mask = __vec_andnot(__builtin_bit_cast(_TV, __y == __x), __negative_input);
          return __vec_or(__vec_andnot(__mask, __y), __vec_and(__mask, __y - _Tp(1)));
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_ceil(_TV __x)
        {
          using _Tp = __value_type_of<_TV>;
          const auto __y = _SuperImpl::_S_trunc(__x);
          const auto __negative_input = __builtin_bit_cast(_TV, __x < _Tp(0));
          const auto __inv_mask = __vec_or(__builtin_bit_cast(_TV, __y == __x), __negative_input);
          return __vec_or(__vec_and(__inv_mask, __y), __vec_andnot(__inv_mask, __y + _Tp(1)));
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isnan([[maybe_unused]] _TV __x)
        {
#if __FINITE_MATH_ONLY__
          return {}; // false
#elif not defined __SUPPORT_SNAN__ or not defined __STDC_IEC_559__
          return ~(__x == __x);
#else
          using _Tp = __value_type_of<_TV>;
          using _IV = __vec_builtin_type<__make_signed_int_t<_Tp>, _S_full_size>;
          const auto __absn = __builtin_bit_cast(_IV, _SuperImpl::_S_abs(__x));
          const auto __infn = __builtin_bit_cast(
                                _IV, __vec_broadcast<_S_size>(__infinity_v<_Tp>));
          return __infn < __absn;
#endif
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isfinite([[maybe_unused]] _TV __x)
        {
#if __FINITE_MATH_ONLY__
          return _S_allbits<_MaskMember<_TV>>;
#else
          // if all exponent bits are set, __x is either inf or NaN
          using _Tp = __value_type_of<_TV>;
          using _IV = __vec_builtin_type<__make_signed_int_t<_Tp>, _S_full_size>;
          const auto __absn = __builtin_bit_cast(_IV, _SuperImpl::_S_abs(__x));
          constexpr _IV __maxn
            = __builtin_bit_cast(_IV, __vec_broadcast<_S_size>(__finite_max_v<_Tp>));
          return _SuperImpl::_S_less_equal(__absn, __maxn);
#endif
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isunordered(_TV __x, _TV __y)
        { return __vec_or(_S_isnan(__x), _S_isnan(__y)); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_signbit(_TV __x)
        {
          using _Tp = __value_type_of<_TV>;
          using _IV = __vec_builtin_type<__make_signed_int_t<_Tp>, _S_full_size>;
          return __builtin_bit_cast(_IV, __x) < 0;
          // Arithmetic right shift (SRA) would also work (instead of compare), but
          // 64-bit SRA isn't available on x86 before AVX512. And in general,
          // compares are more likely to be efficient than SRA.
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isinf([[maybe_unused]] _TV __x)
        {
#if __FINITE_MATH_ONLY__
          return {}; // false
#else
          using _Tp = __value_type_of<_TV>;
          return _SuperImpl::_S_equal_to(_SuperImpl::_S_abs(__x),
                                         __vec_broadcast<_S_size>(__infinity_v<_Tp>));
          // alternative:
          // compare to inf using the corresponding integer type
          /*
             return
             __vector_bitcast<_Tp>(__vector_bitcast<__make_signed_int_t<_Tp>>(
          _S_abs(__x))
          ==
          __vector_bitcast<__make_signed_int_t<_Tp>>(__vec_broadcast<_Np>(
          __infinity_v<_Tp>)));
           */
#endif
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isnormal(_TV __x)
        {
          using _Tp = __value_type_of<_TV>;
          using _IV = __vec_builtin_type<__make_signed_int_t<_Tp>, _S_full_size>;
          const auto __absn = __builtin_bit_cast(_IV, _SuperImpl::_S_abs(__x));
          const auto __minn = __builtin_bit_cast(
                                _IV, __vec_broadcast<_S_size>(__norm_min_v<_Tp>));
#if __FINITE_MATH_ONLY__
          return __absn >= __minn;
#else
          const auto __maxn
            = __builtin_bit_cast(_IV, __vec_broadcast<_S_size>(__finite_max_v<_Tp>));
          return __minn <= __absn and __absn <= __maxn;
#endif
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr
        typename _Abi::template _Rebind<__make_dependent_t<int, _TV>>::template _SimdMember<int>
        _S_fpclassify(_TV __x)
        {
          using _RAbi = typename _Abi::template _Rebind<int>;
          using _RV = typename _RAbi::template _SimdMember<int>;
          using _Tp = __value_type_of<_TV>;
          using _Ip = __make_signed_int_t<_Tp>;
          using _IV = __vec_builtin_type<_Ip, _S_full_size>;
          const auto __xn = __builtin_bit_cast(_IV, _SuperImpl::_S_abs(__x));
          constexpr size_t _NI = sizeof(__xn) / sizeof(_Ip);
          constexpr auto __minn = __builtin_bit_cast(_IV, __vec_broadcast<_NI>(__norm_min_v<_Tp>));
          constexpr auto __fp_normal = __vec_broadcast<_NI, _Ip>(FP_NORMAL);
#if !__FINITE_MATH_ONLY__
          constexpr auto __infn = __builtin_bit_cast(_IV, __vec_broadcast<_NI>(__infinity_v<_Tp>));
          constexpr auto __fp_nan = __vec_broadcast<_NI, _Ip>(FP_NAN);
          constexpr auto __fp_infinite = __vec_broadcast<_NI, _Ip>(FP_INFINITE);
#endif
#ifndef __FAST_MATH__
          constexpr auto __fp_subnormal = __vec_broadcast<_NI, _Ip>(FP_SUBNORMAL);
#endif
          constexpr auto __fp_zero = __vec_broadcast<_NI, _Ip>(FP_ZERO);
          _IV __tmp = (__xn < __minn)
#ifdef __FAST_MATH__
                        ? __fp_zero
#else
                        ? (__xn == 0 ? __fp_zero : __fp_subnormal)
#endif
#if __FINITE_MATH_ONLY__
                        : __fp_normal;
#else
                        : (__xn < __infn ? __fp_normal
                                         : (__xn == __infn ? __fp_infinite : __fp_nan));
#endif
          if constexpr (is_same_v<_IV, _RV>)
            return __tmp;
          else if constexpr (__vec_builtin<_RV>)
            return __builtin_convertvector(__tmp, _RV);
          else
            return _RAbi::template _S_generator<int>([&] [[__gnu__::__always_inline__]] (auto __i) {
                     return static_cast<int>(__tmp[__i]);
                   });
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_increment(_TV& __x)
        { __x = __x + 1; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_decrement(_TV& __x)
        { __x = __x - 1; }

      // If called from mask (or mask::reference), then this returns a signed integer
      // instead of bool. However, the implicit conversion to bool always yields the correct value.
      // (reinterpretation of the bits, would not be correct, though.)
      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr __value_type_of<_TV>
        _S_get(_TV __v, _SimdSizeType __i)
        { return __vec_get(__v, __i); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_get(_MaskVec128<_S_size> __v, _SimdSizeType __i)
      { return __vec_get(__v._M_data, __i * 2); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_set(_TV& __v, _SimdSizeType __i, __value_type_of<_TV> __x)
        { __vec_set(__v, __i, __x); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_set(_TV& __k, _SimdSizeType __i, bool __x)
        { __vec_set(__k, __i, -__x); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_is_constprop_none_of(_TV __k)
        { return _VecOps<_TV>::_S_is_constprop_equal_to(__k, 0); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_is_constprop_all_of(_TV __k)
        { return _VecOps<_TV>::_S_is_constprop_equal_to(__k, -1); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(_MaskMember<_TV> __k, _TV& __lhs, __type_identity_t<_TV> __rhs)
        {
          if (_S_is_constprop_none_of(__k))
            return;
          else if (_S_is_constprop_all_of(__k))
            __lhs = __rhs;
          else if (_S_is_constprop_single_value(__rhs))
            _SuperImpl::_S_masked_assign(__k, __lhs, __rhs[0]);
          else if (_S_is_constprop_all_equal(__lhs, 0))
            __lhs = __vec_and(__builtin_bit_cast(_TV, __k), __rhs);
          else
            __lhs = __k ? __rhs : __lhs;
        }

      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_masked_assign(_MaskVec128<_S_size> __k, _MaskVec128<_S_size>& __lhs,
                       _MaskVec128<_S_size> __rhs)
      {
        using _VO = _VecOps<typename _MaskVec128<_S_size>::_VecType>;
        if (_VO::_S_is_constprop_equal_to(__k._M_data, 0))
          return;
        else if (_VO::_S_is_constprop_equal_to(__k._M_data, -1))
          __lhs._M_data = __rhs._M_data;
        else if (_VO::_S_is_constprop_equal_to(__lhs._M_data, 0))
          __lhs._M_data = __vec_and(__k._M_data, __rhs._M_data);
        else
          __lhs._M_data = __k._M_data ? __rhs._M_data : __lhs._M_data;
      }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(_MaskMember<_TV> __k, _TV& __lhs,
                         __type_identity_t<__value_type_of<_TV>> __rhs)
        {
          if (_S_is_constprop_none_of(__k) or _S_is_constprop_all_equal(__lhs, __rhs))
            return;
          else if (_S_is_constprop_all_of(__k))
            __lhs = __vec_broadcast<_S_size>(__rhs);
          else if (__builtin_constant_p(__rhs) and __rhs == 0)
            __lhs = __vec_andnot(__builtin_bit_cast(_TV, __k), __lhs);
          else if (_S_is_constprop_all_equal(__lhs, 0))
            {
              if constexpr (is_integral_v<__value_type_of<_TV>>)
                {
                  if (sizeof(__lhs) >= 8 and __builtin_constant_p(__rhs)
                        and __is_power2_minus_1(__rhs))
                    {
                      using _Up = __make_unsigned_int_t<__value_type_of<_TV>>;
                      auto __ku = __vec_bitcast<_Up>(__k);
                      const int __zeros = std::__countl_zero(__builtin_bit_cast(_Up, __rhs));
                      __lhs = reinterpret_cast<_TV>(_SuperImpl::_S_bit_shift_right(__ku, __zeros));
                      return;
                    }
                  else if constexpr (sizeof(_TV) < 8)
                    {
                      using _Up = __make_unsigned_int_t<_TV>;
                      _Up __tmp = __builtin_bit_cast(_Up, __k)
                                    & _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                        return ((_Up(__rhs) << (_Is * __CHAR_BIT__)) | ...);
                                      });
                      __lhs = __builtin_bit_cast(_TV, __tmp);
                      return;
                    }
                }
              __lhs = __vec_and(__builtin_bit_cast(_TV, __k), __vec_broadcast<_S_size>(__rhs));
            }
          else
            __lhs = __k ? __vec_broadcast<_S_size>(__rhs) : __lhs;
        }

      template <typename _Op, __vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(_MaskMember<_TV> __k, _TV& __lhs, __type_identity_t<_TV> __rhs, _Op __op)
        {
          if (_S_is_constprop_none_of(__k))
            return;
          else if (_S_is_constprop_all_of(__k))
            __lhs = __op(_SuperImpl{}, __lhs, __rhs);
          else
            __lhs = __k ? __op(_SuperImpl{}, __lhs, __rhs) : __lhs;
        }

      // mask blending
      template <__vec_builtin _TV>
        requires std::signed_integral<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(_TV __k, _TV& __lhs, bool __rhs)
        {
          if (__builtin_constant_p(__rhs))
            {
              if (__rhs == false)
                __lhs = __vec_andnot(__k, __lhs);
              else
                __lhs = __vec_or(__k, __lhs);
            }
          else
            __lhs = __k ? (__rhs ? -1 : 0) : __lhs;
        }

      template <__vec_builtin _TV, size_t _Np, bool _Sanitized>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_convert_mask(_BitMask<_Np, _Sanitized> __x)
        {
          using _Tp = __value_type_of<_TV>;
          static_assert(is_same_v<_Tp, __mask_integer_from<sizeof(_Tp)>>);
          return _GLIBCXX_SIMD_VEC_GEN(_TV, _S_size, _Is,
                                       {((_Is < _Np and __x[_Is]) ? _Tp(-1) : _Tp())...});
        }

      template <same_as<_MaskVec128<_S_size>> _RV, bool _Sanitized>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _RV
        _S_convert_mask(_BitMask<_S_size, _Sanitized> __x)
        {
          if constexpr (_S_size == 1)
            return __x[0] ? __vec_not(_RV()) : _RV();
          else if constexpr (_S_size == 2)
            return _RV{{__x[0] ? -1 : 0, __x[0] ? -1 : 0,
                        __x[1] ? -1 : 0, __x[1] ? -1 : 0}};
        }

      template <__vec_builtin _RV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _RV
        _S_convert_mask(same_as<bool> auto __x)
        {
          static_assert(__width_of<_RV> == 1);
          return __x ? ~_RV() : _RV();
        }

      template <same_as<_MaskVec128<1>> _RV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _RV
        _S_convert_mask(same_as<bool> auto __x)
        { return __x ? __vec_not(_RV()) : _RV(); }

      template <__vec_builtin _RV, __vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _RV
        _S_convert_mask(_TV __x)
        {
          using _Rp = __value_type_of<_RV>;
          using _Tp = __value_type_of<_TV>;
          static_assert(is_same_v<_RV, _MaskMember<_Rp>>);
          static_assert(__width_of<_RV> == __width_of<_TV>);
          if constexpr (sizeof(_Rp) == sizeof(_Tp) && sizeof(_TV) == sizeof(_RV))
            return __builtin_bit_cast(_RV, __x);
          else
            // TODO: __builtin_convertvector doesn't know that only -1 and 0 are valid inputs
            return __builtin_convertvector(__x, _RV);
        }

      template <__vec_builtin _RV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _RV
        _S_convert_mask(_MaskVec128<_S_size> __x)
        { return _SuperImpl::template _S_convert_mask<_RV>(__x._M_compress_even()); }

      template <__vec_builtin _RV, int _N0, size_t _N1>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _RV
        _S_convert_mask(array<_MaskVec128<_N0>, _N1> __x)
        {
          return _SuperImpl::template _S_convert_mask<_RV>(
                   _GLIBCXX_SIMD_INT_PACK(_N1, _Is, {
                     return array {__x[_Is]._M_compress_even()...};
                   }));
        }

      template <same_as<_MaskVec128<_S_size>> _RV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _RV
        _S_convert_mask(_MaskVec128<_S_size> __x)
        { return __x; }

      template <same_as<_MaskVec128<_S_size>> _RV, __vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _RV
        _S_convert_mask(_TV __x)
        {
          using _Vp = typename _RV::_VecType;
          using _Tp = __value_type_of<_TV>;
          if constexpr (sizeof(_Tp) == 8)
            return _RV{_VecOps<_Vp>::_S_dup_each(__x)};
          else
            return _RV{_VecOps<_Vp>::_S_dup_each(
                         __builtin_convertvector(__x, __vec_builtin_type<__mask_integer_from<8>,
                                                                         _S_full_size>))};
        }

      template <__vec_builtin _RV, __vec_builtin _TV, size_t _TN>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _RV
        _S_convert_mask(array<_TV, _TN> __x)
        {
          using _Rp = __value_type_of<_RV>;
          using _Tp = __value_type_of<_TV>;
          static_assert(is_same_v<_RV, _MaskMember<_Rp>>);
          static_assert(__width_of<_RV> >= _TN * __width_of<_TV>
                          and __width_of<_RV> < 2 * _TN * __width_of<_TV>);
          const auto __k = _GLIBCXX_SIMD_INT_PACK(_TN, _Is, {
                             return __vec_concat(__x[_Is]...);
                           });
          if constexpr (sizeof(_Rp) == sizeof(_Tp))
            return __builtin_bit_cast(_RV, __k);
          else
            // TODO: __builtin_convertvector doesn't know that only -1 and 0 are valid inputs
            return __builtin_convertvector(__k, _RV);
        }

      template <typename _Tp, size_t _Bs, typename _UAbi>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_convert(const std::datapar::basic_simd_mask<_Bs, _UAbi>& __x)
        {
          if constexpr (is_same_v<decltype(__x._M_data), _MaskMember<_Tp>>)
            return __x._M_data;
          else
            return _SuperImpl::template _S_convert_mask<_MaskMember<_Tp>>(__x._M_data);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SanitizedBitMask<_S_size>
        _S_to_bits(_TV __x)
        {
          static_assert(_S_size <= __CHAR_BIT__ * sizeof(unsigned long long));
          return _SanitizedBitMask<_S_size>::__create_unchecked(
                   _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                     return ((static_cast<unsigned long long>(-__vec_get(__x, _Is)) << _Is) | ...);
                   }));
        }

      template <same_as<_MaskVec128<_S_size>> _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SanitizedBitMask<_S_size>
        _S_to_bits(_TV __x)
        {
          if constexpr (_S_size == 1)
            return _SanitizedBitMask<1>::__create_unchecked(__x._M_data[0] != 0);
          else
            return _S_to_bits(__x._M_compress_even());
        }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr typename _Abi::template _MaskMember<_Tp>
        _S_mask_broadcast(bool __x)
        {
          using _MV = typename _Abi::template _MaskMember<_Tp>;
          return __x ? _Abi::template _S_implicit_mask<_Tp> : _MV();
        }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_is_constprop_single_value(__vec_builtin auto __v)
      {
        const bool __single_value = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                      return ((__vec_get(__v, _Is) == __vec_get(__v, 0)) and ...);
                                    });
        return __builtin_constant_p(__single_value) and __single_value;
      }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_is_constprop_all_equal(_TV __v, __value_type_of<_TV> __value)
        {
          const bool __all_equal = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                     return ((__vec_get(__v, _Is) == __value) and ...);
                                   });
          return __builtin_constant_p(__all_equal) and __all_equal;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_is_constprop_all_equal(_TV __v, _TV __w)
        {
          const bool __all_equal = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                     return ((__vec_get(__v, _Is) == __vec_get(__w, _Is)) and ...);
                                   });
          return __builtin_constant_p(__all_equal) and __all_equal;
        }

      template <typename _TV>
        requires __vec_builtin<_TV> or same_as<_TV, _MaskVec128<_S_size>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_logical_and(_TV __x, _TV __y)
        { return __vec_and(__x, __y); }

      template <typename _TV>
        requires __vec_builtin<_TV> or same_as<_TV, _MaskVec128<_S_size>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_logical_or(_TV __x, _TV __y)
        { return __vec_or(__x, __y); }

      template <typename _TV>
        requires __vec_builtin<_TV> or same_as<_TV, _MaskVec128<_S_size>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_bit_not(_TV __x)
        {
          using _Tp = __value_type_of<_TV>;
          if constexpr (_Abi::_S_is_partial)
            return __vec_andnot(__x, __builtin_bit_cast(_TV, _Abi::template _S_implicit_mask<_Tp>));
          else
            return __vec_not(__x);
        }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_all_of(__vec_builtin auto __k)
      { return _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, { return (... and (__k[_Is] != 0)); }); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_all_of(_MaskVec128<_S_size> __k)
      { return _MaskVec128<_S_size>::template _UnderlyingImpl<_Abi>::_S_all_of(__k._M_data); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_any_of(__vec_builtin auto __k)
      { return _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, { return (... or (__k[_Is] != 0)); }); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_any_of(_MaskVec128<_S_size> __k)
      { return _MaskVec128<_S_size>::template _UnderlyingImpl<_Abi>::_S_any_of(__k._M_data); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_none_of(__vec_builtin auto __k)
      { return _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, { return (... and (__k[_Is] == 0)); }); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_none_of(_MaskVec128<_S_size> __k)
      { return _MaskVec128<_S_size>::template _UnderlyingImpl<_Abi>::_S_none_of(__k._M_data); }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdSizeType
        _S_reduce_min_index(std::datapar::basic_simd_mask<_Bs, abi_type> __k)
        { return __lowest_bit(_SuperImpl::_S_to_bits(__data(__k))._M_to_bits()); }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdSizeType
        _S_reduce_max_index(std::datapar::basic_simd_mask<_Bs, abi_type> __k)
        { return __highest_bit(_SuperImpl::_S_to_bits(__data(__k))._M_sanitized()._M_to_bits()); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr auto
        _S_and_neighbors(_TV __k)
        {
          __k = __vec_and(__k, _S_swap_neighbors(__k));
          using _Tp = __value_type_of<_TV>;
          if constexpr (sizeof(_Tp) == 8)
            return _MaskVec128<_S_size / 2> {__k};
          else
            return reinterpret_cast<__vec_builtin_type_bytes<
                                      __mask_integer_from<sizeof(_Tp) * 2>, sizeof(_TV)>>(__k);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr auto
        _S_or_neighbors(_TV __k)
        {
          __k = __vec_or(__k, _S_swap_neighbors(__k));
          using _Tp = __value_type_of<_TV>;
          if constexpr (sizeof(_Tp) == 8)
            return _MaskVec128<_S_size / 2> {__k};
          else
            return reinterpret_cast<__vec_builtin_type_bytes<
                                      __mask_integer_from<sizeof(_Tp) * 2>, sizeof(_TV)>>(__k);
        }

      template <__vectorizable_canon _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr typename _Abi::template _MaskMember<_Tp>
        _S_mask_with_n_true(__type_identity_t<_Tp> __n)
        {
          return _SuperImpl::_S_less(_S_vec_iota<_SimdMember<_Tp>>,
                                     __vec_broadcast<_S_full_size>(__n));
        }

      template <typename _Tp>
        requires (not requires { typename _Abi::_MaskInteger; })
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_mask_generator(auto&& __gen)
        {
          return _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                   if constexpr (is_same_v<_Tp, _MaskUInt128>)
                     {
                       const bool __tmp[_S_size] = {bool(__gen(__ic<_Is>))...};
                       return _MaskMember<_Tp>{
                         _GLIBCXX_SIMD_INT_PACK(2 * _S_size, _Js, {
                           return typename _MaskMember<_Tp>::_VecType{(-__tmp[_Js / 2])...};
                         })
                       };
                     }
                   else
                     {
                       static_assert(__vec_builtin<_MaskMember<_Tp>>);
                       return _MaskMember<_Tp>{_Tp(-bool(__gen(__ic<_Is>)))...};
                     }
                 });
        }

      // can permute to a size different to _S_size; __x must always be of _S_size, though
      template <_SimdSizeType _RetSize = _S_size, _SimdSizeType _IndexOffset = 0,
                __vec_builtin _TV, typename _Fp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr
        __vec_builtin_type<__value_type_of<_TV>, __signed_bit_ceil(_RetSize)>
        _S_permute(_TV __x, const _Fp __idx_perm) noexcept
        {
          static_assert(is_same_v<_TV, _SimdMember<__value_type_of<_TV>>>);
          constexpr auto __idx_perm2 = [=](auto __i) {
            if constexpr (__detail::__index_permutation_function_nosize<_Fp>)
              return __ic<__idx_perm(_IndexOffset + __i)>;
            else if constexpr (__i >= _S_size)
              return __i;
            else
              return __ic<__idx_perm(_IndexOffset + __i, _S_size)>;
          };
          constexpr auto __adj_idx = [](auto __i) {
            constexpr int __j = __i;
            if constexpr (__j == datapar::zero_element)
              return __ic<_S_full_size>;
            else if constexpr (__j == datapar::uninit_element)
              return __ic<-1>;
            else if constexpr (__i >= _S_size)
              return  __ic<(__j >= 0 and __j < _S_full_size ? __j : -1)>;
            else
              {
                static_assert (__j >= 0 and __j < _S_size, "invalid index");
                return __ic<__j>;
              }
          };
          return [&]<_SimdSizeType... _Is>(_SimdIndexSequence<_Is...>) {
            return __builtin_shufflevector(__x, _TV(), __adj_idx(__idx_perm2(__ic<_Is>)).value...);
          }(_MakeSimdIndexSequence<_S_full_size>());
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_swap_neighbors(_TV __x) noexcept
        {
          static_assert((_S_size & 1) == 0);
          return _VecOps<_TV>::_S_swap_neighbors(__x);
        }
    };
}

#pragma GCC diagnostic pop

#endif // PROTOTYPE_SIMD_BUILTIN_H_

