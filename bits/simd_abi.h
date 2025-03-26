/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_ABI_H_
#define PROTOTYPE_SIMD_ABI_H_

#include "simd_converter.h"
#include "simd_scalar.h"
#include "simd_builtin.h"
#include "simd_x86.h"
#include "detail.h"

#include <cstdint>

namespace std::__detail
{
  template <int _Bytes>
    using _ScalarAbiWrapper = _ScalarAbi;

  template <template <int> class...>
    struct _AbiList
    {
      template <typename, int> static constexpr bool _S_has_valid_abi = false;
      template <typename, int> using _FirstValidAbi = void;
      template <typename, int> using _BestPartialAbi = void;
    };

  template <template <int> class _A0, template <int> class... _Rest>
    struct _AbiList<_A0, _Rest...>
    {
      template <typename _Tp, int _Np>
        static constexpr bool _S_A0_is_valid
          = _A0<_Np>::template _IsValid<_Tp>::value and _A0<_Np>::_S_size == _Np;

      template <typename _Tp, int _Np>
        static constexpr bool _S_has_valid_abi
          = _S_A0_is_valid<_Tp, _Np> or _AbiList<_Rest...>::template _S_has_valid_abi<_Tp, _Np>;

      template <typename _Tp, int _Np>
        using _FirstValidAbi = conditional_t<
                                 _S_A0_is_valid<_Tp, _Np>, _A0<_Np>,
                                 typename _AbiList<_Rest...>::template _FirstValidAbi<_Tp, _Np>>;

      template <typename _Tp, int _Np>
        static consteval auto
        _S_find_next_valid_abi()
        {
          constexpr int _Next = std::__bit_ceil(_Np) / 2;
          using _NextAbi = _A0<_Next>;
          if constexpr (_Next <= 1) // break recursion
            return _A0<_Np>();
          else if constexpr (_NextAbi::_S_is_partial == false
                               and _NextAbi::template _IsValid<_Tp>::value)
            return _NextAbi();
          else
            return _S_find_next_valid_abi<_Tp, _Next>();
        }

      template <typename _Tp, int _Np>
        static constexpr auto
        _S_determine_best_abi()
        {
          static_assert(_Np >= 1);
          if constexpr (_Np == 1)
            return _ScalarAbi();
          else
            {
              constexpr int __fullsize = _A0<_Np>::_S_full_size;
              constexpr bool __excess_padding = __fullsize / 2 >= _Np;
              // _A0<_Np> is good if:
              // 1. The ABI tag is valid for _Tp
              // 2. The storage overhead is no more than padding to fill the next
              //    power-of-2 number of bytes
              if constexpr (_S_A0_is_valid<_Tp, _Np> and not __excess_padding)
                return _A0<_Np>();
              else if constexpr (__fullsize == 1)
                return _A0<_Np>();
              else
                {
                  using _Bp = decltype(_S_find_next_valid_abi<_Tp, _Np>());
                  if constexpr (_Bp::template _IsValid<_Tp>::value
                                  and _Bp::_S_size <= _Np)
                    return _Bp{};
                  else
                    return _AbiList<_Rest...>::template _S_determine_best_abi<_Tp, _Np>();
                }
            }
        }

          /**
           * Alias for the best ABI tag that can either hold all _Np values of type _Tp or as much
           * as possible.
           */
      template <typename _Tp, int _Np>
        using _BestPartialAbi = decltype(_S_determine_best_abi<_Tp, _Np>());
    };

  using _AllNativeAbis = _AbiList<_Avx512Abi, _VecAbi, _ScalarAbiWrapper>;

  template <typename _Abi0, _BuildFlags _Flags, size_t... _Is>
    struct _SimdImplArray;

  template <typename _Abi0, _BuildFlags _Flags, size_t... _Is>
    struct _MaskImplArray;

  template <_SimdSizeType _Np, typename _Tag, _BuildFlags = {}>
    struct _SimdImplAbiCombine;

  template <_SimdSizeType _Np, typename _Tag, _BuildFlags = {}>
    struct _MaskImplAbiCombine;

  template <__vectorizable_canon _Tp, __valid_abi_tag<_Tp>... _As>
    struct _SimdTuple;

  template <typename _Tp, _SimdSizeType _Np>
    struct _NextAbiTuple
    {
      using _Native = _AllNativeAbis::_BestPartialAbi<_Tp, _Np>;
      static constexpr int _S_native_size = __simd_size_v<_Tp, _Native>;
      static constexpr int _S_array_size = _Np / _S_native_size;
      using type
        = std::conditional_t<_S_array_size >= 2, _AbiArray<_Native, _S_array_size>, _Native>;
    };

  template <typename _Tp, int _Np, typename _Tuple,
            typename _Next = typename _NextAbiTuple<_Tp, _Np>::type,
            int _Remain = _Np - int(__simd_size_v<_Tp, _Next>)>
    struct __fixed_size_storage_builder;

  template <typename _Tp, int _Np, typename... _As, typename _Next>
    struct __fixed_size_storage_builder<_Tp, _Np, _SimdTuple<_Tp, _As...>, _Next, 0>
    { using type = _SimdTuple<_Tp, _As..., _Next>; };

  template <typename _Tp, int _Np, typename... _As, typename _Next, int _Remain>
    struct __fixed_size_storage_builder<_Tp, _Np, _SimdTuple<_Tp, _As...>, _Next, _Remain>
    {
      using type = typename __fixed_size_storage_builder<
                     _Tp, _Remain, _SimdTuple<_Tp, _As..., _Next>>::type;
    };

  template <typename _Tp, int _Np>
    using __fixed_size_storage_t
      = typename __fixed_size_storage_builder<_Tp, _Np, _SimdTuple<_Tp>>::type;
}

namespace std
{
  template <typename _Abi0, int _Np>
    struct _AbiArray
    {
      using _Abi0Type = _Abi0;

      using _SimdSizeType = __detail::_SimdSizeType;

      static constexpr _SimdSizeType _S_abiarray_size = _Np;

      static constexpr _SimdSizeType _S_size = _Np * _Abi0::_S_size;

      static constexpr _SimdSizeType _S_full_size = _Np * _Abi0::_S_size;

      static constexpr bool _S_is_partial = false;

      // consider simd<float>::mask_type::operator- on AVX w/o AVX2:
      // This cannot be 'n * Abi0::_S_size' because otherwise mask cannot be implemented.
      static constexpr _SimdSizeType _S_max_size = 256;

      struct _IsValidAbiTag
      : bool_constant<_Abi0::_IsValidAbiTag::value and not _Abi0::_S_is_partial
                        and _Np >= 2 and _S_size <= _S_max_size>
      {};

      template <typename _Tp>
        using _IsValidSizeFor = _Abi0::template _IsValidSizeFor<_Tp>;

      template <typename _Tp>
        struct _IsValid
        : conjunction<_IsValidAbiTag, __detail::__is_vectorizable<_Tp>, _IsValidSizeFor<_Tp>>
        {};

      using _SimdImpl = decltype([]<size_t... _Is>(index_sequence<_Is...>)
                                   -> __detail::_SimdImplArray<_Abi0, {}, _Is...> {
                                     return {};
                                   }(make_index_sequence<_Np>()));

      using _MaskImpl = decltype([]<size_t... _Is>(index_sequence<_Is...>)
                                   -> __detail::_MaskImplArray<_Abi0, {}, _Is...> {
                                     return {};
                                   }(make_index_sequence<_Np>()));

      template <__detail::__vectorizable_canon _Tp>
        using _SimdMember = std::array<typename _Abi0::template _SimdMember<_Tp>, _Np>;

      template <__detail::__vectorizable_canon _Tp>
        using _MaskMember = std::array<typename _Abi0::template _MaskMember<_Tp>, _Np>;

      template <typename _Tp>
        struct __traits
        : __detail::_InvalidTraits
        {};

      template <typename _Tp>
        requires _IsValid<_Tp>::value
        struct __traits<_Tp>
        {
          // conversions to _AbiArray should be explicit unless _FromAbi is _AbiCombine
          template <typename _FromAbi>
            static constexpr bool _S_explicit_mask_conversion
              = not is_same_v<_FromAbi, _AbiCombine<_S_size, __detail::_NativeAbi<_Tp>>>;

          using _SimdImpl = _AbiArray::_SimdImpl;

          using _MaskImpl = _AbiArray::_MaskImpl;

          using _SimdMember = std::array<typename _Abi0::template _SimdMember<_Tp>, _Np>;

          using _MaskMember = std::array<typename _Abi0::template _MaskMember<_Tp>, _Np>;

          static constexpr size_t _S_simd_align = alignof(_SimdMember);

          static constexpr size_t _S_mask_align = alignof(_MaskMember);

          static constexpr _SimdSizeType _S_size = _Np * _Abi0::_S_size;

          static constexpr _SimdSizeType _S_full_size = _Np * _Abi0::_S_size;

          static constexpr bool _S_is_partial = false;

          class _MaskCastType
          {};

          class _SimdCastType
          {};
        };
    };

  template <__detail::_SimdSizeType _Np, typename _Tag>
    struct _AbiCombine
    {
      using _AbiCombineTag = void;

      static constexpr __detail::_SimdSizeType _S_size = _Np;

      static constexpr __detail::_SimdSizeType _S_full_size = _Np;

      struct _IsValidAbiTag
      : public bool_constant<(_Np > 1)>
      {};

      template <typename _Tp>
        static constexpr __detail::_SimdSizeType _S_max_size = 64;

      template <typename _Tp>
        struct _IsValidSizeFor
        : bool_constant<(_Np <= _S_max_size<_Tp>)>
        {};

      template <typename _Tp>
        struct _IsValid
        : conjunction<_IsValidAbiTag, __detail::__is_vectorizable<_Tp>, _IsValidSizeFor<_Tp>>
        {};

