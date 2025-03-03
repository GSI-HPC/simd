/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_FLAGS_H_
#define PROTOTYPE_FLAGS_H_

#include "simd_config.h"
#include "fwddecl.h"

#include <bit>
#include <concepts>

namespace std::__detail
{
  struct _LoadStoreTag
  {};

  struct _AllowPartialLoadStore
  : _LoadStoreTag
  {};

  struct _Throw
  : _LoadStoreTag
  {};

  struct _Convert
  : _LoadStoreTag
  {};

  template <typename _To>
    struct _ConvertTo
    : _LoadStoreTag
    {
      using type = _To;
    };

  struct _Aligned
  : _LoadStoreTag
  {
    template <typename _Tp, typename _Up>
      _GLIBCXX_SIMD_INTRINSIC static constexpr _Up*
      _S_adjust_pointer(_Up* __ptr)
      {
        if consteval
          {
            return __ptr;
          }
        else
          {
            return static_cast<_Up*>(__builtin_assume_aligned(__ptr, simd_alignment_v<_Tp, _Up>));
          }
      }
  };

  template <std::size_t _Np>
    struct _Overaligned
    : _LoadStoreTag
    {
      static_assert(std::__has_single_bit(_Np));

      template <typename, typename _Up>
        _GLIBCXX_SIMD_INTRINSIC static constexpr _Up*
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

  struct _Streaming
  : _LoadStoreTag
  {};

  template <int _L1, int _L2 /*, exclusive vs. shared*/>
    struct _Prefetch
    : _LoadStoreTag
    {
      template <typename, typename _Up>
        _GLIBCXX_SIMD_ALWAYS_INLINE static _Up*
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

  template <typename _Tp>
    concept __loadstore_tag = std::is_base_of_v<_LoadStoreTag, _Tp>;
} // namespace __detail

namespace std
{

  template <typename... _Flags>
    requires (__detail::__loadstore_tag<_Flags> and ...)
    struct simd_flags<_Flags...>
    {
      consteval bool
      _M_is_equal(simd_flags) const
      { return true; }

      template <typename... _Other>
        consteval bool
        _M_is_equal(simd_flags<_Other...> __y) const
        { return std::same_as<simd_flags<>, decltype(_M_xor(__y))>; }

      template <typename... _Other>
        consteval bool
        _M_test(simd_flags<_Other...> __x) const noexcept
        { return __x._M_is_equal(_M_and(__x)); }

      friend consteval auto
      operator|(simd_flags, simd_flags<>)
      { return simd_flags{}; }

      template <typename _T0, typename... _More>
        friend consteval auto
        operator|(simd_flags, simd_flags<_T0, _More...>)
        {
          if constexpr ((std::same_as<_Flags, _T0> or ...))
            return simd_flags<_Flags...>{} | simd_flags<_More...>{};
          else
            return simd_flags<_Flags..., _T0>{} | simd_flags<_More...>{};
        }

      consteval auto
      _M_and(simd_flags<>) const
      { return simd_flags<>{}; }

      template <typename _T0, typename... _More>
        consteval auto
        _M_and(simd_flags<_T0, _More...>) const
        {
          if constexpr ((std::same_as<_Flags, _T0> or ...))
            return simd_flags<_T0>{} | (simd_flags{}._M_and(simd_flags<_More...>{}));
          else
            return simd_flags{}._M_and(simd_flags<_More...>{});
        }

      consteval auto
      _M_xor(simd_flags<>) const
      { return simd_flags{}; }

      template <typename _T0, typename... _More>
        consteval auto
        _M_xor(simd_flags<_T0, _More...>) const
        {
          if constexpr ((std::same_as<_Flags, _T0> or ...))
            {
              constexpr auto __removed
                = (std::conditional_t<std::same_as<_Flags, _T0>, simd_flags<>,
                                      simd_flags<_Flags>>{} | ...);
              return __removed._M_xor(simd_flags<_More...>{});
            }
          else
            return (simd_flags{} | simd_flags<_T0>{})._M_xor(simd_flags<_More...>{});
        }

      // LEWG took this out
#if LEWG_WANTS_FLAGS_WITH_FULL_API
      template <typename... _Other>
        consteval bool
        test(simd_flags<_Other...> __x) const noexcept
        { return _M_test(__x); }

      template <typename... _Other>
        friend consteval bool
        operator==(simd_flags __x, simd_flags<_Other...> __y)
        { return __x._M_is_equal(__y); }

      template <typename... _Other>
        friend consteval auto
        operator&(simd_flags __lhs, simd_flags<_Other...> __rhs)
        { return __lhs._M_and(__rhs); }

      template <typename... _Other>
        friend consteval auto
        operator^(simd_flags __lhs, simd_flags<_Other...> __rhs)
        { return __lhs._M_xor(__rhs); }
#endif

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
          (_S_apply_adjust_pointer<_Flags, _Tp>(__ptr), ...);
          return __ptr;
        }
    };

  // [simd.simd_flags]
  inline constexpr simd_flags<> simd_flag_default;

  inline constexpr simd_flags<__detail::_Convert> simd_flag_convert;

  inline constexpr simd_flags<__detail::_Aligned> simd_flag_aligned;

  template <std::size_t _Np>
    requires(std::has_single_bit(_Np))
    inline constexpr simd_flags<__detail::_Overaligned<_Np>> simd_flag_overaligned;

  // extensions
  template <typename _To>
    inline constexpr simd_flags<__detail::_ConvertTo<_To>> flag_convert_to;

  inline constexpr simd_flags<__detail::_AllowPartialLoadStore> __allow_partial_loadstore;

  inline constexpr simd_flags<__detail::_Throw> __flag_throw;

  inline constexpr std::simd_flags<__detail::_Streaming> __flag_streaming;

  template <int _L1, int _L2>
    inline constexpr std::simd_flags<__detail::_Prefetch<_L1, _L2>> __flag_prefetch;

  [[deprecated("use simd_flag_default")]]
  inline constexpr auto element_aligned = simd_flag_default;

  [[deprecated("use simd_flag_aligned")]]
  inline constexpr auto vector_aligned = simd_flag_aligned;

  template <size_t _Np>
    [[deprecated("use simd_flag_overaligned")]]
    inline constexpr auto overaligned = simd_flag_overaligned<_Np>;
} // namespace std

#endif  // PROTOTYPE_FLAGS_H_
