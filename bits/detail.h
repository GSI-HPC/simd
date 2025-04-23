/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_DETAIL_H_
#define PROTOTYPE_DETAIL_H_

#include "fwddecl.h"
#include "x86_detail.h"

#include <concepts>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>
#if defined _GLIBCXX_ASSERTIONS and __GNUC__ < 15
#include <iostream>
#endif

namespace std::__detail
{
  // implemented here so that __hadd_insn overloads in target-specific *_detail.h are visible
  template <__vec_builtin _TV, int _Np, int... _Is>
    _GLIBCXX_SIMD_INTRINSIC constexpr _TV
    _VecOps<_TV, _Np, integer_sequence<int, _Is...>>::_S_hadd(_TV __x, _TV __y)
    {
      if (not __builtin_is_constant_evaluated()
            and not (__builtin_constant_p(__x) and __builtin_constant_p(__y)))
        {
          if constexpr (requires {__hadd_insn(__x, __y);})
            return __hadd_insn(__x, __y);
        }
      // Clang recognizes this pattern, GCC doesn't (yet)
      return __builtin_shufflevector(__x, __y, (_Is * 2)...)
               + __builtin_shufflevector(__x, __y, (_Is * 2 + 1)...);
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

  consteval void
  __assert_little_endian()
  {
    static_assert(__builtin_bit_cast(array<char, sizeof(int)>, 1)[0] == 1, "expect little endian");
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

  template <size_t _Np, bool _Sanitized = false>
    struct _BitMask;

  template <size_t _Np>
    using _SanitizedBitMask = _BitMask<_Np, true>;

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

  /**@internal
   * You must use this type as template argument to function templates that are not declared
   * always_inline (to avoid issues when linking code compiled with different compiler flags).
   */
  struct _BuildFlags
  : _ArchFlags, _OptFlags
  {};
}

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

namespace std::__detail
{
  template <typename... _Args>
    [[noreturn]] _GLIBCXX_SIMD_ALWAYS_INLINE inline void
    __invoke_ub([[maybe_unused]] const char* __msg, [[maybe_unused]] const _Args&... __args)
    {
#ifdef _GLIBCXX_ASSERTIONS
#if __GNUC__ < 15
      ((std::cerr << __msg) << ... << __args) << '\n';
#else
      __builtin_fprintf(stderr, __msg, __args...);
      __builtin_fprintf(stderr, "\n");
#endif
      __builtin_abort();
#elif _GLIBCXX_HARDEN >= 3
      __builtin_trap();
#else
      __builtin_unreachable();
#endif
    }

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
    requires (not __complex_like<_Tp>) and _Abi::template _IsValid<_Tp>::value
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

  template <__vectorizable _Tp, _SimdSizeType _Np = __simd_size_v<_Tp, _NativeAbi<_Tp>>>
    requires (not is_same_v<typename __deduce_t<_Tp, _Np>::template __traits<_Tp>,
                            _InvalidTraits>)
    using __deduced_traits = typename __deduce_t<_Tp, _Np>::template __traits<_Tp>;

  template <typename _Rg>
    consteval size_t
    __static_range_size(_Rg&& __r)
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

  template <__vectorizable _Tp, __simd_abi_tag _Abi>
    requires requires { _Abi::_S_size; }
    struct __simd_size<_Tp, _Abi>
    : integral_constant<_SimdSizeType, _Abi::_S_size>
    {};

  template <typename _Tp>
    struct __complex_of;

  template <typename _Tp>
    using __complex_of_t = typename __complex_of<_Tp>::type;

  template <>
    struct __complex_of<_Float16>
    { using type = _Complex _Float16; };

  template <>
    struct __complex_of<_Float32>
    { using type = _Complex _Float32; };

  template <>
    struct __complex_of<_Float64>
    { using type = _Complex _Float64; };

  template <>
    struct __complex_of<float>
    { using type = _Complex float; };

  template <>
    struct __complex_of<double>
    { using type = _Complex double; };

  // TODO: rm
  template <std::floating_point _Tp>
    struct _SimpleComplex
    {
      using value_type = _Tp;

      _Tp _M_real = {};
      _Tp _M_imag = {};

      constexpr _Tp
      real() const
      { return _M_real; }

      constexpr _Tp
      imag() const
      { return _M_real; }

      friend constexpr _Tp
      real(const _SimpleComplex& __x)
      { return __x._M_real; }

      friend constexpr _Tp
      imag(const _SimpleComplex& __x)
      { return __x._M_imag; }

      // unary
      friend constexpr _SimpleComplex
      operator+(const _SimpleComplex& __x)
      { return __x; }

      friend constexpr _SimpleComplex
      operator-(const _SimpleComplex& __x)
      { return {-__x._M_real, -__x._M_imag}; }

      // compound assignment
      friend constexpr _SimpleComplex&
      operator+=(_SimpleComplex& __x, _Tp __y)
      { __x._M_real += __y; return __x; }

      friend constexpr _SimpleComplex&
      operator-=(_SimpleComplex& __x, _Tp __y)
      { __x._M_real -= __y; return __x; }

      friend constexpr _SimpleComplex&
      operator*=(_SimpleComplex& __x, _Tp __y)
      { __x._M_real *= __y; __x._M_imag *= __y; return __x; }

      friend constexpr _SimpleComplex&
      operator/=(_SimpleComplex& __x, _Tp __y)
      { __x._M_real /= __y; __x._M_imag /= __y; return __x; }

      friend constexpr _SimpleComplex&
      operator+=(_SimpleComplex& __x, const _SimpleComplex& __y)
      { __x._M_real += __y._M_real; __x._M_imag += __y._M_imag; return __x; }

      friend constexpr _SimpleComplex&
      operator-=(_SimpleComplex& __x, const _SimpleComplex& __y)
      { __x._M_real -= __y._M_real; __x._M_imag -= __y._M_imag; return __x; }

      friend constexpr _SimpleComplex&
      operator*=(_SimpleComplex& __x, const _SimpleComplex& __y)
      { return __x = __x * __y; }

      friend constexpr _SimpleComplex&
      operator/=(_SimpleComplex& __x, const _SimpleComplex& __y)
      { return __x = __x / __y; }

      // +,-,*,/
      friend constexpr _SimpleComplex
      operator+(_SimpleComplex& __x, _Tp __y)
      { return _SimpleComplex(__x) += __y; }

      friend constexpr _SimpleComplex
      operator-(_SimpleComplex& __x, _Tp __y)
      { return _SimpleComplex(__x) -= __y; }

      friend constexpr _SimpleComplex
      operator*(_SimpleComplex& __x, _Tp __y)
      { return _SimpleComplex(__x) *= __y; }

      friend constexpr _SimpleComplex
      operator/(_SimpleComplex& __x, _Tp __y)
      { return _SimpleComplex(__x) /= __y; }

      friend constexpr _SimpleComplex
      operator+(_SimpleComplex& __x, const _SimpleComplex& __y)
      { return _SimpleComplex(__x) += __y; }

      friend constexpr _SimpleComplex
      operator-(_SimpleComplex& __x, const _SimpleComplex& __y)
      { return _SimpleComplex(__x) -= __y; }

      friend constexpr _SimpleComplex
      operator*(const _SimpleComplex& __x, const _SimpleComplex& __y)
      {
        return {__x._M_real * __y._M_real - __x._M_imag * __y._M_imag,
                __x._M_real * __y._M_imag + __x._M_imag * __y._M_real};
      }

      friend constexpr _SimpleComplex
      operator/(const _SimpleComplex& __x, const _SimpleComplex& __y)
      { return (__x * conj(__y)) / norm(__y); }

      // associated functions
      friend constexpr _Tp
      abs(const _SimpleComplex& __x)
      {
        const _Tp __scale = max(abs(__x._M_real), abs(__x._M_imag));
        if (__scale == _Tp())
          return _Tp();
        else
          return __scale * sqrt(norm(__x / __scale));
      }

      friend constexpr _Tp
      norm(const _SimpleComplex& __x)
      { return __x._M_real * __x._M_real + __x._M_imag * __x._M_imag; }

      friend constexpr _SimpleComplex
      conj(const _SimpleComplex& __x)
      { return {__x._M_real, -__x._M_imag}; }
    };

  template <floating_point _Tp>
    inline constexpr __make_unsigned_int_t<_Tp> __fp_sign_mask
      = __builtin_bit_cast(__make_unsigned_int_t<_Tp>, _Tp(1))
          ^ __builtin_bit_cast(__make_unsigned_int_t<_Tp>, _Tp(-1));

  template <floating_point _Tp>
    inline constexpr __make_unsigned_int_t<_Tp> __fp_abs_mask = ~__fp_sign_mask<_Tp>;

  template <floating_point _Tp>
    inline constexpr __make_unsigned_int_t<_Tp> __fp_exponent_mask
      = __builtin_bit_cast(__make_unsigned_int_t<_Tp>, numeric_limits<_Tp>::infinity());

  template <floating_point _Tp>
    inline constexpr __make_unsigned_int_t<_Tp> __fp_exponent_bias
      = __builtin_bit_cast(__make_unsigned_int_t<_Tp>, _Tp(1));

  // Squaring a value with exponent >= big_exponent overflows
  template <floating_point _Tp>
    inline constexpr __make_unsigned_int_t<_Tp> __fp_big_exponent
      = (__fp_exponent_mask<_Tp> + __fp_exponent_bias<_Tp>) / 2;

  template <typename _Vp>
    requires requires { typename _Vp::abi_type; } and (not requires { _Vp::_S_bytes; })
    consteval bool
    __why_is_it_disabled()
    {
      if constexpr (not is_destructible_v<_Vp>)
        {
          using _Tp = typename _Vp::value_type;
          using _Abi = typename _Vp::abi_type;
          static_assert(__vectorizable<_Tp>, "the value type is not vectorizable");
          if constexpr (__simd_abi_tag<_Abi>)
            {
              static_assert(_Abi::_IsValidAbiTag::value,
                            "the ABI tag template was specialized with an invalid argument");
              static_assert(_Abi::template _IsValidSizeFor<_Tp>::value,
                            "the requested value type and width require a different ABI tag");
              static_assert(_Abi::template _IsValid<_Tp>::value,
                            "the ABI tag cannot be combined with the given value type (see preceding "
                            "static_asserts)");
            }
          else
            static_assert(__simd_abi_tag<_Abi>, "the ABI tag argument is not an ABI tag");
        }
      return true;
    }

  template <typename _Vp>
    requires requires { typename _Vp::abi_type; } and requires { _Vp::_S_bytes; }
    consteval bool
    __why_is_it_disabled()
    {
      if constexpr (not is_destructible_v<_Vp>)
        {
          constexpr size_t _Bytes = _Vp::_S_bytes;
          using _Tp = __mask_integer_from<_Bytes>;
          using _Abi = typename _Vp::abi_type;
          static_assert(__has_single_bit(_Bytes),
                        "the requested mask element width is not a power-of-2 value");
          if constexpr (__simd_abi_tag<_Abi>)
            {
              static_assert(_Abi::_IsValidAbiTag::value,
                            "the ABI tag template was specialized with an invalid argument");
              static_assert(_Abi::template _IsValidSizeFor<_Tp>::value,
                            "the requested value type and width require a different ABI tag");
              static_assert(_Abi::template _IsValid<_Tp>::value,
                            "the ABI tag cannot be combined with the given value type (see preceding "
                            "static_asserts)");
            }
          else
            static_assert(__simd_abi_tag<_Abi>, "the ABI tag argument is not an ABI tag");
        }
      return true;
    }

  template <int _OutputBits = 4, _BuildFlags _Flags = {}>
    constexpr _UInt<1>
    __bit_extract_even(_UInt<1> __x)
    {
      static_assert(_OutputBits <= 4);
      constexpr _UInt<1> __mask = 0x55u >> ((4 - _OutputBits) * 2);
#if defined __BMI2__
      return __builtin_ia32_pext_si(__x, __mask);
#else
      __x &= __mask;
      __x |= __x >> 1;
      __x &= 0x33u;
      __x |= __x >> 2;
      __x &= 0x0Fu;
      return __x;
#endif
    }

  template <int _OutputBits = 8, _BuildFlags _Flags = {}>
    constexpr _UInt<1>
    __bit_extract_even(_UInt<2> __x)
    {
      if constexpr (_OutputBits <= 4)
        return __bit_extract_even<_OutputBits>(_UInt<1>(__x));
      else
        {
          static_assert(_OutputBits <= 8);
          constexpr _UInt<2> __mask = 0x5555u >> ((8 - _OutputBits) * 2);
#if defined __BMI2__
          return __builtin_ia32_pext_si(__x, __mask);
#else
          __x &= __mask;
          __x |= __x >> 1;
          __x &= 0x3333u;
          __x |= __x >> 2;
          __x &= 0x0F0Fu;
          __x |= __x >> 4;
          return __x;
#endif
        }
    }

  template <int _OutputBits = 16, _BuildFlags _Flags = {}>
    constexpr _UInt<_OutputBits <= 8 ? 1 : 2>
    __bit_extract_even(_UInt<4> __x)
    {
      if constexpr (_OutputBits <= 4)
        return __bit_extract_even<_OutputBits>(_UInt<1>(__x));
      else if constexpr (_OutputBits <= 8)
        return __bit_extract_even<_OutputBits>(_UInt<2>(__x));
      else
        {
          static_assert(_OutputBits <= 16);
          constexpr _UInt<4> __mask = 0x5555'5555u >> ((16 - _OutputBits) * 2);
#if defined __BMI2__
          return __builtin_ia32_pext_si(__x, __mask);
#else
          __x &= __mask;
          __x |= __x >> 1;
          __x &= 0x3333'3333u;
          __x |= __x >> 2;
          __x &= 0x0F0F'0F0Fu;
          __x |= __x >> 4;
          __x &= 0x00FF'00FFu;
          __x |= __x >> 8;
          return __x;
#endif
        }
    }

  template <int _OutputBits = 32, _BuildFlags _Flags = {}>
    constexpr _UInt<_OutputBits <= 8 ? 1 : _OutputBits <= 16 ? 2 : 4>
    __bit_extract_even(_UInt<8> __x)
    {
      if constexpr (_OutputBits <= 4)
        return __bit_extract_even<_OutputBits>(_UInt<1>(__x));
      else if constexpr (_OutputBits <= 8)
        return __bit_extract_even<_OutputBits>(_UInt<2>(__x));
      else if constexpr (_OutputBits <= 16)
        return __bit_extract_even<_OutputBits>(_UInt<4>(__x));
      else
        {
          static_assert(_OutputBits <= 32);
          constexpr _UInt<8> __mask = 0x5555'5555'5555'5555ull >> ((32 - _OutputBits) * 2);
#if defined __BMI2__ and __has_builtin(__builtin_ia32_pext_di)
          return __builtin_ia32_pext_di(__x, __mask);
#elif defined __BMI2__
          return __builtin_ia32_pext_si(__x, static_cast<unsigned>(__mask))
                   | (__builtin_ia32_pext_si(__x >> 32, __mask >> 32) << 16);
#else
          __x &= __mask;
          __x |= __x >> 1;
          __x &= 0x3333'3333'3333'3333ull;
          __x |= __x >> 2;
          __x &= 0x0F0F'0F0F'0F0F'0F0Full;
          __x |= __x >> 4;
          __x &= 0x00FF'00FF'00FF'00FFull;
          __x |= __x >> 8;
          __x &= 0x0000'FFFF'0000'FFFFull;
          __x |= __x >> 16;
          return __x;
#endif
        }
    }

  // input bits must be 0 for all bits > _InputBits
  template <int _InputBits = 8, _BuildFlags _Flags = {}>
    constexpr _UInt<_InputBits <= 4 ? 1 : 2>
    __duplicate_each_bit(_UInt<1> __x)
    {
      static_assert(_InputBits <= 8);
      constexpr _UInt<2> __mask = 0x5555u >> ((8 - _InputBits) * 2);
      if constexpr (_InputBits == 1)
        return __x * 3;
#if defined __BMI2__
      else
        return 3 * __builtin_ia32_pdep_si(__x, __mask);
#else
      else if constexpr (_InputBits == 2) // 0000'00BA
        return ((__x + 0b0010u) & 0b0101u) * 3; // 0B?A -> 0B0A -> BBAA
      else if constexpr (_InputBits <= 4) // 0000'DCBA
        {
          __x = ((__x << 2) | __x ) & 0b0011'0011u; // 00DC'??BA -> 00DC'00BA
          return ((__x + 0b0010'0010u) & __mask) * 3;     // -> DDCC'BBAA
        }
      else
        { // HGFE'DCBA
          _UInt<2> __y = ((__x << 4) | __x) & 0x0F0Fu; // HGFE'0000'DCBA
          __y |= __y << 2; // 00HG'??FE'00DC'??BA
          __y &= 0x3333u;  // 00HG'00FE'00DC'00BA
          __y += 0x2222u;  // 0H?G'0F?E'0D?C'0B?A
          return (__y & __mask) * 3; // HHGG'FFEE'DDCC'BBAA
        }
#endif
    }

  template <int _InputBits = 16, _BuildFlags _Flags = {}>
    constexpr _UInt<4>
    __duplicate_each_bit(_UInt<2> __x)
    {
      if constexpr (_InputBits <= 8)
        return __duplicate_each_bit<_InputBits>(_UInt<1>(__x));
      else
        {
          static_assert(_InputBits <= 16);
          constexpr _UInt<4> __mask = 0x5555'5555u >> ((16 - _InputBits) * 2);
#if defined __BMI2__
          return 3 * __builtin_ia32_pdep_si(__x, __mask);
#else
          _UInt<4> __y = ((__x << 8) | __x) & 0x00FF00FFu;
          __y |= __y << 4;
          __y &= 0x0F0F'0F0Fu;
          __y |= __y << 2;
          __y &= 0x3333'3333u;
          return ((__y + 0x2222'2222u) & __mask) * 3;
#endif
        }
    }

  template <int _InputBits = 32, _BuildFlags _Flags = {}>
    constexpr _UInt<8>
    __duplicate_each_bit(_UInt<4> __x)
    {
      if constexpr (_InputBits <= 8)
        return __duplicate_each_bit<_InputBits>(_UInt<1>(__x));
      else if constexpr (_InputBits <= 16)
        return __duplicate_each_bit<_InputBits>(_UInt<2>(__x));
      else
        {
          static_assert(_InputBits <= 32);
          constexpr _UInt<8> __mask = 0x5555'5555'5555'5555u >> ((32 - _InputBits) * 2);
#if defined __BMI2__ and __has_builtin(__builtin_ia32_pdep_di)
          return 3 * __builtin_ia32_pdep_di(__x, __mask);
#elif defined __BMI2__
          const _UInt<8> __hi = 3 * __builtin_ia32_pdep_si(__x >> 16, __mask >> 32);
          return (3 * __builtin_ia32_pdep_si(__x, static_cast<unsigned>(__mask))) | __hi << 32;
#else
          _UInt<8> __y = ((__x & 0xFFFF'0000ull) << 16) | (__x & 0x0000'FFFFu);
          __y |= __y << 8;
          __y &= 0x00FF'00FF'00FF'00FFull;
          __y |= __y << 4;
          __y &= 0x0F0F'0F0F'0F0F'0F0Full;
          __y |= __y << 2;
          __y &= 0x3333'3333'3333'3333ull;
          return ((__y + 0x2222'2222'2222'2222ull) & __mask) * 3;
#endif
        }
    }
}

#endif  // PROTOTYPE_DETAIL_H_
