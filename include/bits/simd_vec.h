/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef BITS_SIMD_VEC_H_
#define BITS_SIMD_VEC_H_

#include "simd_details.h"
#include "vec_ops.h"
#ifdef __SSE__
#include "simd_x86.h"
#endif

#include <bit>
#include <bitset>
#include <bits/utility.h>
#include <bits/stl_function.h>
#include <cmath>

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

namespace std::simd
{
  // [simd.traits]
  // --- alignment ---
  template <typename _Tp, typename _Up = typename _Tp::value_type>
    struct alignment
    {};

  template <size_t _Bytes, typename _Abi>
    struct alignment<basic_mask<_Bytes, _Abi>, bool>
    : integral_constant<size_t, alignof(basic_mask<_Bytes, _Abi>)>
    {};

  template <typename _Tp, typename _Abi, __vectorizable _Up>
    struct alignment<basic_vec<_Tp, _Abi>, _Up>
    : integral_constant<size_t, alignof(basic_vec<_Tp, _Abi>)>
    {};

  template <typename _Tp, typename _Up = typename _Tp::value_type>
    constexpr size_t alignment_v = alignment<_Tp, _Up>::value;

  // --- rebind ---
  template <typename _Tp, typename _Vp, _ArchFlags _Traits = {}>
    struct rebind
    {};

  template <__vectorizable _Tp, __simd_vec_type _Vp, _ArchFlags _Traits>
    //requires requires { typename __deduce_abi_t<_Tp, _Vp::size()>; }
    struct rebind<_Tp, _Vp, _Traits>
    {
      using _A1 = decltype(__abi_rebind<_Tp, _Vp::size(), typename _Vp::abi_type>());

      using type = basic_vec<_Tp, _A1>;
    };

  template <__vectorizable _Tp, __simd_mask_type _Mp, _ArchFlags _Traits>
    //requires requires { typename __deduce_abi_t<_Tp, _Mp::size()>; }
    struct rebind<_Tp, _Mp, _Traits>
    {
      using _A1 = decltype(__abi_rebind<_Tp, _Mp::size(), typename _Mp::abi_type>());

      using type = basic_mask<sizeof(_Tp), _A1>;
    };

  template <typename _Tp, typename _Vp>
    using rebind_t = typename rebind<_Tp, _Vp>::type;

  // --- resize ---
  template <__simd_size_type _Np, typename _Vp, _ArchFlags _Traits = {}>
    struct resize
    {};

  template <__simd_size_type _Np, __data_parallel_type _Vp, _ArchFlags _Traits>
    requires requires { typename _Vp::mask_type; }
    //requires requires { typename __deduce_abi_t<typename _Vp::value_type, _Np>; }
    struct resize<_Np, _Vp, _Traits>
    {
      using _A1 = decltype(__abi_rebind<typename _Vp::value_type, _Np, typename _Vp::abi_type>());

      using type = basic_vec<typename _Vp::value_type, _A1>;
    };

  template <__simd_size_type _Np, __simd_mask_type _Mp, _ArchFlags _Traits>
    //requires requires { typename __deduce_abi_t<typename _Mp::value_type, _Np>; }
    struct resize<_Np, _Mp, _Traits>
    {
      using _A1 = decltype(__abi_rebind<__mask_element_size<_Mp>, _Np, typename _Mp::abi_type,
                                        true>());

      using type = basic_mask<__mask_element_size<_Mp>, _A1>;
    };

  template <__simd_size_type _Np, typename _Vp>
    using resize_t = typename resize<_Np, _Vp>::type;

  // [simd.flags] -------------------------------------------------------------
  struct _LoadStoreTag
  {};

  struct __convert_flag
  : _LoadStoreTag
  {};

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
            return static_cast<_Up*>(__builtin_assume_aligned(__ptr, simd::alignment_v<_Tp, _Up>));
          }
      }
  };

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

#if 0
  template <typename _To>
    struct __convert_to_flag
    : _LoadStoreTag
    { using type = _To; };

  struct __partial_loadstore_flag
  : _LoadStoreTag
  {};

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
#endif

  template <typename _Tp>
    concept __loadstore_tag = is_base_of_v<_LoadStoreTag, _Tp>;

  template <typename...>
    struct flags;

  template <typename... _Flags>
    requires (__loadstore_tag<_Flags> and ...)
    struct flags<_Flags...>
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
          if constexpr ((same_as<_Flags, _T0> or ...))
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
          if constexpr ((same_as<_Flags, _T0> or ...))
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
          if constexpr ((same_as<_Flags, _T0> or ...))
            {
              constexpr auto __removed
                = (conditional_t<same_as<_Flags, _T0>, flags<>,
                                      flags<_Flags>>{} | ...);
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
          (_S_apply_adjust_pointer<_Flags, _Tp>(__ptr), ...);
          return __ptr;
        }
    };

  inline constexpr flags<> flag_default {};

  inline constexpr flags<__convert_flag> flag_convert {};

  inline constexpr flags<__aligned_flag> flag_aligned {};

  template <size_t _Np>
    requires(__has_single_bit(_Np))
    inline constexpr flags<__overaligned_flag<_Np>> flag_overaligned {};

#if 0
  // extensions
  template <typename _To>
    inline constexpr flags<__convert_to_flag<_To>> __flag_convert_to {};

  inline constexpr flags<__partial_loadstore_flag> __allow_partial_loadstore {};

  inline constexpr flags<__throw_flag> __flag_throw {};

  inline constexpr flags<__streaming_flag> __flag_streaming {};

  template <int _L1, int _L2>
    inline constexpr flags<__prefetch_flag<_L1, _L2>> __flag_prefetch {};
