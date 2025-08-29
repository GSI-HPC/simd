/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef BITS_SIMD_DETAILS_H_
#define BITS_SIMD_DETAILS_H_

#include <bit>
#include <concepts>
#include <limits>
#include <complex>

#include <bits/ranges_base.h>
#include <bits/utility.h>

#ifndef __clang__
#include <bits/c++config.h>
#endif

#if __CHAR_BIT__ != 8
// There are simply too many constants and bit operators that currently depend on CHAR_BIT == 8.
// Generalization to CHAR_BIT != 8 does not make sense without testability (i.e. a test target).
#error "<simd> is not supported for CHAR_BIT != 8"
#endif

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

// x86 macros {

#ifdef __MMX__
#define _GLIBCXX_SIMD_HAVE_MMX 1ull
#else
#define _GLIBCXX_SIMD_HAVE_MMX 0ull
#endif

#if defined __SSE__ || defined __x86_64__
#define _GLIBCXX_SIMD_HAVE_SSE 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE 0ull
#endif

#if defined __SSE2__ || defined __x86_64__
#define _GLIBCXX_SIMD_HAVE_SSE2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE2 0ull
#endif

#ifdef __SSE3__
#define _GLIBCXX_SIMD_HAVE_SSE3 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE3 0ull
#endif

#ifdef __SSSE3__
#define _GLIBCXX_SIMD_HAVE_SSSE3 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSSE3 0ull
#endif

#ifdef __SSE4_1__
#define _GLIBCXX_SIMD_HAVE_SSE4_1 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE4_1 0ull
#endif

#ifdef __SSE4_2__
#define _GLIBCXX_SIMD_HAVE_SSE4_2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE4_2 0ull
#endif

#ifdef __XOP__
#define _GLIBCXX_SIMD_HAVE_XOP 1ull
#else
#define _GLIBCXX_SIMD_HAVE_XOP 0ull
#endif

#ifdef __AVX__
#define _GLIBCXX_SIMD_HAVE_AVX 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX 0ull
#endif

#ifdef __AVX2__
#define _GLIBCXX_SIMD_HAVE_AVX2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX2 0ull
#endif

#ifdef __BMI__
#define _GLIBCXX_SIMD_HAVE_BMI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_BMI 0ull
#endif

#ifdef __BMI2__
#define _GLIBCXX_SIMD_HAVE_BMI2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_BMI2 0ull
#endif

#ifdef __LZCNT__
#define _GLIBCXX_SIMD_HAVE_LZCNT 1ull
#else
#define _GLIBCXX_SIMD_HAVE_LZCNT 0ull
#endif

#ifdef __SSE4A__
#define _GLIBCXX_SIMD_HAVE_SSE4A 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE4A 0ull
#endif

#ifdef __FMA__
#define _GLIBCXX_SIMD_HAVE_FMA 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FMA 0ull
#endif

#ifdef __FMA4__
#define _GLIBCXX_SIMD_HAVE_FMA4 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FMA4 0ull
#endif

#ifdef __F16C__
#define _GLIBCXX_SIMD_HAVE_F16C 1ull
#else
#define _GLIBCXX_SIMD_HAVE_F16C 0ull
#endif

#ifdef __AVXIFMA__
#define _GLIBCXX_SIMD_HAVE_AVXIFMA 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXIFMA 0ull
#endif

#ifdef __AVXNECONVERT__
#define _GLIBCXX_SIMD_HAVE_AVXNECONVERT 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXNECONVERT 0ull
#endif

#ifdef __AVXVNNI__
#define _GLIBCXX_SIMD_HAVE_AVXVNNI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXVNNI 0ull
#endif

#ifdef __AVXVNNIINT8__
#define _GLIBCXX_SIMD_HAVE_AVXVNNIINT8 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXVNNIINT8 0ull
#endif

#ifdef __AVXVNNIINT16__
#define _GLIBCXX_SIMD_HAVE_AVXVNNIINT16 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVXVNNIINT16 0ull
#endif

#ifdef __POPCNT__
#define _GLIBCXX_SIMD_HAVE_POPCNT 1ull
#else
#define _GLIBCXX_SIMD_HAVE_POPCNT 0ull
#endif

#ifdef __AVX512F__
#define _GLIBCXX_SIMD_HAVE_AVX512F 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512F 0ull
#endif

#ifdef __AVX512DQ__
#define _GLIBCXX_SIMD_HAVE_AVX512DQ 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512DQ 0ull
#endif

#ifdef __AVX512VL__
#define _GLIBCXX_SIMD_HAVE_AVX512VL 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VL 0ull
#endif

#ifdef __AVX512BW__
#define _GLIBCXX_SIMD_HAVE_AVX512BW 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512BW 0ull
#endif

#ifdef __AVX512BITALG__
#define _GLIBCXX_SIMD_HAVE_AVX512BITALG 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512BITALG 0ull
#endif

#ifdef __AVX512VBMI2__
#define _GLIBCXX_SIMD_HAVE_AVX512VBMI2 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VBMI2 0ull
#endif

#ifdef __AVX512VBMI__
#define _GLIBCXX_SIMD_HAVE_AVX512VBMI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VBMI 0ull
#endif

#ifdef __AVX512IFMA__
#define _GLIBCXX_SIMD_HAVE_AVX512IFMA 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512IFMA 0ull
#endif

#ifdef __AVX512CD__
#define _GLIBCXX_SIMD_HAVE_AVX512CD 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512CD 0ull
#endif

#ifdef __AVX512VNNI__
#define _GLIBCXX_SIMD_HAVE_AVX512VNNI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VNNI 0ull
#endif

#ifdef __AVX512VPOPCNTDQ__
#define _GLIBCXX_SIMD_HAVE_AVX512VPOPCNTDQ 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VPOPCNTDQ 0ull
#endif

#ifdef __AVX512VP2INTERSECT__
#define _GLIBCXX_SIMD_HAVE_AVX512VP2INTERSECT 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512VP2INTERSECT 0ull
#endif

#ifdef __AVX512FP16__
#define _GLIBCXX_SIMD_HAVE_AVX512FP16 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512FP16 0ull
#endif

#ifdef __AVX512BF16__
#define _GLIBCXX_SIMD_HAVE_AVX512BF16 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512BF16 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_SSE
#define _GLIBCXX_SIMD_HAVE_SSE_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_SSE_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_SSE2
#define _GLIBCXX_SIMD_HAVE_FULL_SSE_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FULL_SSE_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_AVX
#define _GLIBCXX_SIMD_HAVE_AVX_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_AVX2
#define _GLIBCXX_SIMD_HAVE_FULL_AVX_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FULL_AVX_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_AVX512F
#define _GLIBCXX_SIMD_HAVE_AVX512_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_AVX512_ABI 0ull
#endif

#if _GLIBCXX_SIMD_HAVE_AVX512BW
#define _GLIBCXX_SIMD_HAVE_FULL_AVX512_ABI 1ull
#else
#define _GLIBCXX_SIMD_HAVE_FULL_AVX512_ABI 0ull
#endif

#if defined __x86_64__ && !_GLIBCXX_SIMD_HAVE_SSE2
#error "Use of SSE2 is required on AMD64"
#endif

