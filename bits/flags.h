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
            return static_cast<_Up*>(__builtin_assume_aligned(
                                       __ptr, std::datapar::alignment_v<_Tp, _Up>));
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

namespace std::datapar
{

  template <typename... _Flags>
    requires (__detail::__loadstore_tag<_Flags> and ...)
    struct flags<_Flags...>
    {
      consteval bool
      _M_is_equal(flags) const
      { return true; }

      template <typename... _Other>
        consteval bool
        _M_is_equal(flags<_Other...> __y) const
        { return std::same_as<flags<>, decltype(_M_xor(__y))>; }

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
          if constexpr ((std::same_as<_Flags, _T0> or ...))
            return flags<_Flags...>{} | flags<_More...>{};
          else
            return flags<_Flags..., _T0>{} | flags<_More...>{};
        }

      consteval auto
      _M_and(flags<>) const
      { return flags<>{}; }

      template <typename _T0, typename... _More>
        consteval auto
        _M_and(flags<_T0, _More...>) const
        {
          if constexpr ((std::same_as<_Flags, _T0> or ...))
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
          if constexpr ((std::same_as<_Flags, _T0> or ...))
            {
              constexpr auto __removed
                = (std::conditional_t<std::same_as<_Flags, _T0>, flags<>,
                                      flags<_Flags>>{} | ...);
              return __removed._M_xor(flags<_More...>{});
            }
          else
            return (flags{} | flags<_T0>{})._M_xor(flags<_More...>{});
        }

      // LEWG took this out
#if LEWG_WANTS_FLAGS_WITH_FULL_API
      template <typename... _Other>
        consteval bool
        test(flags<_Other...> __x) const noexcept
        { return _M_test(__x); }

      template <typename... _Other>
        friend consteval bool
        operator==(flags __x, flags<_Other...> __y)
        { return __x._M_is_equal(__y); }

      template <typename... _Other>
        friend consteval auto
        operator&(flags __lhs, flags<_Other...> __rhs)
        { return __lhs._M_and(__rhs); }

      template <typename... _Other>
        friend consteval auto
        operator^(flags __lhs, flags<_Other...> __rhs)
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

  // [simd.flags]
  inline constexpr flags<> flag_default;

  inline constexpr flags<__detail::_Convert> flag_convert;

  inline constexpr flags<__detail::_Aligned> flag_aligned;

  template <std::size_t _Np>
    requires(std::has_single_bit(_Np))
    inline constexpr flags<__detail::_Overaligned<_Np>> flag_overaligned;

  // extensions
  template <typename _To>
    inline constexpr flags<__detail::_ConvertTo<_To>> flag_convert_to;

  inline constexpr flags<__detail::_AllowPartialLoadStore> __allow_partial_loadstore;

  inline constexpr flags<__detail::_Throw> __flag_throw;

  inline constexpr flags<__detail::_Streaming> __flag_streaming;

  template <int _L1, int _L2>
    inline constexpr flags<__detail::_Prefetch<_L1, _L2>> __flag_prefetch;

  [[deprecated("use flag_default")]]
  inline constexpr auto element_aligned = flag_default;

  [[deprecated("use flag_aligned")]]
  inline constexpr auto vector_aligned = flag_aligned;

  template <size_t _Np>
    [[deprecated("use flag_overaligned")]]
    inline constexpr auto overaligned = flag_overaligned<_Np>;
} // namespace std

#endif  // PROTOTYPE_FLAGS_H_
