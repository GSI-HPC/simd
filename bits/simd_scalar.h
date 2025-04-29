/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_SCALAR_H_
#define PROTOTYPE_SIMD_SCALAR_H_

#include "detail.h"
#include "detail_bitmask.h"

#include <cmath>

namespace std::__detail
{
  // __promote_preserving_unsigned
  // work around crazy semantics of unsigned integers of lower rank than int:
  // Before applying an operator the operands are promoted to int. In which case
  // over- or underflow is UB, even though the operand types were unsigned.
  template <typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC constexpr decltype(auto)
    __promote_preserving_unsigned(const _Tp& __x)
    {
      if constexpr (std::is_integral_v<_Tp>)
        {
          if constexpr (is_signed_v<decltype(+__x)> && is_unsigned_v<_Tp>)
            return static_cast<unsigned int>(__x);
          else
            return __x;
        }
      else
        return __x;
    }

  struct _SimdImplScalar;

  struct _MaskImplScalar;
}

namespace std
{
  struct _ScalarAbi
  {
    static constexpr __detail::_SimdSizeType _S_size = 1;

    static constexpr __detail::_SimdSizeType _S_full_size = 1;

    static constexpr bool _S_is_partial = false;

    static constexpr bool _S_pass_by_reference = false;

    template <typename>
      static constexpr bool _S_defer_to_scalar_abi = false;

    struct _IsValidAbiTag
    : true_type
    {};

    template <typename>
      struct _IsValidSizeFor
      : true_type
      {};

    template <typename _Tp>
      using _IsValid = __detail::__is_vectorizable<_Tp>;

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_masked(bool __x)
    { return __x; }

    using _SimdImpl = __detail::_SimdImplScalar;

    using _MaskImpl = __detail::_MaskImplScalar;

    template <typename _Tp>
      using _SimdMember = _Tp;

    template <typename>
      using _MaskMember = bool;

    template <typename _Tp>
      struct __traits
      : __detail::_InvalidTraits
      {};

    template <typename _Tp>
      requires _IsValid<_Tp>::value
      struct __traits<_Tp>
      {
        // conversions to _ScalarAbi should always be implicit
        template <typename _FromAbi>
          static constexpr bool _S_explicit_mask_conversion = false;

        using _SimdImpl = __detail::_SimdImplScalar;

        using _MaskImpl = __detail::_MaskImplScalar;

        using _SimdMember = _Tp;

        using _MaskMember = bool;

        static constexpr __detail::_SimdSizeType _S_size = 1;

        static constexpr __detail::_SimdSizeType _S_full_size = 1;

        static constexpr bool _S_is_partial = false;

        static constexpr size_t _S_simd_align = alignof(_SimdMember);

        static constexpr size_t _S_mask_align = alignof(_MaskMember);

        struct _SimdCastType { _SimdCastType() = delete; };

        struct _MaskCastType { _MaskCastType() = delete; };
      };

      template <__detail::__vectorizable_canon _Up, __detail::_SimdSizeType _NewW = 1>
        using _Rebind = __detail::__deduce_t<_Up, _NewW>;
  };
}

namespace std::__detail
{
  struct _SimdImplScalar
  {
    using abi_type = _ScalarAbi;

    template <typename _Tp>
      using _TypeTag = _Tp*;

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_broadcast(_Tp __x) noexcept
      { return __x; }

    template <typename _Tp, typename _Fp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_generator(_Fp&& __gen)
      { return __gen(__ic<0>); }

    template <typename _Tp, typename _Up>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_load(const _Up* __mem, _TypeTag<_Tp>) noexcept
      { return static_cast<_Tp>(__mem[0]); }

    template <typename _Tp, typename _Up>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_partial_load(const _Up* __mem, size_t __mem_size, _TypeTag<_Tp>) noexcept
      { return __mem_size == 0 ? _Tp() : static_cast<_Tp>(__mem[0]); }

    template <typename _Tp, typename _Up>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_masked_load(bool __k, const _Up* __mem, _TypeTag<_Tp>) noexcept
      { return __k ? static_cast<_Tp>(__mem[0]) : _Tp(); }

    template <typename _Tp, typename _Up>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_masked_load(_Tp __merge, bool __k, const _Up* __mem) noexcept
      {
        if (__k)
          __merge = static_cast<_Tp>(__mem[0]);
        return __merge;
      }