//}

// ARM macros {
#if defined __ARM_NEON
#define _GLIBCXX_SIMD_HAVE_NEON 1
#else
#define _GLIBCXX_SIMD_HAVE_NEON 0
#endif
#if defined __ARM_NEON && (__ARM_ARCH >= 8 || defined __aarch64__)
#define _GLIBCXX_SIMD_HAVE_NEON_A32 1
#else
#define _GLIBCXX_SIMD_HAVE_NEON_A32 0
#endif
#if defined __ARM_NEON && defined __aarch64__
#define _GLIBCXX_SIMD_HAVE_NEON_A64 1
#else
#define _GLIBCXX_SIMD_HAVE_NEON_A64 0
#endif
#if (__ARM_FEATURE_SVE_BITS > 0 && __ARM_FEATURE_SVE_VECTOR_OPERATORS==1)
#define _GLIBCXX_SIMD_HAVE_SVE 1
#else
#define _GLIBCXX_SIMD_HAVE_SVE 0
#endif
// }

#ifndef _GLIBCXX_SIMD_INTRINSIC
#ifdef _GLIBCXX_SIMD_NO_ALWAYS_INLINE
#define _GLIBCXX_SIMD_INTRINSIC inline
#define _GLIBCXX_SIMD_ALWAYS_INLINE
#else
#define _GLIBCXX_SIMD_INTRINSIC [[__gnu__::__always_inline__, __gnu__::__artificial__]] inline
#define _GLIBCXX_SIMD_ALWAYS_INLINE [[__gnu__::__always_inline__]]
#endif
#endif

#if __cpp_deleted_function >= 202403L
#define _GLIBCXX_DELETE_MSG(msg) delete(msg)
#else
#define _GLIBCXX_DELETE_MSG(msg) delete
#endif

#define _GLIBCXX_SIMD_TOSTRING_IMPL(x) #x
#define _GLIBCXX_SIMD_TOSTRING(x) _GLIBCXX_SIMD_TOSTRING_IMPL(x)
#define _GLIBCXX_SIMD_LOC __FILE__ ":" _GLIBCXX_SIMD_TOSTRING(__LINE__) ": "

#if not IFNDR_SIMD_PRECONDITIONS
#define __glibcxx_simd_precondition(expr, msg, ...)                                                \
  do {                                                                                             \
    if (__builtin_expect(!bool(expr), false))                                                      \
      std::simd::__invoke_ub(                                                                      \
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
      std::simd::__invoke_ub(                                                                      \
        _GLIBCXX_SIMD_LOC "precondition failure in '%s':\n" msg " ('" #expr "' does not hold)",    \
        __PRETTY_FUNCTION__ __VA_OPT__(,) __VA_ARGS__);                                            \
  } while(false)
#endif

namespace std::simd
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

  template <typename _Tp>
    inline constexpr _Tp
    __iota = [] { static_assert(false, "invalid __iota specialization"); }();

#if __has_builtin(__integer_pack)
  template <typename _Tp, std::size_t _Np>
    inline constexpr type_identity_t<_Tp[_Np]>
    __iota<_Tp[_Np]> = {__integer_pack(_Tp(_Np))...};
#else
  template<typename _Tp, typename>
    struct __iota_array;

  template<typename _Tp, _Tp... _Is>
    struct __iota_array<_Tp, integer_sequence<_Tp, _Is...>>
    { static constexpr _Tp _S_data[sizeof...(_Is)] = {_Is...}; };

  template <typename _Tp, std::size_t _Np>
    inline constexpr auto&
    __iota<_Tp[_Np]> = __iota_array<_Tp, make_integer_sequence<_Tp, _Np>>::_S_data;
#endif

  template <auto _Value0, auto... _Values>
    requires (is_same_v<decltype(_Value0), decltype(_Values)> and ...)
    inline constexpr type_identity_t<decltype(_Value0)[1 + sizeof...(_Values)]>
    __static_array = {_Value0, _Values...};
}

#if __cpp_structured_bindings >= 202411L
#define _GLIBCXX_SIMD_INT_PACK(N, pack, ...)                                            \
  [&] [[__gnu__::__always_inline__]] {                                                  \
    constexpr auto [...pack] = std::simd::__iota<int[N]>;                               \
    __VA_ARGS__                                                                         \
  }()
#else
#define _GLIBCXX_SIMD_INT_PACK(N, pack, ...)                                            \
  [&]<int... pack> [[__gnu__::__always_inline__]] (std::integer_sequence<int, pack...>) \
  __VA_ARGS__ (std::make_integer_sequence<int, N>())
#endif

namespace std::simd
{
  // [simd.general] vectorizable types
  template <typename _Cp, auto __re, auto __im, typename _Tp = typename _Cp::value_type>
    constexpr _Cp __complex_object = _Cp {_Tp(__re), _Tp(__im)};

  template <typename _Tp>
    struct _Arr2
    { _Tp _M_data[2]; };

  template <typename _Tp>
    concept __complex_like_impl
      = requires(_Tp __x) {
        typename _Tp::value_type;
        { __x.real() } -> same_as<typename _Tp::value_type>;
        { __x.imag() } -> same_as<typename _Tp::value_type>;
        { real(__x) } -> same_as<typename _Tp::value_type>;
        { imag(__x) } -> same_as<typename _Tp::value_type>;
        { +__x } -> same_as<_Tp>;
        { -__x } -> same_as<_Tp>;
        { __x + __x } -> same_as<_Tp>;
        { __x - __x } -> same_as<_Tp>;
        { __x * __x } -> same_as<_Tp>;
        { __x / __x } -> same_as<_Tp>;
        { __x += __x } -> same_as<_Tp&>;
        { __x -= __x } -> same_as<_Tp&>;
        { __x *= __x } -> same_as<_Tp&>;
        { __x /= __x } -> same_as<_Tp&>;
        { abs(__x) } -> same_as<typename _Tp::value_type>;
        { arg(__x) } -> same_as<typename _Tp::value_type>;
        { norm(__x) } -> same_as<typename _Tp::value_type>;
        { conj(__x) } -> same_as<_Tp>;
        { proj(__x) } -> same_as<_Tp>;
      }
          and (__complex_object<_Tp, 1, 2> + _Tp {} == __complex_object<_Tp, 1, 2>)
          and (__complex_object<_Tp, -1, 5> - __complex_object<_Tp, -1, 5> == _Tp {})
          and (__complex_object<_Tp, 2, 3> * __complex_object<_Tp, 1, 1>
                 == __complex_object<_Tp, -1, 5>)
          and (__complex_object<_Tp, 5, 5> / __complex_object<_Tp, 1, 2>
                 == __complex_object<_Tp, 3, -1>)
          and (conj(__complex_object<_Tp, 5, 3>) == __complex_object<_Tp, 5, -3>)
          // not constexpr: and (abs(__complex_object<_Tp, 3, 4>) == typename _Tp::value_type(5))
          and (norm(__complex_object<_Tp, 5, 5>) == typename _Tp::value_type(50))
          and (2 * sizeof(typename _Tp::value_type) == sizeof(_Tp))
          and (__builtin_bit_cast(_Arr2<typename _Tp::value_type>, __complex_object<_Tp, 1, 2>)
                 ._M_data[0] == 1);

