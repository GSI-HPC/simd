/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef _GLIBCXX_SIMD_FLAGS_H
#define _GLIBCXX_SIMD_FLAGS_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include "simd_details.h"

namespace std::simd
{
  // [simd.traits]
  // --- alignment ---
  template <typename _Tp, typename _Up = typename _Tp::value_type>
    struct alignment
    {};

  template <typename _Tp, typename _Abi, __vectorizable _Up>
    struct alignment<basic_vec<_Tp, _Abi>, _Up>
    : integral_constant<size_t, alignof(basic_vec<_Tp, _Abi>)>
    {};

  template <typename _Tp, typename _Up = typename _Tp::value_type>
    constexpr size_t alignment_v = alignment<_Tp, _Up>::value;

  // [simd.flags] -------------------------------------------------------------
  struct _LoadStoreTag
  {};

  /** @internal
   * <tt>struct convert-flag</tt>
   *
   * C++26 [simd.expos] / [simd.flags]
   */
  struct __convert_flag
  : _LoadStoreTag
  {};

  /** @internal
   * <tt>struct aligned-flag</tt>
   *
   * C++26 [simd.expos] / [simd.flags]
   */
  struct __aligned_flag
  : _LoadStoreTag
  {
    template <typename _Tp, typename _Up>
      [[__gnu__::__always_inline__]]
      static constexpr _Up*
      _S_adjust_pointer(_Up* __ptr)
      {
        if consteval
          {
            return __ptr;
          }
        else
          {
            return static_cast<_Up*>(__builtin_assume_aligned(
                                       __ptr, simd::alignment_v<_Tp, remove_cv_t<_Up>>));
          }
      }
  };

  /** @internal
   * <tt>template<size_t N> struct overaligned-flag</tt>
   *
   * @tparam _Np  alignment in bytes
   *
   * C++26 [simd.expos] / [simd.flags]
   */
  template <size_t _Np>
    struct __overaligned_flag
    : _LoadStoreTag
    {
      static_assert(__has_single_bit(_Np));

      template <typename, typename _Up>
        [[__gnu__::__always_inline__]]
        static constexpr _Up*
        _S_adjust_pointer(_Up* __ptr)
        {
          if consteval
            {
              return __ptr;
            }
          else
            {
              return static_cast<_Up*>(__builtin_assume_aligned(__ptr, _Np));
            }
        }
    };

  struct __partial_loadstore_flag
  : _LoadStoreTag
  {};

#if 0
  template <typename _To>
    struct __convert_to_flag
    : _LoadStoreTag
    { using type = _To; };

  struct __throw_flag
  : _LoadStoreTag
  {};

  struct __streaming_flag
  : _LoadStoreTag
  {};

  template <int _L1, int _L2 /*, exclusive vs. shared*/>
    struct __prefetch_flag
    : _LoadStoreTag
    {
      template <typename, typename _Up>
        [[__gnu__::__always_inline__]]
        static _Up*
        _S_adjust_pointer(_Up* __ptr)
        {
          // one read: 0, 0
          // L1: 0, 1
          // L2: 0, 2
          // L3: 0, 3
          // (exclusive cache line) for writing: 1, 0 / 1, 1
          /*          constexpr int __write = 1;
                      constexpr int __level = 0-3;
          __builtin_prefetch(__ptr, __write, __level)
          _mm_prefetch(reinterpret_cast<char const*>(__ptr), _MM_HINT_T0);
          _mm_prefetch(reinterpret_cast<char const*>(__ptr), _MM_HINT_T1);
          _mm_prefetch(reinterpret_cast<char const*>(__ptr), _MM_HINT_T2);
          _mm_prefetch(reinterpret_cast<char const*>(__ptr), _MM_HINT_ET0);
          _mm_prefetch(reinterpret_cast<char const*>(__ptr), _MM_HINT_ET1);
          _mm_prefetch(reinterpret_cast<char const*>(__ptr), _MM_HINT_NTA);*/
          return __ptr;
        }
    };
#endif

  template <typename _Tp>
    concept __loadstore_tag = is_base_of_v<_LoadStoreTag, _Tp>;

  template <typename...>
    struct flags;

  template <typename... _Traits>
    requires (__loadstore_tag<_Traits> && ...)
    struct flags<_Traits...>
    {
      consteval bool
      _M_is_equal(flags) const
      { return true; }

      template <typename... _Other>
        consteval bool
        _M_is_equal(flags<_Other...> __y) const
        { return same_as<flags<>, decltype(_M_xor(__y))>; }

      template <typename... _Other>
        consteval bool
        _M_test(flags<_Other...> __x) const noexcept
        { return __x._M_is_equal(_M_and(__x)); }

      friend consteval auto
      operator|(flags, flags<>)
      { return flags{}; }

      template <typename _T0, typename... _More>
        friend consteval auto
        operator|(flags, flags<_T0, _More...>)
        {
          if constexpr ((same_as<_Traits, _T0> || ...))
            return flags<_Traits...>{} | flags<_More...>{};
          else
            return flags<_Traits..., _T0>{} | flags<_More...>{};
        }

      consteval auto
      _M_and(flags<>) const
      { return flags<>{}; }

      template <typename _T0, typename... _More>
        consteval auto
        _M_and(flags<_T0, _More...>) const
        {
          if constexpr ((same_as<_Traits, _T0> || ...))
            return flags<_T0>{} | (flags{}._M_and(flags<_More...>{}));
          else
            return flags{}._M_and(flags<_More...>{});
        }

      consteval auto
      _M_xor(flags<>) const
      { return flags{}; }

      template <typename _T0, typename... _More>
        consteval auto
        _M_xor(flags<_T0, _More...>) const
        {
          if constexpr ((same_as<_Traits, _T0> || ...))
            {
              constexpr auto __removed
                = (conditional_t<same_as<_Traits, _T0>, flags<>,
                                      flags<_Traits>>{} | ...);
              return __removed._M_xor(flags<_More...>{});
            }
          else
            return (flags{} | flags<_T0>{})._M_xor(flags<_More...>{});
        }

      template <typename _F0, typename _Tp>
        static constexpr void
        _S_apply_adjust_pointer(auto& __ptr)
        {
          if constexpr (requires{ _F0::template _S_adjust_pointer<_Tp>(__ptr); })
            __ptr = _F0::template _S_adjust_pointer<_Tp>(__ptr);
        }

      template <typename _Tp, typename _Up>
        static constexpr _Up*
        _S_adjust_pointer(_Up* __ptr)
        {
          (_S_apply_adjust_pointer<_Traits, _Tp>(__ptr), ...);
          return __ptr;
        }
    };

  inline constexpr flags<> flag_default {};

  inline constexpr flags<__convert_flag> flag_convert {};

  inline constexpr flags<__aligned_flag> flag_aligned {};

  template <size_t _Np>
    requires(__has_single_bit(_Np))
    inline constexpr flags<__overaligned_flag<_Np>> flag_overaligned {};

  /** @internal
   * Pass to unchecked_load or unchecked_store to make it behave like partial_load / partial_store.
   */
  inline constexpr flags<__partial_loadstore_flag> __allow_partial_loadstore {};

#if 0
  // extensions
  template <typename _To>
    inline constexpr flags<__convert_to_flag<_To>> __flag_convert_to {};

  inline constexpr flags<__throw_flag> __flag_throw {};

  inline constexpr flags<__streaming_flag> __flag_streaming {};

  template <int _L1, int _L2>
    inline constexpr flags<__prefetch_flag<_L1, _L2>> __flag_prefetch {};
#endif
}

#endif // C++26
#endif // _GLIBCXX_SIMD_FLAGS_H
