/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef INCLUDE_BITS_SIMD_MASK_H_
#define INCLUDE_BITS_SIMD_MASK_H_

#include "simd_iterator.h"
#include "vec_ops.h"
#if _GLIBCXX_SIMD_HAVE_SSE
#include "simd_x86.h"
#endif

#include <bit>
#include <bitset>

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

namespace std::simd
{
  // [simd.traits]
  // --- rebind ---
  template <typename _Tp, typename _Vp, _ArchTraits _Traits = {}>
    struct rebind
    {};

  template <__vectorizable _Tp, __simd_vec_type _Vp, _ArchTraits _Traits>
    //requires requires { typename __deduce_abi_t<_Tp, _Vp::size()>; }
    struct rebind<_Tp, _Vp, _Traits>
    {
      using _A1 = decltype(__abi_rebind<_Tp, _Vp::size(), typename _Vp::abi_type>());

      using type = basic_vec<_Tp, _A1>;
    };

  template <__vectorizable _Tp, __simd_mask_type _Mp, _ArchTraits _Traits>
    //requires requires { typename __deduce_abi_t<_Tp, _Mp::size()>; }
    struct rebind<_Tp, _Mp, _Traits>
    {
      using _A1 = decltype(__abi_rebind<_Tp, _Mp::size(), typename _Mp::abi_type>());

      using type = basic_mask<sizeof(_Tp), _A1>;
    };

  template <typename _Tp, typename _Vp>
    using rebind_t = typename rebind<_Tp, _Vp>::type;

  // --- resize ---
  template <__simd_size_type _Np, typename _Vp, _ArchTraits _Traits = {}>
    struct resize
    {};

  template <__simd_size_type _Np, __data_parallel_type _Vp, _ArchTraits _Traits>
    requires requires { typename _Vp::mask_type; }
    //requires requires { typename __deduce_abi_t<typename _Vp::value_type, _Np>; }
    struct resize<_Np, _Vp, _Traits>
    {
      using _A1 = decltype(__abi_rebind<typename _Vp::value_type, _Np, typename _Vp::abi_type>());

      using type = basic_vec<typename _Vp::value_type, _A1>;
    };

  template <__simd_size_type _Np, __simd_mask_type _Mp, _ArchTraits _Traits>
    //requires requires { typename __deduce_abi_t<typename _Mp::value_type, _Np>; }
    struct resize<_Np, _Mp, _Traits>
    {
      using _A1 = decltype(__abi_rebind<__mask_element_size<_Mp>, _Np, typename _Mp::abi_type,
                                        true>());

      using type = basic_mask<__mask_element_size<_Mp>, _A1>;
    };

  template <__simd_size_type _Np, typename _Vp>
    using resize_t = typename resize<_Np, _Vp>::type;

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
        else if constexpr (_S_use_bitmask and _S_size < __CHAR_BIT__)
          return __CHAR_BIT__;
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

