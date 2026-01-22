/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef _GLIBCXX_SIMD_DETAILS_H
#define _GLIBCXX_SIMD_DETAILS_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include <bit>
#include <bits/c++config.h> // _GLIBCXX_FLOAT_IS_IEEE_BINARY32
#include <bits/stl_function.h> // plus, minus, multiplies, ...
#include <bits/utility.h> // integer_sequence, etc.
#include <cmath> // for math_errhandling :(
#include <concepts>
#include <cstdint>
#include <limits>
#include <span> // for dynamic_extent

#if __CHAR_BIT__ != 8
// There are simply too many constants and bit operators that currently depend on CHAR_BIT == 8.
// Generalization to CHAR_BIT != 8 does not make sense without testability (i.e. a test target).
#error "<simd> is not supported for CHAR_BIT != 8"
#endif

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

#if VIR_EXTENSIONS
// TODO: remove the following for libstdc++
// Work around _GLIBCXX_CLANG not being defined with older libstdc++ when compiling with Clang
#if __GLIBCXX__ < 20250922 && defined __clang__ && __GNUC_MINOR__ == 2 && !defined _GLIBCXX_CLANG
#define _GLIBCXX_CLANG __clang__
#endif

#endif
#if defined __x86_64__ || defined __i386__
#define _GLIBCXX_X86 1
#else
#define _GLIBCXX_X86 0
#endif

#if VIR_EXTENSIONS
#if !_GLIBCXX_X86
#error "Not implemented yet. Only supported on x86 for now."
#endif

#endif
#ifndef _GLIBCXX_SIMD_NOEXCEPT
/** @internal
 * For unit-testing preconditions, use this macro to remove noexcept.
 */
#define _GLIBCXX_SIMD_NOEXCEPT noexcept
#endif

#define _GLIBCXX_SIMD_TOSTRING_IMPL(x) #x
#define _GLIBCXX_SIMD_TOSTRING(x) _GLIBCXX_SIMD_TOSTRING_IMPL(x)

#if !VIR_EXTENSIONS
// This is used for unit-testing precondition checking
#define __glibcxx_simd_precondition(expr, msg, ...)                                                \
  __glibcxx_assert(expr)
#else
#define _GLIBCXX_SIMD_LOC __FILE__ ":" _GLIBCXX_SIMD_TOSTRING(__LINE__) ": "

#ifndef _GLIBCXX_SIMD_TRAP_ON_UB
#define _GLIBCXX_SIMD_TRAP_ON_UB 1
#endif

#define __glibcxx_simd_precondition(expr, msg, ...)                                                \
  do {                                                                                             \
    const bool __precondition_result = !bool(expr);                                                \
    if (__builtin_constant_p(__precondition_result) && __precondition_result)                      \
      []() __attribute__((__noipa__, __warning__(                                                  \
        "precondition failure. \n" _GLIBCXX_SIMD_LOC "note: " msg " (precondition '" #expr         \
        "' does not hold)"))) {}();                                                                \
    if (__builtin_expect(__precondition_result, false))                                            \
      std::simd::__invoke_ub(                                                                      \
        _GLIBCXX_SIMD_LOC "precondition failure in '%s':\n" msg " ('" #expr "' does not hold)",    \
        __PRETTY_FUNCTION__ __VA_OPT__(,) __VA_ARGS__);                                            \
  } while(false)
#endif

#if VIR_NEXT_PATCH
namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  // Forward declarations.
  template<typename> class complex;

_GLIBCXX_END_NAMESPACE_VERSION
}

#endif
namespace std::simd
{
#if VIR_EXTENSIONS
  template <typename... _Args>
    [[noreturn, __gnu__::__cold__]]
#if !defined _GLIBCXX_ASSERTIONS
    [[__gnu__::__always_inline__]]
#endif
    inline void
    __invoke_ub([[maybe_unused]] const char* __msg, [[maybe_unused]] const _Args&... __args)
    {
#ifdef _GLIBCXX_ASSERTIONS
      __builtin_fprintf(stderr, __msg, __args...);
      __builtin_fprintf(stderr, "\n");
      __builtin_abort();
#elif _GLIBCXX_SIMD_TRAP_ON_UB
      __builtin_trap();
#else
      __builtin_unreachable();
#endif
    }

#endif
  template <typename _Tp>
    inline constexpr _Tp
    __iota = [] { static_assert(false, "invalid __iota specialization"); }();

  // [simd.general] vectorizable types
#if VIR_NEXT_PATCH
#if VIR_EXTENSIONS
  template <typename _Cp, auto __re, auto __im, typename _Tp = typename _Cp::value_type>
    constexpr _Cp __complex_object = _Cp {_Tp(__re), _Tp(__im)};

  template <typename _Tp>
    struct _Arr2
    { _Tp _M_data[2]; };

  template <typename _Tp, typename _Cx>
    struct __rebind_complex;

  template <typename _Tp, typename _Cx>
    using __rebind_complex_t = typename __rebind_complex<_Tp, _Cx>::type;

  template <typename _Tp, template <typename> class _Cx, typename _Up>
    struct __rebind_complex<_Tp, _Cx<_Up>>
    { using type = _Cx<_Tp>; };

#endif
  template <typename _Tp>
    concept __complex_like_impl
#if !VIR_EXTENSIONS
      = same_as<_Tp, complex<typename _Tp::value_type>>;
#else
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
          && is_same_v<__rebind_complex_t<typename _Tp::value_type, _Tp>, _Tp>
          && (__complex_object<_Tp, 1, 2> + _Tp {} == __complex_object<_Tp, 1, 2>)
          && (__complex_object<_Tp, -1, 5> - __complex_object<_Tp, -1, 5> == _Tp {})
          && (__complex_object<_Tp, 2, 3> * __complex_object<_Tp, 1, 1>
                 == __complex_object<_Tp, -1, 5>)
          && (__complex_object<_Tp, 5, 5> / __complex_object<_Tp, 1, 2>
                 == __complex_object<_Tp, 3, -1>)
          && (conj(__complex_object<_Tp, 5, 3>) == __complex_object<_Tp, 5, -3>)
          // not constexpr: && (abs(__complex_object<_Tp, 3, 4>) == typename _Tp::value_type(5))
          && (norm(__complex_object<_Tp, 5, 5>) == typename _Tp::value_type(50))
          && (2 * sizeof(typename _Tp::value_type) == sizeof(_Tp))
          && (__builtin_bit_cast(_Arr2<typename _Tp::value_type>, __complex_object<_Tp, 1, 2>)
                 ._M_data[0] == 1);
#endif

  /** @internal
   * Satisfied if @p _Tp implements the std::complex interface.
   */
  template <typename _Tp>
    concept __complex_like = __complex_like_impl<remove_cvref_t<_Tp>>;
#endif

  template <typename _Tp>
    concept __vectorizable_scalar
      = same_as<remove_cv_t<_Tp>, _Tp>
#ifdef __STDCPP_BFLOAT16_T__
          && !same_as<_Tp, __gnu_cxx::__bfloat16_t>
#endif
          && ((integral<_Tp> && sizeof(_Tp) <= sizeof(0ULL) && !same_as<_Tp, bool>)
                 || (floating_point<_Tp> && sizeof(_Tp) <= sizeof(double)));

  // [simd.general] p2
  template <typename _Tp>
#if VIR_NEXT_PATCH
    concept __vectorizable
      = __vectorizable_scalar<_Tp>
          || (__complex_like_impl<_Tp> && __vectorizable_scalar<typename _Tp::value_type>
                && floating_point<typename _Tp::value_type>);
#else
    concept __vectorizable = __vectorizable_scalar<_Tp>;
#endif