  template <typename _Tp>
    concept __complex_like = __complex_like_impl<remove_cvref_t<_Tp>>;

  template <typename _Tp>
    concept __vectorizable_scalar
      = same_as<remove_cv_t<_Tp>, _Tp>
          and ((integral<_Tp> and sizeof(_Tp) <= sizeof(0ULL) and not same_as<_Tp, bool>)
                 or (floating_point<_Tp> and sizeof(_Tp) <= sizeof(double)));

  template <typename _Tp>
    concept __vectorizable
      = __vectorizable_scalar<_Tp>
          or (__complex_like_impl<_Tp> and __vectorizable_scalar<typename _Tp::value_type>
                and floating_point<typename _Tp::value_type>);

  enum class _AbiVariant : unsigned long long
  {
    _VecMask = 1 << 0, // default uses vector masks
    _BitMask = 1 << 1, // switch to bit-masks (AVX512)
    _MaskVariants = _VecMask | _BitMask,
    _CxIleav = 1 << 5, // store complex components interleaved (ririri...)
    _CxCtgus = 1 << 6, // ... or store complex components contigously (rrrr iiii)
    _CxVariants = _CxIleav | _CxCtgus,
  };

  consteval _AbiVariant
  __flags_or(_AbiVariant __x, _AbiVariant __y)
  {
    return static_cast<_AbiVariant>(static_cast<unsigned long long>(__x)
                                      | static_cast<unsigned long long>(__y));
  }

  consteval _AbiVariant
  __flags_and(_AbiVariant __x, _AbiVariant __y)
  {
    return static_cast<_AbiVariant>(static_cast<unsigned long long>(__x)
                                      & static_cast<unsigned long long>(__y));
  }

  consteval bool
  __flags_test(_AbiVariant __flags, _AbiVariant __x)
  { return __flags_or(__flags, __x) == __flags; }

  struct _InvalidInteger
  {};

  template <size_t _Bytes>
    using __integer_from
      = decltype([] {
          if constexpr (sizeof(signed char) == _Bytes)
            return static_cast<signed char>(0);
          else if constexpr (sizeof(signed short) == _Bytes)
            return static_cast<signed short>(0);
          else if constexpr (sizeof(signed int) == _Bytes)
            return static_cast<signed int>(0);
          else if constexpr (sizeof(signed long long) == _Bytes)
            return static_cast<signed long long>(0);
          else
            return _InvalidInteger();
        }());

  template <size_t _Bytes>
    using _UInt = make_unsigned_t<__integer_from<_Bytes>>;

  template <typename _Tp>
    constexpr _Tp
    __div_ceil(_Tp __x, _Tp __y)
    { return (__x + __y - 1) / __y; }

  template <typename _Tp>
    struct __canonical_vec_type
    { using type = _Tp; };

  template <typename _Tp>
    using __canonical_vec_type_t = typename __canonical_vec_type<_Tp>::type;

  template <std::same_as<long> _Tp>
    requires (sizeof(_Tp) == sizeof(int))
    struct __canonical_vec_type<_Tp>
    { using type = int; };

  template <std::same_as<long> _Tp>
    requires (sizeof(_Tp) == sizeof(long long))
    struct __canonical_vec_type<_Tp>
    { using type = long long; };

  template <std::same_as<unsigned long> _Tp>
    requires (sizeof(_Tp) == sizeof(unsigned int))
    struct __canonical_vec_type<_Tp>
    { using type = unsigned int; };

  template <std::same_as<unsigned long> _Tp>
    requires (sizeof(_Tp) == sizeof(unsigned long long))
    struct __canonical_vec_type<_Tp>
    { using type = unsigned long long; };

  template <typename _Tp>
    requires std::is_enum_v<_Tp>
    struct __canonical_vec_type<_Tp>
    { using type = __canonical_vec_type<std::underlying_type_t<_Tp>>::type; };

  template <>
    struct __canonical_vec_type<char>
    { using type = std::conditional_t<std::is_signed_v<char>, signed char, unsigned char>; };

  template <>
    struct __canonical_vec_type<char8_t>
    { using type = unsigned char; };

  template <>
    struct __canonical_vec_type<char16_t>
    { using type = uint_least16_t; };

  template <>
    struct __canonical_vec_type<char32_t>
    { using type = uint_least32_t; };

  template <>
    struct __canonical_vec_type<wchar_t>
    {
      using type = std::conditional_t<std::is_signed_v<wchar_t>,
                                      simd::__integer_from<sizeof(wchar_t)>,
                                      simd::_UInt<sizeof(wchar_t)>>;
    };

  template <>
    struct __canonical_vec_type<_Float64>
    { using type = double; };

  template <>
    struct __canonical_vec_type<_Float32>
    { using type = float; };

  template <int _Np = 1>
    struct _ScalarAbi
    {
      static constexpr int _S_size = _Np;

      static constexpr int _S_nreg = _Np;

      static constexpr _AbiVariant _S_variant = {};

      template <typename _Tp>
        using _DataType = __canonical_vec_type_t<_Tp>;

      static constexpr bool _S_use_2_for_1 = false;

      template <size_t>
        using _MaskDataType = bool;

      template <int _N2, int _Nreg2 = _N2>
        consteval _ScalarAbi<_N2>
        _M_resize() const
        {
          static_assert(_N2 == _Nreg2);
          return {};
        }
    };

  template <int _Np, int _Nreg, _AbiVariant _Var
#ifdef __AVX512F__
              = _AbiVariant::_BitMask
#else
              = _AbiVariant::_VecMask
#endif
           >
    struct _Abi
    {
      static constexpr int _S_size = _Np;

      /**\internal
       * The number of registers needed to represent one basic_vec for the element type that was
       * used on ABI deduction.
       *
       * For _CxCtgus the value applies twice, once per reals and once per imags.
       *
       * Examples:
       * - '_Abi< 8, 2>' for 'int' is 2x 128-bit
       * - '_Abi< 9, 3>' for 'int' is 2x 128-bit and 1x 32-bit
       * - '_Abi<10, 3>' for 'int' is 2x 128-bit and 1x 64-bit
       * - '_Abi<10, 1>' for 'int' is 1x 512-bit
       * - '_Abi<10, 2>' for 'int' is 1x 256-bit and 1x 64-bit
       * - '_Abi< 8, 2, _CxIleav>' for 'complex<float>' is 2x 256-bit
       * - '_Abi< 9, 2, _CxIleav>' for 'complex<float>' is 1x 512-bit and 1x 64-bit
       * - '_Abi< 8, 1, _CxCtgus>' for 'complex<float>' is 2x 256-bit
       */
      static constexpr int _S_nreg = _Nreg;

      static constexpr _AbiVariant _S_variant = _Var;

      template <typename _Tp>
        using _DataType = decltype([] {
                            static_assert(_S_nreg == 1);
                            static_assert(not __flags_test(_Var, _AbiVariant::_CxIleav));
                            static_assert(not __flags_test(_Var, _AbiVariant::_CxCtgus));
                            constexpr int __n = __bit_ceil(unsigned(_S_size));
                            using _Vp [[__gnu__::__vector_size__(sizeof(_Tp) * __n)]]
                              = __canonical_vec_type_t<_Tp>;
                            return _Vp();
                          }());