      _GLIBCXX_SIMD_INTRINSIC static constexpr __detail::_SanitizedBitMask<_Np>
      _S_masked(__detail::_BitMask<_Np> __x)
      { return __x._M_sanitized(); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr __detail::_SanitizedBitMask<_Np>
      _S_masked(__detail::_SanitizedBitMask<_Np> __x)
      { return __x; }

      struct _CommonImpl {};

      using _SimdImpl = __detail::_SimdImplAbiCombine<_Np, _Tag>;

      using _MaskImpl = __detail::_MaskImplAbiCombine<_Np, _Tag>;

      template <__detail::__vectorizable_canon _Tp>
        using _SimdMember = __detail::__fixed_size_storage_t<_Tp, _Np>;

      template <__detail::__vectorizable_canon>
        using _MaskMember = __detail::_SanitizedBitMask<_Np>;

      template <typename _Tp>
        struct __traits
        : __detail::_InvalidTraits
        {};

      template <typename _Tp>
        requires _IsValid<_Tp>::value
        struct __traits<_Tp>
        {
          // conversions to _AbiCombine should always be explicit
          template <typename _FromAbi>
            static constexpr bool _S_explicit_mask_conversion = true;

          using _SimdImpl = __detail::_SimdImplAbiCombine<_Np, _Tag>;

          using _MaskImpl = __detail::_MaskImplAbiCombine<_Np, _Tag>;

          using _SimdMember = __detail::__fixed_size_storage_t<_Tp, _Np>;

          using _MaskMember = __detail::_SanitizedBitMask<_Np>;

          static constexpr __detail::_SimdSizeType _S_size = _Np;

          static constexpr __detail::_SimdSizeType _S_full_size = _Np;

          static constexpr size_t _S_simd_align = alignof(_SimdMember);

          static constexpr size_t _S_mask_align = alignof(_MaskMember);

          struct _SimdCastType
          {
            _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
            _SimdCastType(const array<_Tp, _Np>& __a)
            { __builtin_memcpy(&_M_data, __a.data(), _Np * sizeof(_Tp)); }

            _GLIBCXX_SIMD_ALWAYS_INLINE constexpr
            _SimdCastType(const _SimdMember& __dd)
            : _M_data(__dd)
            {}

            _GLIBCXX_SIMD_ALWAYS_INLINE constexpr explicit
            operator const _SimdMember &() const { return _M_data; }

          private:
            _SimdMember _M_data;
          };

          class _MaskCastType
          {
            _MaskCastType() = delete;
          };
        };
    };
}

namespace std::__detail
{
  template <typename _Abi0, _BuildFlags _Flags, size_t... _Is>
    struct _SimdImplArray
    {
      static constexpr _SimdSizeType _Np = sizeof...(_Is);

      using abi_type = _AbiArray<_Abi0, _Np>;

      template <typename _Tp>
        using _TypeTag = _Tp*;

      template <__vectorizable_canon _Tp>
        using _SimdMember = typename abi_type::template _SimdMember<_Tp>;

      template <typename _Tp> // std::array of __vec_builtin
        using _ValueTypeOf = __value_type_of<typename _Tp::value_type>;

      template <typename _Tp>
        struct _MaskMemberImpl
        { using type = typename abi_type::template _MaskMember<_ValueTypeOf<_Tp>>; };

      template <__vectorizable_canon _Tp>
        struct _MaskMemberImpl<_Tp>
        { using type = typename abi_type::template _MaskMember<_Tp>; };

      template <typename _Tp>
        using _MaskMember = typename _MaskMemberImpl<_Tp>::type;

      static constexpr _SimdSizeType _S_size = abi_type::_S_size;

      static constexpr _SimdSizeType _S_chunk_size = _Abi0::_S_size;

      using _Impl0 = typename _Abi0::_SimdImpl;

      using _MaskImpl = typename abi_type::_MaskImpl;

      template <__vectorizable_canon _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember<_Tp>
        _S_broadcast(_Tp __x) noexcept
        { return {((void)_Is, _Impl0::_S_broadcast(__x))...}; }

      template <__vectorizable_canon _Tp, typename _Fp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember<_Tp>
        _S_generator(_Fp&& __gen)
        {
          return {_Impl0::template _S_generator<_Tp>([&] [[__gnu__::__always_inline__]] (auto __i) {
                    return __gen(__ic<__i + _Is * _S_chunk_size>);
                  })...};
        }

      template <__vectorizable_canon _Tp, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember<_Tp>
        _S_load(const _Up* __mem, _TypeTag<_Tp> __tag) noexcept
        { return {_Impl0::_S_load(__mem + _Is * _S_chunk_size, __tag)...}; }

      template <__vectorizable_canon _Tp, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static _SimdMember<_Tp>
        _S_partial_load(const _Up* __mem, size_t __mem_size, _TypeTag<_Tp> __tag)
        {
          if (__mem_size >= _S_size) [[unlikely]]
            return _S_load(__mem, __tag);
          else
            {
              return {
                ((__mem_size < _Is * _S_chunk_size)
                   ? _Impl0::_S_broadcast(_Tp())
                   : (__mem_size < (_Is + 1) * _S_chunk_size
                        ? _Impl0::_S_partial_load(__mem + _Is * _S_chunk_size,
                                                  __mem_size - _Is * _S_chunk_size, __tag)
                        : _Impl0::_S_load(__mem + _Is * _S_chunk_size, __tag)))...
              };
            }
        }

      template <typename _Tp, typename _Up>
        static constexpr inline _Tp
        _S_masked_load(_MaskMember<_Tp> const& __k, const _Up* __mem, _TypeTag<_Tp> __tag)
        { return {_Impl0::_S_masked_load(__k[_Is], __mem + _Is * _S_chunk_size, __tag)...}; }

      template <typename _Tp, typename _Up>
        static constexpr inline _Tp
        _S_masked_load(_Tp const& __merge, _MaskMember<_Tp> const& __k,
                       const _Up* __mem) noexcept
        {
          _SimdMember<_Tp> __ret = __merge;
          (_Impl0::_S_masked_load(__ret[_Is], __k[_Is], __mem + _Is * _S_chunk_size), ...);
          return __ret;
        }

      template <__vec_builtin _TV, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_store(const array<_TV, _Np>& __v, _Up* __mem,
                 _TypeTag<__value_type_of<_TV>> __tag) noexcept
        { (_Impl0::_S_store(__v[_Is], __mem + _Is * _S_chunk_size, __tag), ...); }

      template <__vec_builtin _TV, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_partial_store(const array<_TV, _Np>& __v, _Up* __mem, size_t __mem_size,
                         _TypeTag<__value_type_of<_TV>> __tag)
        {
          if (__mem_size >= _S_size) [[unlikely]]
            _S_store(__v, __mem, __tag);
          else
            ((__mem_size < _Is * _S_chunk_size
                ? []{}()
                : (__mem_size < (_Is + 1) * _S_chunk_size
                     ? _Impl0::_S_partial_store(__v[_Is], __mem + _Is * _S_chunk_size,
                                                __mem_size - _Is * _S_chunk_size, __tag)
                     : _Impl0::_S_store(__v[_Is], __mem + _Is * _S_chunk_size, __tag))), ...);
        }