  /** @internal
   * Describes variants of _Abi.
   */
  enum class _AbiVariant : unsigned long long
  {
    _BitMask      = 0x01, // AVX512 bit-masks
    _MaskVariants = 0x0f, // vector masks if bits [0:3] are 0
#if VIR_NEXT_PATCH
    _CxIleav      = 0x10, // store complex components interleaved (ririri...)
                          // mask elements are stored for both    (001122...)
    _CxCtgus      = 0x20, // ... or store complex components contiguously (rrrr iiii)
                          // mask elements are store for one component    (0123)
    _CxVariants   = _CxIleav | _CxCtgus,
#endif
  };

  /** @internal
   * Return @p __in with only bits set that are set in any of @p __to_keep.
   */
  consteval _AbiVariant
  __filter_abi_variant(_AbiVariant __in, same_as<_AbiVariant> auto... __to_keep)
  {
    using _Up = underlying_type_t<_AbiVariant>;
    return static_cast<_AbiVariant>(static_cast<_Up>(__in) & (static_cast<_Up>(__to_keep) | ...));
  }

  /** @internal
   * Type used whenever no valid integer/value type exists.
   */
  struct _InvalidInteger
  {};

  /** @internal
   * Alias for a signed integer type T such that sizeof(T) equals _Bytes.
   *
   * C++26 [simd.expos.defn]
   */
  template <size_t _Bytes>
    using __integer_from
#if __clang__
      = conditional_t<sizeof(signed char) == _Bytes, signed char,
                      conditional_t<sizeof(signed short) == _Bytes, signed short,
                                    conditional_t<sizeof(signed int) == _Bytes, signed int,
                                                  conditional_t<sizeof(signed long long) == _Bytes,
                                                                signed long long,
                                                                _InvalidInteger>>>>;
#else
      = decltype([] consteval {
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
#endif

  /** @internal
   * Alias for an unsigned integer type T such that sizeof(T) equals _Bytes.
   */
  template <size_t _Bytes>
    using _UInt = make_unsigned_t<__integer_from<_Bytes>>;

  /** @internal
   * Divide @p __x by @p __y while rounding up instead of down.
   *
   * Preconditions: __x >= 0 && __y > 0.
   */
  template <typename _Tp>
    consteval _Tp
    __div_ceil(_Tp __x, _Tp __y)
    { return (__x + __y - 1) / __y; }

  /** @internal
   * Alias for an unsigned integer type that can store at least @p _NBits bits.
   */
  template <int _NBits>
    requires (_NBits > 0 && _NBits <= sizeof(0ll) * __CHAR_BIT__)
    using _Bitmask = _UInt<__div_ceil(__bit_ceil(unsigned(_NBits)), unsigned(__CHAR_BIT__))>;

  /** @internal
   * Map a given type @p _Tp to an equivalent type.
   *
   * This helps with reducing the necessary branches && casts in the implementation as well as
   * reducing the number of template instantiations.
   */
  template <typename _Tp>
    struct __canonical_vec_type
    { using type = _Tp; };

  template <typename _Tp>
    using __canonical_vec_type_t = typename __canonical_vec_type<_Tp>::type;

#if __SIZEOF_INT__ == __SIZEOF_LONG__
  template <>
    struct __canonical_vec_type<long>
    { using type = int; };

  template <>
    struct __canonical_vec_type<unsigned long>
    { using type = unsigned int; };
#elif __SIZEOF_LONG_LONG__ == __SIZEOF_LONG__
  template <>
    struct __canonical_vec_type<long>
    { using type = long long; };

  template <>
    struct __canonical_vec_type<unsigned long>
    { using type = unsigned long long; };
#endif

  template <typename _Tp>
    requires std::is_enum_v<_Tp>
    struct __canonical_vec_type<_Tp>
    { using type = __canonical_vec_type<std::underlying_type_t<_Tp>>::type; };

  template <>
    struct __canonical_vec_type<char>
#if __CHAR_UNSIGNED__
    { using type = unsigned char; };
#else
    { using type = signed char; };
#endif

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

#if defined(__FLT64_DIG__) && defined(_GLIBCXX_DOUBLE_IS_IEEE_BINARY64)
  template <>
    struct __canonical_vec_type<_Float64>
    { using type = double; };
#endif

#if defined(__FLT32_DIG__) && defined(_GLIBCXX_FLOAT_IS_IEEE_BINARY32)
  template <>
    struct __canonical_vec_type<_Float32>
    { using type = float; };
#endif

  /** @internal
   * This ABI tag describes basic_vec objects that store one element per data member and basic_mask
   * objects that store one bool data members.
   *
   * @tparam _Np   The number of elements, which also matches the number of data members in
   *               basic_vec and basic_mask.
   */
  template <int _Np = 1>
    struct _ScalarAbi
    {
      static constexpr int _S_size = _Np;

      static constexpr int _S_nreg = _Np;

      static constexpr _AbiVariant _S_variant = {};

      template <typename _Tp>
        using _DataType = __canonical_vec_type_t<_Tp>;

#if VIR_NEXT_PATCH
      static constexpr bool _S_is_cx_ileav = false;

      static constexpr bool _S_is_cx_ctgus = false;

#endif
      static constexpr bool _S_is_vecmask = false;

      // in principle a bool is a 1-bit bitmask, but this is asking for an AVX512 bitmask
      static constexpr bool _S_is_bitmask = false;

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

  /** @internal
   * This ABI tag describes basic_vec objects that store one or more objects declared with the
   * [[gnu::vector_size(N)]] attribute.
   * Applied to basic_mask objects, this ABI tag either describes corresponding vector-mask objects
   * or bit-mask objects. Which one is used is determined via @p _Var.
   *
   * @tparam _Np    The number of elements.
   * @tparam _Nreg  The number of registers needed to store @p _Np elements.
   * @tparam _Var   Determines how complex value-types are layed out and whether mask types use
   *                bit-masks or vector-masks.
   */
  template <int _Np, int _Nreg, underlying_type_t<_AbiVariant> _Var>
    struct _Abi
    {
      static constexpr int _S_size = _Np;

      /** @internal
       * The number of registers needed to represent one basic_vec for the element type that was
       * used on ABI deduction.
#if VIR_NEXT_PATCH
       *
       * For _CxCtgus the value applies twice, once per reals and once per imags.
#endif
       *
       * Examples:
       * - '_Abi< 8, 2>' for 'int' is 2x 128-bit
       * - '_Abi< 9, 3>' for 'int' is 2x 128-bit and 1x 32-bit
       * - '_Abi<10, 3>' for 'int' is 2x 128-bit and 1x 64-bit
       * - '_Abi<10, 1>' for 'int' is 1x 512-bit
       * - '_Abi<10, 2>' for 'int' is 1x 256-bit and 1x 64-bit
#if VIR_NEXT_PATCH
       * - '_Abi< 8, 2, _CxIleav>' for 'complex<float>' is 2x 256-bit
       * - '_Abi< 9, 2, _CxIleav>' for 'complex<float>' is 1x 512-bit and 1x 64-bit
       * - '_Abi< 8, 1, _CxCtgus>' for 'complex<float>' is 2x 256-bit
#endif
       */
      static constexpr int _S_nreg = _Nreg;

      static_assert(_S_size > 0);
      static_assert(_S_nreg > 0);

      static constexpr _AbiVariant _S_variant = static_cast<_AbiVariant>(_Var);

#if VIR_NEXT_PATCH
      static constexpr bool _S_is_cx_ileav
        = __filter_abi_variant(_S_variant, _AbiVariant::_CxIleav) == _AbiVariant::_CxIleav;

      static constexpr bool _S_is_cx_ctgus
        = __filter_abi_variant(_S_variant, _AbiVariant::_CxCtgus) == _AbiVariant::_CxCtgus;

      static_assert(!(_S_is_cx_ileav && _S_is_cx_ctgus)); // can't be both

      static_assert(_S_size > _S_nreg || (_S_is_cx_ileav && _S_size * 2 > _S_nreg)); // when equal use _ScalarAbi
#endif
      static constexpr bool _S_is_bitmask
        = __filter_abi_variant(_S_variant, _AbiVariant::_BitMask) == _AbiVariant::_BitMask;

      static constexpr bool _S_is_vecmask = !_S_is_bitmask;

      template <typename _Tp>
        using _DataType = decltype([] {
                            static_assert(_S_nreg == 1);
#if VIR_NEXT_PATCH
                            static_assert(!_S_is_cx_ileav);
                            static_assert(!_S_is_cx_ctgus);
#endif
                            constexpr int __n = __bit_ceil(unsigned(_S_size));
                            using _Vp [[__gnu__::__vector_size__(sizeof(_Tp) * __n)]]
                              = __canonical_vec_type_t<_Tp>;
                            return _Vp();
                          }());

      template <size_t _Bytes>
        using _MaskDataType
          = decltype([] {
#if VIR_NEXT_PATCH
              static_assert(!_S_is_cx_ileav);
#endif
              if constexpr (_S_is_vecmask)
                {
                  constexpr unsigned __vbytes = _Bytes * __bit_ceil(unsigned(_S_size));
                  using _Vp [[__gnu__::__vector_size__(__vbytes)]] = __integer_from<_Bytes>;
                  return _Vp();
                }
              else if constexpr (_Nreg > 1)
                return _InvalidInteger();
              else
                return _Bitmask<_S_size>();
            }());

      template <int _N2, int _Nreg2 = __div_ceil(_N2, _S_size)>
        consteval auto
        _M_resize() const
        {
#if VIR_NEXT_PATCH
          if constexpr (_N2 == 1 && !_S_is_cx_ileav)
#else
          if constexpr (_N2 == 1)
#endif
            return _ScalarAbi<1>();
          else
            return _Abi<_N2, _Nreg2, _Var>();
        }
    };

  /** @internal
   * Alias for an _Abi specialization where the _AbiVariant bits are combined into a single integer
   * value.
   *
   * Rationale: Consider diagnostic output and mangling of e.g. vec<int, 4> with AVX512. That's an
   * alias for std::simd::basic_vec<int, std::simd::_Abi<4, 1, 1ull>>. If _AbiVariant were the
   * template argument type of _Abi, the diagnostic output would be 'std::simd::basic_vec<int,
   * std::simd::_Abi<4, 1, (std::simd::_AbiVariant)std::simd::_AbiVariant::_BitMask>>'. That's a lot
   * longer, requires longer mangled names, and bakes the names of the enumerators into the ABI. As
   * soon as bits of multiple _AbiVariants are combined, this becomes hard to parse for humans
   * anyway.
   */
  template <int _Np, int _Nreg, _AbiVariant... _Vs>
    using _Abi_t = _Abi<_Np, _Nreg, (static_cast<underlying_type_t<_AbiVariant>>(_Vs) | ... | 0)>;

  /** @internal
   * This type is used whenever ABI tag deduction can't give a useful answer.
   */
  struct _InvalidAbi
  { static constexpr int _S_size = 0; };

  /** @internal
   * Satisfied if @p _Tp is a valid simd ABI tag. This is a necessary but not sufficient condition
   * for an enabled basic_vec/basic_mask specialization.
   */
  template <typename _Tp>
    concept __abi_tag
      = same_as<decltype(_Tp::_S_variant), const _AbiVariant>
          && (_Tp::_S_size >= _Tp::_S_nreg) && (_Tp::_S_nreg >= 1)
          && requires(_Tp __x) {
            { __x.template _M_resize<_Tp::_S_size, _Tp::_S_nreg>() } -> same_as<_Tp>;
          };

  template <typename _Tp>
    concept __scalar_abi_tag
      = same_as<_Tp, _ScalarAbi<_Tp::_S_size>> && __abi_tag<_Tp>;

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

  /** @internal
   * This type can be used as a template parameter for avoiding ODR violations, where code needs to
   * differ depending on optimization flags (mostly fp-math related).
   */
  struct _OptTraits
  {
#if VIR_NEXT_PATCH
    // keep only finite-math-only flag
    consteval _OptTraits
    _M_math_abi() const
    { return _OptTraits{_M_build_flags & 0b100}; }

#endif
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
    { return !_M_test(3); }

    consteval bool
    _M_reciprocal_math() const
    { return _M_test(4); }

    consteval bool
    _M_no_math_errno() const
    { return _M_test(5); }

    consteval bool
    _M_math_errno() const
    { return !_M_test(5); }

    consteval bool
    _M_associative_math() const
    { return _M_test(6); }

    consteval bool
    _M_conforming_to_STDC_annex_G() const
    { return _M_test(10) && !_M_finite_math_only(); }

    consteval bool
    _M_support_snan() const
    { return _M_test(11); }

    __UINT64_TYPE__ _M_build_flags
      = 0
#if !__NO_TRAPPING_MATH__
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
#if defined __STDC_IEC_60559_COMPLEX__ || defined __STDC_IEC_559_COMPLEX__ || defined _GLIBCXX_CLANG
          + (1 << 10)
#endif
#if __SUPPORT_SNAN__
          + (1 << 11)
#endif
        ;
  };

  /** @internal
   * Return true iff @p __s equals "1".
   */
  consteval bool
  __streq_to_1(const char* __s)
  { return __s != nullptr && __s[0] == '1' && __s[1] == '\0'; }

  /** @internal
   * If the macro given as @p feat is defined to 1, expands to a bit set at position @p off.
   * Otherwise, expand to zero.
   */
#define _GLIBCXX_SIMD_ARCH_FLAG(off, feat) \
  (static_cast<__UINT64_TYPE__>(std::simd::__streq_to_1(_GLIBCXX_SIMD_TOSTRING_IMPL(feat))) << off)

#if VIR_EXTENSIONS && __ARM_ARCH

#if __ARM_ARCH >= 8
#define _GLIBCXX_SIMD_HAVE_ARMv8 1
#endif
#if __ARM_FEATURE_SVE_BITS > 0 && __ARM_FEATURE_SVE_VECTOR_OPERATORS == 1
#define _GLIBCXX_SIMD_HAVE_SVE 1
#endif

#define _GLIBCXX_SIMD_ARCH_TRAITS_INIT {                      \
  _GLIBCXX_SIMD_ARCH_FLAG(0, __ARM_NEON)                      \
    | _GLIBCXX_SIMD_ARCH_FLAG( 1, __aarch64__)                \
    | _GLIBCXX_SIMD_ARCH_FLAG( 2, _GLIBCXX_SIMD_HAVE_ARMv8)   \
    | _GLIBCXX_SIMD_ARCH_FLAG( 3, _GLIBCXX_SIMD_HAVE_SVE)     \
}

  struct _ArchTraits
  {
    __UINT64_TYPE__ _M_flags = _GLIBCXX_SIMD_ARCH_TRAITS_INIT;

    consteval bool
    _M_test(int __bit) const
    { return ((_M_flags >> __bit) & 1) == 1; }

    consteval bool
    _M_have_neon() const
    { return _M_test(0); }

    consteval bool
    _M_have_neon_a32() const
    { return _M_have_neon() && (_M_have_neon_a64() || _M_test(2)); }

    consteval bool
    _M_have_neon_a64() const
    { return _M_have_neon() && _M_test(1); }

    consteval bool
    _M_have_sve() const
    { return _M_test(3); }

    template <typename _Tp>
      consteval bool
      _M_eval_as_f32() const
      { return is_same_v<_Tp, _Float16>; }
  };

#elif VIR_EXTENSIONS && __powerpc__

#define _GLIBCXX_SIMD_ARCH_TRAITS_INIT {                      \
  (_GLIBCXX_SIMD_ARCH_FLAG(0, __ALTIVEC__)                    \
     + _GLIBCXX_SIMD_ARCH_FLAG(0, __VSX__)                    \
     + _GLIBCXX_SIMD_ARCH_FLAG(0, __POWER8_VECTOR__)          \
     + _GLIBCXX_SIMD_ARCH_FLAG(0, __POWER9_VECTOR__))         \
}

  struct _ArchTraits
  {
    __UINT64_TYPE__ _M_flags = _GLIBCXX_SIMD_ARCH_TRAITS_INIT;

    consteval bool _M_altivec()
    { return (_M_flags & 0xf) >= 1; }

    consteval bool _M_vsx()
    { return (_M_flags & 0xf) >= 2; }

    consteval bool _M_power8()
    { return (_M_flags & 0xf) >= 3; }

    consteval bool _M_power9()
    { return (_M_flags & 0xf) >= 4; }

    template <typename _Tp>
      consteval bool
      _M_eval_as_f32() const
      { return is_same_v<_Tp, _Float16>; }
  };

#elif _GLIBCXX_X86

#define _GLIBCXX_SIMD_ARCH_TRAITS_INIT {                      \
  _GLIBCXX_SIMD_ARCH_FLAG(0, __MMX__)                         \
    | _GLIBCXX_SIMD_ARCH_FLAG( 1, __SSE__)                    \
    | _GLIBCXX_SIMD_ARCH_FLAG( 2, __SSE2__)                   \
    | _GLIBCXX_SIMD_ARCH_FLAG( 3, __SSE3__)                   \
    | _GLIBCXX_SIMD_ARCH_FLAG( 4, __SSSE3__)                  \
    | _GLIBCXX_SIMD_ARCH_FLAG( 5, __SSE4_1__)                 \
    | _GLIBCXX_SIMD_ARCH_FLAG( 6, __SSE4_2__)                 \
    | _GLIBCXX_SIMD_ARCH_FLAG( 7, __POPCNT__)                 \
    | _GLIBCXX_SIMD_ARCH_FLAG( 8, __AVX__)                    \
    | _GLIBCXX_SIMD_ARCH_FLAG( 9, __F16C__)                   \
    | _GLIBCXX_SIMD_ARCH_FLAG(10, __BMI__)                    \
    | _GLIBCXX_SIMD_ARCH_FLAG(11, __BMI2__)                   \
    | _GLIBCXX_SIMD_ARCH_FLAG(12, __LZCNT__)                  \
    | _GLIBCXX_SIMD_ARCH_FLAG(13, __AVX2__)                   \
    | _GLIBCXX_SIMD_ARCH_FLAG(14, __FMA__)                    \
    | _GLIBCXX_SIMD_ARCH_FLAG(15, __AVX512F__)                \
    | _GLIBCXX_SIMD_ARCH_FLAG(16, __AVX512CD__)               \
    | _GLIBCXX_SIMD_ARCH_FLAG(17, __AVX512DQ__)               \
    | _GLIBCXX_SIMD_ARCH_FLAG(18, __AVX512BW__)               \
    | _GLIBCXX_SIMD_ARCH_FLAG(19, __AVX512VL__)               \
    | _GLIBCXX_SIMD_ARCH_FLAG(20, __AVX512BITALG__)           \
    | _GLIBCXX_SIMD_ARCH_FLAG(21, __AVX512VBMI__)             \
    | _GLIBCXX_SIMD_ARCH_FLAG(22, __AVX512VBMI2__)            \
    | _GLIBCXX_SIMD_ARCH_FLAG(23, __AVX512IFMA__)             \
    | _GLIBCXX_SIMD_ARCH_FLAG(24, __AVX512VNNI__)             \
    | _GLIBCXX_SIMD_ARCH_FLAG(25, __AVX512VPOPCNTDQ__)        \
    | _GLIBCXX_SIMD_ARCH_FLAG(26, __AVX512FP16__)             \
    | _GLIBCXX_SIMD_ARCH_FLAG(27, __AVX512BF16__)             \
    | _GLIBCXX_SIMD_ARCH_FLAG(28, __AVXIFMA__)                \
    | _GLIBCXX_SIMD_ARCH_FLAG(29, __AVXNECONVERT__)           \
    | _GLIBCXX_SIMD_ARCH_FLAG(30, __AVXVNNI__)                \
    | _GLIBCXX_SIMD_ARCH_FLAG(31, __AVXVNNIINT8__)            \
    | _GLIBCXX_SIMD_ARCH_FLAG(32, __AVXVNNIINT16__)           \
    | _GLIBCXX_SIMD_ARCH_FLAG(33, __AVX10_1__)                \
    | _GLIBCXX_SIMD_ARCH_FLAG(34, __AVX10_2__)                \
    | _GLIBCXX_SIMD_ARCH_FLAG(35, __AVX512VP2INTERSECT__)     \
    | _GLIBCXX_SIMD_ARCH_FLAG(36, __SSE4A__)                  \
    | _GLIBCXX_SIMD_ARCH_FLAG(37, __FMA4__)                   \
    | _GLIBCXX_SIMD_ARCH_FLAG(38, __XOP__)                    \
  }
  // Should this include __APX_F__? I don't think it's relevant for use in constexpr-if branches =>
  // no ODR issue? The same could be said about several other flags above that are not checked
  // anywhere.