      // Use two values (bits, vector elements) per logical element of the mask.
      // This is not used for _DataType<_Tp> because its _Tp can never be complex (cf. basic_vec
      // partial specialization)
      static constexpr bool _S_use_2_for_1 = __flags_test(_Var, _AbiVariant::_CxIleav);

      template <size_t _Bytes>
        using _MaskDataType
          = decltype([] {
              if constexpr (__flags_test(_Var, _AbiVariant::_BitMask))
                {
                  if constexpr (_Nreg > 1)
                    return _InvalidInteger();
                  else
                    return _UInt<__bit_ceil(__div_ceil<unsigned>(
                                              _S_use_2_for_1 ? _S_size * 2 : _S_size,
                                              __CHAR_BIT__))>();
                }
              else
                {
                  constexpr unsigned __vbytes = _Bytes * __bit_ceil(unsigned(_S_size));
                  using _Vp [[__gnu__::__vector_size__(__vbytes)]]
                    = __integer_from<_S_use_2_for_1 ? _Bytes / 2 : _Bytes>;
                  return _Vp();
                }
            }());

      template <int _N2, int _Nreg2 = __div_ceil(_N2, _S_size)>
        consteval auto
        _M_resize() const
        {
          if constexpr (_N2 == 1 and not __flags_test(_Var, _AbiVariant::_CxIleav))
            return _ScalarAbi<1>();
          else
            return _Abi<_N2, _Nreg2, _Var>();
        }
    };

  struct _InvalidAbi
  { static constexpr int _S_size = 0; };

  template <typename _Tp>
    concept __abi_tag
      = same_as<decltype(_Tp::_S_variant), const _AbiVariant>
          and requires(_Tp __x) {
            { __x.template _M_resize<_Tp::_S_size, _Tp::_S_nreg>() } -> same_as<_Tp>;
          };

  // Determine if math functions must *raise* floating-point exceptions.
  // math_errhandling may expand to an extern symbol, in which case we must assume fp exceptions
  // need to be considered.
  template <int = 0>
    requires requires { typename bool_constant<0 != (math_errhandling & MATH_ERREXCEPT)>; }
    consteval bool
    __handle_fpexcept_impl(int)
    { return 0 != (math_errhandling & MATH_ERREXCEPT); }

  // Fallback if math_errhandling doesn't work: implement correct exception behavior.
  consteval bool
  __handle_fpexcept_impl(float)
  { return true; }

  struct _OptTraits
  {
    consteval bool
    _M_test(int __bit) const
    { return ((_M_build_flags >> __bit) & 1) == 1; }

    // true iff floating-point operations can signal an exception (allow non-default handler)
    consteval bool
    _M_fp_may_signal() const
    { return _M_test(0); }

    // true iff floating-point operations can raise an exception flag
    consteval bool
    _M_fp_may_raise() const
    { return _M_test(12); }

    consteval bool
    _M_fast_math() const
    { return _M_test(1); }

    consteval bool
    _M_finite_math_only() const
    { return _M_test(2); }

    consteval bool
    _M_no_signed_zeros() const
    { return _M_test(3); }

    consteval bool
    _M_signed_zeros() const
    { return not _M_test(3); }

    consteval bool
    _M_reciprocal_math() const
    { return _M_test(4); }

    consteval bool
    _M_no_math_errno() const
    { return _M_test(5); }

    consteval bool
    _M_math_errno() const
    { return not _M_test(5); }

    consteval bool
    _M_associative_math() const
    { return _M_test(6); }

    consteval bool
    _M_conforming_to_STDC_annex_G() const
    { return _M_test(10) and not _M_finite_math_only(); }

    consteval bool
    _M_support_snan() const
    { return _M_test(11); }

    __UINT64_TYPE__ _M_build_flags
      = 0
#if not __NO_TRAPPING_MATH__
          + (1 << 0)
#endif
          + (__handle_fpexcept_impl(0) << 12)
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
        // bits 7, 8, and 9 reserved for __FLT_EVAL_METHOD__
#if __FLT_EVAL_METHOD__ == 1
          + (1 << 7)
#elif __FLT_EVAL_METHOD__ == 2
          + (2 << 7)
#elif __FLT_EVAL_METHOD__ != 0
          + (3 << 7)
#endif

        // C Annex G defines the behavior of complex<T> where T is IEC60559 floating-point. If
        // __STDC_IEC_60559_COMPLEX__ is defined then Annex G is implemented - and simd<complex>
        // will do so as well. However, Clang never defines the macro.
#if defined __STDC_IEC_60559_COMPLEX__ or defined __STDC_IEC_559_COMPLEX__ or defined _GLIBCXX_CLANG
          + (1 << 10)
#endif
#if __SUPPORT_SNAN__
          + (1 << 11)
#endif
        ;
  };

#if _GLIBCXX_SIMD_HAVE_NEON or _GLIBCXX_SIMD_HAVE_SVE

  struct _ArchTraits
  {
    __UINT64_TYPE__ _M_flags = (_GLIBCXX_SIMD_HAVE_NEON << 0)
                          | (_GLIBCXX_SIMD_HAVE_NEON_A32 << 1)
                          | (_GLIBCXX_SIMD_HAVE_NEON_A64 << 2)
                          | (_GLIBCXX_SIMD_HAVE_SVE << 3);

    consteval bool
    _M_test(int __bit) const
    { return ((_M_flags >> __bit) & 1) == 1; }

    consteval bool
    _M_have_neon() const
    { return _M_test(0); }

    consteval bool
    _M_have_neon_a32() const
    { return _M_test(1); }

    consteval bool
    _M_have_neon_a64() const
    { return _M_test(2); }

    consteval bool
    _M_have_sve() const
    { return _M_test(3); }

    template <typename _Tp>
      consteval bool
      _M_eval_as_f32() const
      { return is_same_v<_Tp, _Float16>; }
  };

#elif __powerpc__

  struct _ArchTraits
  {
    __UINT64_TYPE__ _M_flags = 0
#ifdef _ARCH_PWR10
                          + 5
#elif defined __POWER9_VECTOR__
                          + 4
#elif defined __POWER8_VECTOR__
                          + 3
#elif defined __VSX__
                          + 2
#elif defined __VMX__
                          + 1
#endif
                        ;

    consteval bool _M_vmx()
    { return (_M_flags & 0xf) >= 1; }

    consteval bool _M_vsx()
    { return (_M_flags & 0xf) >= 2; }

    consteval bool _M_power8()
    { return (_M_flags & 0xf) >= 3; }

    consteval bool _M_power9()
    { return (_M_flags & 0xf) >= 4; }

    consteval bool _M_power10()
    { return (_M_flags & 0xf) >= 5; }

    template <typename _Tp>
      consteval bool
      _M_eval_as_f32() const
      { return is_same_v<_Tp, _Float16>; }
  };

#elif _GLIBCXX_SIMD_HAVE_SSE