      template <typename _Tp, typename _Up>
        static constexpr inline void
        _S_masked_store(_Tp const& __v, _Up* __mem, const _MaskMember<_Tp> __k) noexcept
        { (_Impl0::_S_masked_store(__v[_Is], __mem + _Is * _S_chunk_size, __k[_Is]), ...); }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_complement(_Tp const& __x) noexcept
        { return {_Impl0::_S_complement(__x[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_unary_minus(_Tp const& __x) noexcept
        { return {_Impl0::_S_unary_minus(__x[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_plus(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_plus(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_minus(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_minus(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_multiplies(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_multiplies(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_divides(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_divides(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_modulus(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_modulus(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_and(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_bit_and(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_or(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_bit_or(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_xor(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_bit_xor(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_shift_left(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_bit_shift_left(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_shift_right(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_bit_shift_right(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_shift_left(_Tp const& __x, int __y) noexcept
        { return {_Impl0::_S_bit_shift_left(__x[_Is], __y)...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_shift_right(_Tp const& __x, int __y) noexcept
        { return {_Impl0::_S_bit_shift_right(__x[_Is], __y)...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_equal_to(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_equal_to(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_not_equal_to(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_not_equal_to(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_less(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_less(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_less_equal(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_less_equal(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_negate(_Tp const& __x) noexcept
        { return {_Impl0::_S_negate(__x[_Is])...}; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_increment(auto& __x)
      { (_Impl0::_S_increment(__x[_Is]), ...); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_decrement(auto& __x)
      { (_Impl0::_S_decrement(__x[_Is]), ...); }

      template <typename _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr __value_type_of<_TV>
        _S_get(const array<_TV, _Np>& __v, _SimdSizeType __i)
        { return _Impl0::_S_get(__v[__i / _S_chunk_size], __i % _S_chunk_size); }

      template <typename _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_set(array<_TV, _Np>& __v, _SimdSizeType __i, auto&& __x)
        { _Impl0::_S_set(__v[__i / _S_chunk_size], __i % _S_chunk_size, __x); }

      // There's a curious case of _SimdMember and _MaskMember mismatch for the AVX w/o AVX2 case:
      // The mask type can either be array<int8, 2> or array<int4, 4> depending on whether it
      // originates for an integer or floating-point simd comparison. E.g.:
      // !simd<float, 16>    -> array<int8, 2>
      // !simd<unsigned, 16> -> array<int4, 4>

      template <typename _Tp, typename _UV, size_t _KSize>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(const std::array<_UV, _KSize>& __k, _Tp& __lhs,
                         const __type_identity_t<_Tp>& __rhs)
        {
          if constexpr (_Np == _KSize)
            (_Impl0::_S_masked_assign(__k[_Is], __lhs[_Is], __rhs[_Is]), ...);
          else if constexpr (_Np == _KSize * 2)
            (_Impl0::_S_masked_assign(__vec_extract_part<_Is % 1, 2>(__k[_Is / 2]),
                                      __lhs[_Is], __rhs[_Is]), ...);
          else
            __assert_unreachable<_Tp>();
        }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(const __vec_builtin auto __k, _Tp& __lhs,
                         const __type_identity_t<_Tp>& __rhs)
        {
          static_assert(_Np == 2);
          _Impl0::_S_masked_assign(__vec_extract_part<0, 2>(__k), __lhs[0], __rhs[0]);
          _Impl0::_S_masked_assign(__vec_extract_part<1, 2>(__k), __lhs[1], __rhs[1]);
        }

      template <typename _KV, typename _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(const array<_KV, _Np>& __k, array<_TV, _Np>& __lhs,
                         __value_type_of<_TV> __rhs)
        { (_Impl0::_S_masked_assign(__k[_Is], __lhs[_Is], __rhs), ...); }

      template <__vec_builtin _KV, typename _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(const array<_KV, _Np / 2>& __k, array<_TV, _Np>& __lhs,
                         __value_type_of<_TV> __rhs)
        {
          (_Impl0::_S_masked_assign(__vec_extract_part<_Is % 2, 2>(__k[_Is / 2]),
                                    __lhs[_Is], __rhs), ...);
        }

      template <__vec_builtin _KV, typename _TV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(const _KV __k, array<_TV, _Np>& __lhs, __value_type_of<_TV> __rhs)
        { (_Impl0::_S_masked_assign(__vec_extract_part<_Is, _Np>(__k), __lhs[_Is], __rhs), ...); }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_is_constprop(const _Tp& __x)
        {
          if constexpr (requires {_Impl0::_S_is_constprop(__x[0]);})
            return (_Impl0::_S_is_constprop(__x[_Is]) and ...);
          else if constexpr (requires {__x[0]._M_is_constprop();})
            return (__x[_Is]._M_is_constprop() and ...);
          else
            return (__detail::__is_constprop(__x[_Is]) and ...);
        }

      template <typename _Tp, typename _BinaryOperation>
        static constexpr _Tp
        _S_reduce(std::datapar::basic_simd<_Tp, abi_type> __xx, const _BinaryOperation& __binary_op)
        {
          auto& __x = __data(__xx); // the array was copied by the caller - we're not changing it
          (((_Is % 2) == 1 ? (__x[_Is - 1] = __binary_op(__x[_Is - 1], __x[_Is])) : __x[0]), ...);
          if constexpr (_Np > 2)
            (((_Is % 4) == 2 ? (__x[_Is - 2] = __binary_op(__x[_Is - 2], __x[_Is])) : __x[0]),
             ...);
          if constexpr (_Np > 4)
            (((_Is % 8) == 4 ? (__x[_Is - 4] = __binary_op(__x[_Is - 4], __x[_Is])) : __x[0]),
             ...);
          if constexpr (_Np > 8)
            (((_Is % 16) == 8 ? (__x[_Is - 8] = __binary_op(__x[_Is - 8], __x[_Is])) : __x[0]),
             ...);
          if constexpr (_Np > 16)
            (((_Is % 32) == 16 ? (__x[_Is - 16] = __binary_op(__x[_Is - 16], __x[_Is])) : __x[0]),
             ...);
          if constexpr (_Np > 32)
            (((_Is % 64) == 32 ? (__x[_Is - 32] = __binary_op(__x[_Is - 32], __x[_Is])) : __x[0]),
             ...);
          if constexpr (_Np > 64)
            (((_Is % 128) == 64 ? (__x[_Is - 64] = __binary_op(__x[_Is - 64], __x[_Is]))
                                : __x[0]), ...);
          static_assert(_Np <= 128);
          return std::datapar::reduce(std::datapar::basic_simd<_Tp, _Abi0>(__private_init, __x[0]),
                                      __binary_op);
        }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_min(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_min(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_max(_Tp const& __x, _Tp const& __y) noexcept
        { return {_Impl0::_S_max(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_minmax(_Tp& __min, _Tp& __max) noexcept
        { (_Impl0::_S_minmax(__min[_Is], __max[_Is]), ...); }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_clamp(_Tp const& __v, _Tp const& __lo, _Tp const& __hi) noexcept
        { return {_Impl0::_S_clamp(__v[_Is], __lo[_Is], __hi[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_isinf(const _Tp& __x)
        { return {_Impl0::_S_isinf(__x[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_isfinite(const _Tp& __x)
        { return {_Impl0::_S_isfinite(__x[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_isnan(const _Tp& __x)
        { return {_Impl0::_S_isnan(__x[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_isnormal(const _Tp& __x)
        { return {_Impl0::_S_isnormal(__x[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_signbit(const _Tp& __x)
        { return {_Impl0::_S_signbit(__x[_Is])...}; }
    };

  template <typename _Abi0, _BuildFlags _Flags, size_t... _Is>
    struct _MaskImplArray
    {
      static constexpr _SimdSizeType _Np = sizeof...(_Is);

      using abi_type = _AbiArray<_Abi0, _Np>;

      template <typename _Ts>
        using mask_type = std::datapar::basic_simd_mask<sizeof(_Ts), _Abi0>;

      template <__vectorizable_canon _Tp>
        using _MaskMember0 = typename _Abi0::template _MaskMember<_Tp>;

      template <__vectorizable_canon _Tp>
        using _SimdMember0 = typename _Abi0::template _SimdMember<_Tp>;

      template <__vectorizable_canon _Tp>
        using _MaskMember = std::array<_MaskMember0<_Tp>, _Np>;

      template <__vectorizable_canon _Tp>
        using _SimdMember = typename abi_type::template _SimdMember<_Tp>;

      static constexpr _SimdSizeType _S_size = abi_type::_S_size;

      // equal to full_size because _AbiArray ensures not-partial _Abi0
      static constexpr _SimdSizeType _S_chunk_size = _Abi0::_S_size;

      using _Impl0 = typename _Abi0::_MaskImpl;

      template <typename _Tp, bool _Sanitized>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_convert_mask(_BitMask<_S_size, _Sanitized> __x)
        {
          return {_Impl0::template _S_convert_mask<typename _Tp::value_type>(
                    __x.template _M_extract<_S_chunk_size * _Is, _S_chunk_size>())...};
        }

      template <typename _Tp, size_t _Bs, typename _UAbi>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_convert(std::datapar::basic_simd_mask<_Bs, _UAbi> __x)
        { return _S_convert_mask<_MaskMember<_Tp>>(__data(__x)); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr auto
      _S_to_bits_recursive(auto&& __part0, auto&& __part1, auto&&... __rest)
      {
        if constexpr (sizeof...(__rest))
          return _S_to_bits_recursive(__part1._M_prepend(__part0), __rest...);
        else
          return __part1._M_prepend(__part0);
      }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr auto
        _S_to_bits(const std::array<_Tp, _Np>& __k)
        { return _S_to_bits_recursive(_Impl0::_S_to_bits(__k[_Is])...); }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_mask_broadcast(bool __x)
        { return {((void)_Is, _Impl0::template _S_mask_broadcast<_Tp>(__x))...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC
        static constexpr _Tp
        _S_logical_and(const _Tp& __k0, const _Tp& __k1)
        { return {_Impl0::_S_logical_and(__k0[_Is], __k1[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC
        static constexpr _Tp
        _S_logical_or(const _Tp& __k0, const _Tp& __k1)
        { return {_Impl0::_S_logical_or(__k0[_Is], __k1[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_not(const _Tp& __x)
        { return {_Impl0::_S_bit_not(__x[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_and(const _Tp& __x, const _Tp& __y)
        { return {_Impl0::_S_bit_and(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_or(const _Tp& __x, const _Tp& __y)
        { return {_Impl0::_S_bit_or(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_bit_xor(const _Tp& __x, const _Tp& __y)
        { return {_Impl0::_S_bit_xor(__x[_Is], __y[_Is])...}; }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_get(const _Tp& __k, _SimdSizeType __i)
        { return _Impl0::_S_get(__k[__i / _S_chunk_size], __i % _S_chunk_size); }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_set(_Tp& __k, _SimdSizeType __i, bool __x)
        { return _Impl0::_S_set(__k[__i / _S_chunk_size], __i % _S_chunk_size, __x); }

      template <size_t _Bs>
        static constexpr std::datapar::basic_simd_mask<_Bs, _Abi0>
        _S_submask(std::datapar::basic_simd_mask<_Bs, abi_type> const& __masks, int __i)
        { return {__private_init, __data(__masks)[__i]}; }

      // TODO: The reduction strategy here depends a lot on use case and implementation of the
      // reduction on the register level. If compare + reduction on a register can e.g. be
      // contracted into a single ptest instruction then using short-circuiting 'or' seems like
      // the best implementation in almost every use case. If the reduction on a register is
      // expensive (NEON?) then it might be a better strategy to reduce on SIMD registers first
      // before doing a final reduction to bool.
      // In all cases in between it is dependent on the algorithm/data. Profile-guided
      // optimization that can turn this inside out would be super cool.
      template <typename _Kp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_all_of(const array<_Kp, _Np>& __masks)
        { return (_Impl0::_S_all_of(__masks[_Is]) or ...); }

      template <typename _Kp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_any_of(const array<_Kp, _Np>& __masks)
        { return (_Impl0::_S_any_of(__masks[_Is]) or ...); }

      template <typename _Kp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_none_of(const array<_Kp, _Np>& __masks)
        { return (_Impl0::_S_none_of(__masks[_Is]) or ...); }

      // TODO: benchmark whether it's more efficient to do element-wise reduction of array members
      // on -__k or +__k or a sum of popcount on each sub-mask.
      // Beware of overflow with _Bs == 1.
      template <size_t _Bs>
        requires (_Bs == 1)
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdSizeType
        _S_popcount(const std::datapar::basic_simd_mask<_Bs, abi_type> & __k)
        {
          using _Ip = __mask_integer_from<_Bs>;
          using _Up = __make_unsigned_int_t<_Ip>;
          if constexpr (-_S_size >= __finite_min_v<_Ip>)
            return -_SimdImplArray<_Abi0, _Flags, _Is...>::_S_reduce(-__k, std::plus<>());
          else if constexpr (_S_size <= __finite_max_v<_Up>)
            return _SimdImplArray<_Abi0, _Flags, _Is...>::_S_reduce(
                     static_cast<std::datapar::basic_simd<_Up, abi_type>>(__k), std::plus<>());
          else
            return (std::datapar::reduce_count(_S_submask(__k, _Is)) + ...);
        }

      template <int _Idx = 0, size_t _Bs>
        static constexpr _SimdSizeType
        _S_reduce_min_index(std::datapar::basic_simd_mask<_Bs, abi_type> const& __masks)
        {
          const auto __k = _S_submask(__masks, _Idx);
          if constexpr (_Idx == _Np - 1)
            return std::datapar::reduce_min_index(__k) + _Idx * _S_chunk_size;
          else if (std::datapar::any_of(__k))
            return std::datapar::reduce_min_index(__k) + _Idx * _S_chunk_size;
          else
            return _S_reduce_min_index<_Idx + 1, _Bs>(__masks);
        }

      template <int _Idx = _Np - 1, size_t _Bs>
        static constexpr _SimdSizeType
        _S_reduce_max_index(std::datapar::basic_simd_mask<_Bs, abi_type> const& __masks)
        {
          const auto __k = _S_submask(__masks, _Idx);
          if constexpr (_Idx == 0)
            return std::datapar::reduce_max_index(__k);
          else if (std::datapar::any_of(__k))
            return std::datapar::reduce_max_index(__k) + _Idx * _S_chunk_size;
          else
            return _S_reduce_max_index<_Idx - 1, _Bs>(__masks);
        }

      template <__vectorizable_canon _Tp>
        _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp>
        _S_mask_with_n_true(unsigned __n)
        {
          __glibcxx_simd_precondition(__n < _S_size, "");
          if constexpr (sizeof(_MaskMember<_Tp>) * __CHAR_BIT__ == _S_size)
            {
              static_assert(is_integral_v<_MaskMember0<_Tp>>);
              static_assert(__CHAR_BIT__ == 8);
              static_assert(sizeof(unsigned) == 4);
              static_assert(sizeof(unsigned long long) == 8);
              _MaskMember<_Tp> __ret;
              if constexpr (_Flags._M_have_bmi2())
                {
                  if constexpr (_S_size <= 32)
                    {
                      const auto __bits = __builtin_ia32_bzhi_si(~0u >> (32 - _S_size), __n);
                      __builtin_memcpy(&__ret, &__bits, sizeof(__ret));
                    }
                  else if constexpr (_S_size <= 64)
                    {
                      const auto __bits = __builtin_ia32_bzhi_di(~0ull >> (64 - _S_size), __n);
                      __builtin_memcpy(&__ret, &__bits, sizeof(__ret));
                    }
                  else
                    {
                      constexpr unsigned __parts = __bit_ceil(sizeof(__ret)) / 8u;
                      unsigned long long __bits[__parts] = {};
                      unsigned __i = __n / 64u;
                      if (__i < __parts)
                        __bits[__i] = __builtin_ia32_bzhi_di(~0ull, __n % 64u);
                      while (__i)
                        __bits[--__i] = ~0ull;
                      __builtin_memcpy(&__ret, &__bits, sizeof(__ret));
                    }
                }
              else
                {
                  if constexpr (_S_size < 32)
                    {
                      const auto __bits = (1u << __n) - 1;
                      __builtin_memcpy(&__ret, &__bits, sizeof(__ret));
                    }
                  else if constexpr (_S_size <= 64)
                    {
                      const auto __bits = _S_size == 64 and __n == 64
                                            ? ~0ull : (1ull << __n) - 1;
                      __builtin_memcpy(&__ret, &__bits, sizeof(__ret));
                    }
#ifdef __SIZEOF_INT128__
                  else if constexpr (_S_size <= 128)
                    {
                      using _Up = unsigned __int128;
                      const auto __bits = _S_size == 128 and __n == 128
                                            ? ~_Up() : (_Up(1) << __n) - 1;
                      __builtin_memcpy(&__ret, &__bits, sizeof(__ret));
                    }
#endif
                  else
                    {
                      constexpr unsigned __parts = __bit_ceil(sizeof(__ret)) / 8u;
                      unsigned long long __bits[__parts] = {};
                      unsigned __i = __n / 64u;
                      if (__i < __parts)
                        __bits[__i] = (1ull << __n) - 1;
                      while (__i)
                        __bits[--__i] = ~0ull;
                      __builtin_memcpy(&__ret, &__bits, sizeof(__ret));
                    }
                }
              return __ret;
            }
          else
            {
              static_assert(__has_single_bit(_S_chunk_size));
              if constexpr (_Np <= 4 and _S_size <= 128) // always stays below INT8_MAX
                {
                  constexpr auto __iota0 = _S_vec_iota<_SimdMember0<_Tp>>;
                  const auto __nv0 = _Abi0::_SimdImpl::_S_broadcast(_Tp(__n));
                  return {_Abi0::_SimdImpl::_S_less(__iota0 + _Tp(_S_chunk_size * _Is), __nv0)...};
                }
              else
                {
                  _MaskMember<_Tp> __ret = {};
                  __builtin_memset(&__ret, -1, __n * sizeof(_Tp));
                  return __ret;
                }
#if 0
              constexpr auto __all0 = _Impl0::template _S_mask_broadcast<_Tp>(false);
              constexpr auto __all1 = _Impl0::template _S_mask_broadcast<_Tp>(true);
              const unsigned __masked_n = __n & unsigned(_S_chunk_size - 1);
              return {
                (__n < _S_chunk_size * _Is ? __all0
                                           : (__n >= _S_chunk_size * (_Is + 1)
                                                ? __all1
                                                : _Impl0::template _S_mask_with_n_true<_Tp>(__masked_n))
                )...
              };
#elif 0
              constexpr auto __all1 = _Impl0::template _S_mask_broadcast<_Tp>(true);
              const unsigned __masked_n = __n & unsigned(_S_chunk_size - 1);
              _MaskMember<_Tp> __ret = _S_mask_broadcast<_Tp>(false);
              if (__n == 0)
                return __ret;
              for (auto& __chunk : __ret)
                {
                  if (__n >= _S_chunk_size)
                    {
                      __chunk = __all1;
                      __n -= _S_chunk_size;
                    }
                  else
                    {
                      __chunk = _Impl0::template _S_mask_with_n_true<_Tp>(__masked_n);
                      return __ret;
                    }
                }
#endif
            }
        }

      template <__vectorizable_canon _Tp, typename _Fp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
        _S_mask_generator(_Fp&& __gen)
        {
          if constexpr (requires {_Impl0::template _S_mask_generator<_Tp>(__gen);})
            return {_Impl0::template _S_mask_generator<_Tp>([&](auto __i) {
                      return __gen(__ic<_Is * _S_chunk_size + __i>);
                    })...};
          else if constexpr (_S_chunk_size == 1)
            return {bool(__gen(__ic<_Is>))...};
          else
            return {[&]<size_t... _Js>(__constexpr_wrapper_like auto __i,
                                       std::index_sequence<_Js...>) {
              return _MaskMember0<_Tp>{ _Tp(-bool(__gen(__ic<__i * _S_chunk_size + _Js>)))... };
            }(__ic<_Is>, std::make_index_sequence<_S_chunk_size>())...};
        }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr bool
        _S_is_constprop(const _Tp& __x)
        {
          if constexpr (requires {_Impl0::_S_is_constprop(__x[0]);})
            return (_Impl0::_S_is_constprop(__x[_Is]) and ...);
          else if constexpr (requires {__x[0]._M_is_constprop();})
            return (__x[_Is]._M_is_constprop() and ...);
          else
            return (__is_constprop(__x[_Is]) and ...);
        }
    };

  template <typename _Tp, __valid_abi_tag<_Tp> _A0, _SimdSizeType __offset>
    struct _SimdTupleMeta : public _A0::_SimdImpl
    {
      using _Abi = _A0;
      using _Traits = _SimdTraits<_Tp, _Abi>;
      using _MaskImpl = typename _Abi::_MaskImpl;
      using _MaskMember = typename _Traits::_MaskMember;
      static constexpr auto _S_offset = __ic<__offset>;
      static constexpr auto _S_size = __ic<_SimdSizeType(__simd_size_v<_Tp, _Abi>)>;
      static constexpr _MaskImpl _S_mask_impl = {};

      template <size_t _Np, bool _Sanitized>
        _GLIBCXX_SIMD_INTRINSIC static constexpr auto
        _S_submask(_BitMask<_Np, _Sanitized> __bits)
        { return __bits.template _M_extract<_S_offset, _S_size>(); }

      template <size_t _Np, bool _Sanitized>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
        _S_make_mask(_BitMask<_Np, _Sanitized> __bits)
        {
          return _MaskImpl::template _S_convert_mask<_MaskMember>(
                   _S_submask(__bits)._M_sanitized());
        }

      template <__vec_builtin _KV>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
        _S_make_mask(_KV __k)
        {
          if constexpr (_S_size == 1)
            return __vec_get(__k, __offset) != 0;
          else
            return __vec_extract_part<__offset, __width_of<_KV>, _S_size>(__k);
        }

      _GLIBCXX_SIMD_INTRINSIC static constexpr unsigned long long
      _S_mask_to_shifted_ullong(_MaskMember __k)
      {
        unsigned long long __bits = _MaskImpl::_S_to_bits(__k)._M_sanitized()._M_to_bits();
        return __bits << _S_offset;
      }

      _GLIBCXX_SIMD_INTRINSIC static constexpr std::datapar::basic_simd<_Tp, _A0>
      _S_to_simd(const _Traits::_SimdMember& __x0)
      { return std::datapar::basic_simd<_Tp, _A0>(__private_init, __x0); }
    };

  template <__vectorizable_canon _Tp, __valid_abi_tag<_Tp>... _As>
    struct _SimdTuple
    {
      static_assert(sizeof...(_As) == 0);

      static constexpr auto _S_size = __ic<0>;

      static constexpr auto _S_tuple_size = __ic<0>;

      _GLIBCXX_SIMD_INTRINSIC constexpr bool
      _M_is_constprop() const
      { return true; }
    };

  template <__vectorizable_canon _Tp, __valid_abi_tag<_Tp>... _As>
    struct _SimdTupleData;

  template <__vectorizable_canon _Tp, __valid_abi_tag<_Tp> _A0, __valid_abi_tag<_Tp> _A1,
            __valid_abi_tag<_Tp>... _As>
    struct _SimdTupleData<_Tp, _A0, _A1, _As...>
    {
      using _AbiTail = _AbiCombine<(__simd_size_v<_Tp, _A1> + ... + __simd_size_v<_Tp, _As>), _A1>;

      typename _A0::template _SimdMember<_Tp> _M_x;

      _SimdTuple<_Tp, _A1, _As...> _M_tail;
    };

  template <__vectorizable_canon _Tp, __valid_abi_tag<_Tp> _A0>
    struct _SimdTupleData<_Tp, _A0>
    {
      typename _A0::template _SimdMember<_Tp> _M_x;
      static constexpr _SimdTuple<_Tp> _M_tail = {};
    };

  template <__vectorizable_canon _Tp, __valid_abi_tag<_Tp> _A0, __valid_abi_tag<_Tp>... _As>
    struct _SimdTuple<_Tp, _A0, _As...> : _SimdTupleData<_Tp, _A0, _As...>
    {
      using _Base = _SimdTupleData<_Tp, _A0, _As...>;

      using _Base::_M_x;
      using _Base::_M_tail;

      using _SimdType = std::datapar::basic_simd<_Tp, _A0>;

      using _Abi0 = _A0;

      using _Simpl0 = _A0::_SimdImpl;

      using _Mimpl0 = _A0::_MaskImpl;

      static constexpr bool _S_recurse = sizeof...(_As) != 0;

      static constexpr auto _S_size = __ic<(__simd_size_v<_Tp, _A0>)>;

      static constexpr auto _S_total_size
        = __ic<(__simd_size_v<_Tp, _A0> + ... + __simd_size_v<_Tp, _As>)>;

      static constexpr auto _S_size0 = _A0::_S_size;
      static constexpr auto _S_tail_size = _SimdTuple<_Tp, _As...>::_S_size;

      static constexpr auto _S_tuple_size = __ic<_SimdSizeType(sizeof...(_As) + 1)>;

      _GLIBCXX_SIMD_INTRINSIC constexpr bool
      _M_is_constprop() const
      {
        bool __r;
        if constexpr (requires {_Simpl0::_S_is_constprop(_M_x);})
          __r = _Simpl0::_S_is_constprop(_M_x);
        else if constexpr (requires {_M_x._M_is_constprop();})
          __r = _M_x._M_is_constprop();
        else
          __r = __builtin_constant_p(_M_x);
        return __r and _M_tail._M_is_constprop();
      }

      _GLIBCXX_SIMD_INTRINSIC constexpr _Tp
      operator[](_SimdSizeType __i) const
      {
        if (not __builtin_is_constant_evaluated())
          {
            const auto* __ptr = reinterpret_cast<const std::byte*>(this);
            _Tp __r;
            __builtin_memcpy(&__r, __ptr + __i * sizeof(_Tp), sizeof(_Tp));
            return __r;
          }
        else if (__i < _S_size)
          return _Simpl0::_S_get(_M_x, __i);
        else if constexpr (_S_recurse)
          return _M_tail[__i - _S_size];
        else
          __invoke_ub("out-of-bounds subscript");
      }

      _GLIBCXX_SIMD_INTRINSIC constexpr auto
      _M_simd_at(__constexpr_wrapper_like auto __i) const
      {
        if constexpr (__i == 0)
          return _SimdType(__private_init, _M_x);
        else if constexpr (_S_recurse)
          return _M_tail._M_simd_at(__ic<__i - 1>);
      }

      template <__constexpr_wrapper_like _Cv = _Ic<0>>
        _GLIBCXX_SIMD_INTRINSIC constexpr _SimdTuple&
        _M_forall(auto&& __fun, _Cv __total_offset = {})
        {
          __fun(_SimdTupleMeta<_Tp, _A0, __total_offset>(), _M_x);
          if constexpr (_S_recurse)
            _M_tail._M_forall(__fun, __ic<__total_offset + _S_size>);
          return *this;
        }

      template <__constexpr_wrapper_like _Cv = _Ic<0>>
        _GLIBCXX_SIMD_INTRINSIC constexpr _SimdTuple&
        _M_forall(const _SimdTuple& __a, auto&& __fun, _Cv __total_offset = {})
        {
          __fun(_SimdTupleMeta<_Tp, _A0, __total_offset>(), _M_x, __a._M_x);
          if constexpr (_S_recurse)
            _M_tail._M_forall(__a._M_tail, __fun, __ic<__total_offset + _S_size>);
          return *this;
        }

      template <__constexpr_wrapper_like _Cv = _Ic<0>>
        _GLIBCXX_SIMD_INTRINSIC constexpr _SimdTuple&
        _M_forall(const _SimdTuple& __a, const _SimdTuple& __b,
                  auto&& __fun, _Cv __total_offset = {})
        {
          __fun(_SimdTupleMeta<_Tp, _A0, __total_offset>(), _M_x, __a._M_x, __b._M_x);
          if constexpr (_S_recurse)
            _M_tail._M_forall(__a._M_tail, __b._M_tail, __fun, __ic<__total_offset + _S_size>);
          return *this;
        }

      template <__constexpr_wrapper_like _Cv = _Ic<0>>
        _GLIBCXX_SIMD_INTRINSIC constexpr _SimdTuple&
        _M_forall(_SimdTuple& __a, auto&& __fun, _Cv __total_offset = {})
        {
          __fun(_SimdTupleMeta<_Tp, _A0, __total_offset>(), _M_x, __a._M_x);
          if constexpr (_S_recurse)
            _M_tail._M_forall(__a._M_tail, __fun, __ic<__total_offset + _S_size>);
          return *this;
        }

      template <__constexpr_wrapper_like _Cv = _Ic<0>>
        _GLIBCXX_SIMD_INTRINSIC constexpr const _SimdTuple&
        _M_forall(auto&& __fun, _Cv __total_offset = {}) const
        {
          __fun(_SimdTupleMeta<_Tp, _A0, __total_offset>(), _M_x);
          if constexpr (_S_recurse)
            _M_tail._M_forall(__fun, __ic<__total_offset + _S_size>);
          return *this;
        }

      template <int __total_offset = 0>
        _GLIBCXX_SIMD_INTRINSIC constexpr _SanitizedBitMask<_S_total_size>
        _M_test(auto&& __fun, auto&&... __more) const
        {
          const _SanitizedBitMask<_S_size> __first
            = _A0::_MaskImpl::_S_to_bits(__fun(_SimdTupleMeta<_Tp, _A0, __total_offset>(),
                                               _M_x, __more._M_x...));
          if constexpr (_S_recurse)
            return _M_tail.template _M_test<__total_offset + _S_size>(__fun, __more._M_tail...)
                     ._M_prepend(__first);
          else
            return __first;
        }

      template <__constexpr_wrapper_like _Cv = _Ic<0>>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple
        _S_generate_persimd(auto&& __fun, _Cv __vec_offset = {})
        {
          if constexpr (_S_recurse)
            return _SimdTuple {
              __fun(__vec_offset),
              _SimdTuple<_Tp, _As...>::_S_generate_persimd(__fun, __ic<__vec_offset + 1>)
          };
          else
            return _SimdTuple {__fun(__vec_offset)};
        }

      constexpr
      _SimdTuple() = default;

      _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple
      _S_forall(auto &&__fun)
      {
        _SimdTuple __r;
        __r._M_forall(__fun);
        return __r;
      }

      _GLIBCXX_SIMD_INTRINSIC constexpr explicit
      _SimdTuple(const decltype(_M_x)& __x, const decltype(_M_tail)& __tail)
      requires (sizeof...(_As) != 0)
        : _Base {__x, __tail}
      {}

      _GLIBCXX_SIMD_INTRINSIC constexpr explicit
      _SimdTuple(const decltype(_M_x)& __x)
      requires (sizeof...(_As) == 0)
        : _Base {__x}
      {}
    };

  template <_SimdSizeType _Np, typename _Tag, _BuildFlags>
    struct _SimdImplAbiCombine
    {
      using _MaskMember = _SanitizedBitMask<_Np>;

      using _MaskImpl = _MaskImplAbiCombine<_Np, _Tag>;

      template <typename _Tp>
        using _SimdMember = __fixed_size_storage_t<_Tp, _Np>;

      using abi_type = _AbiCombine<_Np, _Tag>;

      template <typename _Tp>
        using _Simd = std::datapar::basic_simd<_Tp, abi_type>;

      template <typename _Tp>
        using _TypeTag = _Tp*;

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember<_Tp>
        _S_broadcast(_Tp __x) noexcept
        {
          return _SimdMember<_Tp>::_S_forall([&__x] [[__gnu__::__always_inline__]]
                                               (auto __meta, auto& __chunk) {
                   __chunk = __meta._S_broadcast(__x);
                 });
        }

      template <typename _Tp, typename _Fp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember<_Tp>
        _S_generator(_Fp&& __gen)
        {
          return _SimdMember<_Tp>::_S_forall([&__gen] [[__gnu__::__always_inline__]]
                                               (auto __meta, auto& __chunk) {
                   __chunk = __meta.template _S_generator<_Tp>(
                               [&] [[__gnu__::__always_inline__]]
                                 (__constexpr_wrapper_like auto __i) {
                                 return __gen(__ic<__i + __meta._S_offset>);
                               });
                 });
        }

      template <typename _Tp, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember<_Tp>
        _S_load(const _Up* __mem, _TypeTag<_Tp>)
        {
          return _SimdMember<_Tp>::_S_forall(
                   [&] [[__gnu__::__always_inline__]] (auto __meta, auto& __chunk) {
                     __chunk = __meta._S_load(__mem + __meta._S_offset, _TypeTag<_Tp>());
                   });
        }

      template <__vectorizable_canon _Tp, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static _SimdMember<_Tp>
        _S_partial_load(const _Up* __mem, size_t __mem_size, _TypeTag<_Tp> __tag)
        {
          if (__mem_size >= _Np) [[unlikely]]
            return _S_load(__mem, __tag);
          else
            return _SimdMember<_Tp>::_S_forall(
                     [&] [[__gnu__::__always_inline__]] (auto __meta, auto& __chunk) {
                       if (__mem_size > size_t(__meta._S_size + __meta._S_offset))
                         __chunk = __meta._S_load(__mem + __meta._S_offset, __tag);
                       else if (__mem_size <= size_t(__meta._S_offset))
                         __chunk = __meta._S_broadcast(_Tp());
                       else
                         __chunk = __meta._S_partial_load(__mem + __meta._S_offset,
                                                          __mem_size - __meta._S_offset, __tag);
                     });
        }

      template <typename _Tp, typename... _As, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static _SimdTuple<_Tp, _As...>
        _S_masked_load(const _MaskMember __bits, const _Up* __mem, _TypeTag<_Tp> __tag)
        {
          return _SimdMember<_Tp>::_S_forall(
                   [&] [[__gnu__::__always_inline__]] (auto __meta, auto& __chunk) {
                     __chunk = __meta._S_submask(__bits).any()
                                 ? __meta._S_masked_load(__meta._S_make_mask(__bits),
                                                         __mem + __meta._S_offset, __tag)
                                 : remove_reference_t<decltype(__chunk)>();
                   });
        }

      template <typename _Tp, typename... _As, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static _SimdTuple<_Tp, _As...>
        _S_masked_load(const _SimdTuple<_Tp, _As...>& __old,
                       const _MaskMember __bits, const _Up* __mem)
        {
          auto __merge = __old;
          __merge._M_forall([&] [[__gnu__::__always_inline__]] (auto __meta, auto& __chunk) {
            if (__meta._S_submask(__bits).any())
#pragma GCC diagnostic push
              // Dereferencing __mem + __meta._S_offset could be UB ([expr.add]/4.3).
              // It is the responsibility of the caller of the masked load (via the mask's value)
              // to avoid UB. Consequently, the compiler may assume this branch is unreachable, if
              // the pointer arithmetic is UB.
#pragma GCC diagnostic ignored "-Warray-bounds"
              __chunk = __meta._S_masked_load(__chunk, __meta._S_make_mask(__bits),
                                              __mem + __meta._S_offset);
#pragma GCC diagnostic pop
          });
          return __merge;
        }

      template <typename _Tp, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_store(const _SimdMember<_Tp>& __v, _Up* __mem, _TypeTag<_Tp>)
        {
          __v._M_forall([&] [[__gnu__::__always_inline__]] (auto __meta, auto __chunk) {
            __meta._S_store(__chunk, __mem + __meta._S_offset, _TypeTag<_Tp>());
          });
        }

      template <typename _Tp, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_partial_store(const _SimdMember<_Tp>& __v, _Up* __mem, size_t __mem_size,
                         _TypeTag<_Tp> __tag)
        {
          if (__mem_size >= _Np) [[unlikely]]
            _S_store(__v, __mem, __tag);
          else
            __v._M_forall([&] [[__gnu__::__always_inline__]] (auto __meta, auto __chunk) {
              if (__mem_size > size_t(__meta._S_size + __meta._S_offset))
                __meta._S_store(__chunk, __mem + __meta._S_offset, __tag);
              else if (__mem_size <= size_t(__meta._S_offset))
                /*nothing*/;
              else
                __meta._S_partial_store(__chunk, __mem + __meta._S_offset,
                                        __mem_size - __meta._S_offset, __tag);
            });
        }

      template <typename _Tp, typename... _As, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static void
        _S_masked_store(const _SimdTuple<_Tp, _As...>& __v, _Up* __mem, const _MaskMember __bits)
        {
          __v._M_forall([&] [[__gnu__::__always_inline__]] (auto __meta, auto __chunk) {
            if (__meta._S_submask(__bits).any())
#pragma GCC diagnostic push
              // __mem + __mem._S_offset could be UB ([expr.add]/4.3, but it punts
              // the responsibility for avoiding UB to the caller of the masked
              // store via the mask. Consequently, the compiler may assume this
              // branch is unreachable, if the pointer arithmetic is UB.
#pragma GCC diagnostic ignored "-Warray-bounds"
              __meta._S_masked_store(__chunk, __mem + __meta._S_offset,
                                     __meta._S_make_mask(__bits));
#pragma GCC diagnostic pop
          });
        }

      template <typename _Tp, typename... _As>
        static constexpr inline _MaskMember
        _S_negate(const _SimdTuple<_Tp, _As...>& __x)
        {
          _MaskMember __bits = 0;
          __x._M_forall([&__bits] [[__gnu__::__always_inline__]] (auto __meta, auto __chunk) {
            __bits |= __meta._S_mask_to_shifted_ullong(__meta._S_negate(__chunk));
          });
          return __bits;
        }

      template <typename _Tp, typename _BinaryOperation, typename _A0>
        static constexpr _Tp
        _S_reduce(const _BinaryOperation& __binary_op,
                  const std::datapar::basic_simd<_Tp, _A0>& __x)
        { return std::datapar::reduce(__x, __binary_op); }

      template <typename _Tp, typename _BinaryOperation, typename _A0, typename _A1,
                typename... _Abis>
        static constexpr _Tp
        _S_reduce(const _BinaryOperation& __binary_op,
                  const std::datapar::basic_simd<_Tp, _A0>& __x0,
                  const std::datapar::basic_simd<_Tp, _A1>& __x1,
                  const std::datapar::basic_simd<_Tp, _Abis>&... __tail)
        {
          using _X0 = std::datapar::basic_simd<_Tp, _A0>;
          using _X1 = std::datapar::basic_simd<_Tp, _A1>;
          if constexpr (_X0::size.value == _X1::size.value)
            return _S_reduce(__binary_op, __binary_op(__x0, __x1), __tail...);
          else if constexpr (_X0::size.value > _X1::size.value)
            {
              using _X2 = std::datapar::resize_t<__bit_ceil(_X0::size.value) / 2, _X0>;
              auto __x02 = std::datapar::chunk<_X2>(__x0);
              if constexpr (_X1::size.value == _X2::size.value)
                {
                  std::get<0>(__x02) = __binary_op(std::get<0>(__x02), __x1);
                  return _GLIBCXX_SIMD_INT_PACK(tuple_size_v<decltype(__x02)>, _Is, {
                           return _S_reduce(__binary_op, std::get<_Is>(__x02)..., __tail...);
                         });
                }
              else if constexpr (_X1::size.value > _X2::size.value)
                {
                  auto __x12 = std::datapar::chunk<_X2>(__x1);
                  auto __x2 = __binary_op(std::get<0>(__x02), std::get<0>(__x12));
                  return _GLIBCXX_SIMD_INT_PACK(tuple_size_v<decltype(__x02)> - 1, _Is, {
                           return _GLIBCXX_SIMD_INT_PACK(tuple_size_v<decltype(__x12)> - 1, _Js, {
                                    return _S_reduce(__binary_op, __x2, std::get<_Is + 1>(__x02)...,
                                                     std::get<_Js + 1>(__x12)..., __tail...);
                                  });
                         });
                }
              else
                return _GLIBCXX_SIMD_INT_PACK(tuple_size_v<decltype(__x02)>, _Is, {
                         return _S_reduce(__binary_op, std::get<_Is>(__x02)..., __x1, __tail...);
                       });
            }
          else
            {
              using _X2 = std::datapar::resize_t<__bit_ceil(_X1::size.value) / 2, _X1>;
              auto __x12 = std::datapar::chunk<_X2>(__x1);
              if constexpr (_X0::size.value == _X2::size.value)
                {
                  std::get<0>(__x12) = __binary_op(std::get<0>(__x12), __x0);
                  return _GLIBCXX_SIMD_INT_PACK(tuple_size_v<decltype(__x12)>, _Is, {
                           return _S_reduce(__binary_op, std::get<_Is>(__x12)..., __tail...);
                         });
                }
              else if constexpr (_X1::size.value > _X2::size.value)
                {
                  auto __x02 = std::datapar::chunk<_X2>(__x0);
                  auto __x2 = __binary_op(std::get<0>(__x02), std::get<0>(__x12));
                  return _GLIBCXX_SIMD_INT_PACK(tuple_size_v<decltype(__x02)> - 1, _Is, {
                           return _GLIBCXX_SIMD_INT_PACK(tuple_size_v<decltype(__x12)> - 1, _Js, {
                                    return _S_reduce(__binary_op, __x2, std::get<_Is + 1>(__x02)...,
                                                     std::get<_Js + 1>(__x12)..., __tail...);
                                  });
                         });
                }
              else
                return _GLIBCXX_SIMD_INT_PACK(tuple_size_v<decltype(__x12)>, _Is, {
                         return _S_reduce(__binary_op, __x0, std::get<_Is>(__x12)..., __tail...);
                       });
            }
        }

      template <typename _Tp, typename _BinaryOperation>
        static constexpr _Tp
        _S_reduce(const _Simd<_Tp>& __x, const _BinaryOperation& __binary_op)
        {
          using _Tup = _SimdMember<_Tp>;
          const _Tup& __tup = __data(__x);
          if constexpr (_Tup::_S_tuple_size == 1)
            return std::datapar::reduce(__tup._M_simd_at(__ic<0>), __binary_op);
          else
            return _S_reduce(__binary_op, __tup._M_simd_at(__ic<0>), __tup._M_simd_at(__ic<1>));
        }

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple<_Tp, _As...>
        _S_min(_SimdTuple<_Tp, _As...> __a, const _SimdTuple<_Tp, _As...>& __b)
        {
          return __a._M_forall(__b, [] [[__gnu__::__always_inline__]]
                                      (auto __meta, auto& __aa, auto __bb) {
                   __aa = __meta._S_min(__aa, __bb);
                 });
        }

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple<_Tp, _As...>
        _S_max(_SimdTuple<_Tp, _As...> __a, const _SimdTuple<_Tp, _As...>& __b)
        {
          return __a._M_forall(__b, [] [[__gnu__::__always_inline__]]
                                      (auto __meta, auto& __aa, auto __bb) {
                   __aa = __meta._S_max(__aa, __bb);
                 });
        }

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_minmax(_SimdTuple<_Tp, _As...>& __min, _SimdTuple<_Tp, _As...>& __max)
        {
          __min._M_forall(__max, [] [[__gnu__::__always_inline__]]
                                   (auto __meta, auto& __aa, auto& __bb) {
            __meta._S_minmax(__aa, __bb);
          });
        }

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple<_Tp, _As...>
        _S_clamp(_SimdTuple<_Tp, _As...> __v,
                 const _SimdTuple<_Tp, _As...>& __lo, const _SimdTuple<_Tp, _As...>& __hi)
        {
          return __v._M_forall(__lo, __hi, [] [[__gnu__::__always_inline__]]
                                             (auto __meta, auto& __vv, auto __ll, auto __hh) {
                   __vv = __meta._S_clamp(__vv, __ll, __hh);
                 });
        }

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple<_Tp, _As...>
        _S_complement(_SimdTuple<_Tp, _As...> __x) noexcept
        {
          return __x._M_forall([] [[__gnu__::__always_inline__]] (auto __meta, auto& __xx) {
                   __xx = __meta._S_complement(__xx);
                 });
        }

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple<_Tp, _As...>
        _S_unary_minus(_SimdTuple<_Tp, _As...> __x) noexcept
        {
          return __x._M_forall([] [[__gnu__::__always_inline__]] (auto __meta, auto& __xx) {
                   __xx = __meta._S_unary_minus(__xx);
                 });
        }

#define _GLIBCXX_SIMD_FIXED_OP(name_)                                                           \
      template <typename _Tp, typename... _As>                                                  \
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple<_Tp, _As...>                        \
        name_(_SimdTuple<_Tp, _As...> __x, const _SimdTuple<_Tp, _As...>& __y)                  \
        {                                                                                       \
          return __x._M_forall(__y, [] [[__gnu__::__always_inline__]]                           \
                                      (auto __meta, auto& __xx, auto __yy) {                    \
                   __xx = __meta.name_(__xx, __yy);                                             \
                 });                                                                            \
        }

      _GLIBCXX_SIMD_FIXED_OP(_S_plus)
      _GLIBCXX_SIMD_FIXED_OP(_S_minus)
      _GLIBCXX_SIMD_FIXED_OP(_S_multiplies)
      _GLIBCXX_SIMD_FIXED_OP(_S_divides)
      _GLIBCXX_SIMD_FIXED_OP(_S_modulus)
      _GLIBCXX_SIMD_FIXED_OP(_S_bit_and)
      _GLIBCXX_SIMD_FIXED_OP(_S_bit_or)
      _GLIBCXX_SIMD_FIXED_OP(_S_bit_xor)
      _GLIBCXX_SIMD_FIXED_OP(_S_bit_shift_left)
      _GLIBCXX_SIMD_FIXED_OP(_S_bit_shift_right)

#undef _GLIBCXX_SIMD_FIXED_OP

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple<_Tp, _As...>
        _S_bit_shift_left(_SimdTuple<_Tp, _As...> __x, int __y)
        {
          return __x._M_forall([__y] [[__gnu__::__always_inline__]] (auto __impl, auto& __xx) {
                   __xx = __impl._S_bit_shift_left(__xx, __y);
                 });
        }

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple<_Tp, _As...>
        _S_bit_shift_right(_SimdTuple<_Tp, _As...> __x, int __y)
        {
          return __x._M_forall([__y] [[__gnu__::__always_inline__]] (auto __impl, auto& __xx) {
                   __xx = __impl._S_bit_shift_right(__xx, __y);
                 });
        }

#if 0
#define _GLIBCXX_SIMD_APPLY_ON_TUPLE(_RetTp, __name)                                               \
  template <typename _Tp, typename... _As, typename... _More>                                \
    static inline __fixed_size_storage_t<_RetTp, _Np>                                        \
    _S_##__name(const _SimdTuple<_Tp, _As...>& __x, const _More&... __more)                  \
    {                                                                                        \
      if constexpr (sizeof...(_More) == 0)                                                   \
        {                                                                                    \
          if constexpr (is_same_v<_Tp, _RetTp>)                                              \
            {                                                                                \
              auto __r = __x;                                                                \
              return __r._M_forall([] [[__gnu__::__always_inline__]] (auto __meta, auto& __xx) {\
                       __xx = __data(__name(__meta._S_to_simd(__xx)));                       \
                     });                                                                       \
            }                                                                                \
          else                                                                               \
            return __optimize_simd_tuple(                                    \
                     __x.template _M_apply_r<_RetTp>(                        \
                       [] [[__gnu__::__always_inline__]] (auto __impl, auto __xx) \
            { return __impl._S_##__name(__xx); }));               \
        }                                                                    \
      else if constexpr (                                                    \
             is_same_v<                                                           \
               _Tp,                                                               \
               _RetTp> && (... && is_same_v<_SimdTuple<_Tp, _As...>, _More>) )    \
        return __x._M_apply_per_chunk(                                       \
                 [] [[__gnu__::__always_inline__]] (auto __impl, auto __xx, auto... __pack) \
        {                                                           \
                   using _V = typename decltype(__impl)::simd_type;          \
                   return __data(__name(_V(__private_init, __xx),            \
                                        _V(__private_init, __pack)...));     \
                 }, __more...);                                              \
      else if constexpr (is_same_v<_Tp, _RetTp>)                             \
        return __x._M_apply_per_chunk(                                       \
                 [] [[__gnu__::__always_inline__]] (auto __impl, auto __xx, auto... __pack) \
        {                                                           \
                   using _V = typename decltype(__impl)::simd_type;          \
                   return __data(__name(_V(__private_init, __xx),            \
                                        __autocvt_to_simd(__pack)...));      \
                 }, __more...);                                              \
      else                                                                   \
        __assert_unreachable<_Tp>();                                         \
    }

      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, acos)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, asin)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, atan)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, atan2)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, cos)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, sin)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, tan)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, acosh)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, asinh)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, atanh)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, cosh)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, sinh)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, tanh)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, exp)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, exp2)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, expm1)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(int, ilogb)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, log)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, log10)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, log1p)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, log2)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, logb)
        // modf implemented in simd_math.h
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, scalbn) // double scalbn(double x, int exp);
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, scalbln)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, cbrt)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, abs)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fabs)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, pow)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, sqrt)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, erf)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, erfc)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, lgamma)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, tgamma)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, trunc)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, ceil)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, floor)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, nearbyint)

      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, rint)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(long, lrint)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(long long, llrint)

      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, round)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(long, lround)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(long long, llround)

      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, ldexp)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fmod)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, remainder)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, copysign)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, nextafter)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fdim)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fmax)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fmin)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fma)
      _GLIBCXX_SIMD_APPLY_ON_TUPLE(int, fpclassify)
#undef _GLIBCXX_SIMD_APPLY_ON_TUPLE

      template <typename _Tp, typename... _Abis>
        static inline _SimdTuple<_Tp, _Abis...>
        _S_remquo(_SimdTuple<_Tp, _Abis...> __x, const _SimdTuple<_Tp, _Abis...>& __y,
                  __fixed_size_storage_t<int, _SimdTuple<_Tp, _Abis...>::_S_size()>* __z)
        {
          return __x._M_forall(__y, [] [[__gnu__::__always_inline__]]
                                      (auto __impl, auto& __xx, const auto __yy, auto& __zz)
          { return __impl._S_remquo(__xx, __yy, &__zz); },
                               __y, *__z);
        }

      template <typename _Tp, typename... _As>
        static inline _SimdTuple<_Tp, _As...>
        _S_frexp(const _SimdTuple<_Tp, _As...>& __x,
                 __fixed_size_storage_t<int, _Np>& __exp) noexcept
        {
          return __x._M_apply_per_chunk(
                   [] [[__gnu__::__always_inline__]] (auto __impl, const auto& __a, auto& __b) {
                     return __data(frexp(typename decltype(__impl)::simd_type(__private_init, __a),
                                         __autocvt_to_simd(__b)));
                   }, __exp);
        }
#endif

#define _GLIBCXX_SIMD_TEST_ON_TUPLE_(name_)                                                \
      template <typename _Tp, typename... _As>                                             \
        static constexpr _MaskMember                                                       \
        _S_##name_(const _SimdTuple<_Tp, _As...>& __x) noexcept                            \
        {                                                                                  \
          return __x._M_test([] [[__gnu__::__always_inline__]] (auto __impl, auto __xi) {  \
                   return __impl._S_##name_(__xi);                                         \
                 });                                                                       \
        }

      _GLIBCXX_SIMD_TEST_ON_TUPLE_(isinf)
      _GLIBCXX_SIMD_TEST_ON_TUPLE_(isfinite)
      _GLIBCXX_SIMD_TEST_ON_TUPLE_(isnan)
      _GLIBCXX_SIMD_TEST_ON_TUPLE_(isnormal)
      _GLIBCXX_SIMD_TEST_ON_TUPLE_(signbit)
#undef _GLIBCXX_SIMD_TEST_ON_TUPLE_

      template <typename... _Ts>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_increment(_SimdTuple<_Ts...>& __x)
        {
          __x._M_forall([] [[__gnu__::__always_inline__]] (auto __meta, auto& __chunk) {
            __meta._S_increment(__chunk);
          });
        }

      template <typename... _Ts>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_decrement(_SimdTuple<_Ts...>& __x)
        {
          __x._M_forall([] [[__gnu__::__always_inline__]] (auto __meta, auto& __chunk) {
            __meta._S_decrement(__chunk);
          });
        }

#define _GLIBCXX_SIMD_CMP_OPERATIONS(__cmp)                                                      \
      template <typename _Tp, __valid_abi_tag<_Tp>... _As>                                       \
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember                                     \
        __cmp(const _SimdTuple<_Tp, _As...>& __x, const _SimdTuple<_Tp, _As...>& __y)            \
        {                                                                                        \
          return __x._M_test([]<typename _UV> [[__gnu__::__always_inline__]]                     \
                               (auto __impl, _UV __xx, _UV __yy) {                               \
                   return __impl.__cmp(__xx, __yy);                                              \
                 },                                                                              \
                             __y);                                                               \
        }

      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_equal_to)
      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_not_equal_to)
      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_less)
      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_less_equal)
      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_isless)
      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_islessequal)
      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_isgreater)
      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_isgreaterequal)
      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_islessgreater)
      _GLIBCXX_SIMD_CMP_OPERATIONS(_S_isunordered)
#undef _GLIBCXX_SIMD_CMP_OPERATIONS

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
        _S_get(const _SimdTuple<_Tp, _As...>& __v, _SimdSizeType __i)
        { return __v[__i]; }

      template <typename _Tp, typename... _As, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_set(_SimdTuple<_Tp, _As...>& __v, _SimdSizeType __i, _Up&& __x)
        {
          if (not __builtin_is_constant_evaluated())
            {
              auto* __ptr = reinterpret_cast<std::byte*>(&__v);
              const _Tp __y = static_cast<_Up&&>(__x);
              __builtin_memcpy(__ptr + __i * sizeof(_Tp), &__y, sizeof(_Tp));
            }
          else
            __v._M_forall([&](auto __meta, auto& __chunk) {
              if (__i >= __meta._S_offset and __i < __meta._S_offset + __meta._S_size)
                __chunk[__i - __meta._S_offset] = static_cast<_Up&&>(__x);
            });
        }

      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(const auto __bits, _SimdTuple<_Tp, _As...>& __lhs,
                         const __type_identity_t<_SimdTuple<_Tp, _As...>>& __rhs)
        {
          __lhs._M_forall(__rhs, [&] [[__gnu__::__always_inline__]]
                                   (auto __meta, auto& __native_lhs, auto __native_rhs) {
            __meta._S_masked_assign(__meta._S_make_mask(__bits), __native_lhs, __native_rhs);
          });
        }

      // Optimization for the case where the RHS is a scalar. No need to broadcast the scalar to a
      // simd first.
      template <typename _Tp, typename... _As>
        _GLIBCXX_SIMD_INTRINSIC static constexpr void
        _S_masked_assign(const auto __bits, _SimdTuple<_Tp, _As...>& __lhs,
                         const __type_identity_t<_Tp> __rhs)
        {
          __lhs._M_forall([&] [[__gnu__::__always_inline__]]
                            (auto __meta, auto& __native_lhs) {
            __meta._S_masked_assign(__meta._S_make_mask(__bits), __native_lhs, __rhs);
          });
        }
    };

  template <_SimdSizeType _Np, typename _Tag, _BuildFlags _Flags>
    struct _MaskImplAbiCombine
    {
      static_assert(
        sizeof(uint64_t) * __CHAR_BIT__ >= _Np,
        "The _AbiCombine implementation relies on one uint64_t being able to store "
        "all boolean elements."); // required in load & store

      using abi_type = _AbiCombine<_Np, _Tag>;

      using _MaskMember = _SanitizedBitMask<_Np>;

      template <typename _Tp>
        using _FirstAbi = typename __fixed_size_storage_t<_Tp, _Np>::_FirstAbi;

      template <typename _Tp>
        using _TypeTag = _Tp*;

      template <typename>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
        _S_mask_broadcast(bool __x)
        { return __x ? ~_MaskMember() : _MaskMember(); }

      template <typename _Tp, typename _Fp>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
        _S_mask_generator(_Fp&& __gen)
        {
          using _Up = unsigned long long;
          static_assert(_Np <= sizeof(_Up) * __CHAR_BIT__);
          return [&]<size_t... _Is>(std::index_sequence<_Is...>) {
            return ((_Up(bool(__gen(__ic<_Is>))) << _Is) | ...);
          }(std::make_index_sequence<_Np>());
        }

      template <bool _Sanitized>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _BitMask<_Np, _Sanitized>
        _S_to_bits(_BitMask<_Np, _Sanitized> __x)
        { return __x; }

      template <typename _Tp, size_t _Bs, typename _UAbi>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
        _S_convert(std::datapar::basic_simd_mask<_Bs, _UAbi> __x)
        { return _UAbi::_MaskImpl::_S_to_bits(__data(__x)).template _M_extract<0, _Np>(); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
      _S_logical_and(const _MaskMember& __x, const _MaskMember& __y) noexcept
      { return __x & __y; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
      _S_logical_or(const _MaskMember& __x, const _MaskMember& __y) noexcept
      { return __x | __y; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
      _S_bit_not(const _MaskMember& __x) noexcept
      { return ~__x; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
      _S_bit_and(const _MaskMember& __x, const _MaskMember& __y) noexcept
      { return __x & __y; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
      _S_bit_or(const _MaskMember& __x, const _MaskMember& __y) noexcept
      { return __x | __y; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember
      _S_bit_xor(const _MaskMember& __x, const _MaskMember& __y) noexcept
      { return __x ^ __y; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_get(const _MaskMember& __k, _SimdSizeType __i)
      { return __k[__i]; }

      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_set(_MaskMember& __k, _SimdSizeType __i, bool __x)
      { __k.set(__i, __x); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_masked_assign(const _MaskMember& __k, _MaskMember& __lhs, const _MaskMember& __rhs)
      { __lhs = (__lhs & ~__k) | (__rhs & __k); }

      // Optimization for the case where the RHS is a scalar.
      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_masked_assign(const _MaskMember& __k, _MaskMember& __lhs, const bool __rhs)
      {
        if (__rhs)
          __lhs |= __k;
        else
          __lhs &= ~__k;
      }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_all_of(const _MaskMember& __k)
      { return __k.all(); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_any_of(const _MaskMember& __k)
      { return __k.any(); }

      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_none_of(const _MaskMember& __k)
      { return __k.none(); }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdSizeType
        _S_popcount(const std::datapar::basic_simd_mask<_Bs, abi_type> & __k)
        { return __data(__k).count(); }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdSizeType
        _S_reduce_min_index(const std::datapar::basic_simd_mask<_Bs, abi_type> & __k)
        { return __detail::__lowest_bit(__data(__k)._M_to_bits()); }

      template <size_t _Bs>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdSizeType
        _S_reduce_max_index(const std::datapar::basic_simd_mask<_Bs, abi_type> & __k)
        { return __detail::__highest_bit(__data(__k)._M_to_bits()); }

      template <__vectorizable_canon _Tp>
        _GLIBCXX_SIMD_INTRINSIC static _MaskMember
        _S_mask_with_n_true(unsigned __n)
        {
          __glibcxx_simd_precondition(__n < _Np, "");
          if constexpr (_Np <= 65 and _Flags._M_have_bmi2())
            return _MaskMember::__create_unchecked(__builtin_ia32_bzhi_di(~0ull, __n));
        }
    };

  // try all native ABIs (including scalar) first
  template <__vectorizable _Tp, _SimdSizeType _Np>
    requires (_AllNativeAbis::template _S_has_valid_abi<_Tp, _Np>)
    struct _DeduceAbi<_Tp, _Np>
    { using type = _AllNativeAbis::_FirstValidAbi<_Tp, _Np>; };

  // next try _AbiArray of _NativeAbi
  template <__vectorizable _Tp, _SimdSizeType _Np>
    requires (not _AllNativeAbis::template _S_has_valid_abi<_Tp, _Np>
                and _Np % _NativeAbi<_Tp>::_S_size == 0)
    struct _DeduceAbi<_Tp, _Np>
    { using type = _AbiArray<_NativeAbi<_Tp>, _Np / _NativeAbi<_Tp>::_S_size>; };

  // fall back to _AbiCombine of inhomogenous ABI tags
  template <__vectorizable _Tp, _SimdSizeType _Np>
    requires (not _AllNativeAbis::template _S_has_valid_abi<_Tp, _Np>
                and _Np % _NativeAbi<_Tp>::_S_size != 0
                and _AbiCombine<_Np, _NativeAbi<_Tp>>::template _IsValid<_Tp>::value)
    struct _DeduceAbi<_Tp, _Np>
    { using type = _AbiCombine<_Np, _NativeAbi<_Tp>>; };

  template <typename _Tp, _SimdSizeType _Np>
    struct _DeduceAbi
    { using type = _InvalidAbi; };
}

#endif  // PROTOTYPE_SIMD_ABI_H_

// vim: et