  struct _ArchTraits
  {
    __UINT64_TYPE__ _M_flags = _GLIBCXX_SIMD_ARCH_TRAITS_INIT;

#if VIR_NEXT_PATCH
    consteval _ArchTraits
    _M_math_abi() const
    {
      // keep only SSE4.1, AVX, FMA, AVX512F, (AVX512FP16)
      constexpr __UINT64_TYPE__ __v1_mask  = 0b0000000000'0000000110; // SSE2
      constexpr __UINT64_TYPE__ __v2_mask  = 0b0000000000'0000111110; // SSE4.1
      constexpr __UINT64_TYPE__ __v3a_mask = 0b0000000000'0111111110; // AVX
      constexpr __UINT64_TYPE__ __v3b_mask = 0b0000011111'1111111110; // Haswell–...lake / Zen1–3
      constexpr __UINT64_TYPE__ __v4_mask  = 0b1111111111'1111111110; // Xeon / Xen4–5
      // the FP16 flag is implied by passing float16_t vectors
      if ((_M_flags & __v4_mask) == __v4_mask)
        return _ArchTraits{__v4_mask};
      if ((_M_flags & __v3b_mask) == __v3b_mask)
        return _ArchTraits{__v3b_mask};
      if ((_M_flags & __v3a_mask) == __v3a_mask)
        return _ArchTraits{__v3a_mask};
      if ((_M_flags & __v2_mask) == __v2_mask)
        return _ArchTraits{__v2_mask};
      if ((_M_flags & __v1_mask) == __v1_mask)
        return _ArchTraits{__v1_mask};
    }

#endif
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
    _M_have_popcnt() const
    { return _M_test(7); }