  struct _ArchTraits
  {
    __UINT64_TYPE__ _M_flags = (_GLIBCXX_SIMD_HAVE_MMX << 0)
                          | (_GLIBCXX_SIMD_HAVE_SSE << 1)
                          | (_GLIBCXX_SIMD_HAVE_SSE2 << 2)
                          | (_GLIBCXX_SIMD_HAVE_SSE3 << 3)
                          | (_GLIBCXX_SIMD_HAVE_SSSE3 << 4)
                          | (_GLIBCXX_SIMD_HAVE_SSE4_1 << 5)
                          | (_GLIBCXX_SIMD_HAVE_SSE4_2 << 6)
                          | (_GLIBCXX_SIMD_HAVE_XOP << 7)
                          | (_GLIBCXX_SIMD_HAVE_AVX << 8)
                          | (_GLIBCXX_SIMD_HAVE_AVX2 << 9)
                          | (_GLIBCXX_SIMD_HAVE_BMI << 10)
                          | (_GLIBCXX_SIMD_HAVE_BMI2 << 11)
                          | (_GLIBCXX_SIMD_HAVE_LZCNT << 12)
                          | (_GLIBCXX_SIMD_HAVE_SSE4A << 13)
                          | (_GLIBCXX_SIMD_HAVE_FMA << 14)
                          | (_GLIBCXX_SIMD_HAVE_FMA4 << 15)
                          | (_GLIBCXX_SIMD_HAVE_F16C << 16)
                          | (_GLIBCXX_SIMD_HAVE_POPCNT << 17)
                          | (_GLIBCXX_SIMD_HAVE_AVX512F << 18)
                          | (_GLIBCXX_SIMD_HAVE_AVX512DQ << 19)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VL << 20)
                          | (_GLIBCXX_SIMD_HAVE_AVX512BW << 21)
                          | (_GLIBCXX_SIMD_HAVE_AVX512BITALG << 22)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VBMI << 23)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VBMI2 << 24)
                          | (_GLIBCXX_SIMD_HAVE_AVX512IFMA << 25)
                          | (_GLIBCXX_SIMD_HAVE_AVX512CD << 26)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VNNI << 27)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VPOPCNTDQ << 28)
                          | (_GLIBCXX_SIMD_HAVE_AVX512VP2INTERSECT << 29)
                          | (_GLIBCXX_SIMD_HAVE_AVX512FP16 << 30)
                          | (_GLIBCXX_SIMD_HAVE_AVXIFMA << 31)
                          | (_GLIBCXX_SIMD_HAVE_AVXNECONVERT << 32)
                          | (_GLIBCXX_SIMD_HAVE_AVXVNNI << 33)
                          | (_GLIBCXX_SIMD_HAVE_AVXVNNIINT8 << 34)
                          | (_GLIBCXX_SIMD_HAVE_AVXVNNIINT16 << 35);

    consteval bool
    _M_test(int __bit) const
    { return ((_M_flags >> __bit) & 1) == 1; }

    consteval bool
    _M_have_mmx() const
    { return _M_test(0); }

    consteval bool
    _M_have_sse() const
    { return _M_test(1); }

    consteval bool
    _M_have_sse2() const
    { return _M_test(2); }

    consteval bool
    _M_have_sse3() const
    { return _M_test(3); }

    consteval bool
    _M_have_ssse3() const
    { return _M_test(4); }

    consteval bool
    _M_have_sse4_1() const
    { return _M_test(5); }

    consteval bool
    _M_have_sse4_2() const
    { return _M_test(6); }

    consteval bool
    _M_have_xop() const
    { return _M_test(7); }

    consteval bool
    _M_have_avx() const
    { return _M_test(8); }

    consteval bool
    _M_have_avx2() const
    { return _M_test(9); }

    consteval bool
    _M_have_bmi() const
    { return _M_test(10); }

    consteval bool
    _M_have_bmi2() const
    { return _M_test(11); }

    consteval bool
    _M_have_lzcnt() const
    { return _M_test(12); }

    consteval bool
    _M_have_sse4a() const
    { return _M_test(13); }

    consteval bool
    _M_have_fma() const
    { return _M_test(14); }

    consteval bool
    _M_have_fma4() const
    { return _M_test(15); }

    consteval bool
    _M_have_f16c() const
    { return _M_test(16); }

    consteval bool
    _M_have_popcnt() const
    { return _M_test(17); }

    consteval bool
    _M_have_avx512f() const
    { return _M_test(18); }

    consteval bool
    _M_have_avx512dq() const
    { return _M_test(19); }

    consteval bool
    _M_have_avx512vl() const
    { return _M_test(20); }

    consteval bool
    _M_have_avx512bw() const
    { return _M_test(21); }

    consteval bool
    _M_have_avx512bitalg() const
    { return _M_test(22); }

    consteval bool
    _M_have_avx512vbmi() const
    { return _M_test(23); }

    consteval bool
    _M_have_avx512vbmi2() const
    { return _M_test(24); }

    consteval bool
    _M_have_avx512ifma() const
    { return _M_test(25); }

    consteval bool
    _M_have_avx512cd() const
    { return _M_test(26); }

    consteval bool
    _M_have_avx512vnni() const
    { return _M_test(27); }

    consteval bool
    _M_have_avx512vpopcntdq() const
    { return _M_test(28); }

    consteval bool
    _M_have_avx512vp2intersect() const
    { return _M_test(29); }

    consteval bool
    _M_have_avx512fp16() const
    { return _M_test(30); }

    template <typename _Tp>
      consteval bool
      _M_eval_as_f32() const
      { return is_same_v<_Tp, _Float16> and not _M_have_avx512fp16(); }
  };

  template <typename _Tp, _ArchTraits _Traits = {}>
    consteval auto
    __native_abi()
    {
      constexpr int __adj_sizeof = sizeof(_Tp) * (1 + is_same_v<_Tp, _Float16>);
      if constexpr (not __vectorizable<_Tp>)
        return _InvalidAbi();
      else if constexpr (__complex_like<_Tp>)
        {
          constexpr auto __underlying = __native_abi<typename _Tp::value_type>();
          if constexpr (__underlying._S_size == 1)
            return _ScalarAbi<1>();
          else
            return _Abi<__underlying._S_size / 2, 1,
                        __flags_or(__underlying._S_variant, _AbiVariant::_CxIleav)>();
        }
      else if constexpr (_Traits._M_have_avx512fp16())
        return _Abi<64 / sizeof(_Tp), 1, _AbiVariant::_BitMask>();
      else if constexpr (_Traits._M_have_avx512f())
        return _Abi<64 / __adj_sizeof, 1, _AbiVariant::_BitMask>();
      else if constexpr (is_same_v<_Tp, _Float16> and not _Traits._M_have_f16c())
        return _ScalarAbi<1>();
      else if constexpr (_Traits._M_have_avx2())
        return _Abi<32 / __adj_sizeof, 1, _AbiVariant::_VecMask>();
      else if constexpr (_Traits._M_have_avx() and is_floating_point_v<_Tp>)
        return _Abi<32 / __adj_sizeof, 1, _AbiVariant::_VecMask>();
      else if constexpr (_Traits._M_have_sse2())
        return _Abi<16 / __adj_sizeof, 1, _AbiVariant::_VecMask>();
      else if constexpr (_Traits._M_have_sse() and is_floating_point_v<_Tp>
                           and sizeof(_Tp) == sizeof(float))
        return _Abi<16 / __adj_sizeof, 1, _AbiVariant::_VecMask>();
      else
        return _ScalarAbi<1>();
    }

