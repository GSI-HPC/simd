/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_DETAIL_H_
#define PROTOTYPE_DETAIL_H_

#include "fwddecl.h"
#include "x86_detail.h"
#include "constexpr_wrapper.h"

#include <concepts>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>

namespace std::__detail
{
  template <typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC constexpr bool
    __is_power2_minus_1(_Tp __x)
    {
      using _Ip = __make_signed_int_t<_Tp>;
      _Ip __y = __builtin_bit_cast(_Ip, __x);
      return __y == -1 or std::__has_single_bit(__x + 1);
    }

  /**@internal
   * Helper __may_alias<_Tp> that turns _Tp into the type to be used for an
   * aliasing pointer. This adds the __may_alias attribute to _Tp (with compilers
   * that support it).
   */
  template <typename _Tp>
    using __may_alias [[__gnu__::__may_alias__]] = _Tp;

  _GLIBCXX_SIMD_INTRINSIC constexpr bool
  __ptr_is_aligned_to(const void* __ptr, size_t __align)
  {
    const auto __addr = __builtin_bit_cast(__UINTPTR_TYPE__, __ptr);
    return (__addr % __align) == 0;
  }

  /**
   * @internal
   * Tag used for private init constructor of simd and simd_mask
   */
  struct _PrivateInit
  {
    explicit _PrivateInit() = default;
  };

  inline constexpr _PrivateInit __private_init = _PrivateInit{};

  template <std::integral _Tp, typename _Fp>
    _GLIBCXX_SIMD_INTRINSIC static void
    _S_bit_iteration(_Tp __mask, _Fp&& __f)
    {
      static_assert(sizeof(0ULL) >= sizeof(_Tp));
      conditional_t<sizeof(_Tp) <= sizeof(0u), unsigned, unsigned long long> __k = __mask;
      while(__k)
        {
          __f(std::__countr_zero(__k));
          __k &= (__k - 1);
        }
    }

  template <size_t _Np, bool _Sanitized, typename _Fp>
    _GLIBCXX_SIMD_INTRINSIC static void
    _S_bit_iteration(_BitMask<_Np, _Sanitized> __mask, _Fp&& __f)
    { _S_bit_iteration(__mask._M_sanitized()._M_to_bits(), __f); }

#ifdef math_errhandling
  // Determine if math functions must raise floating-point exceptions.
  // math_errhandling may expand to an extern symbol, in which case we must assume fp exceptions
  // need to be considered.
  template <int __me = math_errhandling>
    consteval bool
    __handle_fpexcept_impl(int)
    { return __me & MATH_ERREXCEPT; }

  // Fallback if math_errhandling doesn't work: implement correct exception behavior.
  consteval bool
  __handle_fpexcept_impl(float)
  { return true; }
#endif

  struct _BuildFlags
  : _ArchFlags
  {
    constexpr bool
    _M_test(int __bit) const
    { return ((_M_build_flags >> __bit) & 1) == 1; }

    constexpr bool
    _M_handle_fp_exceptions() const
    { return _M_test(0); }

    constexpr bool
    _M_fast_math() const
    { return _M_test(1); }

    constexpr bool
    _M_finite_math_only() const
    { return _M_test(2); }

    constexpr bool
    _M_no_signed_zeros() const
    { return _M_test(3); }

    constexpr bool
    _M_signed_zeros() const
    { return not _M_test(3); }

    constexpr bool
    _M_reciprocal_math() const
    { return _M_test(4); }

    constexpr bool
    _M_no_math_errno() const
    { return _M_test(5); }

    constexpr bool
    _M_math_errno() const
    { return not _M_test(5); }

    constexpr bool
    _M_associative_math() const
    { return _M_test(6); }

    uint64_t _M_build_flags = 0
#if __NO_TRAPPING_MATH__ or __FAST_MATH__
                          + (1 << 0)
#elif defined math_errhandling
                          + (__handle_fpexcept_impl(0) << 0)
#endif
#if __FAST_MATH__
                          + (1 << 1)
#endif
#if __FINITE_MATH_ONLY__
                          + (1 << 2)
#endif
#if __NO_SIGNED_ZEROS__
                          + (1 << 3)
#endif
#if __RECIPROCAL_MATH__
                          + (1 << 4)
#endif
#if __NO_MATH_ERRNO__
                          + (1 << 5)
#endif
#if __ASSOCIATIVE_MATH__
                          + (1 << 6)
#endif
#if __FLT_EVAL_METHOD__ == 1
                          + (1 << 7)
#elif __FLT_EVAL_METHOD__ == 2
                          + (2 << 7)
#elif __FLT_EVAL_METHOD__ != 0
                          + (3 << 7)
#endif
                        ;
  };