    consteval bool
    _M_have_avx() const
    { return _M_test(8); }

    consteval bool
    _M_have_f16c() const
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
    _M_have_avx2() const
    { return _M_test(13); }

    consteval bool
    _M_have_fma() const
    { return _M_test(14); }

    consteval bool
    _M_have_avx512f() const
    { return _M_test(15); }

    consteval bool
    _M_have_avx512cd() const
    { return _M_test(16); }

    consteval bool
    _M_have_avx512dq() const
    { return _M_test(17); }

    consteval bool
    _M_have_avx512bw() const
    { return _M_test(18); }

    consteval bool
    _M_have_avx512vl() const
    { return _M_test(19); }

    consteval bool
    _M_have_avx512bitalg() const
    { return _M_test(20); }

    consteval bool
    _M_have_avx512vbmi() const
    { return _M_test(21); }

    consteval bool
    _M_have_avx512vbmi2() const
    { return _M_test(22); }

    consteval bool
    _M_have_avx512ifma() const
    { return _M_test(23); }

    consteval bool
    _M_have_avx512vnni() const
    { return _M_test(24); }

    consteval bool
    _M_have_avx512vpopcntdq() const
    { return _M_test(25); }

    consteval bool
    _M_have_avx512fp16() const
    { return _M_test(26); }

    consteval bool
    _M_have_avx512bf16() const
    { return _M_test(27); }

    consteval bool
    _M_have_avxifma() const
    { return _M_test(28); }