#else

  // scalar fallback
  // TODO: add more targets
  struct _ArchTraits
  {
    __UINT64_TYPE__ _M_flags = 0;

    constexpr bool
    _M_test(int __bit) const
    { return ((_M_flags >> __bit) & 1) == 1; }
  };

  template <typename _Tp>
    consteval auto
    __native_abi()
    {
      if constexpr (not __vectorizable<_Tp>)
        return _InvalidAbi();
      else
        return _ScalarAbi<1>();
    }

#endif

  /**@internal
   * You must use this type as template argument to function templates that are not declared
   * always_inline (to avoid issues when linking code compiled with different compiler flags).
   */
  struct _TargetTraits
  : _ArchTraits, _OptTraits
  {};

  // [simd.expos.abi]
  template <typename _Tp>
    using __native_abi_t = decltype(__native_abi<_Tp>());

  template <typename _Tp, int _Np, _TargetTraits _Target = {}>
    consteval auto
    __deduce_abi()
    {
      constexpr auto __native = __native_abi<_Tp>();
      if constexpr (0 == __native._S_size)
        return _InvalidAbi();
      else if constexpr (_Np == __native._S_size)
        return __native;
      else
        return __native.template _M_resize<_Np>();
    }

  template <typename _Tp, int _Np>
    using __deduce_abi_t = decltype(__deduce_abi<_Tp, _Np>());

  // rebind for basic_vec, and basic_mask where we know the destination value-type
  template <typename _Tp, int _Np, __abi_tag _A0, _ArchTraits = {}>
    consteval auto
    __abi_rebind()
    {
      if constexpr (_Np <= 0 or not __vectorizable<_Tp>)
        return _InvalidAbi();
      else
        {
          constexpr auto __native = __native_abi<_Tp>();
          static_assert(0 != __native._S_size);
          constexpr int __nreg = __div_ceil(_Np, __native._S_size);

          if constexpr (is_same_v<_A0, _ScalarAbi<_A0::_S_size>>)
            return __deduce_abi<_Tp, _Np>();

          else if constexpr (__complex_like<_Tp>
                               and __flags_test(_A0::_S_variant, _AbiVariant::_CxCtgus)
                               and __flags_test(__native._S_variant, _AbiVariant::_CxIleav))
            // we need half the number of registers since the number applies twice, to reals and
            // imaginaries.
            return _Abi<_Np, __nreg / 2, _A0::_S_variant>();

          else if constexpr (__complex_like<_Tp>
                               and __flags_test(_A0::_S_variant, _AbiVariant::_CxIleav)
                               and __flags_test(__native._S_variant, _AbiVariant::_CxCtgus))
            return _Abi<_Np, __nreg * 2, _A0::_S_variant>();

          else if constexpr (__complex_like<_Tp>)
            return _Abi<_Np, __nreg, __flags_or(_A0::_S_variant, _AbiVariant::_CxIleav)>();

          else if constexpr (_Np == 1)
            return _ScalarAbi<1>();

          else
            return _Abi<_Np, __nreg, __flags_and(_A0::_S_variant, _AbiVariant::_MaskVariants)>();
        }
    }

  // rebind for basic_mask. The important difference here is that we have no information about the
  // actual value-type other than its sizeof. So _Bytes == 8 could mean complex<float>, double, or
  // int64_t. E.g. _Np == 4 with AVX w/o AVX2 that's 'vector(4) int', 'vector(4) long long', or
  // '2x vector(2) long long'.
  // That's why this overload has the additional _IsOnlyResize parameter, which tells us that the
  // value-type doesn't change.
  template <size_t _Bytes, int _Np, __abi_tag _A0, bool _IsOnlyResize, _ArchTraits _Traits = {}>
    consteval auto
    __abi_rebind()
    {
      constexpr bool __from_cx = __flags_test(_A0::_S_variant, _AbiVariant::_CxCtgus)
                                   or __flags_test(_A0::_S_variant, _AbiVariant::_CxIleav);

      if constexpr (_Bytes == 0 or _Np <= 0)
        return _InvalidAbi();

      // If _Bytes is sizeof(complex<double>) we can be certain it's a mask<complex<double>, _Np>.
      else if constexpr (_Bytes == sizeof(double) * 2)
        return __abi_rebind<complex<double>, _Np, _A0>();

      else if constexpr (is_same_v<_A0, _ScalarAbi<_A0::_S_size>>)
        {
          if constexpr (_IsOnlyResize)
            // stick to _ScalarAbi (likely _Float16 without hardware support)
            return _ScalarAbi<_Np>();
          else
            // otherwise, fresh start via __deduce_abi_t using __integer_from
            return __deduce_abi<__integer_from<_Bytes>, _Np>();
        }

      // If the source ABI is complex, _Bytes == sizeof(complex<float>) or
      // sizeof(complex<float16_t>), and _IsOnlyResize is true, then it's a mask<complex<float>,
      // _Np>
      else if constexpr (__from_cx and _IsOnlyResize and _Bytes == 2 * sizeof(float))
        return __abi_rebind<complex<float>, _Np, _A0>();
      else if constexpr (__from_cx and _IsOnlyResize and _Bytes == 2 * sizeof(_Float16))
        return __abi_rebind<complex<_Float16>, _Np, _A0>();

#if _GLIBCXX_SIMD_HAVE_SSE
      // AVX w/o AVX2:
      // e.g. resize_t<8, mask<float, Whatever>> needs to be _Abi<8, 1> not _Abi<8, 2>
      // We determine whether _A0 identifies an AVX vector by looking at the size of a native
      // register. If it's 32, it's a YMM register, otherwise it's 16 or less.
      else if constexpr (_IsOnlyResize
                           and _Traits._M_have_avx() and not _Traits._M_have_avx2()
                           and __bit_ceil(__div_ceil<unsigned>(
                                            _A0::_S_size, _A0::_S_nreg)) * _Bytes == 32)
        {
          if constexpr (_Bytes == sizeof(double))
            return __abi_rebind<double, _Np, _A0>();
          else if constexpr (_Bytes == sizeof(float))
            return __abi_rebind<float, _Np, _A0>();
          else if constexpr (_Traits._M_have_f16c() and _Bytes == sizeof(_Float16))
            return __abi_rebind<_Float16, _Np, _A0>();
          else // impossible
            static_assert(false);
        }
#endif

      else
        return __abi_rebind<__integer_from<_Bytes>, _Np, _A0>();
    }

  template <typename _To, typename _From>
  consteval bool
    __is_mask_conversion_explicit(size_t __b0, size_t __b1)
    {
      constexpr int __n = _To::_S_size;
      static_assert(__n == _From::_S_size);
      if (__b0 != __b1)
        return true;

      // everything is better than _ScalarAbi, except when converting to a single bool
      if constexpr (is_same_v<_To, _ScalarAbi<__n>>)
        return __n > 1;
      else if constexpr (is_same_v<_From, _ScalarAbi<__n>>)
        return true;

      else
        {
          constexpr _AbiVariant __f0 = _To::_S_variant;
          constexpr _AbiVariant __f1 = _From::_S_variant;

          // converting to a bit-mask is better
          if constexpr (__flags_and(__f0, _AbiVariant::_MaskVariants)
                          != __flags_and(__f1, _AbiVariant::_MaskVariants))
            return __flags_test(__f0, _AbiVariant::_VecMask); // to _VecMask is explicit

          // with vec-masks, fewer registers is better
          else if constexpr (_From::_S_nreg != _To::_S_nreg)
            return _From::_S_nreg < _To::_S_nreg;

          // differ only on _Cx flags
          // interleaved complex is worse
          else if constexpr (__flags_test(__f0, _AbiVariant::_CxIleav))
            return true;
          else if constexpr (__flags_test(__f1, _AbiVariant::_CxIleav))
            return false;

          // prefer non-_Cx over _CxCtgus
          else if constexpr (__flags_test(__f0, _AbiVariant::_CxCtgus))
            return true;
          else
            __builtin_unreachable();
        }
    }

  // [simd.expos]
  using __simd_size_type = int;

  template <typename _Tp, typename _Abi>
    constexpr __simd_size_type __simd_size_v = 0;

  template <__vectorizable _Tp, __abi_tag _Abi>
    constexpr __simd_size_type __simd_size_v<_Tp, _Abi> = _Abi::_S_size;

  // integral_constant shortcut
  template <__simd_size_type _Xp>
    inline constexpr integral_constant<__simd_size_type, _Xp> __simd_size_constant = {};

  // [simd.syn]
  template <typename _Tp, typename _Abi = __native_abi_t<_Tp>>
    class basic_vec;

  template <typename _Tp, __simd_size_type _Np = __simd_size_v<_Tp, __native_abi_t<_Tp>>>
    using vec = basic_vec<_Tp, __deduce_abi_t<_Tp, _Np>>;

  template <size_t _Bytes, typename _Abi = __native_abi_t<__integer_from<_Bytes>>>
    class basic_mask;

  template <typename _Tp, __simd_size_type _Np = __simd_size_v<_Tp, __native_abi_t<_Tp>>>
    using mask = basic_mask<sizeof(_Tp), __deduce_abi_t<_Tp, _Np>>;

  // [simd.general] data-parallel types
  template <typename _Tp>
    concept __data_parallel_type
      = __vectorizable<typename _Tp::value_type>
          and __abi_tag<typename _Tp::abi_type>
          and _Tp::size() >= 1;

  // [simd.ctor] load constructor constraints
