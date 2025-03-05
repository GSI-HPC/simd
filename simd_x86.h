/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_X86_H_
#define PROTOTYPE_SIMD_X86_H_

#include "simd_converter.h"
#include "simd_builtin.h"
#include "x86_detail.h"

#include <x86intrin.h>

namespace std
{
  template <int _Width>
    struct _Avx512Abi
#if not __AVX512F__
    {
      struct _IsValidAbiTag
      : false_type
      {};

      template <typename>
        struct _IsValidSizeFor
        : false_type
        {};

      template <typename>
        using _IsValid = false_type;

      static constexpr __detail::_SimdSizeType _S_size = 0;

      static constexpr __detail::_SimdSizeType _S_full_size = 0;

      static constexpr bool _S_is_partial = false;

      template <typename _Tp>
        struct __traits
        : __detail::_InvalidTraits
        {};
    };
#else
    : _VecAbi<_Width>
    {
      using _SimdSizeType = __detail::_SimdSizeType;

      static constexpr _SimdSizeType _S_size = _Width;

      static constexpr _SimdSizeType _S_full_size = std::__bit_ceil(_Width);

      static constexpr bool _S_is_partial = _S_full_size > _S_size;

      using _MaskInteger = typename __detail::__make_unsigned_int<
                             std::min(8, std::max(8, _S_full_size) / __CHAR_BIT__)>::type;

      template <typename _Tp>
        struct __traits
        : __detail::_InvalidTraits
        {};

      template <typename _Tp>
        requires (_VecAbi<_Width>::template _IsValid<_Tp>::value)
        struct __traits<_Tp>
        {
          // conversions to _Avx512Abi should always be implicit
          template <typename _FromAbi>
            static constexpr bool _S_explicit_mask_conversion = false;

          using _Impl = __detail::_ImplBuiltin<_Avx512Abi>;

          using _SimdImpl = _Impl;

          using _MaskImpl = _Impl;

          static constexpr _SimdSizeType _S_size = _Width;

          static constexpr _SimdSizeType _S_full_size = std::__bit_ceil(_Width);

          static constexpr bool _S_is_partial = _S_full_size > _S_size;

          using _SimdMember = _VecAbi<_Width>::template _SimdMember<_Tp>;

          using _MaskMember = _MaskInteger;

          static constexpr size_t _S_simd_align = alignof(_SimdMember);

          static constexpr size_t _S_mask_align = alignof(_MaskMember);

          template <typename _Arg>
            static constexpr bool _S_is_simd_ctor_arg
              = std::is_same_v<_Arg, _SimdMember>
                  or std::is_same_v<_Arg, __detail::__x86_intrin_t<_SimdMember>>;

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

      using _Impl = __detail::_ImplBuiltin<_Avx512Abi>;

      using _SimdImpl = _Impl;

      using _MaskImpl = _Impl;

      template <__detail::__vectorizable_canon _Up>
        using _Rebind = std::conditional_t<
                          _Avx512Abi<_S_size>::template _IsValid<_Up>::value,
                          _Avx512Abi<_S_size>, __detail::__deduce_t<_Up, _S_size>>;

      template <typename _Tp>
        using _SimdMember = __traits<_Tp>::_SimdMember;

      template <typename>
        using _MaskMember = _MaskInteger;

      static constexpr bool _S_mask_is_partial = _S_size < 8 or not std::__has_single_bit(_S_size);

      // The template argument exists because _VecAbi::_S_implicit_mask needs it. And _ImplBuiltin
      // below needs to work generically for _VecAbi and _Avx512Abi.
      template <__detail::__vectorizable_canon>
        static constexpr _MaskInteger _S_implicit_mask
          = _S_mask_is_partial ? _MaskInteger((1ULL << _S_size) - 1) : ~_MaskInteger();

      _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskInteger
      _S_masked(_MaskInteger __x)
      {
        if constexpr (_S_mask_is_partial)
          // _S_mask_is_partial implies _S_size < 64
          return __x & _MaskInteger((1ULL << _S_size) - 1);
        else
          return __x;
      }

      using _VecAbi<_Width>::_S_masked;

      template <__detail::__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr auto
        __make_padding_nonzero(_TV __x)
        {
          using _Tp = __detail::__value_type_of<_TV>;
          if constexpr (not _S_is_partial)
            return __x;
          else if (__builtin_is_constant_evaluated() or __builtin_constant_p(__x))
            return _GLIBCXX_SIMD_INT_PACK(_S_full_size, _Is, {
                     return _TV{(_Is < _S_size ? __detail::__vec_get(__x, _Is) : _Tp(1))...};
                   });
          else
            return _Impl::_S_select_bitmask_noconstprop(_S_implicit_mask<_Tp>, __x,
                                                        _Impl::_S_broadcast(_Tp(1)));
        }
    };
#endif
}

#if __x86_64__ or __i386__
namespace std::__detail
{
  template <typename _Abi, _BuildFlags _Flags>
    struct _SimdMaskTraits<sizeof(float), _Abi, _Flags>
    : _SimdTraits<conditional_t<_Flags._M_have_avx() and not _Flags._M_have_avx2(),
                                float, __mask_integer_from<sizeof(float)>>, _Abi, _Flags>
    {};

  template <typename _Abi, _BuildFlags _Flags>
    struct _SimdMaskTraits<sizeof(double), _Abi, _Flags>
    : _SimdTraits<conditional_t<_Flags._M_have_avx() and not _Flags._M_have_avx2(),
                                double, __mask_integer_from<sizeof(double)>>, _Abi, _Flags>
    {};

  enum class _X86Round
  {
    _ToNearestInt = 0x00,
    _ToNegInf = 0x01,
    _ToPosInf = 0x02,
    _ToZero = 0x03,
    _CurDirection = 0x04,

    _RaiseException = 0x00,
    _NoException = 0x08,

    _Nint = _ToNearestInt | _RaiseException,
    _Floor = _ToNegInf | _RaiseException,
    _Ceil = _ToPosInf | _RaiseException,
    _Trunc = _ToZero | _RaiseException,
    _Rint = _CurDirection | _RaiseException,
    _NearbyInt = _CurDirection | _NoException,
  };

  template <typename _Abi, _BuildFlags _Flags>
    struct _ImplBuiltin : _ImplBuiltinBase<_Abi, _Flags>
    {
      using _Base = _ImplBuiltinBase<_Abi, _Flags>;

      using abi_type = typename _Base::abi_type;

      template <typename _Tp>
        using _TypeTag = _Tp*;

      template <typename _Tp>
        using _SimdMember = typename _Abi::template _SimdMember<_Tp>;

      // _Tp can be a __vec_builtin or __arithmetic type
      template <typename _Tp>
        using _MaskMember = typename _Abi::template _MaskMember<__value_type_of<_Tp>>;

      static constexpr _SimdSizeType _S_size = _Abi::_S_size;
      static_assert(_S_size > 1);

      static constexpr _SimdSizeType _S_full_size = _Abi::_S_full_size;

      static constexpr bool _S_is_partial = _Abi::_S_is_partial;

      static constexpr bool _S_use_bitmasks = is_same_v<_Abi, _Avx512Abi<_S_size>>;

      using _MaskInteger = typename __detail::__make_unsigned_int<
                             std::min(8, std::max(8, _S_full_size) / __CHAR_BIT__)>::type;

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskInteger
        _S_to_bitmask(_TV __k)
        { return _S_to_bits(__k)._M_to_bits(); }

      template <std::integral _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_to_bitmask(_Tp __k)
        { return __k; }

      using _Base::_S_load;

      template <typename _Tp, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static _SimdMember<_Tp>
        _S_partial_load(const _Up* __mem, size_t __mem_size, _TypeTag<_Tp> __tag)
        {
          constexpr bool __have_masked_load = _S_use_bitmasks
                                                or (sizeof(_Up) >= 4 and _Flags._M_have_avx());
          constexpr bool __use_two_loads = not _Flags._M_have_avx2()
                                             and sizeof(_SimdMember<_Tp>) == 32;
          using _Ucanon = __canonical_vec_type_t<_Up>;
          constexpr bool __need_cvt = not is_same_v<_Tp, _Ucanon>
                                        and not (is_integral_v<_Tp> and is_integral_v<_Up>
                                                   and sizeof(_Tp) == sizeof(_Up));

          if (__mem_size >= _S_size) [[unlikely]]
            return _S_load(__mem, __tag);
          else if constexpr (not __have_masked_load)
            {
#if 1 // allow out-of-bounds read when it cannot lead to a #GP
              if (__ptr_is_aligned_to(__mem, sizeof(_Up) * _S_full_size))
                return _S_bit_and(_S_load(__mem, __tag),
                                  reinterpret_cast<_SimdMember<_Tp>>(
                                    _S_mask_with_n_true<_Tp>(__mem_size)));
              else
#endif
                return _Base::_S_partial_load(__mem, __mem_size, __tag);
            }

          else if constexpr (__need_cvt)
            {
              using _UImpl = typename __deduced_traits<_Ucanon, _S_size>::_SimdImpl;
              constexpr _TypeTag<_Ucanon> __utag = nullptr;
              constexpr _SimdConverter<_Ucanon, typename _UImpl::abi_type, _Tp, abi_type> __cvt;
              return __cvt(_UImpl::_S_partial_load(__mem, __mem_size, __utag));
            }

          else if constexpr (__use_two_loads)
            {
              using _Traits2 = __deduced_traits<_Tp, _S_full_size / 2>;
              using _Impl2 = typename _Traits2::_SimdImpl;
              using _SM2 = typename _Traits2::_SimdMember;
              if (__mem_size <= _S_full_size / 2)
                {
                  const auto __k = _Impl2::template _S_mask_with_n_true<
                                     __mask_integer_from<sizeof(_Up)>>(__mem_size);
                  const auto __low = _Impl2::_S_masked_load( __k, __mem, __tag);
                  const _SM2 __high = {};
                  return __vec_concat(__low, __high);
                }
              else
                {
                  using _Ip = __mask_integer_from<sizeof(_Up)>;
                  const _SM2 __low = _Impl2::_S_load(__mem, __tag);
                  const auto __k
                    = _Impl2::_S_less(
                        _S_vec_iota<typename _VecAbi<_S_full_size / 2>::template _SimdMember<_Ip>,
                                    _S_full_size / 2>,
                        __vec_broadcast<_S_full_size / 2>(_Ip(__mem_size)));
                  const _SM2 __high
                    = _Impl2::_S_masked_load(__k, __mem + _S_full_size / 2, __tag);
                  return __vec_concat(__low, __high);
                }
            }

          else
            return _S_masked_load(_S_mask_with_n_true<__mask_integer_from<sizeof(_Up)>>(__mem_size),
                                  __mem, __tag);
        }

      template <typename _Tp, typename _Up>
        static inline _SimdMember<_Tp>
        _S_masked_load(_MaskMember<_SimdMember<__canonical_vec_type_t<_Up>>> __k, const _Up* __mem,
                       _TypeTag<_Tp> __tag)
        {
          using _TV = _SimdMember<_Tp>;
          constexpr bool __bitwise_conversion = sizeof(_Tp) == sizeof(_Up)
                                                  and is_integral_v<_Tp> == is_integral_v<_Up>;
          using _Ucanon = __canonical_vec_type_t<_Up>;
          constexpr bool __need_cvt = not is_same_v<_Tp, _Ucanon> and not __bitwise_conversion;
          if constexpr (__need_cvt)
            {
              //using _UTraits = _SimdTraits<_Up, _NativeAbi<_Up>>;
              using _UImpl = typename __deduced_traits<_Ucanon>::_SimdImpl;
              /*              constexpr _TypeTag<_Ucanon> __utag = nullptr;
              constexpr int __chunk_size = _UTraits::_S_size);
              if constexpr (_S_size > __chunk_size)
                { // needs multiple loads
                  if constexpr (_S_size / __chunk_size <= 2)
                    {
                      if (__mem_size >= __chunk_size)
                        {
                          _UImpl::_S_load(__mem, __utag);
                          _UImpl::_S_masked_load(__k, __mem, __utag);

                        }
                    }
                  _UImpl::template _S_masked_load<_Up>(__k, __mem, nullptr);
                }
              else*/
                return __builtin_convertvector(_UImpl::template _S_masked_load<_Ucanon>(
                                                 __k, __mem, nullptr), _TV);
            }
          else
            {
              constexpr auto __vecbytes = sizeof(_SimdMember<_Tp>);
              const auto __kk = _S_to_bitmask(__k);

#ifdef __clang__
#define _GLIBCXX_SIMD_LOAD_CAST(type2) reinterpret_cast<const _VecT*>
#else
#define _GLIBCXX_SIMD_LOAD_CAST(type2) reinterpret_cast<const type2*>
#endif

#define _GLIBCXX_SIMD_MASK_LOAD(type, type2)                                                       \
  using _VecT = __vec_builtin_type_bytes<type2, __vecbytes <= 16 ? 16 : __vecbytes>;               \
  if constexpr (__vecbytes <= 16)                                                                  \
    {                                                                                              \
      const auto __v                                                                               \
        = __builtin_ia32_load##type##128_mask(_GLIBCXX_SIMD_LOAD_CAST(type2)(__mem),               \
                                                    _VecT(), __kk);                                \
      if constexpr (__vecbytes < 16)                                                               \
        return reinterpret_cast<_TV>(__vec_lo<__vecbytes>(__v));                                   \
      else                                                                                         \
        return reinterpret_cast<_TV>(__v);                                                         \
    }                                                                                              \
  else if constexpr (__vecbytes == 32)                                                             \
    return reinterpret_cast<_TV>(                                                                  \
             __builtin_ia32_load##type##256_mask(_GLIBCXX_SIMD_LOAD_CAST(type2)(__mem),            \
                                                 _VecT(), __kk));                                  \
  else if constexpr (__vecbytes == 64)                                                             \
    return reinterpret_cast<_TV>(                                                                  \
             __builtin_ia32_load##type##512_mask(reinterpret_cast<const type2*>(__mem),            \
                                                 _VecT(), __kk));                                  \
  else                                                                                             \
    static_assert(false)

              if constexpr (_Flags._M_have_avx512bw() and _Flags._M_have_avx512vl()
                              and sizeof(_Tp) == 1)
                {
                  _GLIBCXX_SIMD_MASK_LOAD(dquqi, char);
                }
              else if constexpr (_Flags._M_have_avx512bw() and _Flags._M_have_avx512vl()
                                   and sizeof(_Tp) == 2)
                {
                  _GLIBCXX_SIMD_MASK_LOAD(dquhi, short);
                }
              else if constexpr (_Flags._M_have_avx512vl() and sizeof(_Tp) == 4
                                   and is_integral_v<_Up>)
                {
                  _GLIBCXX_SIMD_MASK_LOAD(dqusi, int);
                }
              else if constexpr (_Flags._M_have_avx512vl() and sizeof(_Tp) == 4
                                   and is_floating_point_v<_Up>)
                {
                  _GLIBCXX_SIMD_MASK_LOAD(ups, float);
                }
              else if constexpr (_Flags._M_have_avx512vl() and sizeof(_Tp) == 8
                                   and is_integral_v<_Up>)
                {
                  _GLIBCXX_SIMD_MASK_LOAD(dqudi, long long);
                }
              else if constexpr (_Flags._M_have_avx512vl() and sizeof(_Tp) == 8
                                   and is_floating_point_v<_Up>)
                {
                  _GLIBCXX_SIMD_MASK_LOAD(upd, double);
                }
              else
                {
                  static_assert(not _S_use_bitmasks);
                  using _IV = __vec_builtin_type<
                                conditional_t<sizeof(_Up) == 4, int, long long>, _S_full_size>;
                  const auto __vk = reinterpret_cast<_IV>(__k);
                  if constexpr (_Flags._M_have_avx2() and is_integral_v<_Up> and sizeof(_Up) >= 4)
                    {
                      const auto* __vmem = reinterpret_cast<const _IV*>(__mem);
                      if constexpr (sizeof(_Up) == 4 and __vecbytes == 32)
                        return reinterpret_cast<_TV>(__builtin_ia32_maskloadd256(__vmem, __vk));
                      else if constexpr (sizeof(_Up) == 4 and __vecbytes == 16)
                        return reinterpret_cast<_TV>(__builtin_ia32_maskloadd(__vmem, __vk));
                      else if constexpr (sizeof(_Up) == 8 and __vecbytes == 32)
                        return reinterpret_cast<_TV>(__builtin_ia32_maskloadq256(__vmem, __vk));
                      else if constexpr (sizeof(_Up) == 8 and __vecbytes == 16)
                        return reinterpret_cast<_TV>(__builtin_ia32_maskloadq(__vmem, __vk));
                    }
                  else if constexpr (_Flags._M_have_avx() and sizeof(_Up) >= 4)
                    {
                      using _FV = __vec_builtin_type<
                                    conditional_t<sizeof(_Up) == 4, float, double>, _S_full_size>;
                      const auto* __vmem = reinterpret_cast<const _FV*>(__mem);
                      if constexpr (sizeof(_Up) == 4 and __vecbytes == 32)
                        return reinterpret_cast<_TV>(__builtin_ia32_maskloadps256(__vmem, __vk));
                      else if constexpr (sizeof(_Up) == 4 and __vecbytes == 16)
                        return reinterpret_cast<_TV>(__builtin_ia32_maskloadps(__vmem, __vk));
                      else if constexpr (sizeof(_Up) == 8 and __vecbytes == 32)
                        return reinterpret_cast<_TV>(__builtin_ia32_maskloadpd256(__vmem, __vk));
                      else if constexpr (sizeof(_Up) == 8 and __vecbytes == 16)
                        return reinterpret_cast<_TV>(__builtin_ia32_maskloadpd(__vmem, __vk));
                    }
                  return _Base::_S_masked_load(__k, __mem, __tag);
                }
#undef _GLIBCXX_SIMD_MASK_LOAD
#undef _GLIBCXX_SIMD_LOAD_CAST
            }
        }

      template <__vec_builtin _TV, typename _Up>
        static inline _TV
        _S_masked_load(_TV __merge, _MaskMember<_TV> __k, const _Up* __mem)
        {
          using _Tp = __value_type_of<_TV>;
          using _Ucanon = __canonical_vec_type_t<_Up>;
          constexpr bool __no_conversion = is_same_v<_Tp, _Ucanon>;
          constexpr bool __bitwise_conversion = sizeof(_Tp) == sizeof(_Up)
                                                  and is_integral_v<_Tp> == is_integral_v<_Up>;
          if constexpr (__no_conversion or __bitwise_conversion)
            {
              [[maybe_unused]] const auto __intrin = __to_x86_intrin(__merge);
              const auto __kk = _S_to_bitmask(__k);

#define _GLIBCXX_SIMD_MASK_LOAD(type, type2, bits)                                                 \
  __merge = reinterpret_cast<_TV>(__builtin_ia32_loaddqu##type##i##bits##_mask(                    \
              reinterpret_cast<const type2*>(__mem),                                               \
              reinterpret_cast<__vec_builtin_type<type2, bits / 8>>(__merge), __kk))

#define _GLIBCXX_SIMD_MASK_LOAD_FLT(type, type2, bits)                                             \
  __merge = reinterpret_cast<_TV>(__builtin_ia32_loadup##type##i##bits##_mask(                     \
              reinterpret_cast<const type2*>(__mem),                                               \
              reinterpret_cast<__vec_builtin_type<type2, bits / 8>>(__merge), __kk))

              if constexpr (_Flags._M_have_avx512bw() and _Flags._M_have_avx512vl()
                              and sizeof(_Tp) == 1)
                {
                  if constexpr (sizeof(__intrin) == 16)
                    _GLIBCXX_SIMD_MASK_LOAD(q, char, 128);
                  else if constexpr (sizeof(__merge) == 32)
                    _GLIBCXX_SIMD_MASK_LOAD(q, char, 256);
                  else if constexpr (sizeof(__merge) == 64)
                    _GLIBCXX_SIMD_MASK_LOAD(q, char, 512);
                  else
                    static_assert(false);
                }
              else if constexpr (_Flags._M_have_avx512bw() and _Flags._M_have_avx512vl()
                                   and sizeof(_Tp) == 2)
                {
                  if constexpr (sizeof(__intrin) == 16)
                    _GLIBCXX_SIMD_MASK_LOAD(h, short, 128);
                  else if constexpr (sizeof(__intrin) == 32)
                    _GLIBCXX_SIMD_MASK_LOAD(h, short, 256);
                  else if constexpr (sizeof(__intrin) == 64)
                    _GLIBCXX_SIMD_MASK_LOAD(h, short, 512);
                  else
                    static_assert(false);
                }
              else if constexpr (_Flags._M_have_avx512vl() and sizeof(_Tp) == 4
                                   and is_integral_v<_Up>)
                {
                  if constexpr (sizeof(__intrin) == 16)
                    _GLIBCXX_SIMD_MASK_LOAD(s, int, 128);
                  else if constexpr (sizeof(__intrin) == 32)
                    _GLIBCXX_SIMD_MASK_LOAD(s, int, 256);
                  else if constexpr (sizeof(__intrin) == 64)
                    _GLIBCXX_SIMD_MASK_LOAD(s, int, 512);
                  else
                    static_assert(false);
                }
              else if constexpr (_Flags._M_have_avx512vl() and sizeof(_Tp) == 4
                                   and is_floating_point_v<_Up>)
                {
                  if constexpr (sizeof(__intrin) == 16)
                    _GLIBCXX_SIMD_MASK_LOAD_FLT(s, float, 128);
                  else if constexpr (sizeof(__intrin) == 32)
                    _GLIBCXX_SIMD_MASK_LOAD_FLT(s, float, 256);
                  else if constexpr (sizeof(__intrin) == 64)
                    _GLIBCXX_SIMD_MASK_LOAD_FLT(s, float, 512);
                  else
                    static_assert(false);
                }
              else if constexpr (_Flags._M_have_avx2() and sizeof(_Tp) == 4 and is_integral_v<_Up>)
                {
                  static_assert(_S_use_bitmasks == false);
                  static_assert(sizeof(__intrin) == 16 or sizeof(__intrin) == 32);
                  __merge = __vec_or(__vec_andnot(reinterpret_cast<_TV>(__k), __merge),
                                     reinterpret_cast<_TV>(__maskload_epi32(__mem, __k)));
                }
              else if constexpr (_Flags._M_have_avx() and sizeof(_Tp) == 4)
                {
                  static_assert(sizeof(__intrin) == 16 or sizeof(__intrin) == 32);
                  __merge = __vec_or(__vec_andnot(reinterpret_cast<_TV>(__k), __merge),
                                     reinterpret_cast<_TV>(__maskload_ps(__mem, __k)));
                }
              else if constexpr (_Flags._M_have_avx512vl() and sizeof(_Tp) == 8
                                   and is_integral_v<_Up>)
                {
                  if constexpr (sizeof(__intrin) == 16)
                    _GLIBCXX_SIMD_MASK_LOAD(d, long long, 128);
                  else if constexpr (sizeof(__intrin) == 32)
                    _GLIBCXX_SIMD_MASK_LOAD(d, long long, 256);
                  else if constexpr (sizeof(__intrin) == 64)
                    _GLIBCXX_SIMD_MASK_LOAD(d, long long, 512);
                  else
                    static_assert(false);
                }
              else if constexpr (_Flags._M_have_avx512vl() and sizeof(_Tp) == 8
                                   and is_floating_point_v<_Up>)
                {
                  if constexpr (sizeof(__intrin) == 16)
                    _GLIBCXX_SIMD_MASK_LOAD_FLT(d, double, 128);
                  else if constexpr (sizeof(__intrin) == 32)
                    _GLIBCXX_SIMD_MASK_LOAD_FLT(d, double, 256);
                  else if constexpr (sizeof(__intrin) == 64)
                    _GLIBCXX_SIMD_MASK_LOAD_FLT(d, double, 512);
                  else
                    static_assert(false);
                }
              else if constexpr (_Flags._M_have_avx2() and sizeof(_Tp) == 8 and is_integral_v<_Up>)
                {
                  static_assert(sizeof(__intrin) == 16 or sizeof(__intrin) == 32);
                  __merge = __vec_or(__vec_andnot(reinterpret_cast<_TV>(__k), __merge),
                                     reinterpret_cast<_TV>(__maskload_epi64( __mem, __k)));
                }
              else if constexpr (_Flags._M_have_avx() and sizeof(_Tp) == 8)
                {
                  static_assert(sizeof(__intrin) == 16 or sizeof(__intrin) == 32);
                  __merge = __vec_or(__vec_andnot(reinterpret_cast<_TV>(__k), __merge),
                                     reinterpret_cast<_TV>(__maskload_pd(__mem, __k)));
                }
              else
                _S_bit_iteration(_S_to_bits(__k),
                                 [&] [[__gnu__::__always_inline__]] (auto __i) {
                                   __merge[__i] = static_cast<_Tp>(__mem[__i]);
                                 });
            }
	/* Very uncertain, that the following improves anything. Needs benchmarking
	 * before it's activated.
	else if constexpr (sizeof(_Up) <= 8 and // no long double
			   !__converts_via_decomposition_v<
			     _Up, _Tp,
			     sizeof(__merge)> // conversion via decomposition
					      // is better handled via the
					      // bit_iteration fallback below
	)
	  {
	    // TODO: copy pattern from _S_masked_store, which doesn't resort to
	    // fixed_size
	    using _Ap       = simd_abi::deduce_t<_Up, _Np>;
	    using _ATraits = _SimdTraits<_Up, _Ap>;
	    using _AImpl   = typename _ATraits::_SimdImpl;
	    typename _ATraits::_SimdMember __uncvted{};
	    typename _ATraits::_MaskMember __kk = _Ap::_Impl::template _S_convert<_Up>(__k);
	    __uncvted = _AImpl::_S_masked_load(__uncvted, __kk, __mem);
	    _SimdConverter<_Up, _Ap, _Tp, _Abi> __converter;
	    _Base::_S_masked_assign(__k, __merge, __converter(__uncvted));
	  }
	  */
          else
            __merge = _Base::_S_masked_load(__merge, __k, __mem);
          return __merge;