    consteval bool
    _M_have_avxneconvert() const
    { return _M_test(29); }

    consteval bool
    _M_have_avxvnni() const
    { return _M_test(30); }

    consteval bool
    _M_have_avxvnniint8() const
    { return _M_test(31); }

    consteval bool
    _M_have_avxvnniint16() const
    { return _M_test(32); }

    consteval bool
    _M_have_avx10_1() const
    { return _M_test(33); }

    consteval bool
    _M_have_avx10_2() const
    { return _M_test(34); }

    consteval bool
    _M_have_avx512vp2intersect() const
    { return _M_test(35); }

    consteval bool
    _M_have_sse4a() const
    { return _M_test(36); }

    consteval bool
    _M_have_fma4() const
    { return _M_test(37); }

    consteval bool
    _M_have_xop() const
    { return _M_test(38); }

    template <typename _Tp>
      consteval bool
      _M_eval_as_f32() const
      { return is_same_v<_Tp, _Float16> && !_M_have_avx512fp16(); }
  };

  template <typename _Tp, _ArchTraits _Traits = {}>
    consteval auto
    __native_abi()
    {
      constexpr int __adj_sizeof = sizeof(_Tp) * (1 + is_same_v<_Tp, _Float16>);
      if constexpr (!__vectorizable<_Tp>)
        return _InvalidAbi();
#if VIR_NEXT_PATCH
      else if constexpr (__complex_like<_Tp>)
        {
          constexpr auto __underlying = std::simd::__native_abi<typename _Tp::value_type>();
          if constexpr (__underlying._S_size == 1)
            return _ScalarAbi<1>();
          else
            return _Abi_t<__underlying._S_size / 2, 1,
                          __underlying._S_variant, _AbiVariant::_CxIleav>();
        }
#endif
      else if constexpr (_Traits._M_have_avx512fp16())
        return _Abi_t<64 / sizeof(_Tp), 1, _AbiVariant::_BitMask>();
      else if constexpr (_Traits._M_have_avx512f())
        return _Abi_t<64 / __adj_sizeof, 1, _AbiVariant::_BitMask>();
      else if constexpr (is_same_v<_Tp, _Float16> && !_Traits._M_have_f16c())
        return _ScalarAbi<1>();
      else if constexpr (_Traits._M_have_avx2())
        return _Abi_t<32 / __adj_sizeof, 1>();
      else if constexpr (_Traits._M_have_avx() && is_floating_point_v<_Tp>)
        return _Abi_t<32 / __adj_sizeof, 1>();
      else if constexpr (_Traits._M_have_sse2())
        return _Abi_t<16 / __adj_sizeof, 1>();
      else if constexpr (_Traits._M_have_sse() && is_floating_point_v<_Tp>
                           && sizeof(_Tp) == sizeof(float))
        return _Abi_t<16 / __adj_sizeof, 1>();
      // no MMX: we can't emit EMMS where it would be necessary
      else
        return _ScalarAbi<1>();
    }

#else

  // scalar fallback
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
      if constexpr (!__vectorizable<_Tp>)
        return _InvalidAbi();
      else
        return _ScalarAbi<1>();
    }

#endif

  /** @internal
   * You must use this type as template argument to function templates that are not declared
   * always_inline (to avoid issues when linking code compiled with different compiler flags).
   */
  struct _TargetTraits
  : _ArchTraits, _OptTraits
#if VIR_NEXT_PATCH
  {
    consteval _TargetTraits
    _M_math_abi() const
    { return _TargetTraits{_ArchTraits::_M_math_abi(), _OptTraits::_M_math_abi()}; }
  };
#else
  {};