    template <typename _Tp, typename _Up>
      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_store(_Tp __v, _Up* __mem, _TypeTag<_Tp>) noexcept
      { __mem[0] = static_cast<_Up>(__v); }

    template <typename _Tp, typename _Up>
      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_partial_store(_Tp __v, _Up* __mem, size_t __mem_size, _TypeTag<_Tp>) noexcept
      { if (__mem_size > 0) __mem[0] = static_cast<_Up>(__v); }

    template <typename _Tp, typename _Up>
      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_masked_store(const _Tp __v, _Up* __mem, const bool __k) noexcept
      { if (__k) __mem[0] = __v; }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_negate(_Tp __x) noexcept
      { return !__x; }

    template <typename _Tp, typename _BinaryOperation>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_reduce(const std::datapar::basic_simd<_Tp, _ScalarAbi>& __x, const _BinaryOperation&)
      { return __x._M_data; }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_min(const _Tp __a, const _Tp __b)
      { return std::min(__a, __b); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_max(const _Tp __a, const _Tp __b)
      { return std::max(__a, __b); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_minmax(_Tp& __min, _Tp& __max)
      {
        const _Tp __lo = std::min(__min, __max);
        __max = std::max(__min, __max);
        __min = __lo;
      }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_clamp(const _Tp __v, const _Tp __lo, const _Tp __hi)
      { return std::clamp(__v, __lo, __hi); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_complement(_Tp __x) noexcept
      { return static_cast<_Tp>(~__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_unary_minus(_Tp __x) noexcept
      { return static_cast<_Tp>(-__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_plus(_Tp __x, _Tp __y)
      {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x)
                                  + __promote_preserving_unsigned(__y));
      }

        /*    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_add_sat(_Tp __x, _Tp __y)
      { return std::add_sat(__x, __y); }*/

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_minus(_Tp __x, _Tp __y)
      {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x)
                                  - __promote_preserving_unsigned(__y));
      }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_multiplies(_Tp __x, _Tp __y)
      {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x)
                                  * __promote_preserving_unsigned(__y));
      }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_divides(_Tp __x, _Tp __y)
      {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x)
                                  / __promote_preserving_unsigned(__y));
      }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_modulus(_Tp __x, _Tp __y)
      {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x)
                                  % __promote_preserving_unsigned(__y));
      }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_bit_and(_Tp __x, _Tp __y)
      {
        if constexpr (is_floating_point_v<_Tp>)
          {
            using _Ip = __make_unsigned_int_t<_Tp>;
            return bit_cast<_Tp>(bit_cast<_Ip>(__x) & bit_cast<_Ip>(__y));
          }
        else
          return static_cast<_Tp>(__promote_preserving_unsigned(__x)
                                    & __promote_preserving_unsigned(__y));
      }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_bit_or(_Tp __x, _Tp __y)
      {
        if constexpr (is_floating_point_v<_Tp>)
          {
            using _Ip = __make_unsigned_int_t<_Tp>;
            return _bit_cast<_Tp>(bit_cast<_Ip>(__x) | _bit_cast<_Ip>(__y));
          }
        else
          return static_cast<_Tp>(__promote_preserving_unsigned(__x)
                                    | __promote_preserving_unsigned(__y));
      }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_bit_xor(_Tp __x, _Tp __y)
      {
        if constexpr (is_floating_point_v<_Tp>)
          {
            using _Ip = __make_unsigned_int_t<_Tp>;
            return _bit_cast<_Tp>(_bit_cast<_Ip>(__x) ^ _bit_cast<_Ip>(__y));
          }
        else
          return static_cast<_Tp>(__promote_preserving_unsigned(__x)
                                    ^ __promote_preserving_unsigned(__y));
      }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_bit_shift_left(_Tp __x, int __y)
      { return static_cast<_Tp>(__promote_preserving_unsigned(__x) << __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_bit_shift_right(_Tp __x, int __y)
      { return static_cast<_Tp>(__promote_preserving_unsigned(__x) >> __y); }

    // frexp, modf and copysign implemented in simd_math.h
    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_acos(_Tp __x)
      { return std::acos(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_asin(_Tp __x)
      { return std::asin(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_atan(_Tp __x)
      { return std::atan(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_cos(_Tp __x)
      { return std::cos(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_sin(_Tp __x)
      { return std::sin(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_tan(_Tp __x)
      { return std::tan(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_acosh(_Tp __x)
      { return std::acosh(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_asinh(_Tp __x)
      { return std::asinh(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_atanh(_Tp __x)
      { return std::atanh(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_cosh(_Tp __x)
      { return std::cosh(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_sinh(_Tp __x)
      { return std::sinh(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_tanh(_Tp __x)
      { return std::tanh(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_atan2(_Tp __x, _Tp __y)
      { return std::atan2(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_exp(_Tp __x)
      { return std::exp(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_exp2(_Tp __x)
      { return std::exp2(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_expm1(_Tp __x)
      { return std::expm1(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_log(_Tp __x)
      { return std::log(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_log10(_Tp __x)
      { return std::log10(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_log1p(_Tp __x)
      { return std::log1p(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_log2(_Tp __x)
      { return std::log2(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_logb(_Tp __x)
      { return std::logb(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static int
      _S_ilogb(_Tp __x)
      { return std::ilogb(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_pow(_Tp __x, _Tp __y)
      { return std::pow(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_abs(_Tp __x)
      { return std::abs(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_fabs(_Tp __x)
      { return std::fabs(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_sqrt(_Tp __x)
      { return std::sqrt(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_cbrt(_Tp __x)
      { return std::cbrt(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_erf(_Tp __x)
      { return std::erf(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_erfc(_Tp __x)
      { return std::erfc(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_lgamma(_Tp __x)
      { return std::lgamma(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_tgamma(_Tp __x)
      { return std::tgamma(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_trunc(_Tp __x)
      { return std::trunc(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_floor(_Tp __x)
      { return std::floor(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_ceil(_Tp __x)
      { return std::ceil(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_nearbyint(_Tp __x)
      { return std::nearbyint(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_rint(_Tp __x)
      { return std::rint(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static long
      _S_lrint(_Tp __x)
      { return std::lrint(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static long long
      _S_llrint(_Tp __x)
      { return std::llrint(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_round(_Tp __x)
      { return std::round(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static long
      _S_lround(_Tp __x)
      { return std::lround(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static long long
      _S_llround(_Tp __x)
      { return std::llround(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_ldexp(_Tp __x, int __y)
      { return std::ldexp(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_scalbn(_Tp __x, int __y)
      { return std::scalbn(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_scalbln(_Tp __x, long __y)
      { return std::scalbln(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_fmod(_Tp __x, _Tp __y)
      { return std::fmod(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_remainder(_Tp __x, _Tp __y)
      { return std::remainder(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_nextafter(_Tp __x, _Tp __y)
      { return std::nextafter(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_fdim(_Tp __x, _Tp __y)
      { return std::fdim(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_fmax(_Tp __x, _Tp __y)
      { return std::fmax(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_fmin(_Tp __x, _Tp __y)
      { return std::fmin(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_fma(_Tp __x, _Tp __y, _Tp __z)
      { return std::fma(__x, __y, __z); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static _Tp
      _S_remquo(_Tp __x, _Tp __y, int* __z)
      { return std::remquo(__x, __y, __z); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr int
      _S_fpclassify(_Tp __x)
      { return std::fpclassify(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_isfinite(_Tp __x)
      { return std::isfinite(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_isinf(_Tp __x)
      { return std::isinf(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_isnan(_Tp __x)
      { return std::isnan(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_isnormal(_Tp __x)
      { return std::isnormal(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_signbit(_Tp __x)
      { return std::signbit(__x); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_isgreater(_Tp __x, _Tp __y)
      { return std::isgreater(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_isgreaterequal(_Tp __x, _Tp __y)
      { return std::isgreaterequal(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_isless(_Tp __x, _Tp __y)
      { return std::isless(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_islessequal(_Tp __x, _Tp __y)
      { return std::islessequal(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_islessgreater(_Tp __x, _Tp __y)
      { return std::islessgreater(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_isunordered(_Tp __x, _Tp __y)
      { return std::isunordered(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_hypot(_Tp __x, _Tp __y)
      { return std::hypot(__x, __y); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_hypot(_Tp __x, _Tp __y, _Tp __z)
      { return std::hypot(__x, __y, __z); }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_increment(_Tp& __x)
      { ++__x; }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_decrement(_Tp& __x)
      { --__x; }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_equal_to(_Tp __x, _Tp __y)
      { return __x == __y; }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_not_equal_to(_Tp __x, _Tp __y)
      { return __x != __y; }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_less(_Tp __x, _Tp __y)
      { return __x < __y; }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_less_equal(_Tp __x, _Tp __y)
      { return __x <= __y; }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_get(_Tp __v, _SimdSizeType __i)
      {
        if (__i != 0)
          __invoke_ub("Subscript %d is out of range: must be 0", __i);
        return __v;
      }

    template <typename _Tp, typename _Up>
      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_set(_Tp& __v, _SimdSizeType __i, _Up&& __x)
      {
        if (__i != 0)
          __invoke_ub("Subscript %d is out of range: must be 0", __i);
        __v = static_cast<_Up&&>(__x);
      }

    template <typename _Tp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr void
      _S_masked_assign(bool __k, _Tp& __lhs, __type_identity_t<_Tp> __rhs)
      { if (__k) __lhs = __rhs; }

    template <typename _Tp, typename _Fp>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      _S_permute(_Tp __x, const _Fp __idx_perm) noexcept
      {
        constexpr _SimdSizeType __j = [&] consteval {
          if constexpr (__detail::__index_permutation_function_nosize<_Fp>)
            return __idx_perm(0);
          else
            return __idx_perm(0, 1);
        }();
        if constexpr (__j == datapar::zero_element or __j == datapar::uninit_element)
          return _Tp();
        else
          static_assert(__j == 0);
        return __x;
      }
  };

  struct _MaskImplScalar
  {
    using abi_type = _ScalarAbi;

    template <typename _Tp>
      using _TypeTag = _Tp*;

    template <typename>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_mask_broadcast(bool __x)
      { return __x; }

    template <typename>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_mask_generator(auto&& __gen)
      { return __gen(__ic<_SimdSizeType(0)>); }

    _GLIBCXX_SIMD_INTRINSIC static constexpr _SanitizedBitMask<1>
    _S_to_bits(bool __x)
    { return _SanitizedBitMask<1>::__create_unchecked(__x); }

    template <same_as<bool>, bool _Sanitized>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_convert_mask(_BitMask<1, _Sanitized> __x)
      { return __x[__ic<0>]; }

    template <typename, size_t _Bs, typename _UAbi>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_convert(std::datapar::basic_simd_mask<_Bs, _UAbi> __x)
      { return __x[0]; }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_logical_and(bool __x, bool __y)
    { return __x && __y; }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_logical_or(bool __x, bool __y)
    { return __x || __y; }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_bit_not(bool __x)
    { return !__x; }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_bit_and(bool __x, bool __y)
    { return __x && __y; }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_bit_or(bool __x, bool __y)
    { return __x || __y; }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_bit_xor(bool __x, bool __y)
    { return __x != __y; }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_get(bool __k, _SimdSizeType __i)
    {
      if (__i != 0)
        __invoke_ub("Subscript %d is out of range: must be 0", __i);
      return __k;
    }

    _GLIBCXX_SIMD_INTRINSIC static constexpr void
    _S_set(bool& __k, _SimdSizeType __i, bool __x)
    {
      if (__i != 0)
        __invoke_ub("Subscript %d is out of range: must be 0", __i);
      __k = __x;
    }

    _GLIBCXX_SIMD_INTRINSIC static constexpr void
    _S_masked_assign(bool __k, bool& __lhs, bool __rhs)
    {
      if (__k)
        __lhs = __rhs;
    }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_all_of(bool __k) noexcept
    { return __k; }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_any_of(bool __k) noexcept
    { return __k; }

    _GLIBCXX_SIMD_INTRINSIC static constexpr bool
    _S_none_of(bool __k) noexcept
    { return !__k; }

    template <size_t _Bs>
      _GLIBCXX_SIMD_INTRINSIC static constexpr bool
      _S_popcount(std::datapar::basic_simd_mask<_Bs, abi_type> __k)
      { return __data(__k); }

    template <size_t _Bs>
      _GLIBCXX_SIMD_INTRINSIC static constexpr int
      _S_reduce_min_index(std::datapar::basic_simd_mask<_Bs, abi_type>)
      { return 0; }

    template <size_t _Bs>
      _GLIBCXX_SIMD_INTRINSIC static constexpr int
      _S_reduce_max_index(std::datapar::basic_simd_mask<_Bs, abi_type>)
      { return 0; }
  };
}
#endif // PROTOTYPE_SIMD_SCALAR_H_