#undef _GLIBCXX_SIMD_MASK_LOAD
#undef _GLIBCXX_SIMD_MASK_LOAD_FLT
        }

      // Returns: __k ? __a : __b
      // Requires: _TV to be a __vec_builtin_type matching valuetype for the bitmask __k
      template <std::integral _Kp, __vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_select_bitmask(const _Kp __k, const _TV __a, const _TV __b)
        {
          using _Tp = __value_type_of<_TV>;
          static_assert(sizeof(_Tp) <= 8);
          if (__builtin_is_constant_evaluated()
                or (__builtin_constant_p(__k) and __builtin_constant_p(__a)
                      and __builtin_constant_p(__b)))
            {
              const _TV __r = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                return _TV{(((__k >> _Is) & 1) == 1 ? __vec_get(__a, _Is)
                                                                    : __vec_get(__b, _Is))...};
                              });
              if (__builtin_is_constant_evaluated() or __builtin_constant_p(__r))
                return __r;
            }

          if (__builtin_constant_p(__k) and (__k & _Abi::template _S_implicit_mask<_Tp>) == 0)
            return __b;

          if (__builtin_constant_p(__k) and (__k & _Abi::template _S_implicit_mask<_Tp>)
                == _Abi::template _S_implicit_mask<_Tp>)
            return __a;

          return _S_select_bitmask_noconstprop(__k, __a, __b);
        }

      template <std::integral _Kp, __vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static _TV
        _S_select_bitmask_noconstprop(const _Kp __k, const _TV __a, const _TV __b)
        {
#ifdef __clang__
          return _S_convert_mask<__mask_vec_from<_TV>>(_BitMask<_S_size>(__k)) ? __a : __b;
#else
          using _Tp = __value_type_of<_TV>;
          using _IntT = __x86_builtin_int_t<_Tp>;
          using _IntV = __vec_builtin_type_bytes<_IntT, sizeof(__b)>;
          [[maybe_unused]] const auto __aa = reinterpret_cast<_IntV>(__b);
          [[maybe_unused]] const auto __bb = reinterpret_cast<_IntV>(__a);
          if constexpr (sizeof(_TV) == 64)
            {
              if constexpr (sizeof(_Tp) == 1)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmb_512_mask(__aa, __bb, __k));
              else if constexpr (sizeof(_Tp) == 2)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmw_512_mask(__aa, __bb, __k));
              else if constexpr (sizeof(_Tp) == 4 and is_floating_point_v<_Tp>)
                return __builtin_ia32_blendmps_512_mask(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 4)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmd_512_mask(__aa, __bb, __k));
              else if constexpr (sizeof(_Tp) == 8 and is_floating_point_v<_Tp>)
                return __builtin_ia32_blendmpd_512_mask(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 8)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmq_512_mask(__aa, __bb, __k));
              else
                static_assert(false);
            }
          else if constexpr (sizeof(_TV) == 32)
            {
              if constexpr (sizeof(_Tp) == 1)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmb_256_mask(__aa, __bb, __k));
              else if constexpr (sizeof(_Tp) == 2)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmw_256_mask(__aa, __bb, __k));
              else if constexpr (sizeof(_Tp) == 4 and is_floating_point_v<_Tp>)
                return __builtin_ia32_blendmps_256_mask(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 4)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmd_256_mask(__aa, __bb, __k));
              else if constexpr (sizeof(_Tp) == 8 and is_floating_point_v<_Tp>)
                return __builtin_ia32_blendmpd_256_mask(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 8)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmq_256_mask(__aa, __bb, __k));
              else
                static_assert(false);
            }
          else if constexpr (sizeof(_TV) == 16)
            {
              if constexpr (sizeof(_Tp) == 1)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmb_128_mask(__aa, __bb, __k));
              else if constexpr (sizeof(_Tp) == 2)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmw_128_mask(__aa, __bb, __k));
              else if constexpr (sizeof(_Tp) == 4 and is_floating_point_v<_Tp>)
                return __builtin_ia32_blendmps_128_mask(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 4)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmd_128_mask(__aa, __bb, __k));
              else if constexpr (sizeof(_Tp) == 8 and is_floating_point_v<_Tp>)
                return __builtin_ia32_blendmpd_128_mask(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 8)
                return reinterpret_cast<_TV>(__builtin_ia32_blendmq_128_mask(__aa, __bb, __k));
              else
                static_assert(false);
            }
          else if constexpr (sizeof(_TV) < 16)
            return __vec_bitcast_trunc<_TV>(_S_select_bitmask(__k, __vec_zero_pad_to_16(__a),
                                                              __vec_zero_pad_to_16(__b)));
          else
            static_assert(false);
#endif
        }

      template <std::integral _Kp, __vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_select(const _Kp __k, const _TV __a, const _TV __b)
        { return _S_select_bitmask(__k, __a, __b); }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static _TV
        _S_select_on_msb(const _MaskMember<_TV> __k, const _TV __a, const _TV __b)
        {
          using _Tp = __value_type_of<_TV>;
          static_assert(not _Flags._M_have_avx512f(), "really? If yes, you need to implement it.");
          static_assert(sizeof(_TV) <= 32);
          static_assert(is_signed_v<__value_type_of<_MaskMember<_TV>>>);
          const auto __ia = reinterpret_cast<_MaskMember<_TV>>(__a);
          const auto __ib = reinterpret_cast<_MaskMember<_TV>>(__b);
          if (_Base::_S_is_constprop_all_of(__k))
            return __a;
          else if (_Base::_S_is_constprop_none_of(__k))
            return __b;
          else if (__builtin_constant_p(__ia) and _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                                    return ((__ia[_Is] == 0) and ...);
                                                  }))
            return __vec_andnot(reinterpret_cast<_TV>(__k < 0), __b);
          else if (__builtin_constant_p(__ib) and _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                                    return ((__ib[_Is] == 0) and ...);
                                                  }))
            return __vec_and(reinterpret_cast<_TV>(__k < 0), __a);
          else if constexpr (_Flags._M_have_sse4_1()) // for vblend
            {
              using _IV = __vec_builtin_type_bytes<char, sizeof(_TV)>;
              // duplicate msb to high bit of low byte for sizeof(_Tp) == 2 (or rather to all bits)
              const _IV __ki = reinterpret_cast<_IV>(sizeof(_Tp) == 2 ? __k < 0 : __k);
              const _IV __ai = reinterpret_cast<_IV>(__a);
              const _IV __bi = reinterpret_cast<_IV>(__b);
              if constexpr (sizeof(_Tp) == 4 and sizeof(_TV) == 32 and _Flags._M_have_avx())
                return __builtin_ia32_blendvps256(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 8 and sizeof(_TV) == 32 and _Flags._M_have_avx())
                return __builtin_ia32_blendvpd256(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 4 and sizeof(_TV) == 16)
                return __builtin_ia32_blendvps(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 8 and sizeof(_TV) == 16)
                return __builtin_ia32_blendvpd(__b, __a, __k);
              else if constexpr (sizeof(_TV) == 32 and _Flags._M_have_avx2())
                return reinterpret_cast<_TV>(__builtin_ia32_pblendvb256(__bi, __ai, __ki));
              else if constexpr (sizeof(_TV) == 16)
                return reinterpret_cast<_TV>(__builtin_ia32_pblendvb128(__bi, __ai, __ki));
            }
          else
            return __k < 0 ? __a : __b;
        }

      /**
       * Pre-condition: \p __k is a mask (all bits either 0 or 1)
       */
      template <__vec_builtin _TV>
        requires (not std::integral<_MaskMember<_TV>>)
        _GLIBCXX_SIMD_INTRINSIC static _TV
        _S_select(const _MaskMember<_TV> __k, const _TV __a, const _TV __b)
        {
          __glibcxx_simd_precondition(_GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                    return ((__k[_Is] == -1 or __k[_Is] == 0) or ...);
                  }), "_S_select called with invalid mask, values must be 0 or -1");
          const auto __ia = reinterpret_cast<_MaskMember<_TV>>(__a);
          const auto __ib = reinterpret_cast<_MaskMember<_TV>>(__b);
          using _Tp = __value_type_of<_TV>;
          if (_Base::_S_is_constprop_all_of(__k))
            return __a;
          else if (_Base::_S_is_constprop_none_of(__k))
            return __b;
          else if (__builtin_constant_p(__ia) and _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                                    return ((__ia[_Is] == 0) and ...);
                                                  }))
            return __vec_andnot(reinterpret_cast<_TV>(__k), __b);
          else if (__builtin_constant_p(__ib) and _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                                    return ((__ib[_Is] == 0) and ...);
                                                  }))
            return __vec_and(reinterpret_cast<_TV>(__k), __a);
          else if constexpr (_Flags._M_have_sse4_1())
            {
              static_assert(sizeof(_TV) == 16 or sizeof(_TV) == 32);
              if constexpr (is_integral_v<_Tp>)
                {
                  using _IV = __vec_builtin_type_bytes<char, sizeof(_TV)>;
                  const _IV __ki = reinterpret_cast<_IV>(__k);
                  const _IV __ai = reinterpret_cast<_IV>(__a);
                  const _IV __bi = reinterpret_cast<_IV>(__b);
                  if constexpr (sizeof(_TV) == 32 and _Flags._M_have_avx2())
                    return reinterpret_cast<_TV>(__builtin_ia32_pblendvb256(__bi, __ai, __ki));
                  else if constexpr (sizeof(_TV) == 16)
                    return reinterpret_cast<_TV>(__builtin_ia32_pblendvb128(__bi, __ai, __ki));
                  else
                    static_assert(false);
                }
              else if constexpr (sizeof(_Tp) == 4 and sizeof(_TV) == 32)
                return __builtin_ia32_blendvps256(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 4 and sizeof(_TV) == 16)
                return __builtin_ia32_blendvps(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 8 and sizeof(_TV) == 32)
                return __builtin_ia32_blendvpd256(__b, __a, __k);
              else if constexpr (sizeof(_Tp) == 8 and sizeof(_TV) == 16)
                return __builtin_ia32_blendvpd(__b, __a, __k);
              else
                static_assert(false);
            }
          else
            return __k ? __a : __b;
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(_MaskMember<_TV> __k, _TV& __lhs, __type_identity_t<_TV> __rhs)
        {
          if constexpr (_S_use_bitmasks)
            __lhs = _S_select(__k, __rhs, __lhs);
          else
            _Base::_S_masked_assign(__k, __lhs, __rhs);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(_MaskMember<_TV> __k, _TV& __lhs,
                         __type_identity_t<__value_type_of<_TV>> __rhs)
        {
          using _Tp = __value_type_of<_TV>;
          if constexpr (_S_use_bitmasks)
            __lhs = _S_select_bitmask(__k, __vec_broadcast<_S_size>(__rhs), __lhs);
          else if (__builtin_is_constant_evaluated())
            __lhs = __k ? __vec_broadcast<_S_size>(__rhs) : __lhs;
          else if (_Base::_S_is_constprop_none_of(__k)
                     or _Base::_S_is_constprop_all_equal(__lhs, __rhs))
            return;
          else if (_Base::_S_is_constprop_all_of(__k))
            __lhs = __vec_broadcast<_S_size>(__rhs);
          else if constexpr (sizeof(_Tp) == 1 and is_integral_v<_Tp>
                               and sizeof(_TV) >= 8 and sizeof(_TV) <= 32)
            {
              // x86 has no byte-sized SIMD shift, so use `psignb` when available, otherwise use a
              // single `and`, instead of calling the _Base impl which shifts.
              if (_Base::_S_is_constprop_all_equal(__lhs, 0))
                {
                  if constexpr ((_Flags._M_have_ssse3() and sizeof(_TV) <= 16)
                                  or (_Flags._M_have_avx2() and sizeof(_TV) == 32))
                    {
                      if (__builtin_constant_p(__rhs) and __rhs == 1)
                        {
                          // like mask::operator+ / mask conversion to _SimdType
                          const auto __ki = __vec_bitcast<__x86_builtin_int_t<_Tp>>(__k);
                          if constexpr (sizeof(_TV) == 32)
                            __lhs = reinterpret_cast<_TV>(__builtin_ia32_psignb256(__ki, __ki));
                          else
                            {
                              auto __k16 = __vec_zero_pad_to_16(__ki);
                              __lhs = __vec_bitcast_trunc<_TV>(
                                        __builtin_ia32_psignb128(__k16, __k16));
                            }
                        }
                      else
                        __lhs = __vec_and(reinterpret_cast<_TV>(__k),
                                          __vec_broadcast<_S_size>(__rhs));
                    }
                  else
                    __lhs = __vec_and(reinterpret_cast<_TV>(__k),
                                      __vec_broadcast<_S_size>(__rhs));
                }
              else
                _Base::_S_masked_assign(__k, __lhs, __rhs);
            }
          else
            _Base::_S_masked_assign(__k, __lhs, __rhs);
        }

      template <std::unsigned_integral _Kp, size_t _Np, bool _Sanitized>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Kp
        _S_convert_mask(_BitMask<_Np, _Sanitized> __x)
        {
          static_assert(is_same_v<_Kp, typename _Abi::_MaskInteger>);
          return __x._M_to_unsanitized_bits();
        }

      template <__vec_builtin _TV, size_t _Np, bool _Sanitized>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_convert_mask(_BitMask<_Np, _Sanitized> __x)
        {
          using _Tp = __value_type_of<_TV>;
          static_assert(is_same_v<_Tp, __mask_integer_from<sizeof(_Tp)>>);
          const _MaskInteger __k = __x._M_to_unsanitized_bits();
          constexpr bool __bwvl = _Flags._M_have_avx512bw() and _Flags._M_have_avx512vl();
          constexpr bool __dqvl = _Flags._M_have_avx512dq() and _Flags._M_have_avx512vl();

          if (__builtin_is_constant_evaluated() or __builtin_constant_p(__k))
            {
              const _TV __r = _Base::template _S_convert_mask<_TV>(__x);
              if (__builtin_is_constant_evaluated() or __builtin_constant_p(__r))
                return __r;
            }

          if constexpr (__vec_builtin_sizeof<_TV, 1, 64> and _Flags._M_have_avx512bw())
            return __builtin_ia32_cvtmask2b512(__k);

          else if constexpr (__vec_builtin_sizeof<_TV, 1, 32> and __bwvl)
            return __builtin_ia32_cvtmask2b256(__k);

          else if constexpr (__vec_builtin_sizeof<_TV, 1> and __bwvl)
            return __vec_bitcast_trunc<_TV>(__builtin_ia32_cvtmask2b128(__k));

          else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512bw())
            return __builtin_ia32_cvtmask2w512(__k);

          else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and __bwvl)
            return __builtin_ia32_cvtmask2w256(__k);

          else if constexpr (__vec_builtin_sizeof<_TV, 2> and __bwvl)
            return __vec_bitcast_trunc<_TV>(__builtin_ia32_cvtmask2w128(__k));

          else if constexpr (__vec_builtin_sizeof<_TV, 4, 64> and _Flags._M_have_avx512dq())
            return __builtin_ia32_cvtmask2d512(__k);

          else if constexpr (__vec_builtin_sizeof<_TV, 4, 32> and __dqvl)
            return __builtin_ia32_cvtmask2d256(__k);

          else if constexpr (__vec_builtin_sizeof<_TV, 4> and __dqvl)
            return __vec_bitcast_trunc<_TV>(__builtin_ia32_cvtmask2d128(__k));

          else if constexpr (__vec_builtin_sizeof<_TV, 8, 64> and _Flags._M_have_avx512dq())
            return __builtin_ia32_cvtmask2q512(__k);

          else if constexpr (__vec_builtin_sizeof<_TV, 8, 32> and __dqvl)
            return __builtin_ia32_cvtmask2q256(__k);

          else if constexpr (__vec_builtin_sizeof<_TV, 8> and __dqvl)
            return __vec_bitcast_trunc<_TV>(__builtin_ia32_cvtmask2q128(__k));

          else
            return _Base::template _S_convert_mask<_TV>(__x);
        }

      using _Base::_S_convert_mask;