#endif

  // [simd.syn]
  static constexpr __simd_size_type zero_element   = -1 << (sizeof(int) * __CHAR_BIT__ - 1);

  static constexpr __simd_size_type uninit_element = zero_element + 1;

  // [simd.permute.static]
  template<__simd_size_type _Np = 0, __simd_vec_or_mask_type _Vp,
           __index_permutation_function<_Vp> _IdxMap>
    [[__gnu__::__always_inline__]]
    constexpr resize_t<_Np == 0 ? _Vp::size() : _Np, _Vp>
    permute(const _Vp& __v, _IdxMap&& __idxmap)
    { return resize_t<_Np == 0 ? _Vp::size() : _Np, _Vp>::_S_static_permute(__v, __idxmap); }

  // [simd.permute.dynamic]
  template<__simd_vec_or_mask_type _Vp, __simd_integral _Ip>
    [[__gnu__::__always_inline__]]
    constexpr resize_t<_Ip::size(), _Vp>
    permute(const _Vp& __v, const _Ip& __indices)
    { return __v[__indices]; }

  template <typename _Tp, typename _BinaryOperation>
    inline constexpr _Tp __identity_element_for = nullptr;

  template <typename _Tp>
    inline constexpr _Tp __identity_element_for<_Tp, plus<>> = _Tp(0);

  template <typename _Tp>
    inline constexpr _Tp __identity_element_for<_Tp, multiplies<>> = _Tp(1);

  template <typename _Tp>
    inline constexpr _Tp __identity_element_for<_Tp, bit_and<>> = _Tp(~_Tp());

  template <typename _Tp>
    inline constexpr _Tp __identity_element_for<_Tp, bit_or<>> = _Tp(0);

  template <typename _Tp>
    inline constexpr _Tp __identity_element_for<_Tp, bit_xor<>> = _Tp(0);

  template <typename _Tp, typename _BinaryOperation>
    requires same_as<_BinaryOperation, plus<>>
      or same_as<_BinaryOperation, multiplies<>>
      or same_as<_BinaryOperation, bit_and<>>
      or same_as<_BinaryOperation, bit_or<>>
      or same_as<_BinaryOperation, bit_xor<>>
    consteval _Tp __default_identity_element()
    { return __identity_element_for<_Tp, _BinaryOperation>; }

  // [simd.iterator] ----------------------------------------------------------
  template <typename _Vp>
    class __iterator
    {
      friend class __iterator<const _Vp>;

      template <typename, typename>
        friend class basic_vec;

      template <size_t, typename>
        friend class basic_mask;

      _Vp* _M_data = nullptr;

      __simd_size_type _M_offset = 0;

      constexpr
      __iterator(_Vp& __d, __simd_size_type __off)
      : _M_data(&__d), _M_offset(__off)
      {}

    public:
      using value_type = typename _Vp::value_type;

      using iterator_category = input_iterator_tag;

      using iterator_concept = random_access_iterator_tag;

      using difference_type = __simd_size_type;

      constexpr __iterator() = default;

      constexpr
      __iterator(const __iterator &) = default;

      constexpr __iterator&
      operator=(const __iterator &) = default;

      constexpr
      __iterator(const __iterator<remove_const_t<_Vp>> &__i) requires is_const_v<_Vp>
      : _M_data(__i._M_data), _M_offset(__i._M_offset)
      {}

      constexpr value_type
      operator*() const
      { return (*_M_data)[_M_offset]; } // checked in operator[]

      constexpr __iterator&
      operator++()
      {
        ++_M_offset;
        return *this;
      }

      constexpr __iterator
      operator++(int)
      {
        __iterator r = *this;
        ++_M_offset;
        return r;
      }

      constexpr __iterator&
      operator--()
      {
        --_M_offset;
        return *this;
      }

      constexpr __iterator
      operator--(int)
      {
        __iterator r = *this;
        --_M_offset;
        return r;
      }

      constexpr __iterator&
      operator+=(difference_type __x)
      {
        _M_offset += __x;
        return *this;
      }

      constexpr __iterator&
      operator-=(difference_type __x)
      {
        _M_offset -= __x;
        return *this;
      }

      constexpr value_type
      operator[](difference_type __i) const
      { return (*_M_data)[_M_offset + __i]; } // checked in operator[]

      constexpr friend bool operator==(__iterator __a, __iterator __b) = default;

      constexpr friend bool operator==(__iterator __a, std::default_sentinel_t) noexcept
      { return __a._M_offset == _Vp::size.value; }

      constexpr friend auto operator<=>(__iterator __a, __iterator __b)
      { return __a._M_offset <=> __b._M_offset; }

      constexpr friend __iterator
      operator+(const __iterator& __it, difference_type __x)
      { return __iterator(*__it._M_data, __it._M_offset + __x); }

      constexpr friend __iterator
      operator+(difference_type __x, const __iterator& __it)
      { return __iterator(*__it._M_data, __it._M_offset + __x); }

      constexpr friend __iterator
      operator-(const __iterator& __it, difference_type __x)
      { return __iterator(*__it._M_data, __it._M_offset - __x); }

      constexpr friend difference_type
      operator-(__iterator __a, __iterator __b)
      { return __a._M_offset - __b._M_offset; }

      constexpr friend difference_type
      operator-(__iterator __it, std::default_sentinel_t)
      { return __it._M_offset - difference_type(_Vp::size.value); }

      constexpr friend difference_type
      operator-(std::default_sentinel_t, __iterator __it)
      { return difference_type(_Vp::size.value) - __it._M_offset; }
    };

  // [simd.creation] ----------------------------------------------------------
  template<__simd_vec_type _Vp, typename _Ap>
    constexpr auto
    chunk(const basic_vec<typename _Vp::value_type, _Ap>& __x) noexcept
    { return __x.template _M_chunk<_Vp>(); }

  template<__simd_mask_type _Mp, typename _Ap>
    constexpr auto
    chunk(const basic_mask<__mask_element_size<_Mp>, _Ap>& __x) noexcept
    { return __x.template _M_chunk<_Mp>(); }

  template<__simd_size_type _Np, typename _Tp, typename _Ap>
    constexpr auto
    chunk(const basic_vec<_Tp, _Ap>& __x) noexcept
    { return chunk<resize_t<_Np, basic_vec<_Tp, _Ap>>>(__x); }

  template<__simd_size_type _Np, size_t _Bytes, typename _Ap>
    constexpr auto
    chunk(const basic_mask<_Bytes, _Ap>& __x) noexcept
    { return chunk<resize_t<_Np, basic_mask<_Bytes, _Ap>>>(__x); }

  template<typename _Tp, typename... _Abis>
    constexpr basic_vec<_Tp, __deduce_abi_t<_Tp, (_Abis::_S_size + ...)>>
    cat(const basic_vec<_Tp, _Abis>&... __xs) noexcept
    { return basic_vec<_Tp, __deduce_abi_t<_Tp, (_Abis::_S_size + ...)>>::_S_concat(__xs...); }

  template<size_t _Bytes, typename... _Abis>
    constexpr basic_mask<_Bytes, __deduce_abi_t<__integer_from<_Bytes>,
                                                (basic_mask<_Bytes, _Abis>::size() + ...)>>
    cat(const basic_mask<_Bytes, _Abis>&... __xs) noexcept
    { static_assert(false, "TODO: cat"); }

  // [simd.mask] --------------------------------------------------------------
  template <size_t _Bytes, typename _Abi>
    class basic_mask
    {
    public:
      using value_type = bool;

      using abi_type = _Abi;

#define _GLIBCXX_DELETE_SIMD                                                                    \
      _GLIBCXX_DELETE_MSG("This specialization is disabled because of an invalid combination "  \
          "of template arguments to basic_mask.")

      basic_mask() = _GLIBCXX_DELETE_SIMD;

      ~basic_mask() = _GLIBCXX_DELETE_SIMD;

      basic_mask(const basic_mask&) = _GLIBCXX_DELETE_SIMD;

      basic_mask& operator=(const basic_mask&) = _GLIBCXX_DELETE_SIMD;

#undef _GLIBCXX_DELETE_SIMD
    };

  template <size_t _Bytes, __abi_tag _Ap>
    requires (_Ap::_S_nreg == 1)
    class basic_mask<_Bytes, _Ap>
    {
      template <size_t, typename>
        friend class basic_mask;

      template <typename, typename>
        friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr bool _S_is_scalar = is_same_v<_Ap, _ScalarAbi<_Ap::_S_size>>;

      static constexpr bool _S_is_cx_ileav = __flags_test(_Ap::_S_variant, _AbiVariant::_CxIleav);

      static constexpr bool _S_use_bitmask = [] {
        if constexpr (_S_is_scalar)
          return false;
        else
          return __flags_test(_Ap::_S_variant, _AbiVariant::_BitMask);
      }();

      static constexpr int _S_full_size = [] {
        if constexpr (_S_is_scalar)
          return _S_size;
        else if constexpr (_S_use_bitmask)
          return __div_ceil(_S_size, __CHAR_BIT__) * __CHAR_BIT__;
        else
          return __bit_ceil(unsigned(_S_size));
      }();

      static constexpr bool _S_is_partial = _S_size != _S_full_size;

      using _DataType = typename _Ap::template _MaskDataType<_Bytes>;

      static constexpr bool _S_use_2_for_1 = _Ap::_S_use_2_for_1;

      static constexpr _DataType _S_implicit_mask = [] {
        constexpr int __n = _S_use_2_for_1 ? _S_size * 2 : _S_size;
        if constexpr (_S_is_scalar)
          return true;
        else if (not _S_is_partial)
          return _DataType(~_DataType());
        else if constexpr (_S_use_bitmask)
          return _DataType((_DataType(1) << __n) - 1);
        else
          return []<int... _Is> (integer_sequence<int, _Is...>) {
            return _DataType{ (_Is < __n ? -1 : 0)... };
          }(make_integer_sequence<int, _S_full_size>());
      }();

      using _VecType = basic_vec<__integer_from<_Bytes>,
                                 decltype(__abi_rebind<__integer_from<_Bytes>, _S_size, _Ap>())>;

      static constexpr bool _S_has_bool_member = _S_is_scalar;

      // Actual padding bytes, not padding elements.
      // => _S_padding_bytes is 0 even if _S_is_partial is true.
      static constexpr size_t _S_padding_bytes = 0;

      _DataType _M_data;

    public:
      using value_type = bool;

      using abi_type = _Ap;

      using iterator = __iterator<basic_mask>;

      using const_iterator = __iterator<const basic_mask>;

      constexpr iterator
      begin() noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      begin() const noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      cbegin() const noexcept
      { return {*this, 0}; }

      constexpr default_sentinel_t
      end() const noexcept
      { return {}; }

      constexpr default_sentinel_t
      cend() const noexcept
      { return {}; }

      static constexpr auto size = __simd_size_constant<_S_size>;

      // internal but public API ----------------------------------------------
      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_init(_DataType __x)
      {
        basic_mask __r;
        __r._M_data = __x;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr _DataType
      _M_concat_data() const
      {
        if constexpr (_S_is_partial)
          return _M_data & _S_implicit_mask;
        else
          return _M_data;
      }

      template <_ArchFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        static constexpr basic_mask
        _S_partial_mask_of_n(int __n)
        {
          // assume __n > 0 and __n < _S_size
          static_assert(not _S_is_scalar);
          if constexpr (not _S_use_bitmask)
            return _VecType([&](__integer_from<_Bytes> __i) { return __i; })
                     < __integer_from<_Bytes>(__n);
          else
            {
#if __has_builtin(__builtin_ia32_bzhi_si)
              if constexpr (_S_size <= 32 and _Flags._M_have_bmi2())
                return __builtin_ia32_bzhi_si(~0u >> (32 - _S_size), unsigned(__n));
#endif
#if __has_builtin(__builtin_ia32_bzhi_di)
              if constexpr (_S_size <= 64 and _Flags._M_have_bmi2())
                return __builtin_ia32_bzhi_di(~0ull >> (64 - _S_size), unsigned(__n));
#endif
              if constexpr (_S_size <= 32)
                return (1u << unsigned(__n)) - 1;
              else if constexpr (_S_size <= 64)
                return (1ull << unsigned(__n)) - 1;
              else
                static_assert(false);
            }
        }

      // If this basic_mask specialization is used for _CxIleav vecs then it's actually made up of
      // twice as many (with half the number of Bytes) elements. By construction, the _S_nreg == 1
      // specializations can use a mask register with twice the elements still with _S_nreg == 1.
      using _CxElementMask
        = basic_mask<_Bytes / 2,
                     decltype([] consteval {
                       if constexpr (not __flags_test(_Ap::_S_variant, _AbiVariant::_CxIleav))
                         return _InvalidAbi();
                       else if constexpr (is_same_v<_Ap, _ScalarAbi<_S_size>>)
                         return _ScalarAbi<_S_size * 2>();
                       else // _Ap is _Abi<_S_size, 1, Something>
                         return _Abi<_S_size * 2, 1, __flags_and(_Ap::_S_variant,
                                                                 _AbiVariant::_MaskVariants)>();
                     }())>;

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_and_neighbors(const _CxElementMask& __k)
      {
        static_assert(__flags_test(_Ap::_S_variant, _AbiVariant::_CxIleav));
        static_assert(not __flags_test(_CxElementMask::abi_type::_S_variant,
                                       _AbiVariant::_CxIleav));
        basic_mask __r;
        if constexpr (_S_use_bitmask)
          __r._M_data = __k._M_data & (((__k._M_data >> 1) & 0x5555'5555'5555'5555ull)
                                         | ((__k._M_data << 1) & ~0x5555'5555'5555'5555ull));
        else
          __r._M_data = __vec_and(__k._M_data, _VecOps<_DataType>::_S_swap_neighbors(__k._M_data));
        return __r;
      }

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_or_neighbors(const _CxElementMask& __k)
      {
        static_assert(__flags_test(_Ap::_S_variant, _AbiVariant::_CxIleav));
        static_assert(not __flags_test(_CxElementMask::abi_type::_S_variant,
                                       _AbiVariant::_CxIleav));
        basic_mask __r;
        if constexpr (_S_use_bitmask)
          __r._M_data = __k._M_data | (((__k._M_data >> 1) & 0x5555'5555'5555'5555ull)
                                         | ((__k._M_data << 1) & ~0x5555'5555'5555'5555ull));
        else
          __r._M_data = __vec_or(__k._M_data, _VecOps<_DataType>::_S_swap_neighbors(__k._M_data));
        return __r;
      }

      template <typename _Mp>
        [[__gnu__::__always_inline__]]
        constexpr auto _M_chunk() const noexcept
        {
          constexpr int __n = _S_size / _Mp::_S_size;
          constexpr int __rem = _S_size % _Mp::_S_size;
          constexpr int __stride = _S_use_2_for_1 ? 2 * _Mp::_S_size : _Mp::_S_size;
          if constexpr (_S_is_scalar)
            {
              if constexpr (__n == 0)
                return array<_Mp, 1> {*this};
              else
                return tuple<basic_mask> {*this};
            }
          else if constexpr (_S_use_bitmask != _Mp::_S_use_bitmask)
            // convert to whatever _Mp uses first and then recurse into _M_chunk
            return resize_t<_S_size, _Mp>(*this).template _M_chunk<_Mp>();
          else if constexpr (_S_use_bitmask and _Mp::_S_use_bitmask)
            {
              static_assert(is_unsigned_v<_DataType>);
              auto __bits = [] [[__gnu__::__always_inline__]](_DataType __bits) {
                if constexpr (_S_use_2_for_1 == _Mp::_S_use_2_for_1)
                  return __bits;
                else if constexpr (_S_use_2_for_1)
                  // compress two adjacent bits into one
                  return __bit_extract_even<_S_size>(__bits);
                else
                  // duplicate every bit into two
                  return __duplicate_each_bit<_S_size>(__bits);
              }(_M_data);
              if constexpr (__rem == 0)
                return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                         return array<_Mp, __n> {_Mp::_S_init(__bits >> (_Is * __stride))...};
                       });
              else
                {
                  using _Rest = resize_t<__rem, _Mp>;
                  return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                           return tuple {_Mp::_S_init(__bits >> (_Is * __stride))...,
                                         _Rest::_S_init([&] [[__gnu__::__always_inline__]]() {
                                           if constexpr (is_same_v<typename _Rest::_DataType, bool>)
                                             return operator[](__n * _Mp::_S_size);
                                           else
                                             return __bits >> (__n * __stride);
                                         }())};
                         });
                }
            }
          else if constexpr (__rem == 0)
            {
              if constexpr (_Mp::_S_size == 1)
                return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                         return array<_Mp, __n> {_Mp(operator[](_Is))...};
                       });
              else
                return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                         static_assert(is_same_v<__vec_value_type<typename _Mp::_DataType>,
                                                 __vec_value_type<_DataType>>);
                         return array<_Mp, __n> {
                           _Mp::_S_init(
                             _VecOps<typename _Mp::_DataType>::_S_extract(
                               _M_data, integral_constant<int, _Is * __stride>()))...};
                       });
            }
          else
            {
              using _Rest = resize_t<__rem, _Mp>;
              return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                       return tuple {
                         _Mp::_S_init(
                           _VecOps<typename _Mp::_DataType>::_S_extract(
                             _M_data, integral_constant<int, _Is * __stride>()))...,
                         _Rest::_S_init([&] [[__gnu__::__always_inline__]]() {
                           if constexpr (is_same_v<typename _Rest::_DataType, bool>)
                             return operator[](__n * _Mp::_S_size);
                           else
                             return _VecOps<typename _Rest::_DataType>::_S_extract(
                                      _M_data, integral_constant<int, __n * __stride>());
                         }())
                       };
                     });
            }
        }

      // [simd.mask.overview] default constructor -----------------------------
      basic_mask() = default;

      // [simd.mask.overview] conversion extensions ---------------------------
      [[__gnu__::__always_inline__]]
      constexpr
      basic_mask(_DataType __x) requires(not _S_is_scalar and not _S_use_bitmask)
        : _M_data(__x)
      {}

      [[__gnu__::__always_inline__]]
      constexpr
      operator _DataType() requires(not _S_is_scalar and not _S_use_bitmask)
      { return _M_data; }

      // [simd.mask.ctor] broadcast constructor -------------------------------
      [[__gnu__::__always_inline__]]
      constexpr explicit
      basic_mask(bool __x) noexcept
        : _M_data(__x ? _S_implicit_mask : _DataType())
      {}

      // [simd.mask.ctor] conversion constructor ------------------------------
      template <size_t _UBytes, typename _UAbi>
        requires (_S_size == _UAbi::_S_size)
        [[__gnu__::__always_inline__]]
        constexpr explicit(__is_mask_conversion_explicit<_Ap, _UAbi>(_Bytes, _UBytes))
        basic_mask(const basic_mask<_UBytes, _UAbi>& __x) noexcept
          : _M_data([&] {
              using _UV = basic_mask<_UBytes, _UAbi>;
              if constexpr (_S_is_scalar)
                return __x[0];
              else if constexpr (_S_use_bitmask and _UV::_S_use_bitmask)
                {
                  if constexpr (_S_use_2_for_1 == _UV::_S_use_2_for_1)
                    return __x._M_concat_data();
                  else if constexpr (_S_use_2_for_1)
                    return __duplicate_each_bit<_S_size>(__x._M_concat_data());
                  else if constexpr (_S_size > 32)
                    { // __x._M_concat_data() would need uint with more than 64 bits
                      static_assert(_UAbi::_S_nreg > 1);
                      static_assert(__x._M_data0._S_size == 32);
                      static_assert(is_same_v<decltype(__x._M_data0._M_concat_data()),
                                              _UInt<8>>);
                      _UInt<8> __lo = __bit_extract_even<32>(__x._M_data0._M_concat_data());
                      _UInt<8> __hi = __bit_extract_even<_S_size - 32>(__x._M_data1
                                                                         ._M_concat_data());
                      return __lo | (__hi << 32);
                    }
                  else
                    return __bit_extract_even<_S_size>(__x._M_concat_data());
                }
              else if constexpr (_S_use_bitmask and _S_use_2_for_1)
                return _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                         return ((__x[_Is] ? (3ull << (_Is * 2)) : 0ull) | ...);
                       });
              else if constexpr (_S_use_bitmask)
                return _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                         return ((__x[_Is] ? (1ull << _Is) : 0ull) | ...);
                       });
              else if constexpr (sizeof(__x) == sizeof(_M_data) and _Bytes == _UBytes
                                   and not _S_has_bool_member and not _UV::_S_has_bool_member
                                   and not _UV::_S_use_bitmask and _UV::_S_padding_bytes == 0)
                return __builtin_bit_cast(_DataType, __x);
              else if (not __builtin_is_constant_evaluated()
                         and sizeof(__x) == sizeof(_M_data) and _Bytes == _UBytes
                         and not _S_has_bool_member and not _UV::_S_has_bool_member
                         and not _UV::_S_use_bitmask and _UV::_S_padding_bytes != 0)
                {
                  _DataType __tmp = {};
                  __builtin_memcpy(&__tmp, &__x, sizeof(__x) - _UV::_S_padding_bytes);
                  return __tmp;
                }
              else if constexpr (_S_use_2_for_1)
                return _GLIBCXX_SIMD_INT_PACK(_S_size * 2, _Is, {
                         return _DataType{__vec_value_type<_DataType>(__x[_Is / 2] ? -1 : 0)...};
                       });
              else
                return _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                         return _DataType{__vec_value_type<_DataType>(__x[_Is] ? -1 : 0)...};
                       });
              // TODO: optimize the above using __builtin_convertvector (or better)
          }())
        {}

      // [simd.mask.ctor] generator constructor -------------------------------
      template <__simd_generator_invokable<bool, _S_size> _Fp>
        [[__gnu__::__always_inline__]]
        constexpr explicit
        basic_mask(_Fp&& __gen)
          : _M_data(_GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                      if constexpr (_S_is_scalar)
                        return __gen(__simd_size_constant<0>);
                      else if constexpr (_S_use_bitmask)
                        return ((_DataType(__gen(__simd_size_constant<_Is>)) << _Is) | ...);
                      else if constexpr (_S_use_2_for_1)
                        { // for _CxIleav, the results of each __gen call need to initialize two
                          // neighboring elements
                          bool __tmp[_S_size] = {__gen(__simd_size_constant<_Is>)...};
                          return _GLIBCXX_SIMD_INT_PACK(_S_size * 2, _Js, {
                                   return _DataType{__vec_value_type<_DataType>(
                                                        -__tmp[_Js / 2])...};
                                 });
                        }
                      else
                        return _DataType{__vec_value_type<_DataType>(
                                           __gen(__simd_size_constant<_Is>) ? -1 : 0)...};
                    }))
        {}

      template <__almost_simd_generator_invokable<bool, _S_size> _Fp>
        constexpr explicit
        basic_mask(_Fp&&)
          = _GLIBCXX_DELETE_MSG("Invalid return type of the mask generator function: "
                                "Needs to be 'bool'.");

      // [simd.mask.ctor] TODO: bitset constructor ----------------------------------
      // [simd.mask.ctor] TODO: uint constructor ------------------------------------

      // [simd.mask.subscr] ---------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      {
        if constexpr (_S_use_2_for_1)
          __i *= 2;

        if constexpr (_S_is_scalar)
          return _M_data;
        else if constexpr (_S_use_bitmask)
          return bool((_M_data >> __i) & 1);
        else
          return _M_data[__i] != 0;
      }

      // [simd.mask.unary] ----------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr basic_mask
      operator!() const noexcept
      {
        if constexpr (_S_is_scalar)
          return _S_init(!_M_data);
        else
          return _S_init(~_M_data);
      }

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator+() const noexcept
      { return operator _VecType(); }

      constexpr _VecType
      operator+() const noexcept requires (_S_use_2_for_1) = delete;

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator-() const noexcept
      {
        using _Ip = typename _VecType::value_type;
        if constexpr (_S_is_scalar)
          return -_Ip(_M_data);
        else if constexpr (_S_use_bitmask)
          return __select_impl(*this, _Ip(-1), _Ip());
        else
          {
            static_assert(sizeof(_VecType) == sizeof(_M_data));
            return __builtin_bit_cast(_VecType, _M_data);
          }
      }

      constexpr _VecType
      operator-() const noexcept requires (_S_use_2_for_1) = delete;

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator~() const noexcept
      {
        using _Ip = typename _VecType::value_type;
        if constexpr (_S_is_scalar)
          return ~_Ip(_M_data);
        else if constexpr (_S_use_bitmask)
          return __select_impl(*this, _Ip(-2), _Ip(-1));
        else
          {
            static_assert(sizeof(_VecType) == sizeof(_M_data));
            return __builtin_bit_cast(_VecType, _M_data) - _Ip(1);
          }
      }

      constexpr _VecType
      operator~() const noexcept requires (_S_use_2_for_1) = delete;

      // [simd.mask.conv] -----------------------------------------------------
      template <typename _Up, typename _UAbi>
        requires (__simd_size_v<_Up, _UAbi> == _S_size)
        [[__gnu__::__always_inline__]]
        constexpr explicit(sizeof(_Up) != _Bytes)
        operator basic_vec<_Up, _UAbi>() const noexcept
        {
          using _Rp = basic_vec<_Up, _UAbi>;
          if constexpr (_S_is_scalar)
            return _Up(_M_data);
          else if constexpr (_S_use_bitmask)
            return __select_impl(*this, _Up(1), _Up(0));
          else
            {
              static_assert(sizeof(_M_data) == sizeof(_Rp));
              return __builtin_bit_cast(_Rp, _M_data) & _Up(1);
            }
        }

      // [simd.mask.namedconv] ------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr bitset<_S_size>
      to_bitset() const noexcept;

      [[__gnu__::__always_inline__]]
      constexpr _UInt<__bit_ceil(unsigned(__div_ceil(_S_size, __CHAR_BIT__)))>
      _M_to_uint() const
      {
        if constexpr (_S_use_bitmask and _S_is_partial)
          return _M_data & _S_implicit_mask;
        else if constexpr (_S_is_scalar or _S_use_bitmask)
          return _M_data;
        else
          {
            using _Up = _UInt<__bit_ceil(unsigned(__div_ceil(_S_size, __CHAR_BIT__)))>;
            // TODO: with SSE/AVX use movemask instructions
            // TODO: with BMI2 use pext instruction (__builtin_ia32_pext_[sd]i)
            // TODO: with AVX512 use __builtin_ia32_cvt[bwdq]2mask(128|256|512)
            // TODO: Ask for compiler builtin to do the best of the above. This should also combine
            // with a preceding vector-mask compare to produce a bit-mask compare (on AVX512)
            return _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                     return ((static_cast<_Up>(-_M_data[_Is]) << _Is) | ...);
                   });
          }
      }

      [[__gnu__::__always_inline__]]
      constexpr unsigned long long
      to_ullong() const
      { return _M_to_uint(); }

      // [simd.mask.binary] ---------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator&&(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data & __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator||(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data | __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator&(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data & __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator|(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data | __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator^(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data ^ __y._M_data); }

      // [simd.mask.cassign] --------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask&
      operator&=(basic_mask& __x, const basic_mask& __y) noexcept
      {
        __x._M_data &= __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask&
      operator|=(basic_mask& __x, const basic_mask& __y) noexcept
      {
        __x._M_data |= __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask&
      operator^=(basic_mask& __x, const basic_mask& __y) noexcept
      {
        __x._M_data ^= __y._M_data;
        return __x;
      }

      // [simd.mask.comparison] -----------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator==(const basic_mask& __x, const basic_mask& __y) noexcept
      { return !(__x ^ __y); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator!=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return __x ^ __y; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator>=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return __x || !__y; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator<=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return !__x || __y; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator>(const basic_mask& __x, const basic_mask& __y) noexcept
      { return __x && !__y; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator<(const basic_mask& __x, const basic_mask& __y) noexcept
      { return !__x && __y; }

      // [simd.mask.cond] -----------------------------------------------------
      template <__vectorizable _T0, same_as<_T0> _T1>
        requires (sizeof(_T0) == _Bytes)
        [[__gnu__::__always_inline__]]
        friend constexpr vec<_T0, _S_size>
        __select_impl(const basic_mask& __k, const _T0& __t, const _T1& __f)
        {
          if constexpr (not _S_use_bitmask)
            return __k._M_data ? __t : __f;
          else
            return __select_impl(__k, vec<_T0, _S_size>(__t), vec<_T1, _S_size>(__f));
        }

      // [simd.mask.reductions] implementation --------------------------------
      template <_ArchFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        constexpr bool
        _M_all_of() const noexcept
        {
          if constexpr (_S_is_scalar)
            return _M_data;
          else if constexpr (_S_use_bitmask)
            {
              if constexpr (_S_is_partial)
                // PR120925 (partial kortest pattern not recognized)
                return (_M_data & _S_implicit_mask) == _S_implicit_mask;
              else
                return _M_data == _S_implicit_mask;
            }
#if _GLIBCXX_SIMD_HAVE_SSE
          else if (not (__builtin_is_constant_evaluated() or __builtin_constant_p(_M_data)))
            return __x86_vecmask_all<_S_size>(_M_data);
#endif
          else
            return _VecOps<_DataType, _S_size>::_S_all_of(_M_data);
        }

      template <_ArchFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        constexpr bool
        _M_any_of() const noexcept
        {
          if constexpr (_S_is_scalar)
            return _M_data;
          else if constexpr (_S_use_bitmask)
            {
              if constexpr (_S_is_partial)
                // PR120925 (partial kortest pattern not recognized)
                return (_M_data & _S_implicit_mask) != 0;
              else
                return _M_data != 0;
            }
#if _GLIBCXX_SIMD_HAVE_SSE
          else if (not (__builtin_is_constant_evaluated() or __builtin_constant_p(_M_data)))
            return __x86_vecmask_any<_S_size>(_M_data);
#endif
          else
            return _VecOps<_DataType, _S_size>::_S_any_of(_M_data);
        }

      template <_ArchFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        constexpr bool
        _M_none_of() const noexcept
        {
          if constexpr (_S_is_scalar)
            return not _M_data;
          else if constexpr (_S_use_bitmask)
            {
              if constexpr (_S_is_partial)
                // PR120925 (partial kortest pattern not recognized)
                return (_M_data & _S_implicit_mask) == 0;
              else
                return _M_data == 0;
            }
#if _GLIBCXX_SIMD_HAVE_SSE
          else if (not (__builtin_is_constant_evaluated() or __builtin_constant_p(_M_data)))
            return __x86_vecmask_none<_S_size>(_M_data);
#endif
          else
            return _VecOps<_DataType, _S_size>::_S_none_of(_M_data);
        }

      [[__gnu__::__always_inline__]]
      constexpr __simd_size_type
      _M_reduce_count() const noexcept
      {
        if constexpr (_S_is_scalar)
          return int(_M_data);
        else if constexpr (_S_size <= sizeof(int) * __CHAR_BIT__)
          return __builtin_popcount(_M_to_uint());
        else
          return __builtin_popcountll(to_ullong());
      }

      [[__gnu__::__always_inline__]]
      constexpr __simd_size_type
      _M_reduce_min_index() const
      {
        if constexpr (_S_size == 1)
          return 0;
        else
          return __lowest_bit(_M_to_uint());
      }

      [[__gnu__::__always_inline__]]
      constexpr __simd_size_type
      _M_reduce_max_index() const
      {
        if constexpr (_S_size == 1)
          return 0;
        else
          return __highest_bit(_M_to_uint());
      }

      [[__gnu__::__always_inline__]]
      bool
      _M_is_constprop() const
      { return __builtin_constant_p(_M_data); }
    };

  template <size_t _Bytes, __abi_tag _Ap>
    requires (_Ap::_S_nreg > 1)
    class basic_mask<_Bytes, _Ap>
    {
      template <size_t, typename>
        friend class basic_mask;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr bool _S_is_cx_ileav = __flags_test(_Ap::_S_variant, _AbiVariant::_CxIleav);

      static constexpr int _N0 = __bit_ceil(unsigned(_S_size)) / 2;

      static constexpr int _N1 = _S_size - _N0;

      using _Mask0 = basic_mask<_Bytes, decltype(__abi_rebind<_Bytes, _N0, _Ap, true>())>;

      using _Mask1 = basic_mask<_Bytes, decltype(__abi_rebind<_Bytes, _N1, _Ap, true>())>;

      static constexpr bool _S_use_bitmask = _Mask0::_S_use_bitmask;

      static constexpr bool _S_use_2_for_1 = _Mask0::_S_use_2_for_1;

      _Mask0 _M_data0;

      _Mask1 _M_data1;

      using _VecType = basic_vec<__integer_from<_Bytes>,
                                 decltype(__abi_rebind<__integer_from<_Bytes>, _S_size, _Ap>())>;

      static constexpr bool _S_has_bool_member = _Mask1::_S_has_bool_member;

      static constexpr size_t _S_padding_bytes
        = __alignof__(_Mask0) - sizeof(_Mask1) + _Mask1::_S_padding_bytes;

    public:
      using value_type = bool;

      using abi_type = _Ap;

      using iterator = __iterator<basic_mask>;

      using const_iterator = __iterator<const basic_mask>;

      constexpr iterator
      begin() noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      begin() const noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      cbegin() const noexcept
      { return {*this, 0}; }

      constexpr default_sentinel_t
      end() const noexcept
      { return {}; }

      constexpr default_sentinel_t
      cend() const noexcept
      { return {}; }

      static constexpr auto size = __simd_size_constant<_S_size>;

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_init(const _Mask0& __x, const _Mask1& __y)
      {
        basic_mask __r;
        __r._M_data0 = __x;
        __r._M_data1 = __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_concat_data() const
      {
        if constexpr (_S_use_bitmask)
          {
            constexpr int __n = _S_use_2_for_1 ? _S_size * 2 : _S_size;
            static_assert(__n <= sizeof(0ull) * __CHAR_BIT__, "cannot concat more than 64 bits");
            using _Up = _UInt<__bit_ceil(__div_ceil<unsigned>(__n, __CHAR_BIT__))>;
            return _Up(_M_data0._M_concat_data() | (_Up(_M_data1._M_concat_data())
                                                      << (_N0 * (_S_use_2_for_1 + 1))));
          }
        else
          return __vec_concat(_M_data0._M_concat_data(),
                              __vec_zero_pad_to<sizeof(_M_data0)>(_M_data1._M_concat_data()));
      }

      template <_ArchFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        static constexpr basic_mask
        _S_partial_mask_of_n(int __n)
        {
#if __has_builtin(__builtin_ia32_bzhi_di)
          if constexpr (_S_use_bitmask and _S_size <= 64 and _Flags._M_have_bmi2())
            return __builtin_ia32_bzhi_di(~0ull >> (64 - _S_size), unsigned(__n));
#endif
          if (__n < _N0)
            return _S_init(_Mask0::_S_partial_mask_of_n(__n), _Mask1(false));
          else if (__n == _N0)
            return _S_init(_Mask0(true), _Mask1(false));
          else
            return _S_init(_Mask0(true), _Mask1::_S_partial_mask_of_n(__n - _N0));
        }

      using _CxElementMask
        = basic_mask<_Bytes / 2, decltype(__abi_rebind<_Bytes / 2, _S_size * 2, _Ap, false>())>;

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_and_neighbors(const _CxElementMask& __k)
      {
        static_assert(__flags_test(_Ap::_S_variant, _AbiVariant::_CxIleav));
        static_assert(not __flags_test(_CxElementMask::abi_type::_S_variant,
                                       _AbiVariant::_CxIleav));
        return _S_init(_Mask0::_S_and_neighbors(__k._M_data0),
                       _Mask1::_S_and_neighbors(__k._M_data1));
      }

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_or_neighbors(const _CxElementMask& __k)
      {
        static_assert(__flags_test(_Ap::_S_variant, _AbiVariant::_CxIleav));
        static_assert(not __flags_test(_CxElementMask::abi_type::_S_variant,
                                       _AbiVariant::_CxIleav));
        return _S_init(_Mask0::_S_or_neighbors(__k._M_data0),
                       _Mask1::_S_or_neighbors(__k._M_data1));
      }

      template <typename _Mp>
        [[__gnu__::__always_inline__]]
        constexpr auto
        _M_chunk() const noexcept
        {
          constexpr int __n = _S_size / _Mp::_S_size;
          constexpr int __rem = _S_size % _Mp::_S_size;
          if constexpr (_N0 == _Mp::_S_size)
            {
              if constexpr (__rem == 0)
                return array<_Mp, __n> {_M_data0, _M_data1};
              else
                return tuple<_Mp, resize_t<__rem, _Mp>> {_M_data0, _M_data1};
            }
          else if constexpr (__rem == 0)
            {
              using _Rp = array<_Mp, __n>;
              if constexpr (sizeof(_Rp) == sizeof(*this))
                {
                  static_assert(not _Mp::_S_is_partial);
                  return __builtin_bit_cast(_Rp, *this);
                }
              else
                {
                  return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                           return _Rp {_Mp([&](int __i) {
                                         return (*this)[__i + _Is * _Mp::_S_size];
                                       })...};
                         });
                }
            }
          else
            return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                     using _Rest = resize_t<__rem, _Mp>;
                     // can't bit-cast because the member order of tuple is reversed
                     return tuple {
                       _Mp  ([&](int __i) { return (*this)[__i + _Is * _Mp::_S_size]; })...,
                       _Rest([&](int __i) { return (*this)[__i + __n * _Mp::_S_size]; })
                     };
            });
        }

      // [simd.mask.overview] default constructor -----------------------------
      basic_mask() = default;

      // [simd.mask.overview] conversion extensions ---------------------------
      // TODO: any?

      // [simd.mask.ctor] broadcast constructor -------------------------------
      [[__gnu__::__always_inline__]]
      constexpr explicit
      basic_mask(bool __x) noexcept
        : _M_data0(__x), _M_data1(__x)
      {}

      // [simd.mask.ctor] conversion constructor ------------------------------
      template <size_t _UBytes, typename _UAbi>
        requires (_S_size == _UAbi::_S_size)
        [[__gnu__::__always_inline__]]
        constexpr explicit(__is_mask_conversion_explicit<_Ap, _UAbi>(_Bytes, _UBytes))
        basic_mask(const basic_mask<_UBytes, _UAbi>& __x) noexcept
          : _M_data0([&] {
              if constexpr (_UAbi::_S_nreg > 1)
                return __x._M_data0;
              else
                return get<0>(chunk<_N0>(__x));
            }()), _M_data1([&] {
              if constexpr (_UAbi::_S_nreg > 1)
                return __x._M_data1;
              else
                return get<1>(chunk<_N0>(__x));
            }())
        {}

      // [simd.mask.ctor] generator constructor -------------------------------
      template <__simd_generator_invokable<bool, _S_size> _Fp>
        [[__gnu__::__always_inline__]]
        constexpr explicit
        basic_mask(_Fp&& __gen)
          : _M_data0(__gen), _M_data1([&] [[__gnu__::__always_inline__]] (auto __i) {
                               return __gen(__simd_size_constant<__i + _N0>);
                             })
        {}

      template <__almost_simd_generator_invokable<bool, _S_size> _Fp>
        constexpr explicit
        basic_mask(_Fp&&)
          = _GLIBCXX_DELETE_MSG("Invalid return type of the mask generator function: "
                                "Needs to be 'bool'.");

      // [simd.mask.ctor] TODO: bitset constructor ----------------------------------
      // [simd.mask.ctor] TODO: uint constructor ------------------------------------

      // [simd.mask.subscr] ---------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      {
        // in some cases the last element can be 'bool' instead of bit-/vector-mask;
        // e.g. mask<short, 17> is {mask<short, 16>, mask<short, 1>}, where the latter uses
        // _ScalarAbi<1>, which is stored as 'bool'
        if constexpr (_M_data1._S_has_bool_member)
          {
            if (__i < _N0)
              return _M_data0[__i];
            else
              return _M_data1[__i - _N0];
          }
        else if constexpr (__flags_test(abi_type::_S_variant, _AbiVariant::_CxIleav))
          {
            // values are duplicated
            if constexpr (__flags_test(abi_type::_S_variant, _AbiVariant::_BitMask))
              {
                struct _Tmp
                {
                  alignas(basic_mask) unsigned char _M_bytes[__div_ceil(2 * _S_size, __CHAR_BIT__)];
                };
                return bool((__builtin_bit_cast(_Tmp, *this)
                               ._M_bytes[2 * __i / __CHAR_BIT__] >> (2 * __i % __CHAR_BIT__)) & 1);
              }
            else
              {
                struct _Tmp
                {
                  alignas(basic_mask) __integer_from<_Bytes / 2> _M_values[2 * _S_size];
                };
                return __builtin_bit_cast(_Tmp, *this)._M_values[2 * __i] != 0;
              }
          }
        else if constexpr (__flags_test(abi_type::_S_variant, _AbiVariant::_BitMask))
          {
            struct _Tmp
            {
              alignas(basic_mask) unsigned char _M_bytes[__div_ceil(_S_size, __CHAR_BIT__)];
            };
            return bool((__builtin_bit_cast(_Tmp, *this)
                           ._M_bytes[__i / __CHAR_BIT__] >> (__i % __CHAR_BIT__)) & 1);
          }
        else
          {
            struct _Tmp
            {
              alignas(basic_mask) __integer_from<_Bytes> _M_values[_S_size];
            };
            return __builtin_bit_cast(_Tmp, *this)._M_values[__i] != 0;
          }
      }

      // [simd.mask.unary] ----------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr basic_mask
      operator!() const noexcept
      { return _S_init(!_M_data0, !_M_data1); }

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator+() const noexcept
      { return _VecType::_S_init(+_M_data0, +_M_data1); }

      constexpr _VecType
      operator+() const noexcept requires (_S_use_2_for_1) = delete;

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator-() const noexcept
      { return _VecType::_S_init(-_M_data0, -_M_data1); }

      constexpr _VecType
      operator-() const noexcept requires (_S_use_2_for_1) = delete;

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator~() const noexcept
      { return _VecType::_S_init(~_M_data0, ~_M_data1); }

      constexpr _VecType
      operator~() const noexcept requires (_S_use_2_for_1) = delete;

      // [simd.mask.conv] -----------------------------------------------------
      template <typename _Up, typename _UAbi>
        requires (__simd_size_v<_Up, _UAbi> == _S_size)
        [[__gnu__::__always_inline__]]
        constexpr explicit(sizeof(_Up) != _Bytes)
        operator basic_vec<_Up, _UAbi>() const noexcept
        {
          using _Rp = basic_vec<_Up, _UAbi>;
          return _Rp::_S_init(_M_data0, _M_data1);
        }

      // [simd.mask.namedconv] ------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr bitset<_S_size>
      to_bitset() const noexcept;

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_to_uint() const requires (_S_size <= __CHAR_BIT__ * sizeof(0ull))
      {
        using _Up = _UInt<__bit_ceil(unsigned(__div_ceil(_S_size, __CHAR_BIT__)))>;
        return _Up(_M_data0._M_to_uint() | (_Up(_M_data1._M_to_uint()) << _N0));
      }

      [[__gnu__::__always_inline__]]
      constexpr unsigned long long
      to_ullong() const
      {
        if constexpr (_N0 < 64)
          return _M_data0.to_ullong() | (_M_data1.to_ullong() << _N0);
        else
          return _M_data0.to_ullong();
      }

      // [simd.mask.binary]
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator&&(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data0 && __y._M_data0, __x._M_data1 && __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator||(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data0 || __y._M_data0, __x._M_data1 || __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator&(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data0 & __y._M_data0, __x._M_data1 & __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator|(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data0 | __y._M_data0, __x._M_data1 | __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator^(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data0 ^ __y._M_data0, __x._M_data1 ^ __y._M_data1); }

      // [simd.mask.cassign]
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask&
      operator&=(basic_mask& __x, const basic_mask& __y) noexcept
      {
        __x._M_data0 &= __y._M_data0;
        __x._M_data1 &= __y._M_data1;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask&
      operator|=(basic_mask& __x, const basic_mask& __y) noexcept
      {
        __x._M_data0 |= __y._M_data0;
        __x._M_data1 |= __y._M_data1;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask&
      operator^=(basic_mask& __x, const basic_mask& __y) noexcept
      {
        __x._M_data0 ^= __y._M_data0;
        __x._M_data1 ^= __y._M_data1;
        return __x;
      }

      // [simd.mask.comparison] -----------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator==(const basic_mask& __x, const basic_mask& __y) noexcept
      { return !(__x ^ __y); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator!=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return __x ^ __y; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator>=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return __x || !__y; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator<=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return !__x || __y; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator>(const basic_mask& __x, const basic_mask& __y) noexcept
      { return __x && !__y; }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator<(const basic_mask& __x, const basic_mask& __y) noexcept
      { return !__x && __y; }

      // [simd.mask.cond] -----------------------------------------------------
      template <__vectorizable _T0, same_as<_T0> _T1>
        requires (sizeof(_T0) == _Bytes)
        [[__gnu__::__always_inline__]]
        friend constexpr vec<_T0, _S_size>
        __select_impl(const basic_mask& __k, const _T0& __t, const _T1& __f)
        {
          return vec<_T0, _S_size>::_S_init(__select_impl(__k._M_data0, __t, __f),
                                            __select_impl(__k._M_data1, __t, __f));
        }

      template <_ArchFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        constexpr bool
        _M_all_of() const
        {
          if constexpr (_N0 == _N1)
            return (_M_data0 and _M_data1)._M_all_of();
          else
            return _M_data0._M_all_of() and _M_data1._M_all_of();
        }

      template <_ArchFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        constexpr bool
        _M_any_of() const
        {
          if constexpr (_N0 == _N1)
            return (_M_data0 or _M_data1)._M_any_of();
          else
            return _M_data0._M_any_of() or _M_data1._M_any_of();
        }

      template <_ArchFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        constexpr bool
        _M_none_of() const
        {
          if constexpr (_N0 == _N1)
            return (_M_data0 or _M_data1)._M_none_of();
          else
            return _M_data0._M_none_of() and _M_data1._M_none_of();
        }

      [[__gnu__::__always_inline__]]
      bool
      _M_is_constprop() const
      { return _M_data0._M_is_constprop() and _M_data1._M_is_constprop(); }
    };

  // disabled basic_vec
  template <typename _Tp, typename _Abi>
    class basic_vec
    {
    public:
      using value_type = _Tp;

      using abi_type = _Abi;

      using mask_type = basic_mask<0, void>; // disabled

#define _GLIBCXX_DELETE_SIMD                                                                    \
      _GLIBCXX_DELETE_MSG("This specialization is disabled because of an invalid combination "  \
          "of template arguments to basic_vec.")

      basic_vec() = _GLIBCXX_DELETE_SIMD;

      ~basic_vec() = _GLIBCXX_DELETE_SIMD;

      basic_vec(const basic_vec&) = _GLIBCXX_DELETE_SIMD;

      basic_vec& operator=(const basic_vec&) = _GLIBCXX_DELETE_SIMD;

#undef _GLIBCXX_DELETE_SIMD
    };

  template <typename _Tp, typename _Abi>
    class _BinaryOps
    {
      using _Vp = basic_vec<_Tp, _Abi>;

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator+(const _Vp& __x, const _Vp& __y) noexcept
      {
        _Vp __r = __x;
        __r += __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator-(const _Vp& __x, const _Vp& __y) noexcept
      {
        _Vp __r = __x;
        __r -= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator*(const _Vp& __x, const _Vp& __y) noexcept
      {
        _Vp __r = __x;
        __r *= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator/(const _Vp& __x, const _Vp& __y) noexcept
      {
        _Vp __r = __x;
        __r /= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator%(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a % __a; }
      {
        _Vp __r = __x;
        __r %= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator&(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a & __a; }
      {
        _Vp __r = __x;
        __r &= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator|(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a | __a; }
      {
        _Vp __r = __x;
        __r |= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator^(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a ^ __a; }
      {
        _Vp __r = __x;
        __r ^= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator<<(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a << __a; }
      {
        _Vp __r = __x;
        __r <<= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator<<(const _Vp& __x, __simd_size_type __y) noexcept
      requires requires (_Tp __a, __simd_size_type __b) { __a << __b; }
      {
        _Vp __r = __x;
        __r <<= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator>>(const _Vp& __x, const _Vp& __y) noexcept
      requires requires (_Tp __a) { __a >> __a; }
      {
        _Vp __r = __x;
        __r >>= __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr _Vp
      operator>>(const _Vp& __x, __simd_size_type __y) noexcept
      requires requires (_Tp __a, __simd_size_type __b) { __a >> __b; }
      {
        _Vp __r = __x;
        __r >>= __y;
        return __r;
      }
    };

  struct _LoadCtorTag
  {};

  template <unsigned _Np>
    struct _SwapNeighbors
    {
      consteval unsigned
      operator()(unsigned __i, unsigned __size) const
      {
        if (__size % (2 * _Np) != 0)
          __builtin_abort(); // swap_neighbors<N> permutation requires a multiple of 2N elements
        else if (std::has_single_bit(_Np))
          return __i ^ _Np;
        else if (__i % (2 * _Np) >= _Np)
          return __i - _Np;
        else
          return __i + _Np;
      }
    };

  template <__vectorizable _Tp, __abi_tag _Ap>
    requires (_Ap::_S_nreg == 1)
      and (is_same_v<_Ap, _ScalarAbi<_Ap::_S_size>> or not __complex_like<_Tp>)
    class basic_vec<_Tp, _Ap>
    : _BinaryOps<_Tp, _Ap>
    {
      template <typename, typename>
        friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr int _S_full_size = __bit_ceil(unsigned(_S_size));

      static constexpr bool _S_is_scalar = is_same_v<_Ap, _ScalarAbi<_Ap::_S_size>>;

      static_assert(not _S_is_scalar or _S_size == 1);

      static constexpr bool _S_use_bitmask = [] {
        if constexpr (_S_is_scalar)
          return false;
        else
          return __flags_test(_Ap::_S_variant, _AbiVariant::_BitMask);
      }();

      using _DataType = typename _Ap::template _DataType<_Tp>;

      _DataType _M_data;

      static constexpr bool _S_is_partial = sizeof(_M_data) > sizeof(_Tp) * _S_size;

    public:
      using value_type = _Tp;

      using abi_type = _Ap;

      using mask_type = basic_mask<sizeof(_Tp), abi_type>;

      using iterator = __iterator<basic_vec>;

      using const_iterator = __iterator<const basic_vec>;

      constexpr iterator
      begin() noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      begin() const noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      cbegin() const noexcept
      { return {*this, 0}; }

      constexpr default_sentinel_t
      end() const noexcept
      { return {}; }

      constexpr default_sentinel_t
      cend() const noexcept
      { return {}; }

      static constexpr auto size = __simd_size_constant<_S_size>;

      // internal but public API ----------------------------------------------
      [[__gnu__::__always_inline__]]
      static constexpr basic_vec
      _S_init(_DataType __x)
      {
        basic_vec __r;
        __r._M_data = __x;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr bool
      _M_is_constprop() const
      { return __builtin_constant_p(_M_data); }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_concat_data() const
      {
        if constexpr (_S_is_scalar)
          return __vec_builtin_type<value_type, 1>{_M_data};
        else
          return _M_data;
      }

      template <int _Size = _S_size, int _Offset = 0, typename _A0, typename _Fp>
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_static_permute(const basic_vec<value_type, _A0>& __x, _Fp&& __idxmap)
        {
          using _Xp = basic_vec<value_type, _A0>;
          basic_vec __r;
          if constexpr (_S_is_scalar)
            {
              constexpr __simd_size_type __j = [&] consteval {
                if constexpr (__index_permutation_function_nosize<_Fp>)
                  return __idxmap(_Offset);
                else
                  return __idxmap(_Offset, _Size);
              }();
              if constexpr (__j == simd::zero_element or __j == simd::uninit_element)
                return basic_vec();
              else
                static_assert(__j >= 0 and __j < _Xp::_S_size);
              __r._M_data = __x[__j];
            }
          else
            {
              auto __idxmap2 = [=](auto __i) consteval {
                if constexpr (int(__i) >= _Size) // _S_full_size > _Size
                  return __simd_size_constant<simd::uninit_element>;
                else if constexpr (__index_permutation_function_nosize<_Fp>)
                  return __simd_size_constant<__idxmap(__i + _Offset)>;
                else
                  return __simd_size_constant<__idxmap(__i + _Offset, _Size)>;
              };
              constexpr auto __adj_idx = [](auto __i) {
                constexpr int __j = __i;
                if constexpr (__j == simd::zero_element)
                  return __simd_size_constant<__bit_ceil(unsigned(_Xp::_S_size))>;
                else if constexpr (__j == simd::uninit_element)
                  return __simd_size_constant<-1>;
                else
                  {
                    static_assert(__j >= 0 and __j < _Xp::_S_size);
                    return __simd_size_constant<__j>;
                  }
              };
              constexpr bool __needs_zero_element
                = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                    return ((__idxmap2(__simd_size_constant<_Is>).value == simd::zero_element)
                              || ...);
                  });
              if constexpr (_A0::_S_nreg == 2 and not __needs_zero_element)
                {
                  __r._M_data
                     = _GLIBCXX_SIMD_INT_PACK(_S_full_size, _Is, {
                         return __builtin_shufflevector(
                                  __x._M_data0._M_data, __x._M_data1._M_data,
                                  __adj_idx(__idxmap2(__simd_size_constant<_Is>)).value...);
                       });
                }
              else
                {
                  __r._M_data
                     = _GLIBCXX_SIMD_INT_PACK(_S_full_size, _Is, {
                         return __builtin_shufflevector(
                                  __x._M_concat_data(), decltype(__x._M_concat_data())(),
                                  __adj_idx(__idxmap2(__simd_size_constant<_Is>)).value...);
                       });
                }
            }
          return __r;
        }

      using _HalfVec
        = basic_vec<value_type, decltype(__abi_rebind<value_type, _S_size / 2, _Ap>())>;

      [[__gnu__::__always_inline__]]
      constexpr void
      _M_complex_set_real(const _HalfVec& __x) requires ((_S_size & 1) == 0)
      {
        if (_M_is_constprop() and __x._M_is_constprop())
          _M_data = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                      return _DataType { ((_Is & 1) == 0 ? value_type(__x[_Is / 2])
                                                         : _M_data[_Is])...};
                    });
        else if constexpr (_S_size == 2)
          _M_data[0] = __x[0];
        else
          _VecOps<_DataType>::_S_overwrite_even_elements(_M_data, __x);
      }

      [[__gnu__::__always_inline__]]
      constexpr void
      _M_complex_set_imag(const _HalfVec& __x) requires ((_S_size & 1) == 0)
      {
        if (_M_is_constprop() and __x._M_is_constprop())
          _M_data = _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                      return _DataType { ((_Is & 1) == 1 ? value_type(__x[_Is / 2])
                                                         : _M_data[_Is])...};
                    });
        else if constexpr (_S_size == 2)
          _M_data[1] = __x[0];
        else
          _VecOps<_DataType>::_S_overwrite_odd_elements(_M_data, __x);
      }

      template <typename _Vp>
        [[__gnu__::__always_inline__]]
        constexpr auto
        _M_chunk() const noexcept
        {
          constexpr int __n = _S_size / _Vp::_S_size;
          constexpr int __rem = _S_size % _Vp::_S_size;
          if constexpr (__rem == 0)
            {
              return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                       if constexpr (_Vp::_S_is_scalar)
                         return array<_Vp, __n> {_Vp::_S_init(_M_data[_Is])...};
                       else
                         return array<_Vp, __n> {
                           _Vp::_S_init(
                             _VecOps<typename _Vp::_DataType>::_S_extract(
                               _M_data, integral_constant<int, _Is * _Vp::_S_size>()))...
                         };
                     });
            }
          else
            {
              using _Rest = resize_t<__rem, _Vp>;
              _Rest __rest;
              if constexpr (_Rest::_S_size > 1)
                __rest = _VecOps<typename _Rest::_DataType>::_S_extract(
                           _M_data, integral_constant<int, __n * _Vp::_S_size>());
              else
                __rest = _M_data[__n * _Vp::_S_size];
              return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                       return tuple {
                         _Vp::_S_init(
                           _VecOps<typename _Vp::_DataType>::_S_extract(
                             _M_data, integral_constant<int, _Is * _Vp::_S_size>()))...,
                         __rest
                       };
                     });
            }
        }

      template <typename _A0>
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_concat(const basic_vec<value_type, _A0>& __x0) noexcept
        { return basic_vec(__x0); }

      template <typename... _As>
        requires (sizeof...(_As) > 1)
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_concat(const basic_vec<value_type, _As>&... __xs) noexcept
        {
          if constexpr (not _S_is_partial
                          and ((not basic_vec<value_type, _As>::_S_is_partial
                                  and _As::_S_size * sizeof...(_As) == _S_size) and ...))
            return basic_vec::_S_init(__vec_concat(__xs._M_concat_data()...));

          else
            return basic_vec::_S_init(__vec_concat_sized<__xs.size()...>(__xs._M_concat_data()...));
        }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_reduce_1(auto __binary_op) const
      {
        static_assert(__has_single_bit(unsigned(_S_size)));
        auto [__a, __b] = chunk<_S_size / 2>(*this);
        return __binary_op(__a, __b);
      }

      [[__gnu__::__always_inline__]]
      constexpr value_type
      _M_reduce_tail(const auto& __rest, auto __binary_op) const
      {
        if constexpr (__rest.size() > _S_size)
          {
            auto [__a, __b] = __rest.template _M_chunk<basic_vec>();
            return __binary_op(*this, __a)._M_reduce_tail(__b, __binary_op);
          }
        else if constexpr (_S_is_scalar)
          return __binary_op(*this, __rest)._M_data;
        else if constexpr (__rest.size() == _S_size)
          return __binary_op(*this, __rest)._M_reduce(__binary_op);
        else
          return _M_reduce_1(__binary_op)._M_reduce_tail(__rest, __binary_op);
      }

      template <_ArchFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        constexpr value_type
        _M_reduce(auto __binary_op) const
      {
        if constexpr (_S_size == 1)
          return operator[](0);
#ifdef __SSE2__
        else if constexpr (is_integral_v<value_type> and sizeof(value_type) == 1
                             and is_same_v<decltype(__binary_op), multiplies<>>)
          {
            // convert to unsigned short because of missing 8-bit mul instruction
            // we don't need to preserve the order of elements
            using _V16 = resize_t<_S_size / 2, rebind_t<unsigned short, basic_vec>>;
            auto __a = __builtin_bit_cast(_V16, *this);
            return __binary_op(__a, __a >> 8)._M_reduce(__binary_op);
            // alternative: return _V16(*this)._M_reduce(__binary_op);
          }
#endif
        else if constexpr (__has_single_bit(unsigned(_S_size)))
          {
            if constexpr (sizeof(_M_data) > 16)
              return _M_reduce_1(__binary_op)._M_reduce(__binary_op);
            else if constexpr (_S_size == 2)
              return _M_reduce_1(__binary_op)[0];
            else
              {
                static_assert(_S_size <= 16);
                auto __x = *this;
                if constexpr (_S_size > 8)
                  __x = __binary_op(__x, _S_static_permute(__x, _SwapNeighbors<8>()));
                if constexpr (_S_size > 4)
                  __x = __binary_op(__x, _S_static_permute(__x, _SwapNeighbors<4>()));
#ifdef __SSE2__
                // avoid pshufb by "promoting" to int
                if constexpr (is_integral_v<value_type> and sizeof(value_type) <= 2)
                  return resize_t<4, rebind_t<int, basic_vec>>(chunk<4>(__x)[0])
                           ._M_reduce(__binary_op);
#endif
                if constexpr (_S_size > 2)
                  __x = __binary_op(__x, _S_static_permute(__x, _SwapNeighbors<2>()));
                if constexpr (is_integral_v<value_type> and sizeof(value_type) == 1)
                  return __binary_op(vec<value_type, 1>(__x[0]), vec<value_type, 1>(__x[1]))[0];
                else
                  return __binary_op(__x, _S_static_permute(__x, _SwapNeighbors<1>()))[0];
              }
          }
        else
          {
            // e.g. _S_size = 16 + 16 + 15 (vec<char, 47>)
            // -> 8 + 8 + 7 -> 4 + 4 + 3 -> 2 + 2 + 1 -> 1
            auto __chunked = chunk<__bit_floor(unsigned(_S_size)) / 2>(*this);
            using _Cp = decltype(__chunked);
            if constexpr (tuple_size_v<_Cp> == 4)
              {
                const auto& [__a, __b, __c, __rest] = __chunked;
                return __binary_op(__binary_op(__a, __b), __c)._M_reduce_tail(__rest, __binary_op);
              }
            else if constexpr (tuple_size_v<_Cp> == 3)
              {
                const auto& [__a, __b, __rest] = __chunked;
                return __binary_op(__a, __b)._M_reduce_tail(__rest, __binary_op);
              }
            else
              static_assert(false);
          }
      }

      // [simd.math] ----------------------------------------------------------
      template <_OptFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        constexpr mask_type
        _M_isnan() const requires is_floating_point_v<value_type>
        {
          if constexpr (_Flags._M_finite_math_only())
            return mask_type(false);
          else if constexpr (_S_is_scalar)
            return mask_type(std::isnan(_M_data));
          else if constexpr (_S_use_bitmask)
            return _M_isunordered(*this);
          else
            return mask_type([&](int __i) { return std::isnan(_M_data[__i]); });
        }

      template <_OptFlags _Flags = {}>
        [[__gnu__::__always_inline__]]
        constexpr mask_type
        _M_isunordered(basic_vec __y) const requires is_floating_point_v<value_type>
        {
          if constexpr (_Flags._M_finite_math_only())
            return mask_type(false);
          else if constexpr (_S_is_scalar)
            return mask_type(std::isunordered(_M_data, __y._M_data));
#ifdef _GLIBCXX_SIMD_HAVE_SSE
          else if constexpr (_S_use_bitmask)
            return _M_bitmask_cmp<_X86Cmp::_Unord>(__y._M_data);
#endif
          else
            return mask_type([&](int __i) {
                     return std::isunordered(_M_data[__i], __y._M_data[__i]);
                   });
        }

      // [simd.overview] default constructor ----------------------------------
      basic_vec() = default;

      // [simd.overview] impl-def conversions ---------------------------------
      constexpr
      basic_vec(_DataType __x) requires(not _S_is_scalar)
        : _M_data(__x)
      {}

      constexpr
      operator _DataType() requires(not _S_is_scalar)
      { return _M_data; }

      // [simd.ctor] broadcast constructor ------------------------------------
      template <typename _Up>
        requires constructible_from<value_type, _Up>
        [[__gnu__::__always_inline__]]
        constexpr explicit(not __broadcast_constructible<_Up, value_type>)
        basic_vec(_Up&& __x) noexcept
          : _M_data(_DataType() == _DataType() ? static_cast<value_type>(__x) : value_type())
        {}

      // [simd.ctor] conversion constructor -----------------------------------
      template <typename _Up, typename _UAbi>
        requires (__simd_size_v<_Up, _UAbi> == _S_size)
        // FIXME(file LWG issue): missing constraint `constructible_from<value_type, _Up>`
        [[__gnu__::__always_inline__]]
        constexpr
        explicit(not __value_preserving_convertible_to<_Up, value_type>
                   or __higher_rank_than<_Up, value_type>)
        basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
          : _M_data([&] [[__gnu__::__always_inline__]]() {
              if constexpr (_S_is_scalar)
                return static_cast<value_type>(__x[0]);
              else
                return __vec_cast<_DataType>(__x._M_concat_data());
            }())
        {}

      // [simd.ctor] generator constructor ------------------------------------
      template <__simd_generator_invokable<value_type, _S_size> _Fp>
        [[__gnu__::__always_inline__]]
        constexpr explicit
        basic_vec(_Fp&& __gen)
        : _M_data(_GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                    return _DataType{static_cast<value_type>(__gen(__simd_size_constant<_Is>))...};
                  }))
        {}

      template <__almost_simd_generator_invokable<value_type, _S_size> _Fp>
        constexpr explicit
        basic_vec(_Fp&&)
          = _GLIBCXX_DELETE_MSG("Invalid return type of the generator function: "
                                "Requires value-preserving conversion or implicitly "
                                "convertible user-defined type.");

      // [simd.ctor] load constructor -----------------------------------------
      template <typename _Up>
        [[__gnu__::__always_inline__]]
        constexpr
        basic_vec(_LoadCtorTag, const _Up* __ptr)
          : _M_data()
        {
          if constexpr (_S_is_scalar)
            _M_data = static_cast<value_type>(__ptr[0]);
          else if (__builtin_is_constant_evaluated())
            {
              _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                _M_data = _DataType{__ptr[_Is]...};
              });
            }
          else if constexpr (is_integral_v<_Up> == is_integral_v<value_type>
                               and is_floating_point_v<_Up> == is_floating_point_v<value_type>
                               and sizeof(_Up) == sizeof(value_type))
            {
              // This assumes std::floatN_t to be bitwise equal to float/double
              __builtin_memcpy(&_M_data, __ptr, sizeof(value_type) * _S_size);
            }
          else
            {
              __vec_builtin_type<_Up, _S_full_size> __tmp = {};
              __builtin_memcpy(&__tmp, __ptr, sizeof(_Up) * _S_size);
              _M_data = __vec_cast<_DataType>(__tmp);
            }
        }

      template <__static_sized_range<size.value> _Rg, typename... _Flags>
        // FIXME(file LWG issue):
        // 1. missing constraint on `constructible_from<value_type, range_value_t<_Rg>>`
        // 2. Mandates should be Constraints to fix answers of convertible_to and constructible_from
        //
        // Consider `convertible_to<array<complex<float>, 4>, vec<float, 4>>`. It should say false
        // but currently would be true.
        // Also, with `flag_convert` the current Mandates doesn't catch the complex<float> -> float
        // issue and fails with horrible diagnostics somewhere in the instantiation.
        [[__gnu__::__always_inline__]]
        constexpr
        basic_vec(_Rg&& __range, flags<_Flags...> __flags = {})
          : basic_vec(_LoadCtorTag(), __flags.template _S_adjust_pointer<basic_vec>(
                                        std::ranges::data(__range)))
        {
          static_assert(__loadstore_convertible_to<std::ranges::range_value_t<_Rg>, value_type,
                                                   _Flags...>);
        }

      // [simd.subscr] --------------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      {
        if constexpr (_S_is_scalar)
          return _M_data;
        else
          return _M_data[__i];
      }

      // [simd.unary] unary operators -----------------------------------------
      // increment and decrement are implemented in terms of operator+=/-= which avoids UB on
      // padding elements while not breaking UBsan
      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator++() noexcept requires requires(value_type __a) { ++__a; }
      { return *this += value_type(1); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator++(int) noexcept requires requires(value_type __a) { __a++; }
      {
        basic_vec __r = *this;
        *this += value_type(1);
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator--() noexcept requires requires(value_type __a) { --__a; }
      { return *this -= value_type(1); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator--(int) noexcept requires requires(value_type __a) { __a--; }
      {
        basic_vec __r = *this;
        *this -= value_type(1);
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      operator!() const noexcept requires requires(value_type __a) { !__a; }
      { return mask_type::_S_init(!_M_data); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator+() const noexcept requires requires(value_type __a) { +__a; }
      { return *this; }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator-() const noexcept requires requires(value_type __a) { -__a; }
      { return _S_init(-_M_data); }

      // [simd.cassign] binary operators
#define _GLIBCXX_SIMD_DEFINE_OP(sym)                                 \
      [[__gnu__::__always_inline__]]                                 \
      friend constexpr basic_vec&                                    \
      operator sym##=(basic_vec& __x, const basic_vec& __y) noexcept \
      requires requires(value_type __a) { __a sym __a; }             \
      {                                                              \
        __x._M_data sym##= __y._M_data;                              \
        return __x;                                                  \
      }

      _GLIBCXX_SIMD_DEFINE_OP(&)
      _GLIBCXX_SIMD_DEFINE_OP(|)
      _GLIBCXX_SIMD_DEFINE_OP(^)
      _GLIBCXX_SIMD_DEFINE_OP(<<)
      _GLIBCXX_SIMD_DEFINE_OP(>>)

#undef _GLIBCXX_SIMD_DEFINE_OP

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator+=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a + __a; }
      {
        if constexpr (_S_is_partial and is_integral_v<value_type> and is_signed_v<value_type>)
          { // avoid spurious UB on signed integer overflow of the padding element(s). But don't
            // remove UB of the active elements (so that UBsan can still do its job).
            using _UV = typename _Ap::template _DataType<make_unsigned_t<value_type>>;
            const _DataType __result
              = reinterpret_cast<_DataType>(reinterpret_cast<_UV>(__x._M_data)
                                              + reinterpret_cast<_UV>(__y._M_data));
            const auto __positive = __y > value_type();
            const auto __overflow = __positive != (__result > __x);
            if (__overflow._M_any_of())
              __builtin_unreachable(); // trigger UBsan
            __x._M_data = __result;
          }
        else
          __x._M_data += __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator-=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a - __a; }
      {
        if constexpr (_S_is_partial and is_integral_v<value_type> and is_signed_v<value_type>)
          { // avoid spurious UB on signed integer overflow of the padding element(s). But don't
            // remove UB of the active elements (so that UBsan can still do its job).
            using _UV = typename _Ap::template _DataType<make_unsigned_t<value_type>>;
            const _DataType __result
              = reinterpret_cast<_DataType>(reinterpret_cast<_UV>(__x._M_data)
                                              - reinterpret_cast<_UV>(__y._M_data));
            const auto __positive = __y > value_type();
            const auto __overflow = __positive != (__result < __x);
            if (__overflow._M_any_of())
              __builtin_unreachable(); // trigger UBsan
            __x._M_data = __result;
          }
        else
          __x._M_data -= __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator*=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a * __a; }
      {
        if constexpr (_S_is_partial and is_integral_v<value_type> and is_signed_v<value_type>)
          { // avoid spurious UB on signed integer overflow of the padding element(s). But don't
            // remove UB of the active elements (so that UBsan can still do its job).
            for (int __i = 0; __i < _S_size; ++__i)
              {
                if (__builtin_mul_overflow_p(__x._M_data[__i], __y._M_data[__i], value_type()))
                  __builtin_unreachable();
              }
            using _UV = typename _Ap::template _DataType<make_unsigned_t<value_type>>;
            __x._M_data = reinterpret_cast<_DataType>(reinterpret_cast<_UV>(__x._M_data)
                                                        * reinterpret_cast<_UV>(__y._M_data));
          }

        // 'uint16 * uint16' promotes to int and can therefore lead to UB. The standard does not
        // require to avoid the undefined behavior. It's unnecessary and easy to avoid. It's also
        // unexpected because there's no UB on the vector types (which don't promote).
        else if constexpr (_S_is_scalar and is_unsigned_v<value_type>
                             and is_signed_v<decltype(value_type() * value_type())>)
          __x._M_data = unsigned(__x._M_data) * unsigned(__y._M_data);

        else
          __x._M_data *= __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator/=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a / __a; }
      {
#ifdef __SSE2__
        // x86 doesn't have integral SIMD division instructions
        // While division is faster, the required conversions are still a problem:
        // see PR121274, PR121284, and PR121296 for missed optimizations wrt. conversions
        if (not (__x._M_is_constprop() and __y._M_is_constprop()))
          {
            if constexpr (is_integral_v<value_type>
                            and __value_preserving_convertible_to<value_type, float>)
              return __x = basic_vec(rebind_t<float, basic_vec>(__x) / __y);
            else if constexpr (is_integral_v<value_type>
                                 and __value_preserving_convertible_to<value_type, double>)
              return __x = basic_vec(rebind_t<double, basic_vec>(__x) / __y);
          }
#endif
        basic_vec __y1 = __y;
        if constexpr (_S_is_partial)
          {
            if constexpr (is_integral_v<value_type>)
              {
                // Assume integral division doesn't have SIMD instructions and must be done per
                // element anyway. Partial vectors should skip their padding elements.
                if (__builtin_is_constant_evaluated())
                  __x = basic_vec([&](int __i) -> value_type {
                          return __x._M_data[__i] / __y._M_data[__i];
                        });
                else
                  {
                    for (int __i = 0; __i < _S_size; ++__i)
                      __x._M_data[__i] /= __y._M_data[__i];
                  }
                return __x;
              }
            else
              __y1 = __select_impl(mask_type::_S_init(mask_type::_S_implicit_mask),
                                   __y, basic_vec(value_type(1)));
          }
        __x._M_data /= __y1._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator%=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a % __a; }
      {
        static_assert(is_integral_v<value_type>);
        if constexpr (_S_is_partial)
          {
            if (__builtin_is_constant_evaluated())
              __x = basic_vec([&](int __i) -> value_type {
                      return __x._M_data[__i] % __y._M_data[__i];
                    });
            else if (__builtin_constant_p(__x._M_data % __y._M_data))
              __x._M_data %= __y._M_data;
            else if (__y._M_is_constprop())
              __x._M_data %= __select_impl(mask_type::_S_init(mask_type::_S_implicit_mask),
                                           __y, basic_vec(value_type(1)))._M_data;
            else
              {
                // Assume integral division doesn't have SIMD instructions and must be done per
                // element anyway. Partial vectors should skip their padding elements.
                for (int __i = 0; __i < _S_size; ++__i)
                  __x._M_data[__i] %= __y._M_data[__i];
              }
          }
        else
          __x._M_data %= __y._M_data;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator<<=(basic_vec& __x, __simd_size_type __y) noexcept
      requires requires(value_type __a, __simd_size_type __b) { __a << __b; }
      {
        __x._M_data <<= __y;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator>>=(basic_vec& __x, __simd_size_type __y) noexcept
      requires requires(value_type __a, __simd_size_type __b) { __a >> __b; }
      {
        __x._M_data >>= __y;
        return __x;
      }

      // [simd.comparison] ----------------------------------------------------
#ifdef __SSE__
      template <_X86Cmp _Cmp>
        [[__gnu__::__always_inline__]]
        constexpr mask_type
        _M_bitmask_cmp(_DataType __y) const
        {
          static_assert(_S_use_bitmask);
          if (__builtin_is_constant_evaluated()
                or (__builtin_constant_p(_M_data) and __builtin_constant_p(__y)))
            {
              return mask_type::_S_init(
                       _GLIBCXX_SIMD_INT_PACK(_S_size, _Is, {
                         constexpr auto __cmp_op = [] [[__gnu__::__always_inline__]]
                                                     (value_type __a, value_type __b) {
                           if constexpr (_Cmp == _X86Cmp::_Eq)
                             return __a == __b;
                           else if constexpr (_Cmp == _X86Cmp::_Lt)
                             return __a < __b;
                           else if constexpr (_Cmp == _X86Cmp::_Le)
                             return __a <= __b;
                           else if constexpr (_Cmp == _X86Cmp::_Unord)
                             return std::isunordered(__a, __b);
                           else if constexpr (_Cmp == _X86Cmp::_Neq)
                             return __a != __b;
                           else if constexpr (_Cmp == _X86Cmp::_Nlt)
                             return not (__a < __b);
                           else if constexpr (_Cmp == _X86Cmp::_Nle)
                             return not (__a <= __b);
                           else
                             static_assert(false);
                         };
                         return ((__cmp_op(__vec_get(_M_data, _Is), __vec_get(__y, _Is))
                                    ? (1ULL << _Is) : 0) | ...);
                       }));
              }
            else
              return mask_type::_S_init(__x86_bitmask_cmp<_Cmp>(_M_data, __y));
        }
#endif

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator==(const basic_vec& __x, const basic_vec& __y) noexcept
      {
#ifdef __SSE__
        if constexpr (_S_use_bitmask)
          return __x._M_bitmask_cmp<_X86Cmp::_Eq>(__y._M_data);
        else
#endif
          return mask_type::_S_init(__x._M_data == __y._M_data);
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator!=(const basic_vec& __x, const basic_vec& __y) noexcept
      {
#ifdef __SSE__
        if constexpr (_S_use_bitmask)
          return __x._M_bitmask_cmp<_X86Cmp::_Neq>(__y._M_data);
        else
#endif
          return mask_type::_S_init(__x._M_data != __y._M_data);
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<(const basic_vec& __x, const basic_vec& __y) noexcept
      {
#ifdef __SSE__
        if constexpr (_S_use_bitmask)
          return __x._M_bitmask_cmp<_X86Cmp::_Lt>(__y._M_data);
        else
#endif
          return mask_type::_S_init(__x._M_data < __y._M_data);
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<=(const basic_vec& __x, const basic_vec& __y) noexcept
      {
#ifdef __SSE__
        if constexpr (_S_use_bitmask)
          return __x._M_bitmask_cmp<_X86Cmp::_Le>(__y._M_data);
        else
#endif
          return mask_type::_S_init(__x._M_data <= __y._M_data);
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>(const basic_vec& __x, const basic_vec& __y) noexcept
      { return __y < __x; }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return __y <= __x; }

      // [simd.cond] ---------------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec
      __select_impl(const mask_type& __k, const basic_vec& __t, const basic_vec& __f)
      {
        if constexpr (_S_size == 1)
          return __k[0] ? __t : __f;
        else if constexpr (_S_use_bitmask)
          {
#if _GLIBCXX_SIMD_HAVE_SSE
            if (__builtin_is_constant_evaluated()
                  or (__k._M_is_constprop() and __t._M_is_constprop() and __f._M_is_constprop()))
              return basic_vec([&](int __i) { return __k[__i] ? __t[__i] : __f[__i]; });
            else
              return __x86_bitmask_blend(__k._M_data, __t._M_data, __f._M_data);
#else
            static_assert(false, "TODO");
#endif
          }
        else
          return __k._M_data ? __t._M_data : __f._M_data;
      }
    };

  template <__vectorizable _Tp, __abi_tag _Ap>
    requires (_Ap::_S_nreg > 1)
      and (not __complex_like<_Tp>)
    class basic_vec<_Tp, _Ap>
    : _BinaryOps<_Tp, _Ap>
    {
      template <typename, typename>
        friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr int _N0 = __bit_ceil(unsigned(_S_size)) / 2;

      static constexpr int _N1 = _S_size - _N0;

      using _DataType0 = basic_vec<_Tp, decltype(__abi_rebind<_Tp, _N0, _Ap>())>;

      using _DataType1 = basic_vec<_Tp, decltype(__abi_rebind<_Tp, _N1, _Ap>())>;

      _DataType0 _M_data0;

      _DataType1 _M_data1;

    public:
      using value_type = _Tp;

      using abi_type = _Ap;

      using mask_type = basic_mask<sizeof(_Tp), abi_type>;

      using iterator = __iterator<basic_vec>;

      using const_iterator = __iterator<const basic_vec>;

      constexpr iterator
      begin() noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      begin() const noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      cbegin() const noexcept
      { return {*this, 0}; }

      constexpr default_sentinel_t
      end() const noexcept
      { return {}; }

      constexpr default_sentinel_t
      cend() const noexcept
      { return {}; }

      static constexpr auto size = __simd_size_constant<_S_size>;

      [[__gnu__::__always_inline__]]
      static constexpr basic_vec
      _S_init(_DataType0 __x, _DataType1 __y)
      {
        basic_vec __r;
        __r._M_data0 = __x;
        __r._M_data1 = __y;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr bool
      _M_is_constprop() const
      { return _M_data0._M_is_constprop() and _M_data1._M_is_constprop(); }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_concat_data() const
      {
        return __vec_concat(_M_data0._M_concat_data(),
                            __vec_zero_pad_to<sizeof(_M_data0)>(_M_data1._M_concat_data()));
      }

      template <int _Size = _S_size, int _Offset = 0, typename _A0, typename _Fp>
        [[__gnu__::__always_inline__]]
        static constexpr basic_vec
        _S_static_permute(const basic_vec<value_type, _A0>& __x, _Fp&& __idxmap)
        {
          return _S_init(
                   _DataType0::template _S_static_permute<_Size, _Offset>(__x, __idxmap),
                   _DataType1::template _S_static_permute<_Size, _Offset + _N0>(__x, __idxmap));
        }

      template <typename _Vp>
        [[__gnu__::__always_inline__]]
        constexpr auto
        _M_chunk() const noexcept
        {
          constexpr int __n = _S_size / _Vp::_S_size;
          constexpr int __rem = _S_size % _Vp::_S_size;
          if constexpr (_N0 == _Vp::_S_size)
            {
              if constexpr (__rem == 0)
                return array<_Vp, __n> {_M_data0, _M_data1};
              else
                return tuple<_Vp, resize_t<__rem, _Vp>> {_M_data0, _M_data1};
            }
          else if constexpr (__rem == 0)
            {
              using _Rp = array<_Vp, __n>;
              if constexpr (sizeof(_Rp) == sizeof(*this))
                {
                  static_assert(not _Vp::_S_is_partial);
                  return __builtin_bit_cast(_Rp, *this);
                }
              else
                {
                  return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                           return _Rp {_Vp([&](int __i) {
                                         return (*this)[__i + _Is * _Vp::_S_size];
                                       })...};
                         });
                }
            }
          else
            return _GLIBCXX_SIMD_INT_PACK(__n, _Is, {
                     using _Rest = resize_t<__rem, _Vp>;
                     // can't bit-cast because the member order of tuple is reversed
                     return tuple {
                       _Vp  ([&](int __i) { return (*this)[__i + _Is * _Vp::_S_size]; })...,
                       _Rest([&](int __i) { return (*this)[__i + __n * _Vp::_S_size]; })
                     };
            });
        }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_reduce_1(auto __binary_op) const
      {
        static_assert(_N0 == _N1);
        return __binary_op(_M_data0, _M_data1);
      }

      [[__gnu__::__always_inline__]]
      constexpr value_type
      _M_reduce_tail(const auto& __rest, auto __binary_op) const
      {
        if constexpr (__rest.size() > _S_size)
          {
            auto [__a, __b] = __rest.template _M_chunk<basic_vec>();
            return __binary_op(*this, __a)._M_reduce_tail(__b, __binary_op);
          }
        else if constexpr (__rest.size() == _S_size)
          return __binary_op(*this, __rest)._M_reduce(__binary_op);
        else
          return _M_reduce_1(__binary_op)._M_reduce_tail(__rest, __binary_op);
      }

      [[__gnu__::__always_inline__]]
      constexpr value_type
      _M_reduce(auto __binary_op) const
      {
#ifdef __SSE2__
        if constexpr (is_integral_v<value_type> and sizeof(value_type) == 1
                             and is_same_v<decltype(__binary_op), multiplies<>>)
          {
            // convert to unsigned short because of missing 8-bit mul instruction
            // we don't need to preserve the order of elements
#if 1
            using _V16 = resize_t<_S_size / 2, rebind_t<unsigned short, basic_vec>>;
            auto __a = __builtin_bit_cast(_V16, *this);
            return __binary_op(__a, __a >> 8)._M_reduce(__binary_op);
#else
            // alternative:
            using _V16 = rebind_t<unsigned short, basic_vec>;
            return _V16(*this)._M_reduce(__binary_op);
#endif
          }
#endif
        else if constexpr (_N0 == _N1)
          return _M_reduce_1(__binary_op)._M_reduce(__binary_op);
#if 0 // needs benchmarking before we do this
        else if constexpr (sizeof(_M_data0) == sizeof(_M_data1)
                             and requires {
                               __default_identity_element<value_type, decltype(__binary_op)>();
                             })
          { // extend to power-of-2 with identity element for more parallelism
            _DataType0 __v1 = __builtin_bit_cast(_DataType0, _M_data1);
            constexpr _DataType0 __id
              = __default_identity_element<value_type, decltype(__binary_op)>();
            constexpr auto __k = _DataType0::mask_type::_S_partial_mask_of_n(_N1);
            __v1 = __select_impl(__k, __v1, __id);
            return __binary_op(_M_data0, __v1)._M_reduce(__binary_op);
          }
#endif
        else
          return _M_data0._M_reduce_1(__binary_op)._M_reduce_tail(_M_data1, __binary_op);
      }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      _M_isnan() const requires is_floating_point_v<value_type>
      { return mask_type::_S_init(_M_data0._M_isnan(), _M_data1._M_isnan()); }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      _M_isunordered(basic_vec __y) const requires is_floating_point_v<value_type>
      {
        return mask_type::_S_init(_M_data0._M_isunordered(__y._M_data0),
                                  _M_data1._M_isunordered(__y._M_data1));
      }

      basic_vec() = default;

      // [simd.overview] impl-def conversions ---------------------------------
      [[__gnu__::__always_inline__]]
      constexpr
      basic_vec(const __vec_builtin_type<value_type, __bit_ceil(unsigned(_S_size))>& __x)
      : _M_data0(_VecOps<__vec_builtin_type<value_type, _N0>>::_S_extract(__x)),
        _M_data1(_VecOps<__vec_builtin_type<value_type, __bit_ceil(unsigned(_N1))>>
                   ::_S_extract(__x, integral_constant<int, _N0>()))
      {}

      [[__gnu__::__always_inline__]]
      constexpr
      operator __vec_builtin_type<value_type, __bit_ceil(unsigned(_S_size))>()
      { return _M_concat_data(); }

      // [simd.ctor] broadcast constructor ------------------------------------
      template <typename _Up>
        requires constructible_from<value_type, _Up>
        [[__gnu__::__always_inline__]]
        constexpr explicit(not __broadcast_constructible<_Up, value_type>)
        basic_vec(_Up&& __x) noexcept
          : _M_data0(static_cast<value_type>(__x)), _M_data1(static_cast<value_type>(__x))
        {}

      // [simd.ctor] conversion constructor -----------------------------------
      template <typename _Up, typename _UAbi>
        requires (__simd_size_v<_Up, _UAbi> == _S_size)
        // FIXME(file LWG issue): missing constraint `constructible_from<value_type, _Up>`
        [[__gnu__::__always_inline__]]
        constexpr
        explicit(not __value_preserving_convertible_to<_Up, value_type>
                   or __higher_rank_than<_Up, value_type>)
        basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
          : _M_data0(get<0>(chunk<_N0>(__x))),
            _M_data1(get<1>(chunk<_N0>(__x)))
        {}

      // [simd.ctor] generator constructor ------------------------------------
      template <__simd_generator_invokable<value_type, _S_size> _Fp>
        [[__gnu__::__always_inline__]]
        constexpr explicit
        basic_vec(_Fp&& __gen)
          : _M_data0(__gen), _M_data1([&] [[__gnu__::__always_inline__]] (auto __i) {
                               return __gen(__simd_size_constant<__i + _N0>);
                             })
        {}

      template <__almost_simd_generator_invokable<value_type, _S_size> _Fp>
        constexpr explicit
        basic_vec(_Fp&&)
          = _GLIBCXX_DELETE_MSG("Invalid return type of the generator function: "
                                "Requires value-preserving conversion or implicitly "
                                "convertible user-defined type.");

      // [simd.ctor] load constructor -----------------------------------------
      template <typename _Up>
        [[__gnu__::__always_inline__]]
        constexpr
        basic_vec(_LoadCtorTag, const _Up* __ptr)
          : _M_data0(_LoadCtorTag(), __ptr),
            _M_data1(_LoadCtorTag(), __ptr + _N0)
        {}

      template <__static_sized_range<size.value> _Rg, typename... _Flags>
        // FIXME: see load ctor(s) above
        constexpr
        basic_vec(_Rg&& __range, flags<_Flags...> __flags = {})
        : basic_vec(_LoadCtorTag(),
                    __flags.template _S_adjust_pointer<basic_vec>(std::ranges::data(__range)))
        {
          static_assert(__loadstore_convertible_to<std::ranges::range_value_t<_Rg>, value_type,
                                                   _Flags...>);
        }

      // [simd.subscr] --------------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      {
        struct _Tmp
        { alignas(basic_vec) const value_type _M_values[_S_size]; };
        return __builtin_bit_cast(_Tmp, *this)._M_values[__i];
      }

      // [simd.unary] unary operators -----------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator++() noexcept requires requires(value_type __a) { ++__a; }
      {
        ++_M_data0;
        ++_M_data1;
        return *this;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator++(int) noexcept requires requires(value_type __a) { __a++; }
      {
        basic_vec __r = *this;
        ++_M_data0;
        ++_M_data1;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator--() noexcept requires requires(value_type __a) { --__a; }
      {
        --_M_data0;
        --_M_data1;
        return *this;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator--(int) noexcept requires requires(value_type __a) { __a--; }
      {
        basic_vec __r = *this;
        --_M_data0;
        --_M_data1;
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      operator!() const noexcept requires requires(value_type __a) { !__a; }
      { return mask_type::_S_init(!_M_data0, !_M_data1); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator+() const noexcept requires requires(value_type __a) { +__a; }
      { return *this; }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator-() const noexcept requires requires(value_type __a) { -__a; }
      { return _S_init(-_M_data0, -_M_data1); }

      // [simd.cassign] -------------------------------------------------------
#define _GLIBCXX_SIMD_DEFINE_OP(sym)                                 \
      [[__gnu__::__always_inline__]]                                 \
      friend constexpr basic_vec&                                    \
      operator sym##=(basic_vec& __x, const basic_vec& __y) noexcept \
      {                                                              \
        __x._M_data0 sym##= __y._M_data0;                            \
        __x._M_data1 sym##= __y._M_data1;                            \
        return __x;                                                  \
      }

      _GLIBCXX_SIMD_DEFINE_OP(+)
      _GLIBCXX_SIMD_DEFINE_OP(-)
      _GLIBCXX_SIMD_DEFINE_OP(*)
      _GLIBCXX_SIMD_DEFINE_OP(/)
      _GLIBCXX_SIMD_DEFINE_OP(%)
      _GLIBCXX_SIMD_DEFINE_OP(&)
      _GLIBCXX_SIMD_DEFINE_OP(|)
      _GLIBCXX_SIMD_DEFINE_OP(^)
      _GLIBCXX_SIMD_DEFINE_OP(<<)
      _GLIBCXX_SIMD_DEFINE_OP(>>)

#undef _GLIBCXX_SIMD_DEFINE_OP

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator<<=(basic_vec& __x, __simd_size_type __y) noexcept
      requires requires(value_type __a, __simd_size_type __b) { __a << __b; }
      {
        __x._M_data0 <<= __y;
        __x._M_data1 <<= __y;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator>>=(basic_vec& __x, __simd_size_type __y) noexcept
      requires requires(value_type __a, __simd_size_type __b) { __a >> __b; }
      {
        __x._M_data0 >>= __y;
        __x._M_data1 >>= __y;
        return __x;
      }

      // [simd.comparison] ----------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator==(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 == __y._M_data0, __x._M_data1 == __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator!=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 != __y._M_data0, __x._M_data1 != __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 < __y._M_data0, __x._M_data1 < __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 <= __y._M_data0, __x._M_data1 <= __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 > __y._M_data0, __x._M_data1 > __y._M_data1); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_init(__x._M_data0 >= __y._M_data0, __x._M_data1 >= __y._M_data1); }

      // [simd.cond] ---------------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec
      __select_impl(const mask_type& __k, const basic_vec& __t, const basic_vec& __f)
      {
        return _S_init(__select_impl(__k._M_data0, __t._M_data0, __f._M_data0),
                       __select_impl(__k._M_data1, __t._M_data1, __f._M_data1));
      }
    };

  template <__vectorizable _Tp, __abi_tag _Ap>
    requires (_Ap::_S_nreg >= 1)
      and __complex_like<_Tp>
      and (__flags_test(_Ap::_S_variant, _AbiVariant::_CxIleav))
    class basic_vec<_Tp, _Ap>
    : _BinaryOps<_Tp, _Ap>
    {
      template <typename, typename>
        friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr int _S_full_size = __bit_ceil(unsigned(_S_size));

      using _T0 = typename _Tp::value_type;

      using _TSimd = basic_vec<_T0, decltype(__abi_rebind<_T0, 2 * _S_size, _Ap>())>;

      using _RealSimd = basic_vec<_T0, decltype(__abi_rebind<_T0, _S_size, _Ap>())>;

      _TSimd _M_data = {};

      static constexpr bool _S_use_bitmask = __flags_test(_Ap::_S_variant, _AbiVariant::_BitMask);

      static constexpr bool _S_is_partial = sizeof(_M_data) > sizeof(_Tp) * _S_size;

    public:
      using value_type = _Tp;

      using abi_type = _Ap;

      using mask_type = basic_mask<sizeof(_Tp), abi_type>;

      using iterator = __iterator<basic_vec>;

      using const_iterator = __iterator<const basic_vec>;

      constexpr iterator
      begin() noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      begin() const noexcept
      { return {*this, 0}; }

      constexpr const_iterator
      cbegin() const noexcept
      { return {*this, 0}; }

      constexpr default_sentinel_t
      end() const noexcept
      { return {}; }

      constexpr default_sentinel_t
      cend() const noexcept
      { return {}; }

      static constexpr auto size = __simd_size_constant<_S_size>;

      [[__gnu__::__always_inline__]]
      constexpr bool
      _M_is_constprop() const
      { return _M_data._M_is_constprop(); }

      template <typename _Vp>
        [[__gnu__::__always_inline__]]
        constexpr auto
        _M_chunk() const noexcept
        {
          constexpr int __n = _S_size / _Vp::_S_size;
          constexpr int __rem = _S_size % _Vp::_S_size;
          static_assert(false, "TODO");
        }

      basic_vec() = default;

      // TODO: conversion extensions

      // [simd.ctor] broadcast constructor ------------------------------------
      template <typename _Up>
        requires constructible_from<value_type, _Up>
        [[__gnu__::__always_inline__]]
        constexpr explicit(not __broadcast_constructible<_Up, value_type>)
        basic_vec(_Up&& __x) noexcept
          : _M_data([&](int __i) {
              if constexpr (__complex_like<_Up>)
                return (__i & 1) == 0 ? __x.real() : __x.imag();
              else
                return (__i & 1) == 0 ? __x : _T0();
            })
        {}

      // [simd.ctor] conversion constructor -----------------------------------
      template <__complex_like _Up, typename _UAbi>
        requires(__simd_size_v<_Up, _UAbi> == size.value
                   and std::constructible_from<value_type, _Up>)
        [[__gnu__::__always_inline__]]
        constexpr
        explicit(not convertible_to<_Up, value_type>)
        basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
        : _M_data(__x._M_data)
        {}

      template <typename _Up, typename _UAbi> // _Up is not complex!
        requires (__simd_size_v<_Up, _UAbi> == _S_size)
          and std::constructible_from<value_type, _Up>
          and (not is_same_v<_T0, _Up>)
        [[__gnu__::__always_inline__]]
        constexpr
        explicit(not convertible_to<_Up, value_type>)
        basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
        : basic_vec(basic_vec<_T0, typename _UAbi::template _Rebind<_T0>>(__x))
        {}

      // [simd.ctor] generator constructor ------------------------------------
      template <__simd_generator_invokable<value_type, _S_size> _Fp>
        [[__gnu__::__always_inline__]]
        constexpr explicit
        basic_vec(_Fp&& __gen)
        : _M_data([&] {
            using _Arr = std::array<value_type, sizeof(_TSimd) / sizeof(value_type)>;
            const _Arr __tmp
              = _GLIBCXX_SIMD_INT_PACK(size.value, _Is, {
                  return _Arr{
                    static_cast<value_type>(__gen(__simd_size_constant<_Is>))...
                  };
                });
            return __builtin_bit_cast(_TSimd, __tmp);
          }())
        {}

      template <__almost_simd_generator_invokable<value_type, _S_size> _Fp>
        constexpr explicit
        basic_vec(_Fp&& )
          = _GLIBCXX_DELETE_MSG("Invalid return type of the generator function: "
                                "Requires value-preserving conversion or implicitly "
                                "convertible user-defined type.");

      // [simd.ctor] load constructor -----------------------------------------
      template <__complex_like _Up>
        [[__gnu__::__always_inline__]]
        constexpr
        basic_vec(_LoadCtorTag, const _Up* __ptr)
          : _M_data(_LoadCtorTag(), reinterpret_cast<const typename _Up::value_type*>(__ptr))
        {}

      template <typename _Up>
        [[__gnu__::__always_inline__]]
        constexpr
        basic_vec(_LoadCtorTag, const _Up* __ptr)
          : basic_vec(_RealSimd(_LoadCtorTag(), __ptr))
        {}

      template <__static_sized_range<size.value> _Rg, typename... _Flags>
        // FIXME: see load ctor above
        [[__gnu__::__always_inline__]]
        constexpr
        basic_vec(_Rg&& __range, flags<_Flags...> __flags = {})
          : basic_vec(_LoadCtorTag(), __flags.template _S_adjust_pointer<basic_vec>(
                                        std::ranges::data(__range)))
        {
          static_assert(__loadstore_convertible_to<std::ranges::range_value_t<_Rg>, value_type,
                                                   _Flags...>);
        }

      // [simd.ctor] complex init ---------------------------------------------
      // This uses _RealSimd as proposed in LWG4230
      [[__gnu__::__always_inline__]]
      constexpr
      basic_vec(const _RealSimd& __re, const _RealSimd& __im = {}) noexcept
        : _M_data([&](int __i) { return ((__i & 1) == 0 ? __re : __im)[__i / 2]; })
      {}

      // [simd.subscr] --------------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      { return value_type(_M_data[__i * 2], _M_data[__i * 2 + 1]); }

      // [simd.unary] unary operators -----------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator++() noexcept requires requires(value_type __a) { ++__a; }
      {
        _M_data += value_type(_T0(1));
        return *this;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator++(int) noexcept requires requires(value_type __a) { __a++; }
      {
        basic_vec __r = *this;
        _M_data += value_type(_T0(1));
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator--() noexcept requires requires(value_type __a) { --__a; }
      {
        _M_data -= value_type(_T0(1));
        return *this;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator--(int) noexcept requires requires(value_type __a) { __a--; }
      {
        basic_vec __r = *this;
        _M_data -= value_type(_T0(1));
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      operator!() const noexcept requires requires(value_type __a) { !__a; }
      { return mask_type::_S_and_neighbors(!_M_data); }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator+() const noexcept requires requires(value_type __a) { +__a; }
      { return *this; }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator-() const noexcept requires requires(value_type __a) { -__a; }
      {
        basic_vec __r = *this;
        __r._M_data = -_M_data;
        return __r;
      }

      // [simd.cassign] compound assignment -----------------------------------
#define _GLIBCXX_SIMD_DEFINE_OP(sym)                                 \
      [[__gnu__::__always_inline__]]                                 \
      friend constexpr basic_vec&                                    \
      operator sym##=(basic_vec& __x, const basic_vec& __y) noexcept \
      requires requires(value_type __a) { __a sym __a; }             \
      {                                                              \
        __x._M_data sym##= __y._M_data;                              \
        return __x;                                                  \
      }

      _GLIBCXX_SIMD_DEFINE_OP(+)
      _GLIBCXX_SIMD_DEFINE_OP(-)
      _GLIBCXX_SIMD_DEFINE_OP(*)
      _GLIBCXX_SIMD_DEFINE_OP(/)
      _GLIBCXX_SIMD_DEFINE_OP(%)
      _GLIBCXX_SIMD_DEFINE_OP(&)
      _GLIBCXX_SIMD_DEFINE_OP(|)
      _GLIBCXX_SIMD_DEFINE_OP(^)
      _GLIBCXX_SIMD_DEFINE_OP(<<)
      _GLIBCXX_SIMD_DEFINE_OP(>>)

#undef _GLIBCXX_SIMD_DEFINE_OP

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator<<=(basic_vec& __x, __simd_size_type __y) noexcept
      requires requires(value_type __a, __simd_size_type __b) { __a << __b; }
      {
        __x._M_data <<= __y;
        return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator>>=(basic_vec& __x, __simd_size_type __y) noexcept
      requires requires(value_type __a, __simd_size_type __b) { __a >> __b; }
      {
        __x._M_data >>= __y;
        return __x;
      }

      // [simd.comparison] compare operators ----------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator==(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_and_neighbors(__x._M_data == __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator!=(const basic_vec& __x, const basic_vec& __y) noexcept
      {
        return __x._M_data != __y._M_data;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<(const basic_vec& __x, const basic_vec& __y) noexcept
      {
        return __x._M_data < __y._M_data;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator<=(const basic_vec& __x, const basic_vec& __y) noexcept
      {
        return __x._M_data <= __y._M_data;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>(const basic_vec& __x, const basic_vec& __y) noexcept
      { return __y < __x; }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator>=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return __y <= __x; }

      // [simd.complex.access] complex-value accessors ------------------------
      // LWG4230: returns _RealSimd instead of auto
      [[__gnu__::__always_inline__]]
      constexpr _RealSimd
      real() const noexcept
      { return permute<_S_size>(_M_data, [](int __i) { return __i * 2; }); }

      [[__gnu__::__always_inline__]]
      constexpr _RealSimd
      imag() const noexcept
      { return permute<_S_size>(_M_data, [](int __i) { return __i * 2 + 1; }); }

      [[__gnu__::__always_inline__]]
      constexpr void
      real(const _RealSimd& __x) noexcept
      { _M_data._M_complex_set_real(__x); }

      [[__gnu__::__always_inline__]]
      constexpr void
      imag(const _RealSimd& __x) noexcept
      { _M_data._M_complex_set_imag(__x); }

      // [simd.cond] ---------------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec
      __select_impl(const mask_type& __k, const basic_vec& __t, const basic_vec& __f)
      { return _S_init(__select_impl(__k._M_data, __t._M_data, __f._M_data)); }

      // [simd.complex.math] internals ---------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr _RealSimd
      _M_abs() const; // TODO

      // associated functions
      [[__gnu__::__always_inline__]]
      constexpr _RealSimd
      _M_norm() const
      {
#if 0
        return (_M_data * _M_data)._M_hadd();
#elif 0
        const auto __squared = _M_data * _M_data;
        return permute<size.value>(
                 __squared + __squared._M_swap_neighbors(),
                 [](unsigned __i) { return __i * 2; });
#elif 0
        const auto __squared = _M_data * _M_data;
        return permute<size.value>(__squared, [](int __i) { return __i * 2; })
                 + permute<size.value>(__squared, [](int __i) { return __i * 2 + 1; });
#elif 1
        auto __re = real();
        auto __im = imag();
        return __re * __re + __im * __im;
#endif
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      _M_conj() const
      { return _S_init(_M_data._M_complex_conj()); }
    };

  // [simd.overview] deduction guide ------------------------------------------
  template <__static_sized_range _Rg, typename... _Ts>
    basic_vec(_Rg&& __r, _Ts...)
    -> basic_vec<ranges::range_value_t<_Rg>,
                 __deduce_abi_t<ranges::range_value_t<_Rg>,
#if 0 // PR117849
                                ranges::size(__r)>>;
#else
                                decltype(std::span(__r))::extent>>;
#endif

#if 1
  // FIXME: file new LWG issue about this missing deduction guide
  template <size_t _Bytes, typename _Ap>
    basic_vec(basic_mask<_Bytes, _Ap>)
    -> basic_vec<__integer_from<_Bytes>,
                 decltype(__abi_rebind<__integer_from<_Bytes>, basic_mask<_Bytes, _Ap>::size.value,
                                       _Ap>())>;
#endif

  // [simd.mask.reductions] ---------------------------------------------------
  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr bool
    all_of(const basic_mask<_Bytes, _Ap>& __k) noexcept
    { return __k._M_all_of(); }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr bool
    any_of(const basic_mask<_Bytes, _Ap>& __k) noexcept
    { return __k._M_any_of(); }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr bool
    none_of(const basic_mask<_Bytes, _Ap>& __k) noexcept
    { return __k._M_none_of(); }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr __simd_size_type
    reduce_count(const basic_mask<_Bytes, _Ap>& __k) noexcept
    {
      if constexpr (_Ap::_S_size == 1)
        return +__k[0];
      else if constexpr (__flags_test(_Ap::_S_variant, _AbiVariant::_VecMask))
        return -reduce(-__k);
      else
        return __k._M_reduce_count();
    }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr __simd_size_type
    reduce_min_index(const basic_mask<_Bytes, _Ap>& __k)
    { return __k._M_reduce_min_index(); }

  template <size_t _Bytes, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr __simd_size_type
    reduce_max_index(const basic_mask<_Bytes, _Ap>& __k)
    { return __k._M_reduce_max_index(); }

  constexpr bool
  all_of(same_as<bool> auto __x) noexcept
  { return __x; }

  constexpr bool
  any_of(same_as<bool> auto __x) noexcept
  { return __x; }

  constexpr bool
  none_of(same_as<bool> auto __x) noexcept
  { return !__x; }

  constexpr __simd_size_type
  reduce_count(same_as<bool> auto __x) noexcept
  { return __x; }

  constexpr __simd_size_type
  reduce_min_index(same_as<bool> auto __x)
  { return 0; }

  constexpr __simd_size_type
  reduce_max_index(same_as<bool> auto __x)
  { return 0; }

  // [simd.reductions] --------------------------------------------------------
  template <typename _Tp, typename _Ap, __reduction_binary_operation<_Tp> _BinaryOperation = plus<>>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce(const basic_vec<_Tp, _Ap>& __x, _BinaryOperation __binary_op = {})
    { return __x._M_reduce(__binary_op); }

  template <typename _Tp, typename _Ap, __reduction_binary_operation<_Tp> _BinaryOperation = plus<>>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce(const basic_vec<_Tp, _Ap>& __x, const typename basic_vec<_Tp, _Ap>::mask_type& __mask,
           _BinaryOperation __binary_op = {}, type_identity_t<_Tp> __identity_element
             = __default_identity_element<_Tp, _BinaryOperation>())
    { return reduce(select(__mask, __x, __identity_element), __binary_op); }

  template <totally_ordered _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce_min(const basic_vec<_Tp, _Ap>& __x) noexcept
    {
      return reduce(__x, []<typename _UV>(const _UV& __a, const _UV& __b) {
               return select(__a < __b, __a, __b);
             });
    }

  template <totally_ordered _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce_min(const basic_vec<_Tp, _Ap>& __x,
               const typename basic_vec<_Tp, _Ap>::mask_type& __mask) noexcept
    {
      return reduce(select(__mask, __x, numeric_limits<_Tp>::max()),
                    []<typename _UV>(const _UV& __a, const _UV& __b) {
                      return select(__a < __b, __a, __b);
                    });
    }

  template <totally_ordered _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce_max(const basic_vec<_Tp, _Ap>& __x) noexcept
    {
      return reduce(__x, []<typename _UV>(const _UV& __a, const _UV& __b) {
               return select(__a < __b, __b, __a);
             });
    }

  template <totally_ordered _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr _Tp
    reduce_max(const basic_vec<_Tp, _Ap>& __x,
               const typename basic_vec<_Tp, _Ap>::mask_type& __mask) noexcept
    {
      return reduce(select(__mask, __x, numeric_limits<_Tp>::lowest()),
                    []<typename _UV>(const _UV& __a, const _UV& __b) {
                      return select(__a < __b, __b, __a);
                    });
    }

  // [simd.alg] ---------------------------------------------------------------
  template<typename _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<_Tp, _Ap>
    min(const basic_vec<_Tp, _Ap>& __a, const basic_vec<_Tp, _Ap>& __b) noexcept
    { static_assert(false, "TODO"); }

  template<typename _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<_Tp, _Ap>
    max(const basic_vec<_Tp, _Ap>& __a, const basic_vec<_Tp, _Ap>& __b) noexcept
    { static_assert(false, "TODO"); }

  template<typename _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr pair<basic_vec<_Tp, _Ap>, basic_vec<_Tp, _Ap>>
    minmax(const basic_vec<_Tp, _Ap>& __a, const basic_vec<_Tp, _Ap>& __b) noexcept
    { static_assert(false, "TODO"); }

  template<typename _Tp, typename _Ap>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<_Tp, _Ap>
    clamp(const basic_vec<_Tp, _Ap>& __v, const basic_vec<_Tp, _Ap>& __lo,
          const basic_vec<_Tp, _Ap>& __hi)
    { static_assert(false, "TODO"); }

  template<typename _Tp, typename _Up>
    constexpr auto
    select(bool __c, const _Tp& __a, const _Up& __b)
    -> remove_cvref_t<decltype(__c ? __a : __b)>
    { return __c ? __a : __b; }

  template<size_t _Bytes, typename _Ap, typename _Tp, typename _Up>
    [[__gnu__::__always_inline__]]
    constexpr auto
    select(const basic_mask<_Bytes, _Ap>& __c, const _Tp& __a, const _Up& __b)
    noexcept -> decltype(__select_impl(__c, __a, __b))
    { return __select_impl(__c, __a, __b); }

  // [simd.math] --------------------------------------------------------------
  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<int, __deduced_vec_t<_Vp>>
    ilogb(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    ldexp(const _Vp& __x, const rebind_t<int, __deduced_vec_t<_Vp>>& exp)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    scalbn(const _Vp& __x, const rebind_t<int, __deduced_vec_t<_Vp>>& n)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    scalbln(const _Vp& __x, const rebind_t<long int, __deduced_vec_t<_Vp>>& n)
    { static_assert(false, "TODO"); }

  template <signed_integral T, typename Abi>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<T, Abi>
    abs(const basic_vec<T, Abi>& j)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    abs(const _Vp& j)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    fabs(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    ceil(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    floor(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    nearbyint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    rint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    rebind_t<long int, __deduced_vec_t<_Vp>>
    lrint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    rebind_t<long long int, __deduced_vec_t<_Vp>>
    llrint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    round(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<long int, __deduced_vec_t<_Vp>>
    lround(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<long long int, __deduced_vec_t<_Vp>>
    llround(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    fmod(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    trunc(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    remainder(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    copysign(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    nextafter(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    fdim(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    fmax(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    fmin(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1, typename _V2>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1, _V2>
    fma(const _V0& __x, const _V1& __y, const _V2& __z)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<int, __deduced_vec_t<_Vp>>
    fpclassify(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isfinite(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isinf(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isnan(const _Vp& __x)
    { return __x._M_isnan(); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    isnormal(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename __deduced_vec_t<_Vp>::mask_type
    signbit(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    isgreater(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    isgreaterequal(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    isless(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    islessequal(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    islessgreater(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template <typename _V0, typename _V1>
    [[__gnu__::__always_inline__]]
    constexpr typename __math_common_simd_t<_V0, _V1>::mask_type
    isunordered(const _V0& __x, const _V1& __y)
    {
      using _Vp = __math_common_simd_t<_V0, _V1>;
      if constexpr (__simd_integral<_V0> or is_integral_v<_V0>)
        return __y._M_isnan();
      else if constexpr (__simd_integral<_V1> or is_integral_v<_V1>)
        return __x._M_isnan();
      else if constexpr (not is_same_v<_V0, _Vp>)
        return isunordered(static_cast<_Vp>(__x), __y);
      else if constexpr (not is_same_v<_V1, _Vp>)
        return isunordered(__x, static_cast<_Vp>(__y));
      else
        return __x._M_isunordered(__y);
    }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    acos(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    asin(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    atan(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    atan2(const _V0& __y, const _V1& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    cos(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    sin(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    tan(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    acosh(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    asinh(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    atanh(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    cosh(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    sinh(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    tanh(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    exp(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    exp2(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    expm1(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    log(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    log10(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    log1p(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    log2(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    logb(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    cbrt(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    hypot(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1, class _V2>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1, _V2>
    hypot(const _V0& __x, const _V1& __y, const _V2& __z)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    pow(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    sqrt(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    erf(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    erfc(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    lgamma(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    tgamma(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1, class _V2>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1, _V2>
    lerp(const _V0& __a, const _V1& __b, const _V2& __t) noexcept
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    assoc_laguerre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __n,
                   const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __m, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    assoc_legendre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __l,
                   const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __m, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    beta(const _V0& __x, const _V1& __y)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    comp_ellint_1(const _Vp& __k)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    comp_ellint_2(const _Vp& __k)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    comp_ellint_3(const _V0& __k, const _V1& __nu)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    cyl_bessel_i(const _V0& __nu, const _V1& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    cyl_bessel_j(const _V0& __nu, const _V1& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    cyl_bessel_k(const _V0& __nu, const _V1& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    cyl_neumann(const _V0& __nu, const _V1& __x)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    ellint_1(const _V0& __k, const _V1& __phi)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    __math_common_simd_t<_V0, _V1>
    ellint_2(const _V0& __k, const _V1& __phi)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1, class _V2>
    __math_common_simd_t<_V0, _V1, _V2>
    ellint_3(const _V0& __k, const _V1& __nu, const _V2& __phi)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    expint(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    hermite(const rebind_t<unsigned,
                           __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    laguerre(const rebind_t<unsigned,
                            __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    legendre(const rebind_t<unsigned,
                            __deduced_vec_t<_Vp>>& __l, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    riemann_zeta(const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    sph_bessel(const rebind_t<unsigned,
                              __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    __deduced_vec_t<_Vp>
    sph_legendre(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __l,
                 const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __m, const _Vp& __theta)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp> __deduced_vec_t<_Vp>
    sph_neumann(const rebind_t<unsigned, __deduced_vec_t<_Vp>>& __n, const _Vp& __x)
    { static_assert(false, "TODO"); }

  template<__math_floating_point _Vp>
    [[__gnu__::__always_inline__]]
    constexpr __deduced_vec_t<_Vp>
    frexp(const _Vp& __value, rebind_t<int, __deduced_vec_t<_Vp>>* __exp)
    { static_assert(false, "TODO"); }

  template<class _V0, class _V1>
    [[__gnu__::__always_inline__]]
    constexpr __math_common_simd_t<_V0, _V1>
    remquo(const _V0& __x, const _V1& __y, rebind_t<int, __math_common_simd_t<_V0, _V1>>* __quo)
    { static_assert(false, "TODO"); }

  template<class T, class Abi>
    [[__gnu__::__always_inline__]]
    constexpr basic_vec<T, Abi>
    modf(const type_identity_t<basic_vec<T, Abi>>& __value, basic_vec<T, Abi>* __iptr)
    { static_assert(false, "TODO"); }

  // [simd.bit] ---------------------------------------------------------------
  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    byteswap(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    bit_ceil(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    bit_floor(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr typename _Vp::mask_type
    has_single_bit(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _V0, __simd_vec_type _V1>
    [[__gnu__::__always_inline__]]
    constexpr _V0
    rotl(const _V0& __v, const _V1& __s) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    rotl(const _Vp& __v, int __s) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _V0, __simd_vec_type _V1>
    [[__gnu__::__always_inline__]]
    constexpr _V0
    rotr(const _V0& __v, const _V1& __s) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    rotr(const _Vp& __v, int __s) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    bit_width(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_zero(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countl_one(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_zero(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    countr_one(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }

  template<__simd_vec_type _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>
    popcount(const _Vp& __v) noexcept
    { static_assert(false, "TODO"); }


  // [P3319R5] ----------------------------------------------------------------
  template <__vectorizable _Tp>
    requires is_arithmetic_v<_Tp>
    inline constexpr _Tp
    __iota<_Tp> = _Tp();

  template <typename _Tp, typename _Abi>
    inline constexpr basic_vec<_Tp, _Abi>
    __iota<basic_vec<_Tp, _Abi>> = basic_vec<_Tp, _Abi>([](_Tp __i) -> _Tp {
      static_assert (__simd_size_v<_Tp, _Abi> - 1 <= numeric_limits<_Tp>::max(),
                     "iota object would overflow");
      return __i;
    });

  // [simd.complex.math] -----------------------------------------------------
  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    real(const _Vp& __x) noexcept
    { return __x.real(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    imag(const _Vp& __x) noexcept
    { return __x.imag(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    abs(const _Vp& __x) noexcept
    { return __x._M_abs(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    arg(const _Vp& __x) noexcept
    { return __x._M_arg(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr rebind_t<__simd_complex_value_type<_Vp>, _Vp>
    norm(const _Vp& __x) noexcept
    { return __x._M_norm(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    conj(const _Vp& __x) noexcept
    { return __x._M_conj(); }

  template <__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    proj(const _Vp& __x) noexcept
    { return __x._M_proj(); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    exp(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    log(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    log10(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    sqrt(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    sin(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    asin(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    cos(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    acos(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    tan(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    atan(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    sinh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    asinh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    cosh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    acosh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    tanh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    atanh(const _Vp& __v)
    { static_assert(false, "TODO"); }

  template<__simd_floating_point _Vp>
    rebind_t<complex<typename _Vp::value_type>, _Vp>
    polar(const _Vp& __x, const _Vp& __y = {})
    { static_assert(false, "TODO"); }

  template<__simd_complex _Vp>
    [[__gnu__::__always_inline__]]
    constexpr _Vp
    pow(const _Vp& __x, const _Vp& __y)
    { static_assert(false, "TODO"); }
}

namespace std
{
    // See [simd.alg], algorithms
  using simd::min;
  using simd::max;
  using simd::minmax;
  using simd::clamp;

  // See [simd.math], mathematical functions
  using simd::acos;
  using simd::asin;
  using simd::atan;
  using simd::atan2;
  using simd::cos;
  using simd::sin;
  using simd::tan;
  using simd::acosh;
  using simd::asinh;
  using simd::atanh;
  using simd::cosh;
  using simd::sinh;
  using simd::tanh;
  using simd::exp;
  using simd::exp2;
  using simd::expm1;
  using simd::frexp;
  using simd::ilogb;
  using simd::ldexp;
  using simd::log;
  using simd::log10;
  using simd::log1p;
  using simd::log2;
  using simd::logb;
  using simd::modf;
  using simd::scalbn;
  using simd::scalbln;
  using simd::cbrt;
  using simd::abs;
  using simd::abs;
  using simd::fabs;
  using simd::hypot;
  using simd::pow;
  using simd::sqrt;
  using simd::erf;
  using simd::erfc;
  using simd::lgamma;
  using simd::tgamma;
  using simd::ceil;
  using simd::floor;
  using simd::nearbyint;
  using simd::rint;
  using simd::lrint;
  using simd::llrint;
  using simd::round;
  using simd::lround;
  using simd::llround;
  using simd::trunc;
  using simd::fmod;
  using simd::remainder;
  using simd::remquo;
  using simd::copysign;
  using simd::nextafter;
  using simd::fdim;
  using simd::fmax;
  using simd::fmin;
  using simd::fma;
  using simd::lerp;
  using simd::fpclassify;
  using simd::isfinite;
  using simd::isinf;
  using simd::isnan;
  using simd::isnormal;
  using simd::signbit;
  using simd::isgreater;
  using simd::isgreaterequal;
  using simd::isless;
  using simd::islessequal;
  using simd::islessgreater;
  using simd::isunordered;
  using simd::assoc_laguerre;
  using simd::assoc_legendre;
  using simd::beta;
  using simd::comp_ellint_1;
  using simd::comp_ellint_2;
  using simd::comp_ellint_3;
  using simd::cyl_bessel_i;
  using simd::cyl_bessel_j;
  using simd::cyl_bessel_k;
  using simd::cyl_neumann;
  using simd::ellint_1;
  using simd::ellint_2;
  using simd::ellint_3;
  using simd::expint;
  using simd::hermite;
  using simd::laguerre;
  using simd::legendre;
  using simd::riemann_zeta;
  using simd::sph_bessel;
  using simd::sph_legendre;
  using simd::sph_neumann;

  // See [simd.bit], bit manipulation
  using simd::byteswap;
  using simd::bit_ceil;
  using simd::bit_floor;
  using simd::has_single_bit;
  using simd::rotl;
  using simd::rotr;
  using simd::bit_width;
  using simd::countl_zero;
  using simd::countl_one;
  using simd::countr_zero;
  using simd::countr_one;
  using simd::popcount;

  // See [simd.complex.math], vec complex math
  using simd::real;
  using simd::imag;
  using simd::arg;
  using simd::norm;
  using simd::conj;
  using simd::proj;
  using simd::polar;
}

#pragma GCC diagnostic pop

#endif  // BITS_SIMD_VEC_H_
