/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_CONVERTER_H_
#define PROTOTYPE_SIMD_CONVERTER_H_

#include "detail.h"

namespace std::__detail
{
  template <__vectorizable_canon _From, __simd_abi_tag _FAbi,
            __vectorizable_canon _To, __simd_abi_tag _TAbi>
    struct _SimdConverter;

  template <__vectorizable_canon _Tp, __simd_abi_tag _TAbi>
    struct _SimdConverter<_Tp, _TAbi, _Tp, _TAbi>
    {
      using _TV = typename _TAbi::template _SimdMember<_Tp>;

      _GLIBCXX_SIMD_INTRINSIC constexpr _TV
      operator()(_TV __x) const
      { return __x; }
    };

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Width>
    requires (not std::is_same_v<_To, _From>) and (not __bitcast_convertible<_From, _To>)
    struct _SimdConverter<_From, _VecAbi<_Width>, _To, _VecAbi<_Width>>
    {
      using _FromV = typename _VecAbi<_Width>::template _SimdMember<_From>;
      using _ToV = typename _VecAbi<_Width>::template _SimdMember<_To>;

      _GLIBCXX_SIMD_INTRINSIC constexpr _ToV
      operator()(_FromV __from) const
      { return __vec_convert<_ToV>(__from); }
    };

  template <__vectorizable_canon _From, __vectorizable_canon _To, __simd_abi_tag _Abi>
    requires __bitcast_convertible<_From, _To>
    struct _SimdConverter<_From, _Abi, _To, _Abi>
    {
      using _FromV = typename _Abi::template _SimdMember<_From>;
      using _ToV = typename _Abi::template _SimdMember<_To>;

      _GLIBCXX_SIMD_INTRINSIC constexpr _ToV
      operator()(_FromV __from) const
      { return __builtin_bit_cast(_ToV, __from); }
    };

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Width, int _Np>
    struct _SimdConverter<_From, _AbiArray<_VecAbi<_Width>, _Np>, _To, _VecAbi<_Width * _Np>>
    {
      using _FromV = typename _VecAbi<_Width>::template _SimdMember<_From>;
      using _ToV = typename _VecAbi<_Width * _Np>::template _SimdMember<_To>;

      _GLIBCXX_SIMD_INTRINSIC constexpr _ToV
      operator()(const array<_FromV, _Np>& __x) const
      { return _GLIBCXX_SIMD_INT_PACK(_Np, _Is, { return __vec_convert<_ToV>(__x[_Is]...); }); }
    };

  template <__vectorizable_canon _From, __vectorizable_canon _To, int _Np, int _Mp>
    struct _SimdConverter<_From, _AbiCombine<_Np, _VecAbi<_Mp>>, _To, _VecAbi<_Np>>
    {
      // _FromV is a _SimdTuple specialization
      using _FromV = typename _AbiCombine<_Np, _VecAbi<_Mp>>::template _SimdMember<_From>;
      using _ToV = typename _VecAbi<_Np>::template _SimdMember<_To>;

      _GLIBCXX_SIMD_INTRINSIC constexpr _ToV
      operator()(const _FromV& __x) const
      {
        constexpr int __tail_size = _FromV::_S_tail_size;
        if constexpr (sizeof(_FromV) == __bit_ceil(_Np) * sizeof(_From))
          {
            return __vec_convert<_ToV>(__builtin_bit_cast(
                                         __vec_builtin_type<_From, __bit_ceil(_Np)>, __x));
          }
        else if constexpr (__tail_size == 1)
          {
            using _To0V = typename _VecAbi<_FromV::_S_size0>::template _SimdMember<_To>;
            _SimdConverter<_From, typename _FromV::_Abi0, _To, _VecAbi<_FromV::_S_size0>> __cvt0;
            const _To0V __cvted0 = __cvt0(__x._M_x);
            const _To0V __cvted1 = {static_cast<_To>(__x._M_tail._M_x)};
            return __vec_concat(__cvted0, __cvted1);
          }
        else
          {
            static_assert(__tail_size > 0);
            _SimdConverter<_From, typename _FromV::_Abi0, _To, _VecAbi<_FromV::_S_size0>> __cvt0;
            _SimdConverter<_From, typename _FromV::_AbiTail, _To, _VecAbi<__tail_size>> __cvt1;
            const auto __cvted0 = __cvt0(__x._M_x);
            const auto __cvted1 = __cvt1(__x._M_tail);
            return __vec_concat(__cvted0, __vec_zero_pad_to<sizeof(__cvted0)>(__cvted1));
          }
        //return _GLIBCXX_SIMD_INT_PACK(_Mp, _Is, { return __vec_convert<_ToV>(__x[_Is]...); }); }
      }
    };

  // fallback (not optimized)
  template <__vectorizable_canon _From, __simd_abi_tag _FAbi,
            __vectorizable_canon _To, __simd_abi_tag _TAbi>
    struct _SimdConverter
    {
      using _Impl = typename _TAbi::_SimdImpl;
      using _FromImpl = typename _FAbi::_SimdImpl;

      [[deprecated("please optimize")]]
      _GLIBCXX_SIMD_INTRINSIC constexpr auto
      operator()(typename _FAbi::template _SimdMember<_From> const& __x) const
      {
        constexpr auto _NFrom = _FAbi::_S_size;
        constexpr auto _NTo = _TAbi::_S_size;
        static_assert(_NFrom >= _NTo);
        return _Impl::template _S_generator<_To>([&] [[__gnu__::__always_inline__]] (auto __i) {
                 return _FromImpl::_S_get(__x, __i);
               });
      }
    };
}

#endif // PROTOTYPE_SIMD_CONVERTER_H_