#ifndef __clang__
      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_multiplies(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          if (__builtin_is_constant_evaluated() or __builtin_constant_p(__x)
                or __builtin_constant_p(__y))
            return __x * __y;
          else if constexpr (sizeof(_Tp) == 1)
            {
              if constexpr (sizeof(_TV) == 2)
                {
                  const auto __xs = reinterpret_cast<short>(__x);
                  const auto __ys = reinterpret_cast<short>(__y);
                  return reinterpret_cast<__vec_builtin_type<_Tp, 2>>(
                           short(((__xs * __ys) & 0xff) | ((__xs >> 8) * (__ys & 0xff00))));
                }
              else if constexpr (sizeof(_TV) == 4 and _S_size == 3)
                {
                  const auto __xi = reinterpret_cast<int>(__x);
                  const auto __yi = reinterpret_cast<int>(__y);
                  return reinterpret_cast<__vec_builtin_type<_Tp, 4>>(
                           ((__xi * __yi) & 0xff)
                             | (((__xi >> 8) * (__yi & 0xff00)) & 0xff00)
                             | ((__xi >> 16) * (__yi & 0xff0000)));
                }
              else if constexpr (sizeof(_TV) == 4)
                {
                  const auto __xi = reinterpret_cast<int>(__x);
                  const auto __yi = reinterpret_cast<int>(__y);
                  return reinterpret_cast<__vec_builtin_type<_Tp, 4>>(
                           ((__xi * __yi) & 0xff)
                             | (((__xi >> 8) * (__yi & 0xff00)) & 0xff00)
                             | (((__xi >> 16) * (__yi & 0xff0000)) & 0xff0000)
                             | ((__xi >> 24) * (__yi & 0xff000000u)));
                }
              else if constexpr (sizeof(_TV) == 8 and _Flags._M_have_sse4_1() and is_signed_v<_Tp>)
                {
                  auto __x16 = __builtin_ia32_pmovsxbw128(reinterpret_cast<__v16char>(
                                                            __vec_zero_pad_to_16(__x)));
                  auto __y16 = __builtin_ia32_pmovsxbw128(reinterpret_cast<__v16char>(
                                                            __vec_zero_pad_to_16(__y)));
                  static_assert(same_as<decltype(__x16), __vec_builtin_type<short, 8>>);
                  return __vec_convert<_TV>(__x16 * __y16);
                }
              else if constexpr (sizeof(_TV) == 8 and _Flags._M_have_sse4_1() and is_unsigned_v<_Tp>)
                {
                  auto __x16 = __builtin_ia32_pmovzxbw128(reinterpret_cast<__v16char>(
                                                            __vec_zero_pad_to_16(__x)));
                  auto __y16 = __builtin_ia32_pmovzxbw128(reinterpret_cast<__v16char>(
                                                            __vec_zero_pad_to_16(__y)));
                  static_assert(same_as<decltype(__x16), __vec_builtin_type<short, 8>>);
                  return __vec_convert<_TV>(__x16 * __y16);
                }
              else
                {
                  // codegen of `x*y` is suboptimal (as of GCC 13.1)
                  constexpr int _Np = _S_full_size / 2;
                  using _ShortW = __vec_builtin_type<short, _Np>;
                  const _ShortW __even = __vec_bitcast<short, _Np>(__x)
                                           * __vec_bitcast<short, _Np>(__y);
                  constexpr _ShortW __high_byte = _ShortW() - 256;
                  const _ShortW __odd
                    = (__vec_bitcast<short, _Np>(__x) >> 8)
                        * (__vec_bitcast<short, _Np>(__y) & __high_byte);
                  if constexpr (_Flags._M_have_avx512bw() and sizeof(_TV) > 2)
                    return _S_select_bitmask(0xaaaa'aaaa'aaaa'aaaaLL,
                                             __vec_bitcast<_Tp>(__odd), __vec_bitcast<_Tp>(__even));
                  else if constexpr (_Flags._M_have_sse4_1() and sizeof(_TV) > 2)
                    return reinterpret_cast<_TV>(__high_byte) ? reinterpret_cast<_TV>(__odd)
                                                              : reinterpret_cast<_TV>(__even);
                  else
                    return reinterpret_cast<_TV>(__vec_or(__vec_andnot(__high_byte, __even), __odd));
                }
            }
          else
            return _Base::_S_multiplies(__x, __y);
        }
#endif

      // integer division not optimized (PR90993)