      template <_ArchTraits _Traits = {}>
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
              if constexpr (_S_size <= 32 and _Traits._M_have_bmi2())
                return __builtin_ia32_bzhi_si(~0u >> (32 - _S_size), unsigned(__n));
#endif
#if __has_builtin(__builtin_ia32_bzhi_di)
              if constexpr (_S_size <= 64 and _Traits._M_have_bmi2())
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
                       if constexpr (not _S_is_cx_ileav)
                         return _InvalidAbi();
                       else if constexpr (is_same_v<_Ap, _ScalarAbi<_S_size>>)
                         return _ScalarAbi<_S_size * 2>();
                       else // _Ap is _Abi<_S_size, 1, Something>
                         return _Abi<_S_size * 2, 1,
                                     _Ap::_S_variant & _AbiVariant::_MaskVariants>();
                     }())>;

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_and_neighbors(const _CxElementMask& __k)
      {
        static_assert(_S_is_cx_ileav);
        static_assert(not _CxElementMask::_S_is_cx_ileav);
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
        static_assert(_S_is_cx_ileav);
        static_assert(not _CxElementMask::_S_is_cx_ileav);
        basic_mask __r;
        if constexpr (_S_use_bitmask)
          __r._M_data = __k._M_data | (((__k._M_data >> 1) & 0x5555'5555'5555'5555ull)
                                         | ((__k._M_data << 1) & ~0x5555'5555'5555'5555ull));
        else
          __r._M_data = __vec_or(__k._M_data, _VecOps<_DataType>::_S_swap_neighbors(__k._M_data));
        return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr _CxElementMask
      _M_unwrap_complex() const
      { return __builtin_bit_cast(_CxElementMask, _M_data); }

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
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      __select_impl(const basic_mask& __k, const basic_mask& __t, const basic_mask& __f) noexcept
      {
        if constexpr (not _S_use_bitmask)
          return __k._M_data ? __t._M_data : __f._M_data;
        else
          return (__k._M_data & __t._M_data) | (~__k._M_data & __f._M_data);
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      __select_impl(const basic_mask& __k, same_as<bool> auto __t, same_as<bool> auto __f) noexcept
      {
        if (__t == __f)
          return basic_mask(__t);
        else
          return __t ? __k : !__k;
      }

      template <__vectorizable _T0, same_as<_T0> _T1>
        requires (sizeof(_T0) == _Bytes)
        [[__gnu__::__always_inline__]]
        friend constexpr vec<_T0, _S_size>
        __select_impl(const basic_mask& __k, const _T0& __t, const _T1& __f) noexcept
        {
          if constexpr (not _S_use_bitmask)
            return __k._M_data ? __t : __f;
          else
            return __select_impl(__k, vec<_T0, _S_size>(__t), vec<_T1, _S_size>(__f));
        }

      // [simd.mask.reductions] implementation --------------------------------
      template <_ArchTraits _Traits = {}>
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

      template <_ArchTraits _Traits = {}>
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

      template <_ArchTraits _Traits = {}>
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

      template <typename, typename>
        friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr bool _S_is_cx_ileav = __flags_test(_Ap::_S_variant, _AbiVariant::_CxIleav);

      static constexpr int _N0 = __bit_ceil(unsigned(_S_size)) / 2;

      static constexpr int _N1 = _S_size - _N0;

      static constexpr int _Nreg0 = __bit_ceil(unsigned(_Ap::_S_nreg)) / 2;

      static constexpr int _Nreg1 = _Ap::_S_nreg - _Nreg0;

      using _Mask0 = basic_mask<_Bytes, decltype([] consteval {
                                          if constexpr (is_same_v<_Ap, _ScalarAbi<_S_size>>)
                                            return _ScalarAbi<_N0>();
                                          else if constexpr (_N0 > 1)
                                            return _Abi<_N0, _Nreg0, _Ap::_S_variant>();
                                          else
                                            return __abi_rebind<_Bytes, _N0, _Ap, true>();
                                        }())>;

      using _Mask1 = basic_mask<_Bytes, decltype([] consteval {
                                          if constexpr (is_same_v<_Ap, _ScalarAbi<_S_size>>)
                                            return _ScalarAbi<_N1>();
                                          else if constexpr (_N1 > 1)
                                            return _Abi<_N1, _Nreg1, _Ap::_S_variant>();
                                          else
                                            return __abi_rebind<_Bytes, _N1, _Ap, true>();
                                        }())>;

      // _Ap::_S_nreg determines how deep the recursion goes. E.g. basic_mask<4, _Abi<8, 4>> cannot
      // use basic_mask<4, _Abi<4, 1>> as _Mask0/1 types.
      static_assert(_Mask0::abi_type::_S_nreg + _Mask1::abi_type::_S_nreg == _Ap::_S_nreg);

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

      template <_ArchTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        static constexpr basic_mask
        _S_partial_mask_of_n(int __n)
        {
#if __has_builtin(__builtin_ia32_bzhi_di)
          if constexpr (_S_use_bitmask and _S_size <= 64 and _Traits._M_have_bmi2())
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
        = basic_mask<_Bytes / 2,
                     decltype([] consteval {
                       if constexpr (not _S_is_cx_ileav)
                         {
                           if constexpr (is_same_v<_Ap, _ScalarAbi<_S_size>>)
                             return _ScalarAbi<_S_size * 2>();
                           else
                             return _InvalidAbi();
                         }
                       else // _Ap is _Abi<_S_size, 1, Something>
                         return _Abi<_S_size * 2, _Ap::_S_nreg,
                                     _Ap::_S_variant & _AbiVariant::_MaskVariants>();
                     }())>;


      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_and_neighbors(const _CxElementMask& __k)
      {
        static_assert(_S_is_cx_ileav);
        static_assert(not _CxElementMask::_S_is_cx_ileav);
        return _S_init(_Mask0::_S_and_neighbors(__k._M_data0),
                       _Mask1::_S_and_neighbors(__k._M_data1));
      }

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_or_neighbors(const _CxElementMask& __k)
      {
        static_assert(_S_is_cx_ileav);
        static_assert(not _CxElementMask::_S_is_cx_ileav);
        return _S_init(_Mask0::_S_or_neighbors(__k._M_data0),
                       _Mask1::_S_or_neighbors(__k._M_data1));
      }

      [[__gnu__::__always_inline__]]
      constexpr _CxElementMask
      _M_unwrap_complex() const
      {
        return _CxElementMask::_S_init(_M_data0._M_unwrap_complex(),
                                       _M_data1._M_unwrap_complex());
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
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      __select_impl(const basic_mask& __k, const basic_mask& __t, const basic_mask& __f) noexcept
      {
        return _S_init(__select_impl(__k._M_data0, __t._M_data0, __f._M_data0),
                       __select_impl(__k._M_data1, __t._M_data1, __f._M_data1));
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      __select_impl(const basic_mask& __k, same_as<bool> auto __t, same_as<bool> auto __f) noexcept
      {
        if (__t == __f)
          return basic_mask(__t);
        else
          return __t ? __k : !__k;
      }

      template <__vectorizable _T0, same_as<_T0> _T1>
        requires (sizeof(_T0) == _Bytes)
        [[__gnu__::__always_inline__]]
        friend constexpr vec<_T0, _S_size>
        __select_impl(const basic_mask& __k, const _T0& __t, const _T1& __f) noexcept
        {
          return vec<_T0, _S_size>::_S_init(__select_impl(__k._M_data0, __t, __f),
                                            __select_impl(__k._M_data1, __t, __f));
        }

      template <_ArchTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr bool
        _M_all_of() const
        {
          if constexpr (_N0 == _N1)
            return (_M_data0 and _M_data1)._M_all_of();
          else
            return _M_data0._M_all_of() and _M_data1._M_all_of();
        }

      template <_ArchTraits _Traits = {}>
        [[__gnu__::__always_inline__]]
        constexpr bool
        _M_any_of() const
        {
          if constexpr (_N0 == _N1)
            return (_M_data0 or _M_data1)._M_any_of();
          else
            return _M_data0._M_any_of() or _M_data1._M_any_of();
        }

      template <_ArchTraits _Traits = {}>
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
}

#pragma GCC diagnostic pop

#endif  // INCLUDE_BITS_SIMD_MASK_H_