#endif

  /** @internal
   * Alias for an ABI tag such that basic_vec<_Tp, __native_abi_t_<_Tp>> stores one SIMD register of
   * optimal width.
   *
   * @tparam _Tp  A vectorizable type.
   *
   * C++26 [simd.expos.abi]
   */
  template <typename _Tp>
    using __native_abi_t = decltype(std::simd::__native_abi<_Tp>());

  template <typename _Tp, int _Np, _TargetTraits _Target = {}>
    consteval auto
    __deduce_abi()
    {
      constexpr auto __native = std::simd::__native_abi<_Tp>();
      if constexpr (0 == __native._S_size || _Np <= 0)
        return _InvalidAbi();
      else if constexpr (_Np == __native._S_size)
        return __native;
      else
        return __native.template _M_resize<_Np>();
    }

  /** @internal
   * Alias for an ABI tag @c A such that <tt>basic_vec<_Tp, A></tt> stores @p _Np elements.
   *
   * C++26 [simd.expos.abi]
   */
  template <typename _Tp, int _Np>
    using __deduce_abi_t = decltype(std::simd::__deduce_abi<_Tp, _Np>());

  /** @internal
   * \c rebind implementation detail for basic_vec, and basic_mask where we know the destination
   * value-type
   */
  template <typename _Tp, int _Np, __abi_tag _A0, _ArchTraits = {}>
    consteval auto
    __abi_rebind()
    {
      if constexpr (_Np <= 0 || !__vectorizable<_Tp>)
        return _InvalidAbi();
      else
        {
          using _Native = remove_const_t<decltype(std::simd::__native_abi<_Tp>())>;
          static_assert(0 != _Native::_S_size);
          constexpr int __nreg = __div_ceil(_Np, _Native::_S_size);

          if constexpr (__scalar_abi_tag<_A0>)
            return std::simd::__deduce_abi<_Tp, _Np>();

#if VIR_NEXT_PATCH
          else if constexpr (__scalar_abi_tag<_Native> || (_A0::_S_is_cx_ctgus && _Np == 1))
            return _ScalarAbi<_Np>();

          else if constexpr (__complex_like<_Tp> && _A0::_S_is_cx_ctgus && _Native::_S_is_cx_ileav)
            // we need half the number of registers since the number applies twice, to reals and
            // imaginaries.
            return _Abi_t<_Np, __div_ceil(__nreg, 2), _A0::_S_variant>();

          else if constexpr (__complex_like<_Tp> && _A0::_S_is_cx_ileav && _Native::_S_is_cx_ctgus)
            return _Abi_t<_Np, __nreg * 2, _A0::_S_variant>();

          else if constexpr (__complex_like<_Tp> && (_A0::_S_is_cx_ctgus || _A0::_S_is_cx_ileav))
            return _Abi_t<_Np, __nreg, _A0::_S_variant>();

          else if constexpr (__complex_like<_Tp>)
            return _Abi_t<_Np, __nreg, _A0::_S_variant, _AbiVariant::_CxIleav>();

#endif
          else if constexpr (_Np == __nreg)
            return _ScalarAbi<_Np>();

          else
            return _Abi_t<_Np, __nreg, __filter_abi_variant(_A0::_S_variant,
                                                            _AbiVariant::_MaskVariants)>();
        }
    }

  /** @internal
   * @c rebind implementation detail for basic_mask.
   *
   * The important difference here is that we have no information about the actual value-type other
   * than its @c sizeof. So <tt>_Bytes == 8</tt> could mean <tt>complex<float></tt>, @c double, or
   * @c int64_t. E.g. <tt>_Np == 4</tt> with AVX w/o AVX2 that's <tt>vector(4) int</tt>,
   * <tt>vector(4) long long</tt>, or <tt>2x vector(2) long long</tt>.
   * That's why this overload has the additional @p _IsOnlyResize parameter, which tells us that the
   * value-type doesn't change.
   */
  template <size_t _Bytes, int _Np, __abi_tag _A0, bool _IsOnlyResize, _ArchTraits _Traits = {}>
    consteval auto
    __abi_rebind()
    {
#if VIR_NEXT_PATCH
      constexpr bool __from_cx = _A0::_S_is_cx_ctgus || _A0::_S_is_cx_ileav;

#endif
      if constexpr (_Bytes == 0 || _Np <= 0)
        return _InvalidAbi();

      else if constexpr (__scalar_abi_tag<_A0>)
        {
          if constexpr (_IsOnlyResize)
            // stick to _ScalarAbi (e.g. _Float16 without hardware support)
            return _ScalarAbi<_Np>();
#if VIR_NEXT_PATCH
          else if constexpr (_Bytes == sizeof(double) * 2)
            // we can be certain it's a mask<complex<double>, _Np>.
            return __abi_rebind<complex<double>, _Np, _A0>();
#endif
          else
            // otherwise, fresh start via __deduce_abi_t using __integer_from
            return std::simd::__deduce_abi<__integer_from<_Bytes>, _Np>();
        }

#if VIR_NEXT_PATCH
      // If the source ABI is complex, _Bytes == sizeof(complex<float>) or
      // sizeof(complex<float16_t>), and _IsOnlyResize is true, then it's a mask<complex<float>,
      // _Np>
      else if constexpr (__from_cx && _IsOnlyResize && _Bytes == 2 * sizeof(double))
        return __abi_rebind<complex<double>, _Np, _A0>();
      else if constexpr (__from_cx && _IsOnlyResize && _Bytes == 2 * sizeof(float))
        return __abi_rebind<complex<float>, _Np, _A0>();
      else if constexpr (__from_cx && _IsOnlyResize && _Bytes == 2 * sizeof(_Float16))
        return __abi_rebind<complex<_Float16>, _Np, _A0>();

#endif
#if _GLIBCXX_X86
      // AVX w/o AVX2:
      // e.g. resize_t<8, mask<float, Whatever>> needs to be _Abi<8, 1> not _Abi<8, 2>
      // We determine whether _A0 identifies an AVX vector by looking at the size of a native
      // register. If it's 32, it's a YMM register, otherwise it's 16 or less.
      else if constexpr (_IsOnlyResize
                           && _Traits._M_have_avx() && !_Traits._M_have_avx2()
                           && __bit_ceil(__div_ceil<unsigned>(
                                            _A0::_S_size, _A0::_S_nreg)) * _Bytes == 32)
        {
          if constexpr (_Bytes == sizeof(double))
            return __abi_rebind<double, _Np, _A0>();
          else if constexpr (_Bytes == sizeof(float))
            return __abi_rebind<float, _Np, _A0>();
          else if constexpr (_Traits._M_have_f16c() && _Bytes == sizeof(_Float16))
            return __abi_rebind<_Float16, _Np, _A0>();
          else // impossible
            static_assert(false);
        }
#endif

      else
        return __abi_rebind<__integer_from<_Bytes>, _Np, _A0>();
    }

  /** @internal
   * Returns true unless _GLIBCXX_SIMD_COND_EXPLICIT_MASK_CONVERSION is defined.
   *
   * On IvyBridge, (vec<float> == 0.f) == (rebind_t<int, vec<float>> == 0) does not compile. It does
   * compile on basically every other target, though. This is due to the difference in ABI tag:
   * _Abi<8, 1, …> vs. _Abi<8, 2, …> (8 elements, 1 vs. 2 registers).
   * I know how to define this funtion for libstdc++ to avoid interconvertible masks. The question
   * is whether we can specify this in general for C++29.
   *
   * Idea: Is rebind_t<integer-from<...>, mask>::abi_type the same type as
   *   deduce-t<integer-from<...>, mask::size()>? If yes, it's the "better" ABI tag. However, this
   *   makes the conversion behavior dependent on compiler flags. Probably not what we want.
   */
  template <typename _To, typename _From>
  consteval bool
    __is_mask_conversion_explicit([[maybe_unused]] size_t __b0, [[maybe_unused]] size_t __b1)
    {
      constexpr int __n = _To::_S_size;
      static_assert(__n == _From::_S_size);
#ifndef _GLIBCXX_SIMD_COND_EXPLICIT_MASK_CONVERSION
      /// C++26 [simd.mask.ctor] uses unconditional explicit
      return true;
#else
      if (__b0 != __b1)
        return true;

      // everything is better than _ScalarAbi, except when converting to a single bool
      if constexpr (__scalar_abi_tag<_To>)
        return __n > 1;
      else if constexpr (__scalar_abi_tag<_From>)
        return true;

      // converting to a bit-mask is better
      else if constexpr (_To::_S_is_vecmask != _From::_S_is_vecmask)
        return _To::_S_is_vecmask; // to vector-mask is explicit

      // with vec-masks, fewer registers is better
      else if constexpr (_From::_S_nreg != _To::_S_nreg)
        return _From::_S_nreg < _To::_S_nreg;

#if VIR_NEXT_PATCH
      // differ only on _Cx flags
      // interleaved complex is worse
      else if constexpr (_To::_S_is_cx_ileav)
        return true;
      else if constexpr (_From::_S_is_cx_ileav)
        return false;

      // prefer non-_Cx over _CxCtgus
      else if constexpr (_To::_S_is_cx_ctgus)
        return true;
      else if constexpr (_From::_S_is_cx_ctgus)
        return false;
#endif
      else
        __builtin_unreachable();
#endif
    }

  /** @internal
   * An alias for a signed integer type.
   *
   * libstdc++ unconditionally uses @c int here, since it matches the return type of
   * 'Bit Operation Builtins' in GCC.
   *
   * C++26 [simd.expos.defn]
   */
  using __simd_size_type = int;

  /** @internal
   * The width of <tt>basic_vec<T, Abi></tt> if the specialization <tt>basic_vec<T, Abi></tt> is
   * enabled, or @c 0 otherwise.
   *
   * C++26 [simd.expos.defn]
   */
  template <typename _Tp, typename _Ap>
    constexpr __simd_size_type __simd_size_v = 0;

  template <__vectorizable _Tp, __abi_tag _Ap>
    constexpr __simd_size_type __simd_size_v<_Tp, _Ap> = _Ap::_S_size;

  // integral_constant shortcut
  template <__simd_size_type _Xp>
    inline constexpr integral_constant<__simd_size_type, _Xp> __simd_size_c = {};

  // [simd.syn]
  template <typename _Tp, typename _Ap = __native_abi_t<_Tp>>
    class basic_vec;

  template <typename _Tp, __simd_size_type _Np = __simd_size_v<_Tp, __native_abi_t<_Tp>>>
    using vec = basic_vec<_Tp, __deduce_abi_t<_Tp, _Np>>;

  template <size_t _Bytes, typename _Ap = __native_abi_t<__integer_from<_Bytes>>>
    class basic_mask;

  template <typename _Tp, __simd_size_type _Np = __simd_size_v<_Tp, __native_abi_t<_Tp>>>
    using mask = basic_mask<sizeof(_Tp), __deduce_abi_t<_Tp, _Np>>;

  // [simd.ctor] load constructor constraints
  template <typename _Tp, size_t _Np = -1uz>
    concept __static_sized_range
      = ranges::sized_range<_Tp> && requires(_Tp&& __r) {
        typename integral_constant<size_t, ranges::size(__r)>;
        requires (_Np == -1uz || ranges::size(__r) == _Np);
      };

  template <typename _Rg>
    consteval size_t
    __static_range_size(_Rg& __r)
    {
      if constexpr (requires { typename integral_constant<size_t, ranges::size(__r)>; })
        return ranges::size(__r);
      else
        return dynamic_extent;
    }

  // [simd.general] value-reserving
  template <typename _From, typename _To>
    concept __arithmetic_only_value_preserving_convertible_to
      = convertible_to<_From, _To> && is_arithmetic_v<_From> && is_arithmetic_v<_To>
          && !(is_signed_v<_From> && is_unsigned_v<_To>)
          && numeric_limits<_From>::digits <= numeric_limits<_To>::digits
          && numeric_limits<_From>::max() <= numeric_limits<_To>::max()
          && numeric_limits<_From>::lowest() >= numeric_limits<_To>::lowest();

  /** @internal
   * Satisfied if the conversion from @p _From to @p _To is a value-preserving conversion.
   *
   * C++26 [simd.general]
   */
  template <typename _From, typename _To>
    concept __value_preserving_convertible_to