#ifndef __clang__
      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static _TV
        _S_fp_div(_TV __x, _TV __y)
        {
#if __RECIPROCAL_MATH__
          // If -freciprocal-math is active, using the `/` operator is
          // incorrect because it may be translated to an imprecise
          // multiplication with reciprocal. We need to use inline
          // assembly to force a real division.
          using _Tp = __value_type_of<_TV>;
          static_assert(is_floating_point_v<_Tp>);
          _TV __r;
          if constexpr (_Flags._M_have_avx()) // -mno-sse2avx is irrelevant because once -mavx is given, GCC
            { // emits VEX encoded vdivp[sd]
              if constexpr (sizeof(_Tp) == 8)
                asm("vdivpd\t{%2, %1, %0|%0, %1, %2}" : "=x"(__r) : "x"(__x), "x"(__y));
              else
                asm("vdivps\t{%2, %1, %0|%0, %1, %2}" : "=x"(__r) : "x"(__x), "x"(__y));
            }
              else
                {
                  __r = __x;
                  if constexpr (sizeof(_Tp) == 8)
                    asm("divpd\t{%1, %0|%0, %1}" : "=x"(__r) : "x"(__y));
                  else
                    asm("divps\t{%1, %0|%0, %1}" : "=x"(__r) : "x"(__y));
                }
                  return __r;
#else
          return __x / __y;
#endif
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_divides(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          if (not __builtin_is_constant_evaluated() and not __builtin_constant_p(__y))
            {
              if constexpr (is_integral_v<_Tp> and sizeof(_Tp) <= 4)
                { // use divps - codegen of `x/y` is suboptimal (as of GCC 9.0.1)
                  // Note that using floating-point division is likely to raise the
                  // *Inexact* exception flag and thus appears like an invalid
                  // "as-if" transformation. However, C++ doesn't specify how the
                  // fpenv can be observed and points to C. C says that function
                  // calls are assumed to potentially raise fp exceptions, unless
                  // documented otherwise. Consequently, operator/, which is a
                  // function call, may raise fp exceptions.
                  //const struct _CsrGuard
                  //{
                  //  const unsigned _M_data = _mm_getcsr();
                  //  _CsrGuard()
                  //  { _mm_setcsr(0x9f80); } // turn off FP exceptions and flush-to-zero
                  //  ~_CsrGuard() { _mm_setcsr(_M_data); }
                  //} __csr;

                  using _Float = conditional_t<sizeof(_Tp) == 4, double, float>;
                  constexpr int __n_intermediate
                    = std::min(_S_full_size, (_Flags._M_have_avx512f()
                                                ? 64 : _Flags._M_have_avx() ? 32 : 16)
                                 / int(sizeof(_Float)));
                  using _FloatV = __vec_builtin_type<_Float, __n_intermediate>;
                  constexpr int __n_floatv = __div_roundup(_S_size, __n_intermediate);
                  const auto __xf = __vec_convert_all<_FloatV, __n_floatv>(__x);
                  const auto __yf = __vec_convert_all<_FloatV, __n_floatv>(
                                      _Abi::__make_padding_nonzero(__y));
                  return _GLIBCXX_SIMD_INT_PACK(__n_floatv, _Is, {
                           return __vec_convert<_TV>(_S_fp_div(__xf[_Is], __yf[_Is])...);
                         });
                }
              /* 64-bit int division is potentially optimizable via double division if
               * the value in __x is small enough and the conversion between
               * int<->double is efficient enough:
              else if constexpr (is_integral_v<_Tp> and is_unsigned_v<_Tp> and sizeof(_Tp) == 8)
                {
                  if constexpr (_Flags._M_have_sse4_1() and sizeof(__x) == 16)
                    {
                      if (_mm_test_all_zeros(__x, __m128i{0xffe0'0000'0000'0000ull,
                                                          0xffe0'0000'0000'0000ull}))
                        {
                          __x | 0x __vector_convert<__m128d>(__x)
                      }
                    }
                }
               */
            }
          return _Base::_S_divides(__x, __y);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_modulus(_TV __x, _TV __y)
        {
          if (__builtin_is_constant_evaluated() or __builtin_constant_p(__y)
                or sizeof(__value_type_of<_TV>) >= 8)
            return _Base::_S_modulus(__x, __y);
          else
            return _Base::_S_minus(__x, _S_multiplies(__y, _S_divides(__x, __y)));
        }

#else

      using _Base::_S_divides;
      using _Base::_S_modulus;

#endif

      template <std::unsigned_integral _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_and(_Tp __x, _Tp __y)
        { return __x & __y; }

      using _Base::_S_bit_and;

      template <std::unsigned_integral _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_or(_Tp __x, _Tp __y)
        { return __x | __y; }

      using _Base::_S_bit_or;

      template <std::unsigned_integral _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_xor(_Tp __x, _Tp __y)
        { return __x ^ __y; }

      using _Base::_S_bit_xor;

      // Notes on UB. C++2a [expr.shift] says:
      // -1- [...] The operands shall be of integral or unscoped enumeration type
      //     and integral promotions are performed. The type of the result is that
      //     of the promoted left operand. The behavior is undefined if the right
      //     operand is negative, or greater than or equal to the width of the
      //     promoted left operand.
      // -2- The value of E1 << E2 is the unique value congruent to E1×2^E2 modulo
      //     2^N, where N is the width of the type of the result.
      //
      // C++17 [expr.shift] says:
      // -2- The value of E1 << E2 is E1 left-shifted E2 bit positions; vacated
      //     bits are zero-filled. If E1 has an unsigned type, the value of the
      //     result is E1 × 2^E2 , reduced modulo one more than the maximum value
      //     representable in the result type. Otherwise, if E1 has a signed type
      //     and non-negative value, and E1 × 2^E2 is representable in the
      //     corresponding unsigned type of the result type, then that value,
      //     converted to the result type, is the resulting value; otherwise, the
      //     behavior is undefined.
      //
      // Consequences:
      // With C++2a signed and unsigned types have the same UB
      // characteristics:
      // - left shift is not UB for 0 <= RHS < max(32, #bits(T))
      //
      // Even, with C++17 there's little room for optimizations because the standard requires all
      // shifts to happen on promoted integrals (i.e. int). Thus, short and char shifts must assume
      // shifts affect bits of neighboring values.
#if not defined _GLIBCXX_SIMD_NO_SHIFT_OPT and not defined __clang__
      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_bit_shift_left(_TV __x, int __y)
        {
          using _Tp = __value_type_of<_TV>;
          const auto __ix = __to_x86_intrin(__x);
          if (__y < 0 or (sizeof(_Tp) <= 4 and __y >= 32) or (sizeof(_Tp) == 8 and __y >= 64))
            __invoke_ub("Undefined behavior: shift by '%d' is out of bounds", __y);

          else if (__builtin_is_constant_evaluated())
            return __x << __y;

          // after C++17, signed shifts have no additional UB, and behave just like unsigned shifts
          else if constexpr (sizeof(_Tp) == 1)
            {
              // (cf. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83894)
              if (__builtin_constant_p(__y))
                {
                  if (__y == 0)
                    return __x;
                  else if (__y == 1)
                    return __x + __x;
                  else if (__y == 2)
                    {
                      __x = __x + __x;
                      return __x + __x;
                    }
                  else if (__y > 2 and __y < 8)
                    {
                      if constexpr (sizeof(__x) > sizeof(unsigned))
                        {
                          const unsigned char __mask = 0xff << __y;
                          return reinterpret_cast<_TV>(
                                   __vec_bitcast<unsigned char>(
                                     __vec_bitcast<unsigned>(__x) << __y) & __mask);
                        }
                      else
                        {
                          const unsigned __mask = (0xff & (0xff << __y)) * 0x01010101u;
                          using _Ip = __make_signed_int_t<_TV>;
                          return reinterpret_cast<_TV>(
                                   static_cast<_Ip>(
                                     unsigned(reinterpret_cast<_Ip>(__x) << __y) & __mask));
                        }
                    }
                  else if (__y >= 8 and __y < 32)
                    return _TV();
                }

              // general strategy in the following: use an sllv instead of sll
              // instruction, because it's 2 to 4 times faster:
              else if constexpr (_Flags._M_have_avx512bw() and _Flags._M_have_avx512vl()
                                   and sizeof(__x) == 16)
                return reinterpret_cast<_TV>(
                         __builtin_ia32_pmovwb256_mask(
                           __builtin_ia32_psllv16hi_mask(
                             __builtin_ia32_pmovsxbw256(reinterpret_cast<__v16char>(__x)),
                             __vec_broadcast<16, short>(__y), __v16int16(), -1),
                           __v16char(), -1));

              else if constexpr (_Flags._M_have_avx512bw() and sizeof(__x) == 32)
                {
                  const auto __y32 = __vec_broadcast<32, short>(__y);
                  auto __x0 = __builtin_ia32_pmovsxbw512_mask(
                                __vec_bitcast<char>(__x), __v32int16{}, -1);
                  __x0 = __builtin_ia32_psllv32hi_mask (__x0, __y32, __v32int16{}, -1);
                  return reinterpret_cast<_TV>(__builtin_ia32_pmovwb512_mask(__x0, __v32char{}, -1));
                }

              else if constexpr (_Flags._M_have_avx512bw() and sizeof(__x) == 64)
                {
                  const auto __y16 = __vec_broadcast<32, short>(__y);
                  auto __x0 = __builtin_ia32_pmovsxbw512_mask(
                                __vec_bitcast<char>(__vec_lo256(__x)), __v32int16{}, -1);
                  auto __x1 = __builtin_ia32_pmovsxbw512_mask(
                                __vec_bitcast<char>(__vec_hi256(__x)), __v32int16{}, -1);
                  __x0 = __builtin_ia32_psllv32hi_mask(__x0, __y16, __v32int16{}, -1);
                  __x1 = __builtin_ia32_psllv32hi_mask(__x1, __y16, __v32int16{}, -1);
                  return reinterpret_cast<_TV>(
                           __vec_concat(__builtin_ia32_pmovwb512_mask(__x0, __v32char{}, -1),
                                        __builtin_ia32_pmovwb512_mask(__x1, __v32char{}, -1)));
                }

              else if constexpr (_Flags._M_have_avx2() and sizeof(__x) == 32)
                {
                  if (__y >= 8)
                    return _TV();
#if __x86_64__
                  const auto __mask = __vec_bitcast<short>(
                                        __vec_broadcast<4>(
                                          (0x00ff00ff00ff00ffULL << __y) ^ 0xff00ff00ff00ff00ULL));
                  return reinterpret_cast<_TV>((__vec_bitcast<short>(__ix) << __y) & __mask);
#else
                  const auto __mask = __vec_bitcast<short>(
                                        __vec_broadcast<8>((0x00ff00ffULL << __y) ^ 0xff00ff00ULL));
                  return reinterpret_cast<_TV>((__vec_bitcast<short>(__ix) << __y) & __mask);
#endif
                }
              else if constexpr (sizeof(__x) == 16)
                {
                  if (__y >= 8)
                    return _TV();
#if __x86_64__
#else
                  static_assert(sizeof(__ix) == 16);
                  const unsigned __ffy = (0x00ff00ffu << __y) ^ 0xff00ff00u;
                  const auto __mask = __vec_bitcast<short>(__vec_broadcast<4>(__ffy));
                  return __vec_bitcast_trunc<_TV>((__vec_bitcast<short>(__ix) << __y) & __mask);
#endif
                }
              else if constexpr (sizeof(__x) == 8)
                {
                  if (__y >= 8)
                    return _TV();
                  const auto __mask = (0x00ff00ff00ff00ffULL << __y) ^ 0xff00ff00ff00ff00ULL;
                  return __builtin_bit_cast(
                           _TV, (__builtin_bit_cast(unsigned long long, __x) << __y) & __mask);
                }
              else if constexpr (sizeof(__x) == 4)
                {
                  if (__y >= 8)
                    return _TV();
                  const auto __mask = (0x00ff00ffU << __y) ^ 0xff00ff00U;
                  return __builtin_bit_cast(_TV, (__builtin_bit_cast(unsigned, __x) << __y) & __mask);
                }
              else if constexpr (sizeof(__x) == 2)
                {
                  if (__y >= 8)
                    return _TV();
                  const int __mask = (0x00ff << __y) ^ 0xff00;
                  return __builtin_bit_cast(
                           _TV, short((__builtin_bit_cast(short, __x) << __y) & __mask));
                }
            }
          return __x << __y;
        }

      template <__vec_builtin _TV>
        static constexpr _TV
        _S_bit_shift_left(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          constexpr int _Np = _S_size;
          const auto __ix = __to_x86_intrin(__x);
          const auto __iy = __to_x86_intrin(__y);
          if (_GLIBCXX_SIMD_INT_PACK(_Np, _Is, {
                return ((__y[_Is] < 0 or (sizeof(_Tp) <= 4 and __y[_Is] >= 32)
                           or (sizeof(_Tp) == 8 and __y[_Is] >= 64)) or ...); }))
            {
              static_assert(_Np >= 2);
              if constexpr (_Np == 2)
                __invoke_ub("Undefined behavior: left shift by [%d, %d] is out of bounds",
                            int(__y[0]), int(__y[1]));
              else if constexpr (_Np == 3)
                __invoke_ub("Undefined behavior: left shift by [%d, %d, %d] is out of bounds",
                            int(__y[0]), int(__y[1]), int(__y[2]));
              else if constexpr (_Np == 4)
                __invoke_ub("Undefined behavior: left shift by [%d, %d, %d, %d] is out of bounds",
                            int(__y[0]), int(__y[1]), int(__y[2]), int(__y[3]));
              else
                __invoke_ub("Undefined behavior: left shift by [%d, %d, %d, %d, ...] is out of "
                            "bounds", int(__y[0]), int(__y[1]), int(__y[2]), int(__y[3]));
            }

          else if (__builtin_is_constant_evaluated()
                     or (__builtin_constant_p(__x) and __builtin_constant_p(__y)))
            return __x << __y;

          else if (__builtin_constant_p(__y) and _GLIBCXX_SIMD_INT_PACK(_Np - 1, _Is, {
                                                   return ((__y[_Is + 1] == __y[0]) and ...);
                                                 }))
            return _S_bit_shift_left(__x, int(__y[0]));

          // signed shifts have no UB, and behave just like unsigned shifts
          /*        else if constexpr (is_signed_v<_Tp>)
                    return __vec_bitcast<_Tp>(
          _S_bit_shift_left(__vec_bitcast<make_unsigned_t<_Tp>>(__x),
          __vec_bitcast<make_unsigned_t<_Tp>>(__y)));*/

          else if constexpr (sizeof(_Tp) == 1)
            {
              if constexpr (sizeof __ix == 64 and _Flags._M_have_avx512bw())
                return __vec_bitcast<_Tp>(__vec_concat(
                                            _mm512_cvtepi16_epi8(
                                              _mm512_sllv_epi16(_mm512_cvtepu8_epi16(__vec_lo256(__ix)),
                                                                _mm512_cvtepu8_epi16(__vec_lo256(__iy)))),
                                            _mm512_cvtepi16_epi8(
                                              _mm512_sllv_epi16(_mm512_cvtepu8_epi16(__vec_hi256(__ix)),
                                                                _mm512_cvtepu8_epi16(__vec_hi256(__iy))))));
              else if constexpr (sizeof __ix == 32 and _Flags._M_have_avx512bw())
                return __vec_bitcast<_Tp>(_mm512_cvtepi16_epi8(
                                            _mm512_sllv_epi16(_mm512_cvtepu8_epi16(__ix),
                                                              _mm512_cvtepu8_epi16(__iy))));
              else if constexpr (sizeof __x <= 8 and _Flags._M_have_avx512bw()
                                   and _Flags._M_have_avx512vl())
                return __vec_bitcast_trunc<_TV>(
                         _mm_cvtepi16_epi8(_mm_sllv_epi16(_mm_cvtepu8_epi16(__ix),
                                                          _mm_cvtepu8_epi16(__iy))));
              else if constexpr (sizeof __ix == 16 and _Flags._M_have_avx512bw()
                                   and _Flags._M_have_avx512vl())
                return reinterpret_cast<_TV>(_mm256_cvtepi16_epi8(
                                               _mm256_sllv_epi16(_mm256_cvtepu8_epi16(__ix),
                                                                 _mm256_cvtepu8_epi16(__iy))));
              else if constexpr (sizeof __ix == 16 and _Flags._M_have_avx512bw())
                return reinterpret_cast<_TV>(
                         __vec_lo128(_mm512_cvtepi16_epi8(
                                       _mm512_sllv_epi16(
                                         _mm512_cvtepu8_epi16(_mm256_castsi128_si256(__ix)),
                                         _mm512_cvtepu8_epi16(_mm256_castsi128_si256(__iy))))));
              else if constexpr (_Flags._M_have_sse4_1() and sizeof(__x) == 16)
                {
                  using _MV = __mask_vec_from<_TV>;
                  auto __mask = reinterpret_cast<_TV>(__vec_bitcast<short>(__y) << 5);
                  auto __x4 = reinterpret_cast<_TV>(__vec_bitcast<short>(__x) << 4);
                  __x4 &= char(0xf0);
                  __x = _S_select_on_msb(reinterpret_cast<_MV>(__mask), __x4, __x);
                  __mask += __mask;
                  auto __x2 = reinterpret_cast<_TV>(__vec_bitcast<short>(__x) << 2);
                  __x2 &= char(0xfc);
                  __x = _S_select_on_msb(reinterpret_cast<_MV>(__mask), __x2, __x);
                  __mask += __mask;
                  auto __x1 = __x + __x;
                  __x = _S_select_on_msb(reinterpret_cast<_MV>(__mask), __x1, __x);
                  return __x & ((__y & char(0xf8)) == 0); // y > 7 nulls the result
                }
              else if constexpr (sizeof(__x) == 16)
                {
                  auto __mask = __vec_bitcast<unsigned char>(__vec_bitcast<short>(__y) << 5);
                  auto __x4 = __vec_bitcast<_Tp>(__vec_bitcast<short>(__x) << 4);
                  __x4 &= char(0xf0);
                  __x = __vec_bitcast<signed char>(__mask) < 0 ? __x4 : __x;
                  __mask += __mask;
                  auto __x2 = __vec_bitcast<_Tp>(__vec_bitcast<short>(__x) << 2);
                  __x2 &= char(0xfc);
                  __x = __vec_bitcast<signed char>(__mask) < 0 ? __x2 : __x;
                  __mask += __mask;
                  auto __x1 = __x + __x;
                  __x = __vec_bitcast<signed char>(__mask) < 0 ? __x1 : __x;
                  return __x & ((__y & char(0xf8)) == 0); // y > 7 nulls the result
                }
              else
                return __x << __y;
            }

          else if constexpr (sizeof(_Tp) == 2)
            {
              if constexpr (sizeof __ix == 64 and _Flags._M_have_avx512bw())
                return __vec_bitcast<_Tp>(_mm512_sllv_epi16(__ix, __iy));
              else if constexpr (sizeof __ix == 32 and _Flags._M_have_avx512bw()
                                   and _Flags._M_have_avx512vl())
                return __vec_bitcast<_Tp>(_mm256_sllv_epi16(__ix, __iy));
              else if constexpr (sizeof __ix == 32 and _Flags._M_have_avx512bw())
                return __vec_bitcast<_Tp>(
                         __vec_lo256(_mm512_sllv_epi16(_mm512_castsi256_si512(__ix),
                                                       _mm512_castsi256_si512(__iy))));
              else if constexpr (sizeof __ix == 32 and _Flags._M_have_avx2())
                {
                  const auto __ux = __vec_bitcast<unsigned>(__x);
                  const auto __uy = __vec_bitcast<unsigned>(__y);
                  return __vec_bitcast<_Tp>(
                           _mm256_blend_epi16(
                             __to_x86_intrin(__ux << (__uy & 0x0000ffffu)),
                             __to_x86_intrin((__ux & 0xffff0000u) << (__uy >> 16)), 0xaa));
                }
              else if constexpr (sizeof __ix <= 16 and _Flags._M_have_avx512bw()
                                   and _Flags._M_have_avx512vl())
                return __vec_bitcast_trunc<_TV>(_mm_sllv_epi16(__ix, __iy));
              else if constexpr (sizeof __ix <= 16 and _Flags._M_have_avx512bw())
                return __vec_bitcast_trunc<_TV>(
                         __vec_lo128(_mm512_sllv_epi16(_mm512_castsi128_si512(__ix),
                                                       _mm512_castsi128_si512(__iy))));
              else if constexpr (sizeof __ix <= 16 and _Flags._M_have_avx2())
                {
                  const auto __ux = __vec_bitcast<unsigned>(__ix);
                  const auto __uy = __vec_bitcast<unsigned>(__iy);
                  return __vec_bitcast_trunc<_TV>(
                           _mm_blend_epi16(
                             __to_x86_intrin(__ux << (__uy & 0x0000ffffu)),
                             __to_x86_intrin((__ux & 0xffff0000u) << (__uy >> 16)), 0xaa));
                }
              else if constexpr (sizeof __ix <= 16)
                {
#ifdef SHORT_SHIFT0
                  _MaskMember<_TV> __k = reinterpret_cast<_MaskMember<_TV>>(__y);
                  __x = _S_select_on_msb(__k << 15, __x + __x, __x);
                  __x = _S_select_on_msb(__k << 14, __x << 2, __x);
                  __x = _S_select_on_msb(__k << 13, __x << 4, __x);
                  __x = _S_select_on_msb(__k << 12, __x << 8, __x);
                  __x = __y > 15 ? 0 : __x;
                  return __x;
#else
                  using _Float4 = __vec_builtin_type<float, 4>;
                  using _Int4 = __vec_builtin_type<int, 4>;
                  using _UInt4 = __vec_builtin_type<unsigned, 4>;
                  const _UInt4 __yu = reinterpret_cast<_UInt4>(__to_x86_intrin(__y + (0x3f8 >> 3)));
                  // The final operator| before multiplication with x fails to mask off bits [31:16]
                  // of the first operand. We could do that with an additional `& 0xffff`. But that
                  // would increase latency, whereas zeroing __x can interleave with the computation
                  // of the factor.
                  __x = __vec_and(reinterpret_cast<_TV>(__y < 16), __x);
                  return __x * __vec_bitcast_trunc<_TV>(
                                 __vec_convert<_Int4>(reinterpret_cast<_Float4>(__yu << 23))
                                   | (__vec_convert<_Int4>(reinterpret_cast<_Float4>(
                                                             (__yu >> 16) << 23)) << 16));
#endif
                }
              else
                static_assert(false);
            }
          else if constexpr (sizeof(_Tp) == 4 and sizeof __ix == 16 and not _Flags._M_have_avx2())
            {
              if (__builtin_constant_p(__y))
                {
                  // If __y is constant propagated and one of the values is 31, then _mm_cvttps_epi32
                  // below produces 0x7fff'ffff instead of 0x8000'0000. Therefore, create a constprop
                  // vector of all the shifts applied to 1, avoiding the float conversion trick.
                  const _TV __factor = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                                         return _TV {_Tp(1u << __y[_Is])...};
                                       });
                  if (__builtin_constant_p(__factor))
                    return __x * __factor;
                }
              // Shift by 31 using the float exponent is still problematic: The conversion of 2^31 to
              // epi32 via cvttps2dq raises FE_INVALID.
#if __NO_TRAPPING_MATH__
              const auto __shifted1 = __vec_bitcast<float>(
                                        (__vec_bitcast<unsigned>(__iy) << 23) + 0x3f80'0000);
              return __vec_bitcast_trunc<_TV>(
                       __vec_bitcast<unsigned>(__ix)
                         * __vec_bitcast<unsigned>(_mm_cvttps_epi32(__shifted1)));
#else
              return __x << __y;
#endif
            }
          else if constexpr (sizeof(_Tp) == 8 and sizeof __ix == 16 and not _Flags._M_have_avx2())
            {
              const auto __lo = _mm_sll_epi64(__ix, __iy);
              const auto __hi = _mm_sll_epi64(__ix, _mm_unpackhi_epi64(__iy, __iy));
              if constexpr (_Flags._M_have_sse4_1())
                return __vec_bitcast<_Tp>(_mm_blend_epi16(__lo, __hi, 0xf0));
              else
                return __vec_bitcast<_Tp>(_mm_move_sd(__vec_bitcast<double>(__hi),
                                                      __vec_bitcast<double>(__lo)));
            }
          else
            return __x << __y;
        }

      template <__vec_builtin _TV>
        constexpr inline _GLIBCXX_CONST static _TV
        _S_bit_shift_right(_TV __x, int __y)
        {
          using _Tp = __value_type_of<_TV>;
          [[maybe_unused]] const auto __ix = __to_x86_intrin(__x);
          if (__y < 0 or (sizeof(_Tp) <= 4 and __y >= 32) or (sizeof(_Tp) == 8 and __y >= 64))
            __invoke_ub("Undefined behavior: shift by '%d' is out of bounds", __y);

          else if (__builtin_is_constant_evaluated())
            return __x >> __y;

          else if (__builtin_constant_p(__y) and is_unsigned_v<_Tp>
                     and __y >= int(sizeof(_Tp) * __CHAR_BIT__))
            return _TV();

          else if constexpr (sizeof(_Tp) == 1 and is_unsigned_v<_Tp>)
            return __vec_bitcast_trunc<_TV>(
                     __vec_bitcast<unsigned short>(__ix) >> __y) & _Tp(0xff >> __y);

          else if constexpr (sizeof(_Tp) == 1 and is_signed_v<_Tp>)
            return __vec_bitcast_trunc<_TV>(
                     (__vec_bitcast<unsigned short>(__vec_bitcast<short>(__ix) >> (__y + 8)) << 8)
                       | (__vec_bitcast<unsigned short>(
                            __vec_bitcast<short>(__vec_bitcast<unsigned short>(__ix) << 8) >> __y)
                     >> 8));
          // GCC optimizes sizeof == 2, 4, and unsigned 8 as expected
          else if constexpr (sizeof(_Tp) == 8 and is_signed_v<_Tp>)
            {
              if (__y > 32)
                return (reinterpret_cast<_TV>(__vec_bitcast<int>(__ix) >> 32)
                          & _Tp(0xffff'ffff'0000'0000ull))
                         | __vec_bitcast<_Tp>(
                             __vec_bitcast<int>(__vec_bitcast<unsigned long long>(__ix) >> 32)
                         >> (__y - 32));
              else
                return reinterpret_cast<_TV>(__vec_bitcast<unsigned long long>(__ix) >> __y)
                         | __vec_bitcast<_Tp>(__vec_bitcast<int>(__ix & -0x8000'0000'0000'0000ll)
                         >> __y);
            }
          else
            return __x >> __y;
        }

      template <__vec_builtin _TV>
        constexpr inline _GLIBCXX_CONST static _TV
        _S_bit_shift_right(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          constexpr int _Np = _S_size;
          [[maybe_unused]] const auto __ix = __to_x86_intrin(__x);
          [[maybe_unused]] const auto __iy = __to_x86_intrin(__y);

          if (_GLIBCXX_SIMD_INT_PACK(_Np, _Is, {
                return ((__y[_Is] < 0 or (sizeof(_Tp) <= 4 and __y[_Is] >= 32)
                           or (sizeof(_Tp) == 8 and __y[_Is] >= 64)) or ...); }))
            __invoke_ub("Undefined behavior: shift is out of bounds");

          else if (__builtin_is_constant_evaluated()
                     or (__builtin_constant_p(__x) and __builtin_constant_p(__y)
                           and _Base::_S_is_constprop_all_of(__y < __CHAR_BIT__ * sizeof(_Tp))))
            return __x >> __y;

          else if (__builtin_constant_p(__y) and _GLIBCXX_SIMD_INT_PACK(_Np - 1, _Is, {
                                                   return ((__y[_Is + 1] == __y[0]) and ...);
                                                 }))
            return _S_bit_shift_right(__x, int(__y[0]));

          else if constexpr (sizeof(_Tp) == 1)
            {
              if constexpr (sizeof(__x) <= 8 and _Flags._M_have_avx512bw()
                              and _Flags._M_have_avx512vl())
                return __vec_bitcast_trunc<_TV>(
                         _mm_cvtepi16_epi8(is_signed_v<_Tp> ? _mm_srav_epi16(_mm_cvtepi8_epi16(__ix),
                                                                             _mm_cvtepi8_epi16(__iy))
                                                            : _mm_srlv_epi16(_mm_cvtepu8_epi16(__ix),
                                                                             _mm_cvtepu8_epi16(__iy))));

              else if constexpr (sizeof(__x) == 16 and _Flags._M_have_avx512bw()
                                   and _Flags._M_have_avx512vl())
                return reinterpret_cast<_TV>(
                         _mm256_cvtepi16_epi8(is_signed_v<_Tp>
                                                ? _mm256_srav_epi16(_mm256_cvtepi8_epi16(__ix),
                                                                    _mm256_cvtepi8_epi16(__iy))
                                                : _mm256_srlv_epi16(_mm256_cvtepu8_epi16(__ix),
                                                                    _mm256_cvtepu8_epi16(__iy))));

              else if constexpr (sizeof(__x) == 32 and _Flags._M_have_avx512bw())
                return reinterpret_cast<_TV>(
                         _mm512_cvtepi16_epi8(is_signed_v<_Tp>
                                                ? _mm512_srav_epi16(_mm512_cvtepi8_epi16(__ix),
                                                                    _mm512_cvtepi8_epi16(__iy))
                                                : _mm512_srlv_epi16(_mm512_cvtepu8_epi16(__ix),
                                                                    _mm512_cvtepu8_epi16(__iy))));

              else if constexpr (sizeof(__x) == 64 and is_signed_v<_Tp>)
                return reinterpret_cast<_TV>(
                         _mm512_mask_mov_epi8(
                           _mm512_srav_epi16(__ix, _mm512_srli_epi16(__iy, 8)),
                           0x5555'5555'5555'5555ull,
                           _mm512_srav_epi16(_mm512_slli_epi16(__ix, 8),
                                             _mm512_maskz_add_epi8(0x5555'5555'5555'5555ull, __iy,
                                                                   _mm512_set1_epi16(8)))));

              else if constexpr (sizeof(__x) == 64 and is_unsigned_v<_Tp>)
                return reinterpret_cast<_TV>(
                         _mm512_mask_mov_epi8(
                           _mm512_srlv_epi16(__ix, _mm512_srli_epi16(__iy, 8)),
                           0x5555'5555'5555'5555ull,
                           _mm512_srlv_epi16(_mm512_maskz_mov_epi8(0x5555'5555'5555'5555ull, __ix),
                                             _mm512_maskz_mov_epi8(0x5555'5555'5555'5555ull, __iy))));

              /* This has better throughput but higher latency than the impl below
                 else if constexpr (_Flags._M_have_avx2() and sizeof(__x) == 16 and is_unsigned_v<_Tp>)
              {
              const auto __shorts = __to_x86_intrin(_S_bit_shift_right(
              __vec_bitcast<unsigned short>(_mm256_cvtepu8_epi16(__ix)),
              __vec_bitcast<unsigned short>(_mm256_cvtepu8_epi16(__iy))));
              return __vec_bitcast<_Tp>(
              _mm_packus_epi16(__vec_lo128(__shorts), __vec_hi128(__shorts)));
              }
               */
              else if constexpr (_Flags._M_have_avx2() and sizeof(__x) > 8)
                {
                  // the following uses vpsr[al]vd, which requires AVX2
                  if constexpr (is_signed_v<_Tp>)
                    {
                      const auto __r3
                        = __vec_bitcast<unsigned>(
                            (__vec_bitcast<int>(__x) >> (__vec_bitcast<unsigned>(__y) >> 24))
                          ) & 0xff000000u;
                      const auto __r2
                        = __vec_bitcast<unsigned>(
                            ((__vec_bitcast<int>(__x) << 8) >> ((__vec_bitcast<unsigned>(__y) << 8) >> 24))
                          ) & 0xff000000u;
                      const auto __r1
                        = __vec_bitcast<unsigned>(
                            ((__vec_bitcast<int>(__x) << 16) >> ((__vec_bitcast<unsigned>(__y) << 16) >> 24))
                          ) & 0xff000000u;
                      const auto __r0
                        = __vec_bitcast<unsigned>(
                            (__vec_bitcast<int>(__x) << 24) >> ((__vec_bitcast<unsigned>(__y) << 24) >> 24));
                      return reinterpret_cast<_TV>(__r3 | (__r2 >> 8) | (__r1 >> 16) | (__r0 >> 24));
                    }
                  else
                    {
                      const auto __r3
                        = (__vec_bitcast<unsigned>(__x)
                            >> (__vec_bitcast<unsigned>(__y) >> 24))
                            & 0xff000000u;
                      const auto __r2
                        = ((__vec_bitcast<unsigned>(__x) << 8)
                            >> ((__vec_bitcast<unsigned>(__y) << 8) >> 24))
                            & 0xff000000u;
                      const auto __r1
                        = ((__vec_bitcast<unsigned>(__x) << 16)
                            >> ((__vec_bitcast<unsigned>(__y) << 16) >> 24))
                            & 0xff000000u;
                      const auto __r0
                        = (__vec_bitcast<unsigned>(__x) << 24)
                            >> ((__vec_bitcast<unsigned>(__y) << 24) >> 24);
                      return __vec_bitcast<_Tp>(__r3 | (__r2 >> 8) | (__r1 >> 16) | (__r0 >> 24));
                    }
                }
              else if constexpr (_Flags._M_have_sse4_1() and is_unsigned_v<_Tp> and sizeof(__x) > 2)
                {
                  __v16char __x128 = reinterpret_cast<__v16char>(__vec_zero_pad_to_16(__x));
                  __v16uchar __mask = reinterpret_cast<__v16uchar>(
                                        reinterpret_cast<__v8uint16>(__iy) << 5);
                  constexpr unsigned short __ff0f = 0xff0f;
                  constexpr unsigned short __ff3f = 0xff3f;
                  constexpr unsigned short __ff7f = 0xff7f;
                  auto __x4 = __vec_bitcast<_Tp>(
                                (__vec_bitcast<unsigned short>(__x128) >> 4) & __ff0f);
                  __x128 = __builtin_ia32_pblendvb128(__x128, reinterpret_cast<__v16char>(__x4),
                                                      reinterpret_cast<__v16char>(__mask));
                  __mask += __mask;
                  auto __x2 = __vec_bitcast<_Tp>(
                                (__vec_bitcast<unsigned short>(__x128) >> 2) & __ff3f);
                  __x128 = __builtin_ia32_pblendvb128(__x128, reinterpret_cast<__v16char>(__x2),
                                                      reinterpret_cast<__v16char>(__mask));
                  __mask += __mask;
                  auto __x1 = __vec_bitcast<_Tp>(
                                (__vec_bitcast<unsigned short>(__x128) >> 1) & __ff7f);
                  __x128 = __builtin_ia32_pblendvb128(__x128, reinterpret_cast<__v16char>(__x1),
                                                      reinterpret_cast<__v16char>(__mask));
                  // y > 7 nulls the result
                  return __vec_bitcast_trunc<_TV>(
                           reinterpret_cast<__v16uchar>(__x128)
                             & ((__vec_zero_pad_to_16(__y) & char(0xf8)) == 0));
                }
              else if constexpr (_Flags._M_have_sse4_1() and is_signed_v<_Tp> and sizeof(__x) > 2)
                {
                  using _MV = __v8int16;
                  auto __mask = reinterpret_cast<__v16uchar>(
                                  reinterpret_cast<__v8uint16>(__iy) << 5);
                  auto __xh = reinterpret_cast<__v8int16>(__ix);
                  auto __xl = __xh << 8;
                  auto __xh4 = __xh >> 4;
                  auto __xl4 = __xl >> 4;
                  __xh = reinterpret_cast<__v8int16>(
                           __builtin_ia32_pblendvb128(
                             reinterpret_cast<__v16char>(__xh), reinterpret_cast<__v16char>(__xh4),
                             reinterpret_cast<__v16char>(__mask)));
                  __xl = reinterpret_cast<__v8int16>(
                           __builtin_ia32_pblendvb128(
                             reinterpret_cast<__v16char>(__xl), reinterpret_cast<__v16char>(__xl4),
                             reinterpret_cast<__v16char>(reinterpret_cast<_MV>(__mask) << 8)));
                  __mask += __mask;
                  auto __xh2 = __xh >> 2;
                  auto __xl2 = __xl >> 2;
                  __xh = reinterpret_cast<__v8int16>(
                           __builtin_ia32_pblendvb128(
                             reinterpret_cast<__v16char>(__xh), reinterpret_cast<__v16char>(__xh2),
                             reinterpret_cast<__v16char>(__mask)));
                  __xl = reinterpret_cast<__v8int16>(
                           __builtin_ia32_pblendvb128(
                             reinterpret_cast<__v16char>(__xl), reinterpret_cast<__v16char>(__xl2),
                             reinterpret_cast<__v16char>(reinterpret_cast<_MV>(__mask) << 8)));
                  __mask += __mask;
                  auto __xh1 = __xh >> 1;
                  auto __xl1 = __xl >> 1;
                  __xh = reinterpret_cast<__v8int16>(
                           __builtin_ia32_pblendvb128(
                             reinterpret_cast<__v16char>(__xh), reinterpret_cast<__v16char>(__xh1),
                             reinterpret_cast<__v16char>(__mask)));
                  __xl = reinterpret_cast<__v8int16>(
                           __builtin_ia32_pblendvb128(
                             reinterpret_cast<__v16char>(__xl), reinterpret_cast<__v16char>(__xl1),
                             reinterpret_cast<__v16char>(reinterpret_cast<_MV>(__mask) << 8)));
                  // y > 7 must return either 0 or -1 depending on the sign bit of __x
                  return __vec_bitcast_trunc<_TV>(
                           _VecAbi<16>::_Impl::_S_select(
                             __vec_zero_pad_to_16(__y) > char(7),
                             __vec_zero_pad_to_16(__x) < 0,
                             reinterpret_cast<__v16schar>(__xh & short(0xff00))
                               | reinterpret_cast<__v16schar>(
                                   reinterpret_cast<__v8uint16>(__xl) >> 8)));
                }
              else if constexpr (is_unsigned_v<_Tp> and sizeof(__x) > 2) // SSE2
                {
                  constexpr unsigned short __ff0f = 0xff0f;
                  constexpr unsigned short __ff3f = 0xff3f;
                  constexpr unsigned short __ff7f = 0xff7f;
                  auto __mask = __vec_bitcast<_Tp>(__vec_bitcast<unsigned short>(__y) << 5);
                  auto __x4 = __vec_bitcast<_Tp>((__vec_bitcast<unsigned short>(__x) >> 4) & __ff0f);
                  __x = __mask > 0x7f ? __x4 : __x;
                  __mask += __mask;
                  auto __x2 = __vec_bitcast<_Tp>((__vec_bitcast<unsigned short>(__x) >> 2) & __ff3f);
                  __x = __mask > 0x7f ? __x2 : __x;
                  __mask += __mask;
                  auto __x1 = __vec_bitcast<_Tp>((__vec_bitcast<unsigned short>(__x) >> 1) & __ff7f);
                  __x = __mask > 0x7f ? __x1 : __x;
                  return __x & ((__y & char(0xf8)) == 0); // y > 7 nulls the result
                }
              else if constexpr (sizeof(__x) > 2) // signed SSE2
                {
                  _TV __minus1_where_negative = __x < 0;
                  static_assert(is_signed_v<_Tp>);
                  auto __maskh = __vec_bitcast<unsigned short>(__y) << 5;
                  auto __maskl = __vec_bitcast<unsigned short>(__y) << (5 + 8);
                  auto __xh = __vec_bitcast<short>(__x);
                  auto __xl = __vec_bitcast<short>(__x) << 8;
                  auto __xh4 = __xh >> 4;
                  auto __xl4 = __xl >> 4;
                  __xh = __maskh > 0x7fff ? __xh4 : __xh;
                  __xl = __maskl > 0x7fff ? __xl4 : __xl;
                  __maskh += __maskh;
                  __maskl += __maskl;
                  auto __xh2 = __xh >> 2;
                  auto __xl2 = __xl >> 2;
                  __xh = __maskh > 0x7fff ? __xh2 : __xh;
                  __xl = __maskl > 0x7fff ? __xl2 : __xl;
                  __maskh += __maskh;
                  __maskl += __maskl;
                  auto __xh1 = __xh >> 1;
                  auto __xl1 = __xl >> 1;
                  __xh = __maskh > 0x7fff ? __xh1 : __xh;
                  __xl = __maskl > 0x7fff ? __xl1 : __xl;
                  __x = __vec_bitcast<_Tp>((__xh & short(0xff00)))
                          | __vec_bitcast<_Tp>(__vec_bitcast<unsigned short>(__xl) >> 8);
                  return _S_select(__y > 7, __minus1_where_negative, __x);
                }
              else
                return __x >> __y;
            }
          else if constexpr (sizeof(_Tp) == 2 and sizeof(__x) >= 4)
            {
              [[maybe_unused]] auto __blend_0xaa
                = [] [[__gnu__::__always_inline__]] (auto __a, auto __b) {
                if constexpr (sizeof(__a) == 16)
                  return _mm_blend_epi16(__to_x86_intrin(__a), __to_x86_intrin(__b),
                                         0xaa);
                else if constexpr (sizeof(__a) == 32)
                  return _mm256_blend_epi16(__to_x86_intrin(__a), __to_x86_intrin(__b),
                                            0xaa);
                else if constexpr (sizeof(__a) == 64)
                  return _mm512_mask_blend_epi16(0xaaaa'aaaaU, __to_x86_intrin(__a),
                                                 __to_x86_intrin(__b));
                else
                  static_assert(false);
              };
              if constexpr (_Flags._M_have_avx512bw() and _Flags._M_have_avx512vl()
                              and sizeof(_TV) <= 16)
                return __vec_bitcast_trunc<_TV>(is_signed_v<_Tp>
                                                  ? _mm_srav_epi16(__ix, __iy)
                                                  : _mm_srlv_epi16(__ix, __iy));
              else if constexpr (_Flags._M_have_avx512bw() and _Flags._M_have_avx512vl()
                                   and sizeof(_TV) == 32)
                return __vec_bitcast<_Tp>(is_signed_v<_Tp>
                                            ? _mm256_srav_epi16(__ix, __iy)
                                            : _mm256_srlv_epi16(__ix, __iy));
              else if constexpr (_Flags._M_have_avx512bw() and sizeof(_TV) == 64)
                return __vec_bitcast<_Tp>(is_signed_v<_Tp>
                                            ? _mm512_srav_epi16(__ix, __iy)
                                            : _mm512_srlv_epi16(__ix, __iy));
              else if constexpr (_Flags._M_have_avx2() and is_signed_v<_Tp>)
                return __vec_bitcast_trunc<_TV>(
                         __blend_0xaa(((__vec_bitcast<int>(__ix) << 16)
                         >> (__vec_bitcast<int>(__iy) & 0xffffu))
                         >> 16,
                                      __vec_bitcast<int>(__ix)
                         >> (__vec_bitcast<int>(__iy) >> 16)));
              else if constexpr (_Flags._M_have_avx2() and is_unsigned_v<_Tp>)
                return __vec_bitcast_trunc<_TV>(
                         __blend_0xaa((__vec_bitcast<unsigned>(__ix) & 0xffffu)
                         >> (__vec_bitcast<unsigned>(__iy) & 0xffffu),
                                      __vec_bitcast<unsigned>(__ix)
                         >> (__vec_bitcast<unsigned>(__iy) >> 16)));
              else if constexpr (_Flags._M_have_sse4_1())
                {
                  auto __mask = __vec_bitcast<unsigned short>(__iy);
                  auto __x128 = __vec_bitcast<_Tp>(__ix);
                  //__mask *= 0x0808;
                  __mask = (__mask << 3) | (__mask << 11);
                  // do __x128 = -1/0 where __y[4] is set
                  __x128 = __vec_bitcast<_Tp>(
                             _mm_blendv_epi8(__to_x86_intrin(__x128),
                                             is_signed_v<_Tp> ? __to_x86_intrin(__x128 >> 15)
                                                              : __m128i(),
                                             __to_x86_intrin(__mask)));
                  // do __x128 =>> 8 where __y[3] is set
                  __x128 = __vec_bitcast<_Tp>(
                             _mm_blendv_epi8(__to_x86_intrin(__x128), __to_x86_intrin(__x128 >> 8),
                                             __to_x86_intrin(__mask += __mask)));
                  // do __x128 =>> 4 where __y[2] is set
                  __x128 = __vec_bitcast<_Tp>(
                             _mm_blendv_epi8(__to_x86_intrin(__x128), __to_x86_intrin(__x128 >> 4),
                                             __to_x86_intrin(__mask += __mask)));
                  // do __x128 =>> 2 where __y[1] is set
                  __x128 = __vec_bitcast<_Tp>(
                             _mm_blendv_epi8(__to_x86_intrin(__x128), __to_x86_intrin(__x128 >> 2),
                                             __to_x86_intrin(__mask += __mask)));
                  // do __x128 =>> 1 where __y[0] is set
                  return __vec_bitcast_trunc<_TV>(
                           _mm_blendv_epi8(__to_x86_intrin(__x128), __to_x86_intrin(__x128 >> 1),
                                           __to_x86_intrin(__mask + __mask)));
                }
              else
                {
                  auto __k = __vec_bitcast<unsigned short>(__iy) << 11;
                  auto __x128 = __vec_zero_pad_to_16(__x);
                  auto __mask
                    = [] [[__gnu__::__always_inline__]] (__vec_builtin_type<unsigned short, 8> __kk)
                  { return __vec_bitcast<short>(__kk) < 0; };
                  if constexpr (is_unsigned_v<_Tp>)
                    // do __x128 = 0 where __y[4] is set
                    __x128 = __mask(__k) ? decltype(__x128)() : __x128;
                  else
                    // do __x128 = -1/0 where __y[4] is set
                    __x128 = __mask(__k) ? __x128 >> 15 : __x128;
                  // do __x128 =>> 8 where __y[3] is set
                  __x128 = __mask(__k += __k) ? __x128 >> 8 : __x128;
                  // do __x128 =>> 4 where __y[2] is set
                  __x128 = __mask(__k += __k) ? __x128 >> 4 : __x128;
                  // do __x128 =>> 2 where __y[1] is set
                  __x128 = __mask(__k += __k) ? __x128 >> 2 : __x128;
                  // do __x128 =>> 1 where __y[0] is set
                  return __vec_bitcast_trunc<_TV>(__mask(__k + __k) ? __x128 >> 1 : __x128);
                }
            }
          else if constexpr (sizeof(_Tp) == 4 and not _Flags._M_have_avx2())
            {
              static_assert(sizeof(__ix) == 16);
              if constexpr (is_unsigned_v<_Tp> and false)
                {
                  // x >> y == x * 2^-y == (x * 2^(31-y)) >> 31
                  const auto __factor_f
                    = reinterpret_cast<__v4float>(0x4f00'0000u
                                                    - (reinterpret_cast<__v4uint32>(__iy) << 23));
                  const __m128i __factor
                    = __builtin_constant_p(__factor_f)
                        ? __to_x86_intrin(__vec_builtin_type<unsigned, 4>{
                                            unsigned(__factor_f[0]), unsigned(__factor_f[1]),
                                            unsigned(__factor_f[2]), unsigned(__factor_f[3])})
                        : _mm_cvttps_epi32(__factor_f);
                  const auto __r02 = _mm_srli_epi64(_mm_mul_epu32(__ix, __factor), 31);
                  const auto __r13 = _mm_mul_epu32(_mm_srli_si128(__ix, 4),
                                                   _mm_srli_si128(__factor, 4));
                  if constexpr (_Flags._M_have_sse4_1())
                    return reinterpret_cast<_TV>(
                             _mm_blend_epi16(_mm_slli_epi64(__r13, 1), __r02, 0x33));
                  else
                    return __vec_bitcast_trunc<_TV>(
                             __r02 | _mm_slli_si128(_mm_srli_epi64(__r13, 31), 4));
                }
              else
                {
                  auto __shift = [] [[__gnu__::__always_inline__]] (auto __a, auto __b) {
                    if constexpr (is_signed_v<_Tp>)
                      return _mm_sra_epi32(__a, __b);
                    else
                      return _mm_srl_epi32(__a, __b);
                  };
                  const auto __r0 = __shift(__ix, _mm_unpacklo_epi32(__iy, __m128i()));
                  const auto __r1 = __shift(__ix, _mm_srli_epi64(__iy, 32));
                  const auto __r2 = _S_size >= 3
                                      ? __shift(__ix, _mm_unpackhi_epi32(__iy, __m128i()))
                                      : __m128i();
                  const auto __r3 = _S_size == 4
                                      ? __shift(__ix, _mm_srli_si128(__iy, 12))
                                      : __m128i();
                  if constexpr (_Flags._M_have_sse4_1())
                    return __vec_bitcast_trunc<_TV>(
                             _mm_blend_epi16(_mm_blend_epi16(__r1, __r0, 0x3),
                                             _mm_blend_epi16(__r3, __r2, 0x30), 0xf0));
                  else
                    return __vec_bitcast_trunc<_TV>(
                             _mm_unpacklo_epi64(_mm_unpacklo_epi32(__r0, _mm_srli_si128(__r1, 4)),
                                                _mm_unpackhi_epi32(__r2, _mm_srli_si128(__r3, 4))));
                }
            }
          else
            return __x >> __y;
        }
#endif // _GLIBCXX_SIMD_NO_SHIFT_OPT / __clang__

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_equal_to(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          if constexpr (_S_use_bitmasks)
            {
              if (__builtin_is_constant_evaluated()
                    or (__builtin_constant_p(__x) and __builtin_constant_p(__y)))
                {
                  const _MaskMember<_TV> __k
                    = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                        return ((__vec_get(__x, _Is) == __vec_get(__y, _Is)
                                   ? (1ULL << _Is) : 0) | ...);
                      });
                  if (__builtin_is_constant_evaluated() or __builtin_constant_p(__k))
                    return __k;
                }

              constexpr auto __k1 = _Abi::template _S_implicit_mask<_Tp>;
              [[maybe_unused]] const auto __xi = __to_x86_intrin(__x);
              [[maybe_unused]] const auto __yi = __to_x86_intrin(__y);
              if constexpr (is_floating_point_v<_Tp>)
                {
                  if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                    return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_EQ_OQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                    return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_EQ_OQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                    return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_EQ_OQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                    return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_EQ_OQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                    return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_EQ_OQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                    return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_EQ_OQ);
                  else if constexpr (sizeof(_Tp) == 8)
                    return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_EQ_OQ);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_EQ_OQ);
                  else if constexpr (sizeof(_Tp) == 2)
                    return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_EQ_OQ);
                  else
                    static_assert(false);
                }
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmpeq_epi64_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmpeq_epi32_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return _mm512_mask_cmpeq_epi16_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 1, 64>)
                return _mm512_mask_cmpeq_epi8_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmpeq_epi64_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmpeq_epi32_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                return _mm256_mask_cmpeq_epi16_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 1, 32>)
                return _mm256_mask_cmpeq_epi8_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 8)
                return _mm_mask_cmpeq_epi64_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 4)
                return _mm_mask_cmpeq_epi32_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 2)
                return _mm_mask_cmpeq_epi16_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 1)
                return _mm_mask_cmpeq_epi8_mask(__k1, __xi, __yi);
              else
                static_assert(false);
            }

          else
            return _Base::_S_equal_to(__x, __y);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_not_equal_to(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          if constexpr (_S_use_bitmasks)
            {
              if (__builtin_is_constant_evaluated()
                    or (__builtin_constant_p(__x) and __builtin_constant_p(__y)))
                {
                  const _MaskMember<_TV> __k
                    = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                        return ((__x[_Is] != __y[_Is] ? (1ULL << _Is) : 0) | ...);
                      });
                  if (__builtin_is_constant_evaluated() or __builtin_constant_p(__k))
                    return __k;
                }

              constexpr auto __k1 = _Abi::template _S_implicit_mask<_Tp>;
              [[maybe_unused]] const auto __xi = __to_x86_intrin(__x);
              [[maybe_unused]] const auto __yi = __to_x86_intrin(__y);
              if constexpr (is_floating_point_v<_Tp>)
                {
                  if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                    return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_NEQ_UQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                    return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_NEQ_UQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                    return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_NEQ_UQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                    return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_NEQ_UQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                    return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_NEQ_UQ);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                    return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_NEQ_UQ);
                  else if constexpr (sizeof(_Tp) == 8)
                    return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_NEQ_UQ);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_NEQ_UQ);
                  else if constexpr (sizeof(_Tp) == 2)
                    return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_NEQ_UQ);
                  else
                    static_assert(false);
                }
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmpneq_epi64_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmpneq_epi32_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return _mm512_mask_cmpneq_epi16_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 1, 64>)
                return _mm512_mask_cmpneq_epi8_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmpneq_epi64_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmpneq_epi32_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                return _mm256_mask_cmpneq_epi16_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 1, 32>)
                return _mm256_mask_cmpneq_epi8_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 8)
                return _mm_mask_cmpneq_epi64_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 4)
                return _mm_mask_cmpneq_epi32_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 2)
                return _mm_mask_cmpneq_epi16_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 1)
                return _mm_mask_cmpneq_epi8_mask(__k1, __xi, __yi);
              else
                static_assert(false);
            }

          else
            return _Base::_S_not_equal_to(__x, __y);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_less(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          if constexpr (_S_use_bitmasks)
            {
              if (__builtin_is_constant_evaluated()
                    or (__builtin_constant_p(__x) and __builtin_constant_p(__y)))
                {
                  const _MaskMember<_TV> __k
                    = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                        return ((__vec_get(__x, _Is) < __vec_get(__y, _Is)
                                   ? (1ULL << _Is) : 0) | ...);
                      });
                  if (__builtin_is_constant_evaluated() or __builtin_constant_p(__k))
                    return __k;
                }

              constexpr auto __k1 = _Abi::template _S_implicit_mask<_Tp>;
              [[maybe_unused]] const auto __xi = __to_x86_intrin(__x);
              [[maybe_unused]] const auto __yi = __to_x86_intrin(__y);
              if constexpr (is_floating_point_v<_Tp>)
                {
                  if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                    return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LT_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                    return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LT_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                    return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LT_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                    return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LT_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                    return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LT_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                    return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LT_OS);
                  else if constexpr (sizeof(_Tp) == 8)
                    return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LT_OS);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LT_OS);
                  else if constexpr (sizeof(_Tp) == 2)
                    return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LT_OS);
                  else
                    static_assert(false);
                }
              else if constexpr (is_signed_v<_Tp>)
                {
                  if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                    return _mm512_mask_cmplt_epi64_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                    return _mm512_mask_cmplt_epi32_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                    return _mm512_mask_cmplt_epi16_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 1, 64>)
                    return _mm512_mask_cmplt_epi8_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                    return _mm256_mask_cmplt_epi64_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                    return _mm256_mask_cmplt_epi32_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                    return _mm256_mask_cmplt_epi16_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 1, 32>)
                    return _mm256_mask_cmplt_epi8_mask(__k1, __xi, __yi);
                  else if constexpr (sizeof(_Tp) == 8)
                    return _mm_mask_cmplt_epi64_mask(__k1, __xi, __yi);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm_mask_cmplt_epi32_mask(__k1, __xi, __yi);
                  else if constexpr (sizeof(_Tp) == 2)
                    return _mm_mask_cmplt_epi16_mask(__k1, __xi, __yi);
                  else if constexpr (sizeof(_Tp) == 1)
                    return _mm_mask_cmplt_epi8_mask(__k1, __xi, __yi);
                  else
                    static_assert(false);
                }
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmplt_epu64_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmplt_epu32_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return _mm512_mask_cmplt_epu16_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 1, 64>)
                return _mm512_mask_cmplt_epu8_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmplt_epu64_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmplt_epu32_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                return _mm256_mask_cmplt_epu16_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 1, 32>)
                return _mm256_mask_cmplt_epu8_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 8)
                return _mm_mask_cmplt_epu64_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 4)
                return _mm_mask_cmplt_epu32_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 2)
                return _mm_mask_cmplt_epu16_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 1)
                return _mm_mask_cmplt_epu8_mask(__k1, __xi, __yi);
              else
                static_assert(false);
            }

          else
            return _Base::_S_less(__x, __y);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_less_equal(_TV __x, _TV __y)
        {
          using _Tp = __value_type_of<_TV>;
          if constexpr (_S_use_bitmasks)
            {
              if (__builtin_is_constant_evaluated()
                    or (__builtin_constant_p(__x) and __builtin_constant_p(__y)))
                {
                  const _MaskMember<_Tp> __k
                    = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                        return ((__vec_get(__x, _Is) <= __vec_get(__y, _Is)
                                   ? (1ULL << _Is) : 0) | ...);
                      });
                  if (__builtin_is_constant_evaluated() or __builtin_constant_p(__k))
                    return __k;
                }

              constexpr auto __k1 = _Abi::template _S_implicit_mask<_Tp>;
              [[maybe_unused]] const auto __xi = __to_x86_intrin(__x);
              [[maybe_unused]] const auto __yi = __to_x86_intrin(__y);
              if constexpr (is_floating_point_v<_Tp>)
                {
                  if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                    return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LE_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                    return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LE_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                    return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LE_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                    return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LE_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                    return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LE_OS);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                    return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LE_OS);
                  else if constexpr (sizeof(_Tp) == 8)
                    return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LE_OS);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LE_OS);
                  else if constexpr (sizeof(_Tp) == 2)
                    return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LE_OS);
                  else
                    static_assert(false);
                }
              else if constexpr (is_signed_v<_Tp>)
                {
                  if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                    return _mm512_mask_cmple_epi64_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                    return _mm512_mask_cmple_epi32_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                    return _mm512_mask_cmple_epi16_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 1, 64>)
                    return _mm512_mask_cmple_epi8_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                    return _mm256_mask_cmple_epi64_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                    return _mm256_mask_cmple_epi32_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                    return _mm256_mask_cmple_epi16_mask(__k1, __xi, __yi);
                  else if constexpr (__vec_builtin_sizeof<_TV, 1, 32>)
                    return _mm256_mask_cmple_epi8_mask(__k1, __xi, __yi);
                  else if constexpr (sizeof(_Tp) == 8)
                    return _mm_mask_cmple_epi64_mask(__k1, __xi, __yi);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm_mask_cmple_epi32_mask(__k1, __xi, __yi);
                  else if constexpr (sizeof(_Tp) == 2)
                    return _mm_mask_cmple_epi16_mask(__k1, __xi, __yi);
                  else if constexpr (sizeof(_Tp) == 1)
                    return _mm_mask_cmple_epi8_mask(__k1, __xi, __yi);
                  else
                    static_assert(false);
                }
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmple_epu64_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmple_epu32_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return _mm512_mask_cmple_epu16_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 1, 64>)
                return _mm512_mask_cmple_epu8_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmple_epu64_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmple_epu32_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                return _mm256_mask_cmple_epu16_mask(__k1, __xi, __yi);
              else if constexpr (__vec_builtin_sizeof<_TV, 1, 32>)
                return _mm256_mask_cmple_epu8_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 8)
                return _mm_mask_cmple_epu64_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 4)
                return _mm_mask_cmple_epu32_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 2)
                return _mm_mask_cmple_epu16_mask(__k1, __xi, __yi);
              else if constexpr (sizeof(_Tp) == 1)
                return _mm_mask_cmple_epu8_mask(__k1, __xi, __yi);
              else
                static_assert(false);
            }

          else
            return _Base::_S_less_equal(__x, __y);
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_negate(_TV __x)
        {
          if constexpr (_S_use_bitmasks)
            return _S_equal_to(__x, _TV());
          else
            return _Base::_S_negate(__x);
        }

      using _Base::_S_abs;

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_sqrt(_TV __x)
        {
          if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512fp16())
            return __builtin_ia32_sqrtph512_mask_round(__x, _TV(), -1, _X86Round::_CurDirection);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
            return __builtin_ia32_sqrtps512_mask(__x, _TV(), -1, _X86Round::_CurDirection);
          else if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
            return __builtin_ia32_sqrtpd512_mask(__x, _TV(), -1, _X86Round::_CurDirection);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512fp16())
            return __builtin_ia32_sqrtph256_mask(__x, _TV(), -1);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
            return __builtin_ia32_sqrtps256(__x);
          else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
            return __builtin_ia32_sqrtpd256(__x);
          else if constexpr (__vec_builtin_sizeof<_TV, 2> and _Flags._M_have_avx512fp16())
            return __builtin_ia32_sqrtph128_mask(__x, _TV(), -1);
          else if constexpr (__vec_builtin_sizeof<_TV, 4>)
            return __builtin_ia32_sqrtps(__x);
          else if constexpr (__vec_builtin_sizeof<_TV, 8>)
            return __builtin_ia32_sqrtps(__x);
          else
            static_assert(false);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_ldexp(_TV __x, typename _Abi::template _Rebind<
                            __make_dependent_t<int, _TV>>::template _SimdMember<int> __exp)
        {
          using _ExpAbi = typename _Abi::template _Rebind<int>;
          using _Tp = __value_type_of<_TV>;
          constexpr int _Np = _S_size;
          if constexpr (sizeof(__x) == 64 or _Flags._M_have_avx512vl())
            {
              const auto __xi = __to_x86_intrin(__x);
              constexpr _SimdConverter<int, _ExpAbi, _Tp, _Abi> __cvt;
              const auto __expi = __to_x86_intrin(__cvt(__exp));
              using _Up = _MaskMember<_Tp>;
              constexpr _Up __k1 = _Np < sizeof(_Up) * __CHAR_BIT__ ? _Up((1ULL << _Np) - 1) : ~_Up();
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_maskz_scalef_pd(__k1, __xi, __expi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_maskz_scalef_ps(__k1, __xi, __expi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return _mm512_maskz_scalef_ph(__k1, __xi, __expi);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_maskz_scalef_pd(__k1, __xi, __expi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_maskz_scalef_ps(__k1, __xi, __expi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                return _mm256_maskz_scalef_ph(__k1, __xi, __expi);
              else if constexpr (__vec_builtin_sizeof<_TV, 8>)
                return _mm_maskz_scalef_pd(__k1, __xi, __expi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4>)
                return _mm_maskz_scalef_ps(__k1, __xi, __expi);
              else if constexpr (__vec_builtin_sizeof<_TV, 2>)
                return _mm_maskz_scalef_ph(__k1, __xi, __expi);
              else
                static_assert(false);
            }
          else
            return _Base::_S_ldexp(__x, __exp);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_trunc(_TV __x)
        {
          if (not __builtin_is_constant_evaluated())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_roundscale_pd(__x, 0x0b);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_roundscale_ps(__x, 0x0b);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return _mm512_roundscale_ph(__x, 0x0b);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_round_pd(__x, 0xb);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_round_ps(__x, 0xb);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                return _mm256_round_ph(__x, 0xb);
              else if constexpr (__vec_builtin_sizeof<_TV, 8> and _Flags._M_have_sse4_1())
                return _mm_round_pd(__x, 0xb);
              else if constexpr (__vec_builtin_sizeof<_TV, 4> and _Flags._M_have_sse4_1())
                return _mm_round_ps(__x, 0xb);
              else if constexpr (__vec_builtin_sizeof<_TV, 2>)
                return _mm_round_ph(__x, 0xb);
              else if constexpr (__vec_builtin_sizeof<_TV, 4>)
                {
                  auto __truncated = _mm_cvtepi32_ps(_mm_cvttps_epi32(__to_x86_intrin(__x)));
                  const auto __no_fractional_values
                    = __vec_bitcast<int>(__vec_bitcast<unsigned>(__to_x86_intrin(__x)) & 0x7f800000u)
                        < 0x4b000000; // the exponent is so large that no mantissa bits signify fractional
                  // values (0x3f8 + 23*8 = 0x4b0)
                  return __no_fractional_values ? __truncated : __to_x86_intrin(__x);
                }
            }
          return _Base::_S_trunc(__x);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_round(_TV __x)
        {
          // Note that _MM_FROUND_TO_NEAREST_INT rounds ties to even, not away
          // from zero as required by std::round. Therefore this function is more
          // complicated.
          _TV __truncated;
          if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
            __truncated = _mm512_roundscale_pd(__x, 0x0b);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
            __truncated = _mm512_roundscale_ps(__x, 0x0b);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
            __truncated = _mm512_roundscale_ph(__x, 0x0b);
          else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
            __truncated = _mm256_round_pd(__x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
            __truncated = _mm256_round_ps(__x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
            __truncated = _mm256_round_ph(__x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 8> and _Flags._M_have_sse4_1())
            __truncated = _mm_round_pd(__x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 4> and _Flags._M_have_sse4_1())
            __truncated = _mm_round_ps(__x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 2>)
            __truncated = _mm_round_ph(__x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 4>)
            __truncated = _mm_cvtepi32_ps(_mm_cvttps_epi32(__x));
          else
            return _Base::_S_round(__x);

          // x < 0 => truncated <= 0 and truncated >= x => x - truncated <= 0
          // x > 0 => truncated >= 0 and truncated <= x => x - truncated >= 0

          using _Tp = __value_type_of<_TV>;
          const _TV __rounded = __truncated + (__vec_and(_S_absmask<_TV>, __x - __truncated) >= _Tp(.5)
                                                 ? __vec_or(__vec_and(_S_signmask<_TV>, __x), _TV() + 1)
                                                 : _TV());
          if constexpr (_Flags._M_have_sse4_1())
            return __rounded;
          else // adjust for missing range in cvttps_epi32
            return __vec_and(_S_absmask<_TV>, __x) < 0x1p23f ? __rounded : __x;
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_nearbyint(_TV __x)
        {
          if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
            return _mm512_roundscale_pd(__x, 0x0c);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
            return _mm512_roundscale_ps(__x, 0x0c);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
            return _mm512_roundscale_ph(__x, 0x0c);
          else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
            return _mm256_round_pd(__x, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
            return _mm256_round_ps(__x, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
            return _mm256_round_ph(__x, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 8> and _Flags._M_have_sse4_1())
            return _mm_round_pd(__x, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 4> and _Flags._M_have_sse4_1())
            return _mm_round_ps(__x, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
          else if constexpr (__vec_builtin_sizeof<_TV, 2>)
            return _mm_round_ph(__x, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
          else
            return _Base::_S_nearbyint(__x);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_rint(_TV __x)
        {
          if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
            return _mm512_roundscale_pd(__x, 0x04);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
            return _mm512_roundscale_ps(__x, 0x04);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
            return _mm512_roundscale_ph(__x, 0x04);
          else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
            return _mm256_round_pd(__x, _MM_FROUND_CUR_DIRECTION);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
            return _mm256_round_ps(__x, _MM_FROUND_CUR_DIRECTION);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
            return _mm256_round_ph(__x, _MM_FROUND_CUR_DIRECTION);
          else if constexpr (__vec_builtin_sizeof<_TV, 8> and _Flags._M_have_sse4_1())
            return _mm_round_pd(__x, _MM_FROUND_CUR_DIRECTION);
          else if constexpr (__vec_builtin_sizeof<_TV, 4> and _Flags._M_have_sse4_1())
            return _mm_round_ps(__x, _MM_FROUND_CUR_DIRECTION);
          else if constexpr (__vec_builtin_sizeof<_TV, 2>)
            return _mm_round_ph(__x, _MM_FROUND_CUR_DIRECTION);
          else
            return _Base::_S_rint(__x);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_floor(_TV __x)
        {
          if (not __builtin_is_constant_evaluated())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_roundscale_pd(__x, 0x09);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_roundscale_ps(__x, 0x09);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return _mm512_roundscale_ph(__x, 0x09);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_round_pd(__x, 0x09);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_round_ps(__x, 0x09);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                return _mm256_round_ph(__x, 0x09);
              else if constexpr (__vec_builtin_sizeof<_TV, 8> and _Flags._M_have_sse4_1())
                return _mm_round_pd(__x, 0x09);
              else if constexpr (__vec_builtin_sizeof<_TV, 4> and _Flags._M_have_sse4_1())
                return _mm_round_ps(__x, 0x09);
              else if constexpr (__vec_builtin_sizeof<_TV, 2>)
                return _mm_round_ph(__x, 0x09);
            }
          return _Base::_S_floor(__x);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _TV
        _S_ceil(_TV __x)
        {
          if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
            return _mm512_roundscale_pd(__x, 0x0a);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
            return _mm512_roundscale_ps(__x, 0x0a);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
            return _mm512_roundscale_ph(__x, 0x0a);
          else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
            return _mm256_round_pd(__x, 0x0a);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
            return _mm256_round_ps(__x, 0x0a);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
            return _mm256_round_ph(__x, 0x0a);
          else if constexpr (__vec_builtin_sizeof<_TV, 8, 16> and _Flags._M_have_sse4_1())
            return _mm_round_pd(__x, 0x0a);
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 16> and _Flags._M_have_sse4_1())
            return _mm_round_ps(__x, 0x0a);
          else if constexpr (__vec_builtin_sizeof<_TV, 2, 16>)
            return _mm_round_ph(__x, 0x0a);
          else
            return _Base::_S_ceil(__x);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_signbit(_TV __x)
        {
          if constexpr (_S_use_bitmasks)
            {
              const auto __xi = __to_x86_intrin(__x);
              [[maybe_unused]] constexpr auto __k1 = _Abi::template _S_implicit_mask<__value_type_of<_TV>>;
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64> and _Flags._M_have_avx512dq())
                return _mm512_movepi64_mask(reinterpret_cast<__m512i>(__x));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64> and _Flags._M_have_avx512dq())
                return _mm512_movepi32_mask(reinterpret_cast<__m512i>(__x));
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512bw())
                return _mm512_movepi16_mask(reinterpret_cast<__m512i>(__x));
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32> and _Flags._M_have_avx512dq()
                                   and _Flags._M_have_avx512vl())
                return _mm256_movepi64_mask(reinterpret_cast<__m256i>(__x));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32> and _Flags._M_have_avx512dq()
                                   and _Flags._M_have_avx512vl())
                return _mm256_movepi32_mask(reinterpret_cast<__m256i>(__x));
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512bw()
                                   and _Flags._M_have_avx512vl())
                return _mm256_movepi16_mask(reinterpret_cast<__m256i>(__x));
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16> and _Flags._M_have_avx512dq()
                                   and _Flags._M_have_avx512vl())
                return _mm_movepi64_mask(reinterpret_cast<__m128i>(__x));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16> and _Flags._M_have_avx512dq()
                                   and _Flags._M_have_avx512vl())
                return _mm_movepi32_mask(reinterpret_cast<__m128i>(__x));
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16> and _Flags._M_have_avx512bw()
                                   and _Flags._M_have_avx512vl())
                return _mm_movepi16_mask(reinterpret_cast<__m128i>(__x));
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_movemask_pd(__xi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_movemask_ps(__xi);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_movemask_pd(__xi);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_movemask_ps(__xi);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm512_mask_cmplt_epi64_mask(__k1, reinterpret_cast<__m512i>(__xi), __m512i());
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm512_mask_cmplt_epi32_mask(__k1, reinterpret_cast<__m512i>(__xi), __m512i());
              else
                static_assert(false);
            }
          else
            return _Base::_S_signbit(__x);
        }

      // (isnormal | is subnormal == !isinf & !isnan & !is zero)
      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr auto
        _S_isnonzerovalue_mask(_TV __x)
        {
          using _Tp = __value_type_of<_TV>;
          if constexpr (_Flags._M_have_avx512dq() and _Flags._M_have_avx512vl())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 4, 16> or __vec_builtin_sizeof<_TV, 4, 8>)
                return _knot_mask8(_mm_fpclass_ps_mask(__to_x86_intrin(__x), 0x9f));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _knot_mask8(_mm256_fpclass_ps_mask(__x, 0x9f));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _knot_mask16(_mm512_fpclass_ps_mask(__x, 0x9f));
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _knot_mask8(_mm_fpclass_pd_mask(__x, 0x9f));
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _knot_mask8(_mm256_fpclass_pd_mask(__x, 0x9f));
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _knot_mask8(_mm512_fpclass_pd_mask(__x, 0x9f));
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_avx512f())
            {
              const auto __a = __x * __infinity_v<_Tp>; // NaN if __x == 0
              const auto __b = __x * _Tp();             // NaN if __x == inf
              if constexpr (_Flags._M_have_avx512vl() and __vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_cmp_ps_mask(__to_x86_intrin(__a), __to_x86_intrin(__b), _CMP_ORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return __mmask8(0xf & _mm512_cmp_ps_mask(__to_x86_intrin(__a), __to_x86_intrin(__b),
                                                         _CMP_ORD_Q));
              else if constexpr (_Flags._M_have_avx512vl() and __vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_cmp_pd_mask(__a, __b, _CMP_ORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return __mmask8(0x3 & _mm512_cmp_pd_mask(__to_x86_intrin(__a), __to_x86_intrin(__b),
                                                         _CMP_ORD_Q));
              else if constexpr (_Flags._M_have_avx512vl() and __vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_cmp_ps_mask(__a, __b, _CMP_ORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return __mmask8(_mm512_cmp_ps_mask(__to_x86_intrin(__a), __to_x86_intrin(__b),
                                                   _CMP_ORD_Q));
              else if constexpr (_Flags._M_have_avx512vl() and __vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_cmp_pd_mask(__a, __b, _CMP_ORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return __mmask8(0xf & _mm512_cmp_pd_mask(__to_x86_intrin(__a), __to_x86_intrin(__b),
                                                         _CMP_ORD_Q));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_cmp_ps_mask(__a, __b, _CMP_ORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_cmp_pd_mask(__a, __b, _CMP_ORD_Q);
              else
                static_assert(false);
            }
          else
            static_assert(false);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isfinite(_TV __x)
        {
#if not __FINITE_MATH_ONLY__
          using _Tp = __value_type_of<_TV>;
          if constexpr (_S_use_bitmasks and _Flags._M_have_avx512dq())
            {
              const auto __xi = __to_x86_intrin(__x);
              constexpr auto __k1 = _Abi::template _S_implicit_mask<_Tp>;
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return __k1 ^ _mm512_mask_fpclass_pd_mask(__k1, __xi, 0x99);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return __k1 ^ _mm512_mask_fpclass_ps_mask(__k1, __xi, 0x99);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return __k1 ^ _mm512_mask_fpclass_ph_mask(__k1, __xi, 0x99);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return __k1 ^ _mm256_mask_fpclass_pd_mask(__k1, __xi, 0x99);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return __k1 ^ _mm256_mask_fpclass_ps_mask(__k1, __xi, 0x99);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                return __k1 ^ _mm256_mask_fpclass_ph_mask(__k1, __xi, 0x99);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return __k1 ^ _mm_mask_fpclass_pd_mask(__k1, __xi, 0x99);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return __k1 ^ _mm_mask_fpclass_ps_mask(__k1, __xi, 0x99);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16>)
                return __k1 ^ _mm_mask_fpclass_ph_mask(__k1, __xi, 0x99);
              else
                static_assert(false);
            }
          else
#endif
            return _Base::_S_isfinite(__x);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isinf(_TV __x)
        {
#if !__FINITE_MATH_ONLY__
          if constexpr (_S_use_bitmasks and _Flags._M_have_avx512dq())
            {
              const auto __xi = __to_x86_intrin(__x);
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_fpclass_pd_mask(__xi, 0x18);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_fpclass_ps_mask(__xi, 0x18);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return _mm512_fpclass_ph_mask(__xi, 0x18);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_fpclass_pd_mask(__xi, 0x18);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_fpclass_ps_mask(__xi, 0x18);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32>)
                return _mm256_fpclass_ph_mask(__xi, 0x18);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_fpclass_pd_mask(__xi, 0x18);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_fpclass_ps_mask(__xi, 0x18);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16>)
                return _mm_fpclass_ph_mask(__xi, 0x18);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_avx512dq() and _Flags._M_have_avx512vl())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_movm_epi64(_mm_fpclass_pd_mask(__x, 0x18));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_movm_epi32(_mm_fpclass_ps_mask(__to_x86_intrin(__x), 0x18));
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_movm_epi64(_mm256_fpclass_pd_mask(__x, 0x18));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_movm_epi32(_mm256_fpclass_ps_mask(__x, 0x18));
              else
                static_assert(false);
            }
          else
#endif
            return _Base::_S_isinf(__x);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isnormal(_TV __x)
        {
#if __FINITE_MATH_ONLY__
          [[maybe_unused]] constexpr int __mode = 0x26;
#else
          [[maybe_unused]] constexpr int __mode = 0xbf;
#endif
          using _Tp = __value_type_of<_TV>;
          if constexpr (_S_use_bitmasks and _Flags._M_have_avx512dq())
            {
              const auto __xi = __to_x86_intrin(__x);
              const auto __k1 = __to_x86_intrin(_Abi::template _S_implicit_mask<_Tp>);
              if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return __k1 ^ _mm_mask_fpclass_pd_mask(__k1, __xi, __mode);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return __k1 ^ _mm_mask_fpclass_ps_mask(__k1, __xi, __mode);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return __k1 ^ _mm_mask_fpclass_ph_mask(__k1, __xi, __mode);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return __k1 ^ _mm256_mask_fpclass_pd_mask(__k1, __xi, __mode);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return __k1 ^ _mm256_mask_fpclass_ps_mask(__k1, __xi, __mode);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return __k1 ^ _mm256_mask_fpclass_ph_mask(__k1, __xi, __mode);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return __k1 ^ _mm512_mask_fpclass_pd_mask(__k1, __xi, __mode);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return __k1 ^ _mm512_mask_fpclass_ps_mask(__k1, __xi, __mode);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512fp16())
                return __k1 ^ _mm512_mask_fpclass_ph_mask(__k1, __xi, __mode);
              else
                static_assert(false);
            }
          else if constexpr (_S_use_bitmasks)
            {
              constexpr int _Np = _S_size;
              using _Ip = __make_signed_int_t<_Tp>;
              const auto __absn = __vec_bitcast<_Ip>(_S_abs(__x));
              const auto __minn = __vec_bitcast<_Ip>(__vec_broadcast<_Np>(__norm_min_v<_Tp>));
#if __FINITE_MATH_ONLY__
              return _S_less_equal(__minn, __absn);
#else
              const auto __infn = __vec_bitcast<_Ip>(__vec_broadcast<_Np>(__infinity_v<_Tp>));
              return _S_less_equal(__minn, __absn) & _S_less(__absn, __infn);
#endif
            }
          else if constexpr (_Flags._M_have_avx512dq())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 8, 16> and _Flags._M_have_avx512vl())
                return _mm_movm_epi64(_knot_mask8(_mm_fpclass_pd_mask(__x, __mode)));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16> and _Flags._M_have_avx512vl())
                return _mm_movm_epi32(_knot_mask8(_mm_fpclass_ps_mask(__to_x86_intrin(__x), __mode)));
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16> and _Flags._M_have_avx512vl())
                return _mm_movm_epi16(_knot_mask8(_mm_fpclass_ph_mask(__to_x86_intrin(__x), __mode)));
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32> and _Flags._M_have_avx512vl())
                return _mm256_movm_epi64(_knot_mask8(_mm256_fpclass_pd_mask(__x, __mode)));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32> and _Flags._M_have_avx512vl())
                return _mm256_movm_epi32(_knot_mask8(_mm256_fpclass_ps_mask(__x, __mode)));
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512vl())
                return _mm256_movm_epi16(_knot_mask16(_mm256_fpclass_ph_mask(__x, __mode)));
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_movm_epi64(_knot_mask8(_mm512_fpclass_pd_mask(__x, __mode)));
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_movm_epi32(_knot_mask16(_mm512_fpclass_ps_mask(__x, __mode)));
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64>)
                return _mm512_movm_epi16(_knot_mask32(_mm512_fpclass_ph_mask(__x, __mode)));
              else
                static_assert(false);
            }
          else
            return _Base::_S_isnormal(__x);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isnan(_TV __x)
        { return _S_isunordered(__x, __x); }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isunordered([[maybe_unused]] _TV __x, [[maybe_unused]] _TV __y)
        {
#if __FINITE_MATH_ONLY__
          return _MaskMember<_TV>{}; // false
#else
          using _Tp = __value_type_of<_TV>;
          const auto __xi = __to_x86_intrin(__x);
          const auto __yi = __to_x86_intrin(__y);
          if constexpr (_S_use_bitmasks)
            {
              constexpr auto __k1 = _Abi::template _S_implicit_mask<_Tp>;
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_UNORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_UNORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512fp16())
                return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_UNORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_UNORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_UNORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_UNORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_UNORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_UNORD_Q);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_UNORD_Q);
              else
                static_assert(false);
            }
          else if constexpr (sizeof(__xi) == 32 and sizeof(_Tp) == 4)
            return _mm256_cmp_ps(__xi, __yi, _CMP_UNORD_Q);
          else if constexpr (sizeof(__xi) == 32 and sizeof(_Tp) == 8)
            return _mm256_cmp_pd(__xi, __yi, _CMP_UNORD_Q);
          else if constexpr (sizeof(__xi) == 16 and sizeof(_Tp) == 4)
            return _mm_cmpunord_ps(__xi, __yi);
          else if constexpr (sizeof(__xi) == 16 and sizeof(_Tp) == 8)
            return _mm_cmpunord_pd(__xi, __yi);
          else
            static_assert(false);
#endif
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isgreater(_TV __x, _TV __y)
        {
          const auto __xi = __to_x86_intrin(__x);
          const auto __yi = __to_x86_intrin(__y);
          if constexpr (_S_use_bitmasks)
            {
              constexpr auto __k1 = _Abi::template _S_implicit_mask<__value_type_of<_TV>>;
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512fp16())
                return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_GT_OQ);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_avx())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_cmp_pd(__xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_cmp_ps(__xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_cmp_pd(__xi, __yi, _CMP_GT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_cmp_ps(__xi, __yi, _CMP_GT_OQ);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_sse2() and __vec_builtin_sizeof<_TV, 4, 16>)
            {
              using _IV = __vec_builtin_type<int, 4>;
              const auto __xn = reinterpret_cast<_IV>(__xi);
              const auto __yn = reinterpret_cast<_IV>(__yi);
              const auto __xp = __xn < 0 ? -(__xn & 0x7fff'ffff) : __xn;
              const auto __yp = __yn < 0 ? -(__yn & 0x7fff'ffff) : __yn;
              return reinterpret_cast<_IV>(_mm_cmpord_ps(__xi, __yi)) & (__xp > __yp);
            }
          else if constexpr (_Flags._M_have_sse2() and __vec_builtin_sizeof<_TV, 8, 16>)
            return __vec_builtin_type<long long, 2>{
              -_mm_ucomigt_sd(__xi, __yi),
              -_mm_ucomigt_sd(_mm_unpackhi_pd(__xi, __xi), _mm_unpackhi_pd(__yi, __yi))};
          else
            return _Base::_S_isgreater(__x, __y);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isgreaterequal(_TV __x, _TV __y)
        {
          const auto __xi = __to_x86_intrin(__x);
          const auto __yi = __to_x86_intrin(__y);
          if constexpr (_S_use_bitmasks)
            {
              constexpr auto __k1 = _Abi::template _S_implicit_mask<__value_type_of<_TV>>;
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512fp16())
                return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_GE_OQ);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_avx())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_cmp_pd(__xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_cmp_ps(__xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_cmp_pd(__xi, __yi, _CMP_GE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_cmp_ps(__xi, __yi, _CMP_GE_OQ);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_sse2() and __vec_builtin_sizeof<_TV, 4, 16>)
            {
              using _IV = __vec_builtin_type<int, 4>;
              const auto __xn = reinterpret_cast<_IV>(__xi);
              const auto __yn = reinterpret_cast<_IV>(__yi);
              const auto __xp = __xn < 0 ? -(__xn & 0x7fff'ffff) : __xn;
              const auto __yp = __yn < 0 ? -(__yn & 0x7fff'ffff) : __yn;
              return reinterpret_cast<_IV>(_mm_cmpord_ps(__xi, __yi)) & (__xp >= __yp);
            }
          else if constexpr (_Flags._M_have_sse2() and __vec_builtin_sizeof<_TV, 8, 16>)
            return __vec_builtin_type<long long, 2>{
              -_mm_ucomige_sd(__xi, __yi),
              -_mm_ucomige_sd(_mm_unpackhi_pd(__xi, __xi), _mm_unpackhi_pd(__yi, __yi))};
          else
            return _Base::_S_isgreaterequal(__x, __y);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_isless(_TV __x, _TV __y)
        {
          const auto __xi = __to_x86_intrin(__x);
          const auto __yi = __to_x86_intrin(__y);
          if constexpr (_S_use_bitmasks)
            {
              constexpr auto __k1 = _Abi::template _S_implicit_mask<__value_type_of<_TV>>;
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512fp16())
                return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LT_OQ);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_avx())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_cmp_pd(__xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_cmp_ps(__xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_cmp_pd(__xi, __yi, _CMP_LT_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_cmp_ps(__xi, __yi, _CMP_LT_OQ);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_sse2() and __vec_builtin_sizeof<_TV, 4, 16>)
            {
              using _IV = __vec_builtin_type<int, 4>;
              const auto __xn = reinterpret_cast<_IV>(__xi);
              const auto __yn = reinterpret_cast<_IV>(__yi);
              const auto __xp = __xn < 0 ? -(__xn & 0x7fff'ffff) : __xn;
              const auto __yp = __yn < 0 ? -(__yn & 0x7fff'ffff) : __yn;
              return reinterpret_cast<_IV>(_mm_cmpord_ps(__xi, __yi)) & (__xp < __yp);
            }
          else if constexpr (_Flags._M_have_sse2() and __vec_builtin_sizeof<_TV, 8, 16>)
            return __vec_builtin_type<long long, 2>{
              -_mm_ucomigt_sd(__yi, __xi),
              -_mm_ucomigt_sd(_mm_unpackhi_pd(__yi, __yi), _mm_unpackhi_pd(__xi, __xi))};
          else
            return _Base::_S_isless(__x, __y);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_islessequal(_TV __x, _TV __y)
        {
          const auto __xi = __to_x86_intrin(__x);
          const auto __yi = __to_x86_intrin(__y);
          if constexpr (_S_use_bitmasks)
            {
              constexpr auto __k1 = _Abi::template _S_implicit_mask<__value_type_of<_TV>>;
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512fp16())
                return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_LE_OQ);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_avx())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_cmp_pd(__xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_cmp_ps(__xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_cmp_pd(__xi, __yi, _CMP_LE_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_cmp_ps(__xi, __yi, _CMP_LE_OQ);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_sse2() and __vec_builtin_sizeof<_TV, 4, 16>)
            {
              using _IV = __vec_builtin_type<int, 4>;
              const auto __xn = reinterpret_cast<_IV>(__xi);
              const auto __yn = reinterpret_cast<_IV>(__yi);
              const auto __xp = __xn < 0 ? -(__xn & 0x7fff'ffff) : __xn;
              const auto __yp = __yn < 0 ? -(__yn & 0x7fff'ffff) : __yn;
              return reinterpret_cast<_IV>(_mm_cmpord_ps(__xi, __yi)) & (__xp <= __yp);
            }
          else if constexpr (_Flags._M_have_sse2() and __vec_builtin_sizeof<_TV, 8, 16>)
            return __vec_builtin_type<long long, 2>{
              -_mm_ucomige_sd(__yi, __xi),
              -_mm_ucomige_sd(_mm_unpackhi_pd(__yi, __yi), _mm_unpackhi_pd(__xi, __xi))};
          else
            return _Base::_S_islessequal(__x, __y);
        }

      template <__vec_builtin _TV>
        requires std::floating_point<__value_type_of<_TV>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_TV>
        _S_islessgreater(_TV __x, _TV __y)
        {
          const auto __xi = __to_x86_intrin(__x);
          const auto __yi = __to_x86_intrin(__y);
          if constexpr (_S_use_bitmasks)
            {
              constexpr auto __k1 = _Abi::template _S_implicit_mask<__value_type_of<_TV>>;
              if constexpr (__vec_builtin_sizeof<_TV, 8, 64>)
                return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64>)
                return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512fp16())
                return _mm512_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm256_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 2, 16> and _Flags._M_have_avx512fp16()
                                   and _Flags._M_have_avx512vl())
                return _mm_mask_cmp_ph_mask(__k1, __xi, __yi, _CMP_NEQ_OQ);
              else
                static_assert(false);
            }
          else if constexpr (_Flags._M_have_avx())
            {
              if constexpr (__vec_builtin_sizeof<_TV, 8, 32>)
                return _mm256_cmp_pd(__xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32>)
                return _mm256_cmp_ps(__xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 8, 16>)
                return _mm_cmp_pd(__xi, __yi, _CMP_NEQ_OQ);
              else if constexpr (__vec_builtin_sizeof<_TV, 4, 16>)
                return _mm_cmp_ps(__xi, __yi, _CMP_NEQ_OQ);
              else
                static_assert(false);
            }
          else if constexpr (__vec_builtin_sizeof<_TV, 8, 16> and _Flags._M_have_sse2())
            return __vec_and(_mm_cmpord_pd(__xi, __yi), _mm_cmpneq_pd(__xi, __yi));
          else if constexpr (__vec_builtin_sizeof<_TV, 4, 16> and _Flags._M_have_sse())
            return __vec_and(_mm_cmpord_ps(__xi, __yi), _mm_cmpneq_ps(__xi, __yi));
          else
            static_assert(false);
        }

      template <std::unsigned_integral _Kp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_get(_Kp __k, _SimdSizeType __i)
        { return ((__k >> __i) & 1) == 1; }

      using _Base::_S_get;

      template <std::unsigned_integral _Kp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_set(_Kp& __k, _SimdSizeType __i, bool __x)
        {
          const _Kp __bit = _Kp(1) << __i;
          if (__x)
            __k |= __bit;
          else
            __k &= ~__bit;
        }

      using _Base::_S_set;

      static constexpr bool _S_have_sse3 = _Flags._M_have_sse3();

      static constexpr bool _S_have_ssse3 = _Flags._M_have_ssse3();

#ifndef __clang__
      template <typename _Tp>
        requires(std::__has_single_bit(_S_size) and _S_size >= 2
                   and _S_size * sizeof(_Tp) <= 16
                   and ((_S_have_ssse3 and is_integral_v<_Tp>)
                          or (_S_have_sse3 and is_floating_point_v<_Tp>)))
        static constexpr _Tp
        _S_reduce(std::datapar::basic_simd<_Tp, _Abi> __x, const plus<>&)
        {
          constexpr auto _Np = _S_size;
          using _Ip = __x86_builtin_int_t<_Tp>;
          if constexpr (is_integral_v<_Tp>)
            {
              auto __xi = __to_x86_intrin(__data(__x));
              using _IV = __vec_builtin_type_bytes<_Ip, sizeof(__xi)>;
              auto __y = reinterpret_cast<_IV>(__xi);

              if constexpr (sizeof(_Tp) == 1)
                {
                  static_assert(_Np <= 16);
                  auto __z = reinterpret_cast<__v8int16>(__xi);
                  __z = (__z >> 8) + __z;
#if _GLIBCXX_SIMD_IA32_USE_HORIZONTAL_ADD
                  if constexpr (_Np > 8)
                    __z = __builtin_ia32_phaddw128(__z, __z);
                  if constexpr (_Np > 4)
                    __z = __builtin_ia32_phaddw128(__z, __z);
                  if constexpr (_Np > 2)
                    __z = __builtin_ia32_phaddw128(__z, __z);
#else
                  if constexpr (_Np > 8)
                    __z += reinterpret_cast<__v8int16>(
                             __builtin_ia32_psrldqi128(__to_x86_intrin(__z), 64));
                  if constexpr (_Np > 4)
                    __z += __builtin_ia32_pshuflw(__z, 0b11'10'11'10);
                  if constexpr (_Np > 2)
                    __z += __builtin_ia32_pshuflw(__z, 0b11'10'01'01);
#endif
                  return static_cast<_Tp>(__z[0]);
                }

              else if constexpr (sizeof(_Tp) == 2)
                {
                  static_assert(_Np <= 8);
#if _GLIBCXX_SIMD_IA32_USE_HORIZONTAL_ADD
                  if constexpr (_Np > 4)
                    __y = __builtin_ia32_phaddw128(__y, __y);
                  if constexpr (_Np > 2)
                    __y = __builtin_ia32_phaddw128(__y, __y);
                  return __builtin_ia32_phaddw128(__y, __y)[0];
#else
                  if constexpr (_Np > 4)
                    __y += reinterpret_cast<_IV>(
                             __builtin_ia32_psrldqi128(__to_x86_intrin(__y), 64));
                  if constexpr (_Np > 2)
                    __y += __builtin_ia32_pshuflw(__y, 0b11'10'11'10);
                  return (__y + __builtin_ia32_pshuflw(__y, 0b11'10'01'01))[0];
#endif
                }

              else if constexpr (sizeof(_Tp) == 4)
                {
                  static_assert(_Np <= 4);
#if _GLIBCXX_SIMD_IA32_USE_HORIZONTAL_ADD
                  if constexpr (_Np > 2)
                    __y = __builtin_ia32_phaddd128(__y, __y);
                  return __builtin_ia32_phaddd128(__y, __y)[0];
#else
                  if constexpr (_Np > 2)
                    __y += reinterpret_cast<_IV>(
                             __builtin_ia32_psrldqi128(__to_x86_intrin(__y), 64));
                  return (__y + __builtin_ia32_pshufd(__y, 0b11'10'01'01))[0];
#endif
                }

              else if constexpr (sizeof(_Tp) == 8)
                {
                  static_assert(_Np == 2);
                  return __y[0] + __y[1];
                }

              else
                static_assert(false);
            }
          else if constexpr (is_floating_point_v<_Tp>)
            {
              auto __xi = __vec_zero_pad_to_16(__data(__x));
              using _FV = __vec_builtin_type_bytes<__x86_builtin_fp_t<_Tp>, sizeof(__xi)>;
              auto __y = reinterpret_cast<_FV>(__xi);

              if constexpr (sizeof(_Tp) == 4)
                {
                  static_assert(_Np <= 4);
#if _GLIBCXX_SIMD_IA32_USE_HORIZONTAL_ADD
                  if constexpr (_Np > 2)
                    __y = __builtin_ia32_haddps(__y, __y);
                  return __builtin_ia32_haddps(__y, __y)[0];
#else
                  if constexpr (_Np > 2)
                    __y += __builtin_ia32_movhlps(__y, __y);
                  return __builtin_ia32_addss(__y, __builtin_ia32_shufps(
                                                     __y, __y, 0b11'10'01'01))[0];
#endif
                }

              else if constexpr (_Np == 2 and sizeof(_Tp) == 8)
#if _GLIBCXX_SIMD_IA32_USE_HORIZONTAL_ADD
                return __builtin_ia32_haddpd(__y, __y)[0];
#else
                return __builtin_ia32_addsd(__y, __builtin_ia32_shufpd(__y, __y, 0b01'01))[0];
#endif

              else
                static_assert(false);
            }
          else
            static_assert(false);
        }
#endif // __clang__

      template <std::unsigned_integral _Kp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _BitMask<_S_size>
        _S_to_bits(_Kp __x)
        { return __x; }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr auto
        _S_to_bits(_TV __x)
        {
          if (__builtin_is_constant_evaluated())
            return _Base::_S_to_bits(__x);

          else if constexpr (_S_use_bitmasks)
            {
              using _Tp = __value_type_of<_TV>;
              const auto __k = reinterpret_cast<__x86_builtin_int_t<_Tp>>(__x);
              constexpr bool __bwvl = _Flags._M_have_avx512bw() and _Flags._M_have_avx512vl();
              constexpr bool __dqvl = _Flags._M_have_avx512dq() and _Flags._M_have_avx512vl();
              _BitMask<_S_size> __m; // not sanitized

              if constexpr (__vec_builtin_sizeof<_TV, 1, 64> and _Flags._M_have_avx512bw())
                __m = __builtin_ia32_cvtb2mask512(__k);

              else if constexpr (__vec_builtin_sizeof<_TV, 1, 32> and __bwvl)
                __m = __builtin_ia32_cvtb2mask256(__k);

              else if constexpr (__vec_builtin_sizeof<_TV, 1> and __bwvl)
                __m = __builtin_ia32_cvtb2mask128(__vec_zero_pad_to_16(__k));

              else if constexpr (__vec_builtin_sizeof<_TV, 2, 64> and _Flags._M_have_avx512bw())
                __m = __builtin_ia32_cvtw2mask512(__k);

              else if constexpr (__vec_builtin_sizeof<_TV, 2, 32> and __bwvl)
                __m = __builtin_ia32_cvtw2mask256(__k);

              else if constexpr (__vec_builtin_sizeof<_TV, 2> and __bwvl)
                __m = __builtin_ia32_cvtw2mask128(__vec_zero_pad_to_16(__k));

              else if constexpr (__vec_builtin_sizeof<_TV, 4, 64> and _Flags._M_have_avx512dq())
                __m = __builtin_ia32_cvtd2mask512(__k);

              else if constexpr (__vec_builtin_sizeof<_TV, 4, 32> and __dqvl)
                __m = __builtin_ia32_cvtd2mask256(__k);

              else if constexpr (__vec_builtin_sizeof<_TV, 4> and __dqvl)
                __m = __builtin_ia32_cvtd2mask128(__vec_zero_pad_to_16(__k));

              else if constexpr (__vec_builtin_sizeof<_TV, 8, 64> and _Flags._M_have_avx512dq())
                __m = __builtin_ia32_cvtq2mask512(__k);

              else if constexpr (__vec_builtin_sizeof<_TV, 8, 32> and __dqvl)
                __m = __builtin_ia32_cvtq2mask256(__k);

              else if constexpr (__vec_builtin_sizeof<_TV, 8> and __dqvl)
                __m = __builtin_ia32_cvtq2mask128(__vec_zero_pad_to_16(__k));

              else
                __m = _S_not_equal_to(__x, _TV());

              return __m;
            }

          else
            return _Base::_S_to_bits(__x);
        }

      template <std::unsigned_integral _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_logical_and(_Tp __x, _Tp __y)
        { return __x & __y; }

      using _Base::_S_logical_and;

      template <std::unsigned_integral _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_logical_or(_Tp __x, _Tp __y)
        { return __x | __y; }

      using _Base::_S_logical_or;

      template <std::unsigned_integral _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_not(_Tp __x)
        { return ~__x; }

      using _Base::_S_bit_not;

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static bool
        _S_all_of(const std::datapar::basic_simd_mask<_Bs, _Abi> __k)
        {
          using _Tp = __mask_integer_from<_Bs>;
          if constexpr (_S_use_bitmasks)
            {
              constexpr auto __sizemask = _Abi::template _S_implicit_mask<_Tp>;
              const auto __kk = __data(__k);
              if constexpr (sizeof(__kk) == 1)
                {
                  if constexpr (_Flags._M_have_avx512dq())
                    return __builtin_ia32_kortestcqi(
                             __kk, __sizemask == 0xff ? __kk : uint8_t(~__sizemask));
                  else
                    return __builtin_ia32_kortestchi(__kk, uint16_t(~__sizemask));
                }
              else if constexpr (sizeof(__kk) == 2)
                return __builtin_ia32_kortestchi(
                         __kk, __sizemask == 0xffff ? __kk : uint16_t(~__sizemask));
              else if constexpr (sizeof(__kk) == 4 and _Flags._M_have_avx512bw())
                return __builtin_ia32_kortestcsi(
                         __kk, __sizemask == 0xffffffffU ? __kk : uint32_t(~__sizemask));
              else if constexpr (sizeof(__kk) == 8 and _Flags._M_have_avx512bw())
                return __builtin_ia32_kortestcdi(
                         __kk, __sizemask == 0xffffffffffffffffULL ? __kk : uint64_t(~__sizemask));
              else
                static_assert(false);
            }
          else
            {
              static_assert(sizeof(__k) <= 32);
              if constexpr (_Flags._M_have_sse4_1())
                return 0 != _S_testc(__data(__k), _Abi::template _S_implicit_mask<_Tp>);
              else
                {
                  constexpr int __valid_bits = (1 << (_Bs == 2 ? __k.size * 2 : __k.size)) - 1;
                  return (__movmsk(__data(__k)) & __valid_bits) == __valid_bits;
                }
            }
        }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static bool
        _S_any_of(const std::datapar::basic_simd_mask<_Bs, _Abi> __k)
        {
          using _Tp = __mask_integer_from<_Bs>;
          if constexpr (_S_use_bitmasks)
            return (__data(__k) & _Abi::template _S_implicit_mask<_Tp>) != 0;
          else
            {
              static_assert(sizeof(__k) <= 32);
              if constexpr (_Flags._M_have_sse4_1())
                {
                  if constexpr (_Abi::_S_is_partial || sizeof(__k) < 16)
                    return 0 == _S_testz(__data(__k), _Abi::template _S_implicit_mask<_Tp>);
                  else
                    return not _S_is_zero(__data(__k));
                }
              else
                {
                  constexpr int __valid_bits = (1 << (_Bs == 2 ? __k.size * 2 : __k.size)) - 1;
                  return (__movmsk(__data(__k)) & __valid_bits) != 0;
                }
            }
        }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static bool
        _S_none_of(const std::datapar::basic_simd_mask<_Bs, _Abi> __k)
        {
          if constexpr (_S_use_bitmasks)
            return _Abi::_S_masked(__data(__k)) == 0;

          else
            {
              using _Tp = __mask_integer_from<_Bs>;
              if constexpr (_Flags._M_have_sse4_1())
                {
                  if constexpr (_Abi::_S_is_partial || sizeof(__k) < 16)
                    return 0 != _S_testz(__data(__k), _Abi::template _S_implicit_mask<_Tp>);
                  else
                    return _S_is_zero(__data(__k));
                }
              else
                {
                  constexpr int __valid_bits = (1 << (_Bs == 2 ? __k.size * 2 : __k.size)) - 1;
                  return (__movmsk(__data(__k)) & __valid_bits) == 0;
                }
            }
        }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static int
        _S_popcount(std::datapar::basic_simd_mask<_Bs, _Abi> __k)
        {
          const auto __kk = _Abi::_S_masked(__data(__k));
          if constexpr (_S_use_bitmasks)
            {
              if constexpr (__k.size > 32)
                return __builtin_popcountll(__kk);
              else
                return __builtin_popcount(__kk);
            }
          else
            {
              const int __bits = __movmsk(__kk);
              if constexpr (_Flags._M_have_popcnt())
                {
                  const int __count = __builtin_popcount(__bits);
                  return _Bs == 2 ? __count / 2 : __count;
                }

              else if constexpr (__k.size == 2 && _Bs != 2)
                return __bits - (__bits >> 1);

              else if constexpr (__k.size == 2 && _Bs == 2)
                {
                  const int __unmasked_bits = __movmsk(__data(__k));
                  return (__unmasked_bits & 1) + (__unmasked_bits >> 3);
                }

              else if constexpr (__k.size <= 4 and _Bs == 8)
                {
                  auto __x = -(__vec_lo128(__kk) + __vec_hi128(__kk));
                  return __x[0] + __x[1];
                }

              else if constexpr (_Bs == 1 and _S_size <= 16)
                {
                  constexpr auto __one_mask
                    = _Abi::_S_masked(__vec_broadcast<_S_size, __mask_integer_from<_Bs>>(1));
                  auto __ones = __data(__k) & __one_mask;
                  int __i32;
                  if constexpr (_S_size > 8)
                    {
                      auto __i64 = reinterpret_cast<__v2int64>(__ones);
                      __i64[0] += __i64[1];
                      __i32 = (__i64[0] >> 32) + __i64[0];
                    }
                  else if constexpr (_S_size > 4)
                    {
                      auto __i64 = __builtin_bit_cast(int64_t, __ones);
                      __i32 = (__i64 >> 32) + __i64;
                    }
                  else
                    {
                      static_assert(_S_size > 2);
                      __i32 = __builtin_bit_cast(int, __ones);
                    }
                  __i32 += __i32 >> 16;
                  __i32 += __i32 >> 8;
                  return __i32 & 0xff;
                }

              else if constexpr (_Bs == 2 and _S_size <= 8)
                {
                  constexpr auto __one_mask
                    = _Abi::_S_masked(__vec_broadcast<_S_size, __mask_integer_from<_Bs>>(1));
                  auto __ones = _S_is_partial ? (__data(__k) & __one_mask) : __data(+__k);
                  int __i32;
                  if constexpr (_S_size > 4)
                    {
                      auto __i64 = reinterpret_cast<__v2int64>(__ones);
                      __i64[0] += __i64[1];
                      __i32 = (__i64[0] >> 32) + __i64[0];
                    }
                  else
                    {
                      static_assert(_S_size > 2);
                      auto __i64 = __builtin_bit_cast(int64_t, __ones);
                      __i32 = (__i64 >> 32) + __i64;
                    }
                  __i32 += __i32 >> 16;
                  return __i32 & 0xffff;
                }

              else if constexpr (_Bs == 4 and _S_size <= 4)
                {
                  constexpr auto __one_mask
                    = _Abi::_S_masked(__vec_broadcast<_S_size, __mask_integer_from<_Bs>>(1));
                  auto __ones = _S_is_partial ? (__data(__k) & __one_mask) : __data(+__k);
                  static_assert(_S_size > 2);
                  auto __i64 = reinterpret_cast<__v2int64>(__ones);
                  __i64[0] += __i64[1];
                  int __i32 = (__i64[0] >> 32) + __i64[0];
                  return __i32;
                }

              else if constexpr (_Flags._M_have_sse2())
                return -std::datapar::reduce(-__k);

              else
                return __builtin_popcount(__bits);
            }
        }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static int
        _S_reduce_min_index(std::datapar::basic_simd_mask<_Bs, _Abi> __k)
        {
          if constexpr (_S_use_bitmasks)
            return __lowest_bit(__data(__k));
          else
            {
              const uint32_t __bits = __movmsk(__data(__k));
              if constexpr (_Bs == 2)
                return __lowest_bit(__bits) / 2;
              else
                return __lowest_bit(__bits);
            }
        }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static int
        _S_reduce_max_index(std::datapar::basic_simd_mask<_Bs, _Abi> __k)
        {
          if constexpr (_S_use_bitmasks)
            return __highest_bit(_Abi::_S_masked(__data(__k)));
          else
            {
              uint32_t __bits = __movmsk(__data(__k));
              if constexpr (__k.size * _Bs != 16 and __k.size * _Bs < 32)
                __bits &= (1u << __k.size * (1 + (_Bs == 2))) - 1u;
              if constexpr (_Bs == 2)
                return __highest_bit(__bits) / 2;
              else
                return __highest_bit(__bits);
            }
        }

      template <__vectorizable_canon _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr typename _Abi::template _MaskMember<_Tp>
        _S_mask_with_n_true(auto __n)
        {
          __glibcxx_simd_precondition(unsigned(__n) < _S_size, "");
          if constexpr (not _S_use_bitmasks)
            return _Base::template _S_mask_with_n_true<_Tp>(__n);
          else if constexpr (_S_size <= 32 and _Flags._M_have_bmi2())
            return __builtin_ia32_bzhi_si(~0u >> (32 - _S_size), unsigned(__n));
          else if constexpr (_S_size <= 64 and _Flags._M_have_bmi2())
            return __builtin_ia32_bzhi_di(~0ull >> (64 - _S_size), unsigned(__n));
          else if constexpr (_S_size <= 32)
            return (1u << unsigned(__n)) - 1;
          else if constexpr (_S_size <= 64)
            return (1ull << unsigned(__n)) - 1;
          else
            static_assert(false);
        }

      // AVX-512 needs to generate a bit-mask
      template <__vectorizable_canon _Tp, typename _Fp>
        requires requires { typename _Abi::_MaskInteger; }
        _GLIBCXX_SIMD_INTRINSIC static constexpr auto
        _S_mask_generator(_Fp&& __gen)
        {
          return static_cast<_Abi::_MaskInteger>(
                   _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                     return ((uint64_t(bool(__gen(__ic<_Is>))) << _Is) | ...);
                   }));
        }

    private:
      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_is_zero(_TV __a)
        {
          if (not __builtin_is_constant_evaluated())
            {
              const auto __ai = __to_x86_intrin(__a);
              if constexpr (_Flags._M_have_avx())
                {
                  if constexpr (sizeof(_TV) == 32)
                    {
                      if constexpr (not is_floating_point_v<__value_type_of<_TV>>)
                        return _mm256_testz_si256(__ai, __ai);
                      else if constexpr (sizeof(__value_type_of<_TV>) == 8)
                        return _mm256_testz_pd(__a, __a);
                      else if constexpr (sizeof(__value_type_of<_TV>) == 4)
                        return _mm256_testz_ps(__a, __a);
                      else // TODO:ph
                        static_assert(false);
                    }
                  else if constexpr (sizeof(_TV) <= 16)
                    {
                      if constexpr (not is_floating_point_v<__value_type_of<_TV>>)
                        return _mm_testz_si128(__ai, __ai);
                      else if constexpr (sizeof(__value_type_of<_TV>) == 8)
                        return _mm_testz_pd(__a, __a);
                      else if constexpr (sizeof(__value_type_of<_TV>) == 4)
                        return _mm_testz_ps(__ai, __ai);
                      else // TODO:ph
                        static_assert(false);
                    }
                  else
                    static_assert(false);
                }
              else if constexpr (_Flags._M_have_sse4_1())
                return _mm_testz_si128(reinterpret_cast<__m128i>(__ai),
                                       reinterpret_cast<__m128i>(__ai));
              else
                static_assert(false);
            }
          else if constexpr (sizeof(_TV) <= 8)
            return __builtin_bit_cast(__make_signed_int_t<_TV>, __a) == 0;
          else
            {
              const auto __b
                = reinterpret_cast<__vec_builtin_type_bytes<long long, sizeof(_TV)>>(__a);
              if constexpr (sizeof(__b) == 16)
                return (__b[0] | __b[1]) == 0;
              else if constexpr (sizeof(__b) == 32)
                return _S_is_zero(__vec_lo128(__b) | __vec_hi128(__b));
              else if constexpr (sizeof(__b) == 64)
                return _S_is_zero(__vec_lo256(__b) | __vec_hi256(__b));
              else
                static_assert(false);
            }
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr int
        _S_testz(_TV __a, _TV __b)
        {
          if (__builtin_is_constant_evaluated())
            return _S_is_zero(__vec_and(__a, __b));
          else
            {
              using _Tp = __value_type_of<_TV>;
              const auto __ai = __to_x86_intrin(__a);
              const auto __bi = __to_x86_intrin(__b);
              if constexpr (sizeof(_TV) == 32)
                {
                  if constexpr (not is_floating_point_v<_Tp>)
                    return _mm256_testz_si256(__ai, __bi);
                  else if constexpr (sizeof(_Tp) == 8)
                    return _mm256_testz_pd(__ai, __bi);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm256_testz_ps(__ai, __bi);
                  else // TODO:ph
                    static_assert(false);
                }
              else if constexpr (is_floating_point_v<_Tp> and _Flags._M_have_avx())
                {
                  if constexpr (sizeof(_Tp) == 8)
                    return _mm_testz_pd(__ai, __bi);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm_testz_ps(__ai, __bi);
                  else // TODO:ph
                    static_assert(false);
                }
              else if constexpr (_Flags._M_have_sse4_1())
                return _mm_testz_si128(reinterpret_cast<__m128i>(__ai),
                                       reinterpret_cast<__m128i>(__bi));
              else
                return __movmsk(0 == __and(__a, __b)) != 0;
            }
        }

      template <__vec_builtin _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr int
        _S_testc(_TV __a, _TV __b)
        {
          static_assert(_Flags._M_have_sse4_1());

          if (__builtin_is_constant_evaluated())
            return _S_is_zero(__vec_andnot(__a, __b));

          else
            {
              using _Tp = __value_type_of<_TV>;
              const auto __ai = __to_x86_intrin(__a);
              const auto __bi = __to_x86_intrin(__b);
              if constexpr (sizeof(_TV) == 32)
                {
                  if constexpr (not is_floating_point_v<_Tp>)
                    return _mm256_testc_si256(__ai, __bi);
                  else if constexpr (sizeof(_Tp) == 8)
                    return _mm256_testc_pd(__ai, __bi);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm256_testc_ps(__ai, __bi);
                  else // TODO:ph
                    static_assert(false);
                }
              else if constexpr (is_floating_point_v<_Tp> and _Flags._M_have_avx())
                {
                  if constexpr (sizeof(_Tp) == 8)
                    return _mm_testc_pd(__ai, __bi);
                  else if constexpr (sizeof(_Tp) == 4)
                    return _mm_testc_ps(__ai, __bi);
                  else // TODO:ph
                    static_assert(false);
                }
              else
                return _mm_testc_si128(reinterpret_cast<__m128i>(__ai),
                                       reinterpret_cast<__m128i>(__bi));
            }
        }

    };

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Width>
    struct _SimdConverter<_From, _Avx512Abi<_Width>, _To, _Avx512Abi<_Width>>
    : _SimdConverter<_From, _VecAbi<_Width>, _To, _VecAbi<_Width>>
    {};

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Width>
    struct _SimdConverter<_From, _Avx512Abi<_Width>, _To, _VecAbi<_Width>>
    : _SimdConverter<_From, _VecAbi<_Width>, _To, _VecAbi<_Width>>
    {};

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Width>
    struct _SimdConverter<_From, _VecAbi<_Width>, _To, _Avx512Abi<_Width>>
    : _SimdConverter<_From, _VecAbi<_Width>, _To, _VecAbi<_Width>>
    {};

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Width, int _Np>
    struct _SimdConverter<_From, _AbiArray<_Avx512Abi<_Width>, _Np>, _To, _Avx512Abi<_Width * _Np>>
    : _SimdConverter<_From, _AbiArray<_VecAbi<_Width>, _Np>, _To, _VecAbi<_Width * _Np>>
    {};

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Width, int _Np>
    struct _SimdConverter<_From, _AbiArray<_VecAbi<_Width>, _Np>, _To, _Avx512Abi<_Width * _Np>>
    : _SimdConverter<_From, _AbiArray<_VecAbi<_Width>, _Np>, _To, _VecAbi<_Width * _Np>>
    {};

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Width, int _Np>
    struct _SimdConverter<_From, _AbiArray<_Avx512Abi<_Width>, _Np>, _To, _VecAbi<_Width * _Np>>
    : _SimdConverter<_From, _AbiArray<_VecAbi<_Width>, _Np>, _To, _VecAbi<_Width * _Np>>
    {};

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Np, int _Mp>
    struct _SimdConverter<_From, _AbiCombine<_Np, _Avx512Abi<_Mp>>, _To, _Avx512Abi<_Np>>
    : _SimdConverter<_From, _AbiCombine<_Np, _VecAbi<_Mp>>, _To, _VecAbi<_Np>>
    {};

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Np, int _Mp>
    struct _SimdConverter<_From, _AbiCombine<_Np, _VecAbi<_Mp>>, _To, _Avx512Abi<_Np>>
    : _SimdConverter<_From, _AbiCombine<_Np, _VecAbi<_Mp>>, _To, _VecAbi<_Np>>
    {};

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Np, int _Mp>
    struct _SimdConverter<_From, _AbiCombine<_Np, _Avx512Abi<_Mp>>, _To, _VecAbi<_Np>>
    : _SimdConverter<_From, _AbiCombine<_Np, _VecAbi<_Mp>>, _To, _VecAbi<_Np>>
    {};

}
#endif // __x86_64__ or __i386__
#endif // PROTOTYPE_SIMD_X86_H_