#ifdef __clang__
  template <typename _Tp>
    static constexpr remove_cvref_t<_Tp> __static_sized_range_obj = {};
#endif

  template <typename _Tp, size_t _Np = 0>
    concept __static_sized_range
      = ranges::contiguous_range<_Tp> and ranges::sized_range<_Tp>
          and requires(_Tp&& __r) {
#if 1 // PR117849
            typename integral_constant<size_t, ranges::size(__r)>;
#else
            requires (decltype(std::span(__r))::extent != dynamic_extent);
#endif
#ifdef __clang__
            requires (_Np == 0 or ranges::size(__static_sized_range_obj<_Tp>) == _Np);
#else
            requires (_Np == 0 or ranges::size(__r) == _Np);
#endif
          };

  // [simd.general] value-reserving
  template <typename _From, typename _To>
    concept __arithmetic_only_value_preserving_convertible_to
      = convertible_to<_From, _To> and is_arithmetic_v<_From> and is_arithmetic_v<_To>
          and not (is_signed_v<_From> and is_unsigned_v<_To>)
          and numeric_limits<_From>::digits <= numeric_limits<_To>::digits
          and numeric_limits<_From>::max() <= numeric_limits<_To>::max()
          and numeric_limits<_From>::lowest() >= numeric_limits<_To>::lowest();

  template <typename _From, typename _To>
    concept __value_preserving_convertible_to
      = __arithmetic_only_value_preserving_convertible_to<_From, _To>
          or (__complex_like<_To> and __arithmetic_only_value_preserving_convertible_to<
                                        _From, typename _To::value_type>);

  // [simd.expos]
  template <typename _Tp>
    constexpr size_t __mask_element_size = 0;

  template<typename _Tp>
    concept __constexpr_wrapper_like
      = convertible_to<_Tp, decltype(_Tp::value)>
          and equality_comparable_with<_Tp, decltype(_Tp::value)>
          and bool_constant<_Tp() == _Tp::value>::value
          and bool_constant<static_cast<decltype(_Tp::value)>(_Tp()) == _Tp::value>::value;

  // [simd.ctor] explicit(...) of broadcast ctor
  template <typename _From, typename _To>
    concept __non_narrowing_constexpr_conversion
      = __constexpr_wrapper_like<_From> and convertible_to<_From, _To>
          and requires { { _From::value } -> std::convertible_to<_To>; }
          and static_cast<decltype(_From::value)>(_To(_From::value)) == _From::value
          and not (std::unsigned_integral<_To> and _From::value < decltype(_From::value)())
          and _From::value <= std::numeric_limits<_To>::max()
          and _From::value >= std::numeric_limits<_To>::lowest();

  // [simd.ctor] p4
  template <typename _From, typename _To>
    concept __broadcast_constructible
      = convertible_to<_From, _To> // 4
          and ((not is_arithmetic_v<remove_cvref_t<_From>>
                  and not __constexpr_wrapper_like<remove_cvref_t<_From>>) // 4.1
                 or __value_preserving_convertible_to<remove_cvref_t<_From>, _To> // 4.2
                 or __non_narrowing_constexpr_conversion<remove_cvref_t<_From>, _To>); // 4.3

  // __higher_floating_point_rank_than<_Tp, U> (_Tp has higher or equal floating point rank than U)
  template <typename _From, typename _To>
    concept __higher_floating_point_rank_than
      = floating_point<_From> && floating_point<_To>
          && same_as<common_type_t<_From, _To>, _From>;

  // __higher_integer_rank_than<_Tp, U> (_Tp has higher or equal integer rank than U)
  template <typename _From, typename _To>
    concept __higher_integer_rank_than
      = integral<_From> && integral<_To>
          && (sizeof(_From) > sizeof(_To) || same_as<common_type_t<_From, _To>, _From>);

  template <typename _From, typename _To>
    concept __higher_rank_than
      = __higher_floating_point_rank_than<_From, _To> || __higher_integer_rank_than<_From, _To>;

  struct __convert_flag;

  template <typename _From, typename _To, typename... _Traits>
    concept __loadstore_convertible_to
      = same_as<_From, _To>
          or (__vectorizable<_From> and __vectorizable<_To>
                and (__value_preserving_convertible_to<_From, _To>
#if SIMD_STD_BYTE
                       or (requires(_From __x) { static_cast<_To>(__x); }
#else
                       or (std::convertible_to<_From, _To>
#endif
                             and (std::same_as<_Traits, __convert_flag> or ...))));

  template <typename _From, typename _To>
    concept __simd_generator_convertible_to
      = std::convertible_to<_From, _To>
          and (not is_arithmetic_v<_From> or __value_preserving_convertible_to<_From, _To>);

  template <typename _Fp, typename _Tp, __simd_size_type... _Is>
    requires (__simd_generator_convertible_to<
                decltype(declval<_Fp>()(__simd_size_constant<_Is>)), _Tp> and ...)
    constexpr void
    __simd_generator_invokable_impl(integer_sequence<__simd_size_type, _Is...>);

  template <typename _Fp, typename _Tp, __simd_size_type _Np>
    concept __simd_generator_invokable = requires {
      __simd_generator_invokable_impl<_Fp, _Tp>(make_integer_sequence<__simd_size_type, _Np>());
    };

  template <typename _Fp, typename _Tp, __simd_size_type... _Is>
    requires (not __simd_generator_convertible_to<
                    decltype(declval<_Fp>()(__simd_size_constant<_Is>)), _Tp>
                or ...)
    constexpr void
    __almost_simd_generator_invokable_impl(integer_sequence<__simd_size_type, _Is...>);

  template <typename _Fp, typename _Tp, __simd_size_type _Np>
    concept __almost_simd_generator_invokable = requires(_Fp&& __gen) {
      __gen(__simd_size_constant<0>);
      __almost_simd_generator_invokable_impl<_Fp, _Tp>(
        make_integer_sequence<__simd_size_type, _Np>());
    };

  template <typename _Fp>
    concept __index_permutation_function_nosize = requires(_Fp const& __f)
      {
        { __f(0) } -> std::integral;
      };

  template <typename _Fp, typename _Simd>
    concept __index_permutation_function_size = requires(_Fp const& __f)
      {
        { __f(0, 0) } -> std::integral;
      };

  template <typename _Fp, typename _Simd>
    concept __index_permutation_function
      = __index_permutation_function_size<_Fp, _Simd> or __index_permutation_function_nosize<_Fp>;

  // [simd.expos]
  template <size_t _Bytes, __abi_tag _Abi>
    constexpr size_t __mask_element_size<basic_mask<_Bytes, _Abi>> = _Bytes;

  template <typename _Vp>
    concept __simd_vec_type
      = same_as<_Vp, basic_vec<typename _Vp::value_type, typename _Vp::abi_type>>
          and is_default_constructible_v<_Vp>;

  template <typename _Vp>
    concept __simd_mask_type
      = same_as<_Vp, basic_mask<__mask_element_size<_Vp>, typename _Vp::abi_type>>
        and is_default_constructible_v<_Vp>;

  template <typename _Vp>
    concept __simd_vec_or_mask_type = __simd_vec_type<_Vp> or __simd_mask_type<_Vp>;

  template <typename _Vp>
    concept __simd_floating_point
      = __simd_vec_type<_Vp> and floating_point<typename _Vp::value_type>;

  template <typename _Vp>
    concept __simd_integral
      = __simd_vec_type<_Vp> and integral<typename _Vp::value_type>;

  template <typename _Vp>
    using __simd_complex_value_type = typename _Vp::value_type::value_type;

  template <typename _Vp>
    concept __simd_complex
      = __simd_vec_type<_Vp> and __complex_like_impl<typename _Vp::value_type>;

  template <typename _Tp>
    using __deduced_vec_t
      = decltype([] {
          using _Up = decltype(declval<const _Tp&>() + declval<const _Tp&>());
          if constexpr (__data_parallel_type<_Up>)
            return _Up();
      }());

  static_assert(is_same_v<__deduced_vec_t<int>, void>);

  template <typename _Vp, typename _Tp>
    using __make_compatible_simd_t
      = decltype([] {
          using _Up = decltype(declval<const _Tp&>() + declval<const _Tp&>());
          if constexpr (__simd_vec_type<_Up>)
            return _Up();
          else
            return vec<_Up, _Vp::size()>();
      }());

  template <typename... _Ts>
    concept __math_floating_point = (__simd_floating_point<__deduced_vec_t<_Ts>> or ...);

  template <typename...>
    struct __math_common_simd_impl;

  template <typename... _Ts>
    requires __math_floating_point<_Ts...>
    using __math_common_simd_t = typename __math_common_simd_impl<_Ts...>::type;

  template <typename _T0>
    struct __math_common_simd_impl<_T0>
    { using type = __deduced_vec_t<_T0>; };

  template <typename _T0, typename _T1>
    struct __math_common_simd_impl<_T0, _T1>
    {
      using type = decltype([] {
                     if constexpr (__math_floating_point<_T0> and __math_floating_point<_T1>)
                       return common_type_t<__deduced_vec_t<_T0>, __deduced_vec_t<_T1>>();
                     else if constexpr (__math_floating_point<_T0>)
                       return common_type_t<__deduced_vec_t<_T0>, _T1>();
                     else if constexpr (__math_floating_point<_T1>)
                       return common_type_t<_T0, __deduced_vec_t<_T1>>();
                     // else void
                   }());
    };

  template <typename _T0, typename _T1, typename... _TRest>
    struct __math_common_simd_impl<_T0, _T1, _TRest...>
    { using type = common_type_t<__math_common_simd_t<_T0, _T1>, _TRest...>; };

  template <typename _T0, typename _T1, typename... _TRest>
    requires is_void_v<__math_common_simd_t<_T0, _T1>>
    struct __math_common_simd_impl<_T0, _T1, _TRest...>
    { using type = common_type_t<__math_common_simd_t<_TRest...>, _T0, _T1>; };

  template <typename _BinaryOperation, typename _Tp>
    concept __reduction_binary_operation
      = requires (const _BinaryOperation __binary_op, const vec<_Tp, 1> __v) {
        { __binary_op(__v, __v) } -> same_as<vec<_Tp, 1>>;
      };

  [[__gnu__::__always_inline__]]
  constexpr __simd_size_type
  __lowest_bit(std::integral auto __bits)
  {
    if constexpr (sizeof(__bits) <= sizeof(int))
      return __builtin_ctz(__bits);
    else if constexpr (sizeof(__bits) <= sizeof(long))
      return __builtin_ctzl(__bits);
    else if constexpr (sizeof(__bits) <= sizeof(long long))
      return __builtin_ctzll(__bits);
    else
      static_assert(false);
  }

  [[__gnu__::__always_inline__]]
  constexpr __simd_size_type
  __highest_bit(std::integral auto __bits)
  {
    if constexpr (sizeof(__bits) <= sizeof(int))
      return sizeof(int) * __CHAR_BIT__ - 1 - __builtin_clz(__bits);
    else if constexpr (sizeof(__bits) <= sizeof(long))
      return sizeof(long) * __CHAR_BIT__ - 1 - __builtin_clzl(__bits);
    else if constexpr (sizeof(__bits) <= sizeof(long long))
      return sizeof(long long) * __CHAR_BIT__ - 1 - __builtin_clzll(__bits);
    else
      static_assert(false);
  }
}

#pragma GCC diagnostic pop

#endif  // BITS_SIMD_DETAILS_H_