#if VIR_NEXT_PATCH
      = __arithmetic_only_value_preserving_convertible_to<_From, _To>
          || (__complex_like<_To> && __arithmetic_only_value_preserving_convertible_to<
                                        _From, typename _To::value_type>);
#else
      = __arithmetic_only_value_preserving_convertible_to<_From, _To>;
#endif

  // LWG4420
  template <typename _From, typename _To>
    concept __explicitly_convertible_to = requires {
      static_cast<_To>(declval<_From>());
    };

  /** @internal
   * C++26 [simd.expos]
   */
  template<typename _Tp>
    concept __constexpr_wrapper_like
      = convertible_to<_Tp, decltype(_Tp::value)>
          && equality_comparable_with<_Tp, decltype(_Tp::value)>
          && bool_constant<_Tp() == _Tp::value>::value
          && bool_constant<static_cast<decltype(_Tp::value)>(_Tp()) == _Tp::value>::value;

  // [simd.ctor] explicit(...) of broadcast ctor
  template <auto _From, typename _To>
    concept __non_narrowing_constexpr_conversion
      = is_arithmetic_v<decltype(_From)>
          && static_cast<decltype(_From)>(static_cast<_To>(_From)) == _From
          && !(unsigned_integral<_To> && _From < decltype(_From)())
          && _From <= std::numeric_limits<_To>::max()
          && _From >= std::numeric_limits<_To>::lowest();

  // [simd.ctor] p4
  // This implements LWG4436 (submitted on 2025-10-28)
  template <typename _From, typename _To>
    concept __broadcast_constructible
      = ((convertible_to<_From, _To> && !is_arithmetic_v<remove_cvref_t<_From>>
            && !__constexpr_wrapper_like<remove_cvref_t<_From>>) // 4.1
           || __value_preserving_convertible_to<remove_cvref_t<_From>, _To> // 4.2
           || (__constexpr_wrapper_like<remove_cvref_t<_From>> // 4.3
                 && __non_narrowing_constexpr_conversion<auto(remove_cvref_t<_From>::value),
                                                         _To>));

  // __higher_floating_point_rank_than<_Tp, U> (_Tp has higher or equal floating point rank than U)
  template <typename _From, typename _To>
    consteval bool
    __higher_floating_point_rank_than()
    {
      return floating_point<_From> && floating_point<_To>
               && is_same_v<common_type_t<_From, _To>, _From> && !is_same_v<_From, _To>;
    }

  // __higher_integer_rank_than<_Tp, U> (_Tp has higher or equal integer rank than U)
  template <typename _From, typename _To>
    consteval bool
    __higher_integer_rank_than()
    {
      return integral<_From> && integral<_To>
               && (sizeof(_From) > sizeof(_To) || is_same_v<common_type_t<_From, _To>, _From>)
               && !is_same_v<_From, _To>;
    }

  template <typename _From, typename _To>
    concept __higher_rank_than
      = __higher_floating_point_rank_than<_From, _To>() || __higher_integer_rank_than<_From, _To>();

  struct __convert_flag;

  template <typename _From, typename _To, typename... _Flags>
    concept __loadstore_convertible_to
      = same_as<_From, _To>
          || (__vectorizable<_From> && __vectorizable<_To>
                && (__value_preserving_convertible_to<_From, _To>
                       || (__explicitly_convertible_to<_From, _To>
                             && (std::is_same_v<_Flags, __convert_flag> || ...))));

  template <typename _From, typename _To>
    concept __simd_generator_convertible_to
      = std::convertible_to<_From, _To>
          && (!is_arithmetic_v<_From> || __value_preserving_convertible_to<_From, _To>);

  template <typename _Fp, typename _Tp, __simd_size_type... _Is>
    requires (__simd_generator_convertible_to<
                decltype(declval<_Fp>()(__simd_size_c<_Is>)), _Tp> && ...)
    constexpr void
    __simd_generator_invokable_impl(integer_sequence<__simd_size_type, _Is...>);

  template <typename _Fp, typename _Tp, __simd_size_type _Np>
    concept __simd_generator_invokable = requires {
      __simd_generator_invokable_impl<_Fp, _Tp>(make_integer_sequence<__simd_size_type, _Np>());
    };

  template <typename _Fp>
    concept __index_permutation_function_nosize = requires(_Fp const& __f)
      {
        { __f(0) } -> std::integral;
      };

  template <typename _Fp, typename _Simd>
    concept __index_permutation_function
      = __index_permutation_function_nosize<_Fp> || requires(_Fp const& __f) {
        { __f(0, 0) } -> std::integral;
      };

  /** @internal
   * The value of the @c _Bytes template argument to a @c basic_mask specialization.
   *
   * C++26 [simd.expos.defn]
   */
  template <typename _Tp>
    constexpr size_t __mask_element_size = 0;

  template <size_t _Bytes, __abi_tag _Ap>
    constexpr size_t __mask_element_size<basic_mask<_Bytes, _Ap>> = _Bytes;

  // [simd.expos]
  template <typename _Vp>
    concept __simd_vec_type
      = same_as<_Vp, basic_vec<typename _Vp::value_type, typename _Vp::abi_type>>
          && is_default_constructible_v<_Vp>;

  template <typename _Vp>
    concept __simd_mask_type
      = same_as<_Vp, basic_mask<__mask_element_size<_Vp>, typename _Vp::abi_type>>
        && is_default_constructible_v<_Vp>;

  /** @internal
   * Satisfied if @p _Tp is a data-parallel type.
   */
  template <typename _Vp>
    concept __simd_vec_or_mask_type = __simd_vec_type<_Vp> || __simd_mask_type<_Vp>;

  template <typename _Vp>
    concept __simd_floating_point
      = __simd_vec_type<_Vp> && floating_point<typename _Vp::value_type>;

  template <typename _Vp>
    concept __simd_integral
      = __simd_vec_type<_Vp> && integral<typename _Vp::value_type>;

#if VIR_NEXT_PATCH
  template <typename _Vp>
    using __simd_complex_value_type = typename _Vp::value_type::value_type;

  template <typename _Vp>
    concept __simd_complex
      = __simd_vec_type<_Vp> && __complex_like_impl<typename _Vp::value_type>;

#endif
  template <typename _Tp>
    concept __converts_to_vec
      = __simd_vec_type<decltype(declval<const _Tp&>() + declval<const _Tp&>())>;

  template <__converts_to_vec _Tp>
    using __deduced_vec_t = decltype(declval<const _Tp&>() + declval<const _Tp&>());

  template <typename _Vp, typename _Tp>
    using __make_compatible_simd_t
      = decltype([] {
          using _Up = decltype(declval<const _Tp&>() + declval<const _Tp&>());
          if constexpr (__simd_vec_type<_Up>)
            return _Up();
          else
            return vec<_Up, _Vp::size()>();
      }());

  template <typename _Tp>
    concept __math_floating_point = __simd_floating_point<__deduced_vec_t<_Tp>>;

  template <typename _BinaryOperation, typename _Tp>
    concept __reduction_binary_operation
      = requires (const _BinaryOperation __binary_op, const vec<_Tp, 1> __v) {
        { __binary_op(__v, __v) } -> same_as<vec<_Tp, 1>>;
      };

  /** @internal
   * Returns the highest index @c i where <tt>(__bits >> i) & 1</tt> equals @c 1.
   */
  [[__gnu__::__always_inline__]]
  constexpr __simd_size_type
  __highest_bit(std::unsigned_integral auto __bits)
  {
    using __gnu_cxx::__int_traits;
    constexpr auto _Nd = __int_traits<decltype(__bits)>::__digits;
    return _Nd - 1 - __countl_zero(__bits);
  }

  template <__vectorizable _Tp, __simd_size_type _Np, __abi_tag _Ap>
    using __similar_mask = basic_mask<sizeof(_Tp), decltype(__abi_rebind<_Tp, _Np, _Ap>())>;

  // Allow _Tp to be _InvalidInteger for __integer_from<16>
  template <typename _Tp, __simd_size_type _Np, __abi_tag _Ap>
    using __similar_vec = basic_vec<_Tp, decltype(__abi_rebind<_Tp, _Np, _Ap>())>;

  template <typename _Tp, __simd_size_type _Np, __abi_tag _Ap>
    using __similar_resized_vec = conditional_t<__scalar_abi_tag<_Ap>, basic_vec<_Tp, _ScalarAbi<_Np>>,
                                                basic_vec<_Tp, decltype(__abi_rebind<_Tp, _Np, _Ap>())>>;

  // LWG4470 [simd.expos]
  template <size_t _Bytes, typename _Ap>
    using __simd_vec_from_mask_t = __similar_vec<__integer_from<_Bytes>, _Ap::_S_size, _Ap>;