  /**@internal
   * You must use this type as template argument to function templates that are not declared
   * always_inline (to avoid issues when linking code compiled with different compiler flags).
   */
  using __build_flags = _BuildFlags;
}

#define _GLIBCXX_SIMD_TOSTRING_IMPL(x) #x
#define _GLIBCXX_SIMD_TOSTRING(x) _GLIBCXX_SIMD_TOSTRING_IMPL(x)
#define _GLIBCXX_SIMD_LOC __FILE__ ":" _GLIBCXX_SIMD_TOSTRING(__LINE__) ": "

#if not IFNDR_SIMD_PRECONDITIONS
#define __glibcxx_simd_precondition(expr, msg, ...)                                                \
  do {                                                                                             \
    if (__builtin_expect(!bool(expr), false))                                                      \
      std::__detail::__invoke_ub(                                                                  \
        _GLIBCXX_SIMD_LOC "precondition failure in '%s':\n" msg " ('" #expr "' does not hold)",    \
        __PRETTY_FUNCTION__ __VA_OPT__(,) __VA_ARGS__);                                            \
  } while(false)
#else
#define __glibcxx_simd_precondition(expr, msg, ...)                                                \
  do {                                                                                             \
    const bool __precondition_result = !bool(expr);                                                \
    if (__builtin_constant_p(__precondition_result) && __precondition_result)                      \
      []() __attribute__((__noinline__, __noipa__, __error__("precondition failure."               \
        "\n" _GLIBCXX_SIMD_LOC "note: " msg " (precondition '" #expr "' does not hold)")))         \
      { __builtin_unreachable(); }();                                                              \
    else if (__builtin_expect(__precondition_result, false))                                       \
      std::__detail::__invoke_ub(                                                                  \
        _GLIBCXX_SIMD_LOC "precondition failure in '%s':\n" msg " ('" #expr "' does not hold)",    \
        __PRETTY_FUNCTION__ __VA_OPT__(,) __VA_ARGS__);                                            \
  } while(false)
#endif

namespace std
{
  namespace __detail
  {
    template <typename... _Args>
      [[noreturn]] _GLIBCXX_SIMD_ALWAYS_INLINE inline void
      __invoke_ub([[maybe_unused]] const char* __msg, [[maybe_unused]] const _Args&... __args)
      {
#ifdef _GLIBCXX_ASSERTIONS
        __builtin_fprintf(stderr, __msg, __args...);
        __builtin_fprintf(stderr, "\n");
        __builtin_abort();
#elif _GLIBCXX_HARDEN >= 3
        __builtin_trap();
#else
        __builtin_unreachable();
#endif
      }

    // work around __builtin_constant_p returning false unless passed a variable
    // (__builtin_constant_p(x[0]) is false while __is_constprop(x[0]) is true)
    _GLIBCXX_SIMD_ALWAYS_INLINE constexpr bool
    __is_constprop(const auto& __x)
    { return __builtin_is_constant_evaluated() or __builtin_constant_p(__x); }

    struct _InvalidTraits
    {
      struct _Unusable
      {
        _Unusable() = delete;
        _Unusable(const _Unusable&) = delete;
        _Unusable& operator=(const _Unusable&) = delete;
        ~_Unusable() = delete;
      };

      template <typename>
        static constexpr bool _S_explicit_mask_conversion = true;

      static constexpr int _S_size = 0;
      static constexpr int _S_full_size = 0;
      static constexpr bool _S_is_partial = false;

      static constexpr size_t _S_simd_align = 1;
      struct _SimdImpl;
      using _SimdMember = _Unusable;
      struct _SimdCastType;

      static constexpr size_t _S_mask_align = 1;
      struct _MaskImpl;
      using _MaskMember = _Unusable;
      struct _MaskCastType;
    };

    template <typename _Tp, typename _Abi, _BuildFlags = {}>
      struct _SimdTraits
      : _InvalidTraits
      {};

    template <typename _Tp, typename _Abi, _BuildFlags _Flags>
      requires (_Abi::template _IsValid<_Tp>::value)
      struct _SimdTraits<_Tp, _Abi, _Flags>
      : _Abi::template __traits<_Tp>
      {};

    /**
     * Masks need to be different for AVX without AVX2.
     */
    template <size_t _Bs, typename _Abi, _BuildFlags _Flags = {}>
      struct _SimdMaskTraits
      : _SimdTraits<__mask_integer_from<_Bs>, _Abi, _Flags>
      {};

    template <__vectorizable _Tp,
              _SimdSizeType _Np = std::datapar::__simd_size_v<_Tp, _NativeAbi<_Tp>>>
      requires (not is_same_v<typename __deduce_t<_Tp, _Np>::template __traits<_Tp>,
                              _InvalidTraits>)
      using __deduced_traits = typename __deduce_t<_Tp, _Np>::template __traits<_Tp>;

    template <typename _Rg>
      constexpr size_t
      __static_range_size(_Rg&& __r)
      {
        if consteval
          {
#if 0 // PR117849
            if constexpr (requires {
                            typename integral_constant<size_t, ranges::size(__r2)>;
                          })
              return ranges::size(__r);
            else
              return dynamic_extent;
#else
            return decltype(span(__r))::extent;
#endif
          }
      }

    _GLIBCXX_SIMD_INTRINSIC _SimdSizeType
    __lowest_bit(std::integral auto __bits)
    {
      if constexpr (sizeof(__bits) <= sizeof(int))
        return __builtin_ctz(__bits);
      else if constexpr (sizeof(__bits) <= sizeof(long))
        return __builtin_ctzl(__bits);
      else if constexpr (sizeof(__bits) <= sizeof(long long))
        return __builtin_ctzll(__bits);
      else
        __assert_unreachable<decltype(__bits)>();
    }

    _GLIBCXX_SIMD_INTRINSIC _SimdSizeType
    __highest_bit(std::integral auto __bits)
    {
      if constexpr (sizeof(__bits) <= sizeof(int))
        return sizeof(int) * __CHAR_BIT__ - 1 - __builtin_clz(__bits);
      else if constexpr (sizeof(__bits) <= sizeof(long))
        return sizeof(long) * __CHAR_BIT__ - 1 - __builtin_clzl(__bits);
      else if constexpr (sizeof(__bits) <= sizeof(long long))
        return sizeof(long long) * __CHAR_BIT__ - 1 - __builtin_clzll(__bits);
      else
        __assert_unreachable<decltype(__bits)>();
    }

    // std::common_type but without integral promotions
    template <typename _T0, typename _T1>
      struct __nopromot_common_type : std::common_type<_T0, _T1>
      {};

    template <typename _Tp>
      struct __nopromot_common_type<_Tp, _Tp>
      { using type = _Tp; };

    template <typename _T0, typename _T1>
      requires __higher_integer_rank_than<int, _T0> and __higher_integer_rank_than<int, _T1>
        and (std::is_signed_v<_T0> == std::is_signed_v<_T1>)
      struct __nopromot_common_type<_T0, _T1>
      : std::conditional<__higher_integer_rank_than<_T0, _T1>, _T0, _T1>
      {};

    template <typename _T0, typename _T1>
      requires __higher_integer_rank_than<int, _T0> and __higher_integer_rank_than<int, _T1>
        and (std::is_signed_v<_T0> != std::is_signed_v<_T1>)
      struct __nopromot_common_type<_T0, _T1>
      {
        using _Up = std::conditional_t<std::is_signed_v<_T0>, _T1, _T0>;
        using _Sp = std::conditional_t<std::is_signed_v<_T0>, _T0, _T1>;
        using type = std::conditional_t<(sizeof(_Up) >= sizeof(_Sp)), _Up, _Sp>;
      };

    template <typename _T0, typename _T1>
      using __nopromot_common_type_t = typename __nopromot_common_type<_T0, _T1>::type;
  }
}

namespace std::__detail
{
  template <typename _Up, typename _Accessor = _Up,
            typename _ValueType = typename _Up::value_type>
    class _SmartReference
    {
      friend _Accessor;
      _SimdSizeType _M_index;
      _Up& _M_obj;

      _GLIBCXX_SIMD_INTRINSIC constexpr _ValueType
      _M_read() const noexcept
      { return _Accessor::_S_get(_M_obj, _M_index); }

      template <typename _Tp>
        _GLIBCXX_SIMD_INTRINSIC constexpr void
        _M_write(_Tp&& __x) const
        { _Accessor::_S_set(_M_obj, _M_index, static_cast<_Tp&&>(__x)); }

    public:
      _GLIBCXX_SIMD_INTRINSIC constexpr
      _SmartReference(_Up& __o, _SimdSizeType __i) noexcept
      : _M_index(__i), _M_obj(__o) {}

      using value_type = _ValueType;

      _SmartReference(const _SmartReference&) = delete;

      _GLIBCXX_SIMD_INTRINSIC constexpr
      operator value_type() const noexcept
      { return _M_read(); }

      template <__broadcast_constructible<value_type> _Tp>
        _GLIBCXX_SIMD_INTRINSIC constexpr _SmartReference
        operator=(_Tp&& __x) &&
        {
          _M_write(static_cast<_Tp&&>(__x));
          return {_M_obj, _M_index};
        }

#define _GLIBCXX_SIMD_OP_(__op)                                                                    \
      template <__broadcast_constructible<value_type> _Tp>                                         \
        requires __broadcast_constructible<decltype(declval<value_type>() __op declval<_Tp>()),    \
                                           value_type>                                             \
        _GLIBCXX_SIMD_INTRINSIC constexpr _SmartReference                                          \
        operator __op##=(_Tp&& __x) &&                                                             \
        {                                                                                          \
          const value_type& __lhs = _M_read();                                                     \
          _M_write(__lhs __op __x);                                                                \
          return {_M_obj, _M_index};                                                               \
        }

      _GLIBCXX_SIMD_ALL_ARITHMETICS(_GLIBCXX_SIMD_OP_);
      _GLIBCXX_SIMD_ALL_SHIFTS(_GLIBCXX_SIMD_OP_);
      _GLIBCXX_SIMD_ALL_BINARY(_GLIBCXX_SIMD_OP_);
#undef _GLIBCXX_SIMD_OP_

      template <typename _Tp = void,
                typename = decltype(++declval<conditional_t<true, value_type, _Tp>&>())>
        _GLIBCXX_SIMD_INTRINSIC constexpr _SmartReference
        operator++() &&
        {
          value_type __x = _M_read();
          _M_write(++__x);
          return {_M_obj, _M_index};
        }

      template <typename _Tp = void,
                typename = decltype(declval<conditional_t<true, value_type, _Tp>&>()++)>
        _GLIBCXX_SIMD_INTRINSIC constexpr value_type
        operator++(int) &&
        {
          const value_type __r = _M_read();
          value_type __x = __r;
          _M_write(++__x);
          return __r;
        }

      template <typename _Tp = void,
                typename = decltype(--declval<conditional_t<true, value_type, _Tp>&>())>
        _GLIBCXX_SIMD_INTRINSIC constexpr _SmartReference
        operator--() &&
        {
          value_type __x = _M_read();
          _M_write(--__x);
          return {_M_obj, _M_index};
        }

      template <typename _Tp = void,
                typename = decltype(declval<conditional_t<true, value_type, _Tp>&>()--)>
        _GLIBCXX_SIMD_INTRINSIC constexpr value_type
        operator--(int) &&
        {
          const value_type __r = _M_read();
          value_type __x = __r;
          _M_write(--__x);
          return __r;
        }

      _GLIBCXX_SIMD_INTRINSIC friend constexpr void
      swap(_SmartReference&& __a, _SmartReference&& __b) noexcept(
        conjunction<
          is_nothrow_constructible<value_type, _SmartReference&&>,
          is_nothrow_assignable<_SmartReference&&, value_type&&>>::value)
      {
        value_type __tmp = static_cast<_SmartReference&&>(__a);
        static_cast<_SmartReference&&>(__a) = static_cast<value_type>(__b);
        static_cast<_SmartReference&&>(__b) = std::move(__tmp);
      }

      _GLIBCXX_SIMD_INTRINSIC friend constexpr void
      swap(value_type& __a, _SmartReference&& __b) noexcept(
        conjunction<
          is_nothrow_constructible<value_type, value_type&&>,
          is_nothrow_assignable<value_type&, value_type&&>,
          is_nothrow_assignable<_SmartReference&&, value_type&&>>::value)
      {
        value_type __tmp(std::move(__a));
        __a = static_cast<value_type>(__b);
        static_cast<_SmartReference&&>(__b) = std::move(__tmp);
      }

      _GLIBCXX_SIMD_INTRINSIC friend constexpr void
      swap(_SmartReference&& __a, value_type& __b) noexcept(
        conjunction<
          is_nothrow_constructible<value_type, _SmartReference&&>,
          is_nothrow_assignable<value_type&, value_type&&>,
          is_nothrow_assignable<_SmartReference&&, value_type&&>>::value)
      {
        value_type __tmp(__a);
        static_cast<_SmartReference&&>(__a) = std::move(__b);
        __b = std::move(__tmp);
      }
    };
}

namespace std::datapar
{
  template <__detail::__vectorizable _Tp, __detail::__simd_abi_tag _Abi>
    requires requires { _Abi::_S_size; }
    struct __simd_size<_Tp, _Abi>
    : integral_constant<__detail::_SimdSizeType, _Abi::_S_size>
    {};
}

#endif  // PROTOTYPE_DETAIL_H_