#if _GLIBCXX_SIMD_THROW_ON_BAD_VALUE // used for unit tests (also see P3844)
  class __bad_value_preserving_cast
  {};

#define __glibcxx_on_bad_value_preserving_cast throw __bad_value_preserving_cast
#else
  void __bad_value_preserving_cast(); // not defined

#define __glibcxx_on_bad_value_preserving_cast __bad_value_preserving_cast
#endif

  template <typename _To, typename _From>
#if _GLIBCXX_SIMD_THROW_ON_BAD_VALUE // see P3844
    [[__gnu__::__optimize__("exceptions")]] // work around potential -fno-exceptions
#endif
    consteval _To
    __value_preserving_cast(const _From& __x)
    {
      static_assert(is_arithmetic_v<_From>);
      if constexpr (!__value_preserving_convertible_to<_From, _To>)
        {
          using _Up = typename __make_unsigned<_From>::__type;
          if (static_cast<_Up>(static_cast<_To>(__x)) != static_cast<_Up>(__x))
            __glibcxx_on_bad_value_preserving_cast();
          else if constexpr (is_signed_v<_From> && is_unsigned_v<_To>)
            {
              if (__x < _From())
                __glibcxx_on_bad_value_preserving_cast();
            }
          else if constexpr (unsigned_integral<_From> && signed_integral<_To>)
            {
              if (__x > numeric_limits<_To>::max())
                __glibcxx_on_bad_value_preserving_cast();
            }
        }
      return static_cast<_To>(__x);
    }

  template <typename _From, typename _To>
    concept __simd_vec_bcast_consteval
      = __explicitly_convertible_to<_From, _To>
          && is_arithmetic_v<remove_cvref_t<_From>> && convertible_to<_From, _To>
          && !__value_preserving_convertible_to<remove_cvref_t<_From>, _To>
          && (is_same_v<common_type_t<_From, _To>, _To>
                || (is_same_v<remove_cvref_t<_From>, int> && is_integral_v<_To>)
                || (is_same_v<remove_cvref_t<_From>, unsigned> && unsigned_integral<_To>));

  /** @internal
   * std::pair is not trivially copyable, this one is
   */
  template <typename _T0, typename _T1>
    struct __trivial_pair
    {
      _T0 _M_first;
      _T1 _M_second;
    };

  template <typename _From, typename _To>
    concept __converts_trivially = convertible_to<_From, _To>
                                     && sizeof(_From) == sizeof(_To)
                                     && is_integral_v<_From> == is_integral_v<_To>
                                     && is_floating_point_v<_From> == is_floating_point_v<_To>;

  [[__gnu__::__always_inline__]]
  constexpr void
  __bit_foreach(unsigned_integral auto __bits, auto&& __fun)
  {
    static_assert(sizeof(__bits) >= sizeof(int)); // avoid promotion to int
    while (__bits)
      {
        __fun(__countr_zero(__bits));
        __bits &= (__bits - 1);
      }
  }

  /** @internal
   * Optimized @c memcpy for use in partial loads and stores.
   *
   * The implementation uses at most two fixed-size power-of-2 @c memcpy calls and reduces the
   * number of branches to a minimum. The variable size is achieved by overlapping two @c memcpy
   * calls.
   *
   * @tparam _Chunk   Copies @p __n times @p _Chunk bytes.
   * @tparam _Max     Copy no more than @p _Max bytes.
   *
   * @param  __dst    The destination pointer.
   * @param  __src    The source pointer.
   * @param  __n      Thu number of chunks that need to be copied.
   */
  template <size_t _Chunk, size_t _Max>
    inline void
    __memcpy_chunks(byte* __restrict__ __dst, const byte* __restrict__ __src,
                    size_t __n)
    {
      static_assert(_Max <= 64);
      static_assert(__has_single_bit(_Chunk) && _Chunk <= 8);
      size_t __bytes = _Chunk * __n;
      if (__builtin_constant_p(__bytes))
        { // If __n is known via constant propagation use a single memcpy call. Since this is still
          // a fixed-size memcpy to the compiler, this leaves more room for optimization.
          __builtin_memcpy(__dst, __src, __bytes);
        }
      else if (__bytes > 32 && _Max > 32)
        {
          __builtin_memcpy(__dst, __src, 32);
          __bytes -= 32;
          __builtin_memcpy(__dst + __bytes, __src + __bytes, 32);
        }
      else if (__bytes > 16 && _Max > 16)
        {
          __builtin_memcpy(__dst, __src, 16);
          if constexpr (_Chunk == 8)
            {
              __bytes -= 8;
              __builtin_memcpy(__dst + __bytes, __src + __bytes, 8);
            }
          else
            {
              __bytes -= 16;
              __builtin_memcpy(__dst + __bytes, __src + __bytes, 16);
            }
        }
      else if (__bytes > 8 && _Max > 8)
        {
          __builtin_memcpy(__dst, __src, 8);
          if constexpr (_Chunk == 4)
            {
              __bytes -= 4;
              __builtin_memcpy(__dst + __bytes, __src + __bytes, 4);
            }
          else if constexpr (_Chunk < 4)
            {
              __bytes -= 8;
              __builtin_memcpy(__dst + __bytes, __src + __bytes, 8);
            }
        }
      else if (__bytes > 4 && _Max > 4)
        {
          __builtin_memcpy(__dst, __src, 4);
          if constexpr (_Chunk == 2)
            {
              __bytes -= 2;
              __builtin_memcpy(__dst + __bytes, __src + __bytes, 2);
            }
          else if constexpr (_Chunk == 1)
            {
              __bytes -= 4;
              __builtin_memcpy(__dst + __bytes, __src + __bytes, 4);
            }
        }
      else if (__bytes >= 2)
        {
          __builtin_memcpy(__dst, __src, 2);
          if constexpr (_Chunk == 2)
            {
              __bytes -= 2;
              __builtin_memcpy(__dst + __bytes, __src + __bytes, 2);
            }
          else if constexpr (_Chunk == 1)
            {
              __bytes -= 1;
              __builtin_memcpy(__dst + __bytes, __src + __bytes, 1);
            }
        }
      else if (__bytes == 1)
        __builtin_memcpy(__dst, __src, 1);
    }

  [[__gnu__::__always_inline__]]
  inline bool
  __ptr_is_aligned_to(const void* __ptr, size_t __align)
  {
    const auto __addr = __builtin_bit_cast(__UINTPTR_TYPE__, __ptr);
    return (__addr % __align) == 0;
  }

  // [simd.reductions] identity_element = *see below*
  template <typename _Tp, typename _BinaryOperation>
    requires __is_one_of<_BinaryOperation,
                         plus<>, multiplies<>, bit_and<>, bit_or<>, bit_xor<>>::value
    consteval _Tp
    __default_identity_element()
    {
      if constexpr (same_as<_BinaryOperation, multiplies<>>)
        return _Tp(1);
      else if constexpr (same_as<_BinaryOperation, bit_and<>>)
        return _Tp(~_Tp());
      else
        return _Tp(0);
    }
}

#pragma GCC diagnostic pop
#endif // C++26
#endif // _GLIBCXX_SIMD_DETAILS_H
