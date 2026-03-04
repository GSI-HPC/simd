/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* Copyright © 2025–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef _GLIBCXX_SIMD_COMPLEX_H
#define _GLIBCXX_SIMD_COMPLEX_H 1

#ifdef _GLIBCXX_SYSHDR
#pragma GCC system_header
#endif

#if __cplusplus >= 202400L

#include "simd_vec.h"

// psabi warnings are bogus because the ABI of the internal types never leaks into user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpsabi"

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION
namespace simd
{
  /** \internal
   * The mask type, whose _DataType member is \p _Mp.
   */
  template <__simd_mask_type _Mp>
    requires (_Mp::size() % 2 == 0)
      && (__filter_abi_variant(_Mp::abi_type::_S_variant, _AbiVariant::_CxVariants)
	    == _AbiVariant())
    using __cx_ileav_mask
      = basic_mask<__mask_element_size<_Mp> * 2,
		   _Abi_t<_Mp::size() / 2, _Mp::abi_type::_S_nreg,
			  _Mp::abi_type::_S_variant, _AbiVariant::_CxIleav>>;

  template <__simd_mask_type _Mp>
    [[__gnu__::__always_inline__]]
    constexpr __cx_ileav_mask<_Mp>
    __to_cx_ileav(const _Mp& __k)
    { return __cx_ileav_mask<_Mp>::_S_init(__k); }

  constexpr void
  __check_hi_bits_for_zero(unsigned_integral auto __x)
  {
    __glibcxx_simd_precondition(__x == 0,
				"to_ullong called on mask with 'true' elements at indices"
				"higher than 64");
  }

  template <typename _T0, typename _T1>
    constexpr void
    __check_hi_bits_for_zero(const __trivial_pair<_T0, _T1>& __p)
    {
      __check_hi_bits_for_zero(__p._M_first);
      __check_hi_bits_for_zero(__p._M_second);
    }

  constexpr unsigned long long
  __unwrap_pairs_to_ullong(unsigned_integral auto __x)
  { return __x; }

  template <typename _T0, typename _T1>
    constexpr unsigned long long
    __unwrap_pairs_to_ullong(const __trivial_pair<_T0, _T1>& __p)
    {
      __check_hi_bits_for_zero(__p._M_second);
      return __unwrap_pairs_to_ullong(__p._M_first);
    }

  template <int _Np>
    constexpr bitset<_Np>
    __unwrap_pairs_to_bitset(unsigned_integral auto __x)
    {
      static_assert(_Np <= 64);
      return __x;
    }

  template <size_t _Np, typename _T0, typename _T1>
    constexpr bitset<_Np>
    __unwrap_pairs_to_bitset(const __trivial_pair<_T0, _T1>& __p)
    {
      constexpr size_t _N0 = __bit_floor(_Np);
      constexpr size_t _N1 = _Np - _N0;
      static_assert(_N0 % 64 == 0);
      struct _Tmp
      {
	bitset<__bit_floor(_Np)> _M_lo;
	bitset<_Np - __bit_floor(_Np)> _M_hi;
      };
      _Tmp __tmp = {__unwrap_pairs_to_bitset<_N0>(__p._M_first),
		    __unwrap_pairs_to_bitset<_N1>(__p._M_second)};
      return __builtin_bit_cast(bitset<_Np>, __tmp);
    }

  template <size_t _Bytes>
    consteval auto
    __tree_of_ulong()
    {
      static constexpr size_t _N0 = __bit_floor(_Bytes - 1);
      static constexpr size_t _N1 = _Bytes - _N0;
      if constexpr (_Bytes <= sizeof(unsigned long))
	return 0ul;
      else
	return __trivial_pair {__tree_of_ulong<_N0>(), __tree_of_ulong<_N1>()};
    }

  template <size_t _Bytes>
    using __tree_of_ulong_t = decltype(__tree_of_ulong<_Bytes>());

  template <size_t _Np>
    constexpr auto
    __bitset_to_pairs(const bitset<_Np>& __b) noexcept
    {
      if constexpr (_Np <= 64)
	return __b.to_ullong();
      else
	return __builtin_bit_cast(__tree_of_ulong_t<__div_ceil(_Np, size_t(__CHAR_BIT__))>, __b);
    }

  template <size_t _Bytes, __abi_tag _Ap>
    requires _Ap::_S_is_cx_ileav
    class basic_mask<_Bytes, _Ap> : public _MaskBase<_Bytes, _Ap>
    {
      using _Base = _MaskBase<_Bytes, _Ap>;

      template <size_t, typename>
	friend class basic_mask;

      template <typename, typename>
	friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      using _DataType
	= basic_mask<_Bytes / 2, _Abi_t<_S_size * 2, _Ap::_S_nreg,
					__filter_abi_variant(_Ap::_S_variant,
							     _AbiVariant::_MaskVariants)>>;

      static_assert(_DataType::abi_type::_S_nreg == _Ap::_S_nreg);

      static_assert(is_same_v<__cx_ileav_mask<_DataType>, basic_mask>);

      using _VecType = __similar_vec<__integer_from<_Bytes>, _S_size, _Ap>;

      static constexpr bool _S_is_scalar = _DataType::_S_is_scalar;

      // Interleaved storage requires at least two vector elements and therefore cannot ever be
      // scalar.
      static_assert(!_S_is_scalar);

      static constexpr bool _S_use_bitmask = _DataType::_S_use_bitmask;

      static constexpr int _S_full_size = _DataType::_S_full_size / 2;

      static constexpr bool _S_is_partial = _DataType::_S_is_partial;

      static constexpr bool _S_has_bool_member = _DataType::_S_has_bool_member;

      // same as for _S_is_scalar
      static_assert(!_S_has_bool_member);

      static constexpr size_t _S_padding_bytes = _DataType::_S_padding_bytes;

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

      static constexpr auto size = __simd_size_c<_S_size>;

      // internal but public API ----------------------------------------------
      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_init(const _DataType& __x)
      {
	basic_mask __r;
	__r._M_data = __x;
	return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_concat_data() const
      { return _M_data._M_concat_data(); }

      template <_ArchTraits _Traits = {}>
	[[__gnu__::__always_inline__]]
	static constexpr basic_mask
	_S_partial_mask_of_n(int __n)
	{ return _S_init(_DataType::_S_partial_mask_of_n(__n * 2)); }

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_and_neighbors(_DataType __k)
      { return _S_init(__k._M_and_neighbors()); }

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_or_neighbors(_DataType __k)
      { return _S_init(__k._M_or_neighbors()); }

      template <typename _Mp>
	[[__gnu__::__always_inline__]]
	constexpr auto
	_M_chunk() const noexcept
	{
	  if constexpr (_Mp::abi_type::_S_variant != _Ap::_S_variant)
	    {
	      using _M2 = resize_t<_S_size, _Mp>;
	      static_assert(!is_same_v<_M2, basic_mask>);
	      return static_cast<_M2>(*this).template _M_chunk<_Mp>();
	    }
	  else // _Mp is the same partial specialization
	    {
	      constexpr int __rem = _S_size % _Mp::_S_size;
	      const auto [...__xs] = _M_data.template _M_chunk<typename _Mp::_DataType>();
	      static_assert(is_same_v<decltype(__to_cx_ileav(__xs...[0])), _Mp>);
	      if constexpr (__rem == 0)
		return array{__to_cx_ileav(__xs)...};
	      else
		return tuple(__to_cx_ileav(__xs)...);
	    }
	}

      [[__gnu__::__always_inline__]]
      static constexpr const basic_mask&
      _S_concat(const basic_mask& __x0) noexcept
      { return __x0; }

      template <typename... _As>
	requires (sizeof...(_As) > 1)
	[[__gnu__::__always_inline__]]
	static constexpr basic_mask
	_S_concat(const basic_mask<_Bytes, _As>&... __xs) noexcept
	{ return basic_mask::_S_init(_DataType::_S_concat(__xs._M_data...)); }

      // [simd.mask.overview] default constructor -----------------------------
      basic_mask() = default;

      // [simd.mask.overview] conversion extensions ---------------------------
      template <__vec_builtin _TV>
	[[__gnu__::__always_inline__]]
	constexpr
	basic_mask(const _TV& __x) requires convertible_to<_TV, _DataType>
	: _M_data(__x)
	{}

      template <__vec_builtin _TV>
	[[__gnu__::__always_inline__]]
	constexpr
	operator _TV() requires convertible_to<_DataType, _TV>
	{ return _M_data; }

      // [simd.mask.ctor] broadcast constructor -------------------------------
      [[__gnu__::__always_inline__]]
      constexpr explicit
      basic_mask(same_as<bool> auto __x) noexcept // LWG 4382.
	: _M_data(__x)
      {}

      // [simd.mask.ctor] conversion constructor ------------------------------
      template <size_t _UBytes, typename _UAbi>
	requires (_S_size == _UAbi::_S_size)
	[[__gnu__::__always_inline__]]
	constexpr explicit(__is_mask_conversion_explicit<_Ap, _UAbi>(_Bytes, _UBytes))
	basic_mask(const basic_mask<_UBytes, _UAbi>& __x) noexcept
	  : _M_data([&] {
	      using _UV = basic_mask<_UBytes, _UAbi>;
	      if constexpr (_UV::_S_is_scalar)
		return _DataType([&](int __i) { return __x[__i / 2]; });
	      else if constexpr (_UAbi::_S_is_cx_ileav)
		return __x._M_data; // calls conversion ctor on _DataType
	      else if constexpr (_S_use_bitmask || _UV::_S_use_bitmask)
		return _DataType::_S_init(__duplicate_each_bit<_S_size>(__x._M_to_uint()));
	      else if constexpr (_UAbi::_S_is_cx_ctgus)
		return basic_mask(__x._M_data)._M_data;
	      else if constexpr (sizeof(__x) == sizeof(_M_data) && _Bytes == _UBytes
				   && _UV::_S_padding_bytes == 0)
		{
		  static_assert(!_S_has_bool_member && !_UV::_S_has_bool_member);
		  return __builtin_bit_cast(_DataType, __x);
		}
	      else if constexpr (_Bytes <= 8)
		{
		  using _U2 = __similar_mask<__integer_from<_Bytes>, _S_size, _UAbi>;
		  if constexpr (_U2::_S_has_bool_member)
		    return _DataType::_S_recursive_bit_cast(_U2(__x));
		  else if constexpr (sizeof(_DataType) == sizeof(_U2))
		    return __builtin_bit_cast(_DataType, _U2(__x));
		  else
		    { // the sizeof can differ e.g. on mask<float, 18> -> mask<complex<float>, 18>
		      using _U3 = __similar_mask<__integer_from<_Bytes / 2>, _S_size * 2,
						 typename _U2::abi_type>;
		      static_assert(sizeof(_U3) == sizeof(_U2));
		      return __builtin_bit_cast(_U3, _U2(__x));
		    }
		}
	      else if constexpr (_UBytes > 1) // call conversion ctor on _DataType
		{
		  using _U2 = __similar_mask<__integer_from<_UBytes / 2>, _S_size * 2, _UAbi>;
		  return _U2::_S_recursive_bit_cast(__x);
		}
	      else if constexpr (_Bytes >= 4)
		{
		  // 1. convert to larger mask type
		  auto __y = __similar_mask<__integer_from<_Bytes / 2>, _S_size, _UAbi>(__x);
		  // 2. reinterpret to pass to conversion constructor of _DataType
		  using _U2 = __similar_mask<__integer_from<_Bytes / 2 / 2>, _S_size * 2, _UAbi>;
		  return _U2::_S_recursive_bit_cast(__y);
		}
	      else
		static_assert(false);
	  }())
	{}

      using _Base::_MaskBase;

      // [simd.mask.ctor] generator constructor -------------------------------
      template <__simd_generator_invokable<bool, _S_size> _Fp>
	[[__gnu__::__always_inline__]]
	constexpr explicit
	basic_mask(_Fp&& __gen)
	: _M_data([&] [[__gnu__::__always_inline__]] {
	    // for _CxIleav, the results of each __gen call need to initialize two
	    // neighboring elements
	    constexpr auto [...__is] = _IotaArray<_S_size>;
	    bool __tmp[_S_size] = {__gen(__simd_size_c<__is>)...};
	    return _DataType([&] [[__gnu__::__always_inline__]] (size_t __i) {
		     return __tmp[__i / 2];
		   });
	  }())
	{}

      // [simd.mask.ctor] bitset constructor ----------------------------------
      [[__gnu__::__always_inline__]]
      constexpr
      basic_mask(const same_as<bitset<_S_size>> auto& __b) noexcept // LWG 4382.
      : _M_data(_DataType::_S_init(__duplicate_each_bit<_S_size>(__bitset_to_pairs(__b))))
      {}

      // [simd.mask.ctor] uint constructor ------------------------------------
      template <unsigned_integral _Tp>
	requires (!same_as<_Tp, bool>) // LWG 4382.
	[[__gnu__::__always_inline__]]
	constexpr explicit
	basic_mask(_Tp __val) noexcept
	: _M_data(__duplicate_each_bit<_S_size>(__val))
	{}

      // [simd.mask.subscr] ---------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      { return _M_data[__i * 2]; }

      // [simd.mask.unary] ----------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr basic_mask
      operator!() const noexcept
      { return _S_init(!_M_data); }

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator+() const noexcept requires destructible<_VecType>
      { return operator _VecType(); }

      constexpr _VecType
      operator+() const noexcept = delete;

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator-() const noexcept requires destructible<_VecType>
      {
	using _Ip = typename _VecType::value_type;
	if constexpr (_S_use_bitmask)
	  return __select_impl(*this, _Ip(-1), _Ip());
	else
	  return __builtin_bit_cast(_VecType, -_M_data);
      }

      constexpr _VecType
      operator-() const noexcept = delete;

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator~() const noexcept requires destructible<_VecType>
      {
	using _Ip = typename _VecType::value_type;
	if constexpr (_S_use_bitmask)
	  return __select_impl(*this, _Ip(-2), _Ip(-1));
	else
	  return __builtin_bit_cast(_VecType, _M_data) - _Ip(1);
      }

      constexpr _VecType
      operator~() const noexcept = delete;

      // [simd.mask.conv] -----------------------------------------------------
      template <typename _Up, typename _UAbi>
	requires (_UAbi::_S_size == _S_size)
	[[__gnu__::__always_inline__]]
	constexpr explicit(sizeof(_Up) != _Bytes)
	operator basic_vec<_Up, _UAbi>() const noexcept
	{
	  using _Mp = typename basic_vec<_Up, _UAbi>::mask_type;
	  return __select_impl(_Mp(*this), basic_vec<_Up, _UAbi>(1), basic_vec<_Up, _UAbi>(0));
	}

      using _Base::operator basic_vec;

      // [simd.mask.namedconv] ------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr bitset<_S_size>
      to_bitset() const noexcept
      { return __unwrap_pairs_to_bitset<_S_size>(_M_to_uint()); }

      template <int _Offset = 0, _ArchTraits _Traits = {}>
	[[__gnu__::__always_inline__]]
	constexpr auto
	_M_to_uint() const
	{ return _M_data.template _M_to_uint<_Offset, true>(); }

      [[__gnu__::__always_inline__]]
      constexpr unsigned long long
      to_ullong() const
      { return __unwrap_pairs_to_ullong(_M_to_uint()); }

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
      { return _S_init(__x._M_data == __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator!=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data != __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator>=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data >= __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator<=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data <= __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator>(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data > __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator<(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data < __y._M_data); }

      // [simd.mask.cond] -----------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      __select_impl(const basic_mask& __k, const basic_mask& __t, const basic_mask& __f) noexcept
      { return _S_init(__select_impl(__k._M_data, __t._M_data, __f._M_data)); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      __select_impl(const basic_mask& __k, same_as<bool> auto __t, same_as<bool> auto __f) noexcept
      { return _S_init(__select_impl(__k._M_data, __t, __f)); }

      template <__vectorizable _T0, same_as<_T0> _T1>
	requires (sizeof(_T0) == _Bytes)
	[[__gnu__::__always_inline__]]
	friend constexpr vec<_T0, _S_size>
	__select_impl(const basic_mask& __k, const _T0& __t, const _T1& __f) noexcept
	{
	  using _Vp = vec<_T0, _S_size>;
	  return __select_impl(static_cast<typename _Vp::mask_type>(__k), _Vp(__t), _Vp(__f));
	}

      // [simd.mask.reductions] implementation --------------------------------
      [[__gnu__::__always_inline__]]
      constexpr bool
      _M_all_of() const noexcept
      { return _M_data._M_all_of(); }

      [[__gnu__::__always_inline__]]
      constexpr bool
      _M_any_of() const noexcept
      { return _M_data._M_any_of(); }

      [[__gnu__::__always_inline__]]
      constexpr bool
      _M_none_of() const noexcept
      { return _M_data._M_none_of(); }

      [[__gnu__::__always_inline__]]
      constexpr __simd_size_type
      _M_reduce_count() const noexcept
      { return _M_data._M_reduce_count() / 2; }

      [[__gnu__::__always_inline__]]
      constexpr __simd_size_type
      _M_reduce_min_index() const
      { return _M_data._M_reduce_min_index() / 2; }

      [[__gnu__::__always_inline__]]
      constexpr __simd_size_type
      _M_reduce_max_index() const
      { return _M_data._M_reduce_max_index() / 2; }

      [[__gnu__::__always_inline__]]
      friend constexpr bool
      __is_const_known(const basic_mask& __x)
      { return __is_const_known(__x._M_data); }
    };

  template <__vectorizable _Tp, __abi_tag _Ap>
    requires __complex_like<_Tp>
      && _Ap::_S_is_cx_ileav
    class basic_vec<_Tp, _Ap>
    : _VecBase<_Tp, _Ap>
    {
      template <typename, typename>
	friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr int _S_full_size = __bit_ceil(unsigned(_S_size));

      using _T0 = typename _Tp::value_type;

      using _TSimd = __similar_vec<_T0, 2 * _S_size, _Ap>;

      using _RealSimd = __similar_vec<_T0, _S_size, _Ap>;

      _TSimd _M_data = {};

      static constexpr bool _S_use_bitmask = _Ap::_S_is_bitmask;

      static constexpr bool _S_is_partial = sizeof(_M_data) > sizeof(_Tp) * _S_size;

      [[__gnu__::__always_inline__]]
      static constexpr basic_vec
      _S_init(const _TSimd& __x)
      {
	basic_vec __r;
	__r._M_data = __x;
	return __r;
      }

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

      static constexpr auto size = __simd_size_c<_S_size>;

      // internal but public API ----------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr const _TSimd&
      _M_get_ileav() const
      { return _M_data; }

      [[__gnu__::__always_inline__]]
      constexpr const auto&
      _M_get_low() const requires (_Ap::_S_nreg >= 2)
      { return _M_data._M_get_low(); }

      [[__gnu__::__always_inline__]]
      constexpr const auto&
      _M_get_high() const requires (_Ap::_S_nreg >= 2)
      { return _M_data._M_get_high(); }
#if VIR_PATCH_PERMUTE_DYNAMIC

      template <typename _Up, typename _UAbi>
	[[__gnu__::__always_inline__]]
	static constexpr basic_vec
	_S_recursive_bit_cast(const basic_vec<_Up, _UAbi>& __x)
	{ return _S_init(_TSimd::_S_recursive_bit_cast(__x)); }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_concat_data(bool __do_sanitize = false) const
      { return _M_data._M_concat_data(__do_sanitize); }
#endif

      [[__gnu__::__always_inline__]]
      friend constexpr bool
      __is_const_known(const basic_vec& __x)
      { return __is_const_known(__x._M_data); }

      template <typename _Vp>
	[[__gnu__::__always_inline__]]
	constexpr auto
	_M_chunk() const noexcept
	{
	  if constexpr (_Vp::abi_type::_S_is_cx_ileav)
	    {
	      constexpr int __n = _S_size / _Vp::_S_size;
	      constexpr int __rem = _S_size % _Vp::_S_size;
	      const auto __chunked = _M_data.template _M_chunk<typename _Vp::_TSimd>();
	      constexpr auto [...__is] = _IotaArray<__n>;
	      if constexpr (__rem == 0)
		return array<_Vp, __n> {_Vp::_S_init(__chunked[__is])...};
	      else
		{
		  using _Rest = resize_t<__rem, _Vp>;
		  return tuple(_Vp::_S_init(get<__is>(__chunked))...,
			       _Rest::_S_init(get<__n>(__chunked)));
		}
	    }
	  else
	    return resize_t<_S_size, _Vp>(*this).template _M_chunk<_Vp>();
	}

      [[__gnu__::__always_inline__]]
      static constexpr const basic_vec&
      _S_concat(const basic_vec& __x0) noexcept
      { return __x0; }

      template <typename... _As>
	requires (sizeof...(_As) > 1)
	[[__gnu__::__always_inline__]]
	static constexpr basic_vec
	_S_concat(const basic_vec<value_type, _As>&... __xs) noexcept
	{ return basic_vec::_S_init(_TSimd::_S_concat(__xs._M_data...)); }

      template <typename _BinaryOp>
	[[__gnu__::__always_inline__]]
	constexpr auto
	_M_reduce_to_register(_BinaryOp __binary_op) const
	{
	  if constexpr (_TSimd::abi_type::_S_nreg == 1)
	    return *this;
	  else
	    {
	      auto [__lo, __hi] = _M_chunk<resize_t<__bit_ceil(unsigned(_S_size)) / 2,
						    basic_vec>>();
	      auto __a = __lo._M_reduce_to_register(__binary_op);
	      auto __b = __hi._M_reduce_to_register(__binary_op);
	      if constexpr (__a._S_size == __b._S_size)
		return __binary_op(__a, __b);
	      else
		{
		  using _V1 = resize_t<1, basic_vec>;
		  return __binary_op(_V1(__a._M_reduce(__binary_op)),
				     _V1(__b._M_reduce(__binary_op)));
		}
	    }
	}

      template <typename _BinaryOp, _ArchTraits _Traits = {}>
	[[__gnu__::__always_inline__]]
	constexpr value_type
	_M_reduce(_BinaryOp __binary_op) const
	{
	  if constexpr (_S_size == 1)
	    return operator[](0);
	  else if constexpr (_Traits.template _M_eval_as_f32<_T0>())
	    return value_type(rebind_t<complex<float>, basic_vec>(*this)._M_reduce(__binary_op));
	  else if constexpr (_TSimd::abi_type::_S_nreg >= 2)
	    return _M_reduce_to_register(__binary_op)._M_reduce(__binary_op);
	  else if constexpr (__has_single_bit(unsigned(_S_size)))
	    {
	      const auto [__a, __b] = _M_chunk<resize_t<_S_size / 2, basic_vec>>();
	      return __binary_op(__a, __b)._M_reduce(__binary_op);
	    }
	  else
	    {
	      const auto [__a, __b, __c, ...__rest]
		= _M_chunk<resize_t<__bit_floor(unsigned(_S_size)) / 2, basic_vec>>();
	      const auto __ab = __binary_op(__a, __b);
	      static_assert(sizeof...(__rest) <= 1);
	      if constexpr (__a._S_size != __c._S_size)
		return cat(__ab, __c)._M_reduce(__binary_op);
	      else
		return cat(__binary_op(__ab, __c), __rest...)._M_reduce(__binary_op);
	    }
	}

      template <typename _Up>
	[[__gnu__::__always_inline__]]
	static inline basic_vec
	_S_partial_load(const _Up* __mem, size_t __n)
	{
	  if constexpr (__complex_like<_Up>)
	    return _S_init(_TSimd::_S_partial_load(
			     reinterpret_cast<const typename _Up::value_type*>(__mem), __n * 2));
	  else
	    return basic_vec(_RealSimd::_S_partial_load(__mem, __n));
	}

      template <typename _Up, _ArchTraits _Traits = {}>
	static inline basic_vec
	_S_masked_load(const _Up* __mem, mask_type __k)
	{
	  if constexpr (__complex_like<_Up>)
	    return _S_init(_TSimd::_S_masked_load(
			     reinterpret_cast<const typename _Up::value_type*>(__mem),
			     __k._M_data));
	  else
	    return basic_vec(_RealSimd::_S_masked_load(__mem, typename _RealSimd::mask_type(__k)));
	}

      template <typename _Up>
	[[__gnu__::__always_inline__]]
	inline void
	_M_store(_Up* __mem) const
	{
	  static_assert(__complex_like<_Up>);
	  _M_data._M_store(reinterpret_cast<typename _Up::value_type*>(__mem));
	}

      template <typename _Up>
	[[__gnu__::__always_inline__]]
	static inline void
	_S_partial_store(const basic_vec& __v, _Up* __mem, size_t __n)
	{
	  static_assert(__complex_like<_Up>);
	  _TSimd::_S_partial_store(__v._M_data, reinterpret_cast<typename _Up::value_type*>(__mem),
				   __n * 2);
	}

      template <typename _Up>
	[[__gnu__::__always_inline__]]
	static inline void
	_S_masked_store(const basic_vec& __v, _Up* __mem, const mask_type& __k)
	{
	  static_assert(__complex_like<_Up>);
	  _TSimd::_S_masked_store(__v._M_data, reinterpret_cast<typename _Up::value_type*>(__mem),
				  __k._M_data);
	}

      basic_vec() = default;

      // TODO: conversion extensions

      // [simd.ctor] broadcast constructor ------------------------------------
      template <__explicitly_convertible_to<value_type> _Up>
	[[__gnu__::__always_inline__]]
	constexpr explicit(!__broadcast_constructible<_Up, value_type>)
	basic_vec(_Up&& __x) noexcept
	  : _M_data([&](int __i) {
	      if constexpr (__complex_like<_Up>)
		return (__i & 1) == 0 ? __x.real() : __x.imag();
	      else
		return (__i & 1) == 0 ? __x : _T0();
	    })
	{}

      template <__simd_vec_bcast_consteval<value_type> _Up>
	consteval
	basic_vec(const _Up& __x)
	: basic_vec(__value_preserving_cast<value_type>(__x))
	{}

      // [simd.ctor] conversion constructor -----------------------------------
      template <__complex_like _Up, typename _UAbi>
	requires (_S_size == _UAbi::_S_size)
	  && __explicitly_convertible_to<_Up, value_type>
	  && _UAbi::_S_is_cx_ileav
	[[__gnu__::__always_inline__]]
	constexpr
	explicit(!convertible_to<_Up, value_type>)
	basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
	: _M_data(__x._M_data)
	{}

      template <__complex_like _Up, typename _UAbi>
	requires (_S_size == _UAbi::_S_size)
	  && __explicitly_convertible_to<_Up, value_type>
	  && (!_UAbi::_S_is_cx_ileav)
	[[__gnu__::__always_inline__]]
	constexpr
	explicit(!convertible_to<_Up, value_type>)
	basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
	: basic_vec(static_cast<_RealSimd>(__x._M_real), static_cast<_RealSimd>(__x._M_imag))
	{}

      template <typename _Up, typename _UAbi>
	requires (!__complex_like<_Up>)
	  && (_S_size == _UAbi::_S_size)
	  && __explicitly_convertible_to<_Up, value_type>
	[[__gnu__::__always_inline__]]
	constexpr
	explicit(!convertible_to<_Up, value_type>)
	basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
	: basic_vec(_RealSimd(__x))
	{}

      using _VecBase<_Tp, _Ap>::_VecBase;

      // [simd.ctor] generator constructor ------------------------------------
      template <__simd_generator_invokable<value_type, _S_size> _Fp>
	[[__gnu__::__always_inline__]]
	constexpr explicit
	basic_vec(_Fp&& __gen)
	: _M_data([&] {
	    using _Arr = std::array<value_type, sizeof(_TSimd) / sizeof(value_type)>;
	    constexpr auto [...__is] = _IotaArray<_S_size>;
	    const _Arr __tmp = { static_cast<value_type>(__gen(__simd_size_c<__is>))... };
	    return __builtin_bit_cast(_TSimd, __tmp);
	  }())
	{}

      // [simd.ctor] load constructor -----------------------------------------
      template <__complex_like _Up>
	[[__gnu__::__always_inline__]]
	constexpr
	basic_vec(_LoadCtorTag, const _Up* __ptr)
	: _M_data([&] {
	    if consteval
	      {
		return _TSimd([&](int __i) {
			 const _Up& __cx = __ptr[__i / 2];
			 return static_cast<_T0>(__i % 2 == 0 ? __cx.real() : __cx.imag());
		       });
	      }
	    else
	      {
		return _TSimd(_LoadCtorTag(),
			      reinterpret_cast<const typename _Up::value_type*>(__ptr));
	      }
	  }())
	{}

      template <typename _Up>
	[[__gnu__::__always_inline__]]
	constexpr
	basic_vec(_LoadCtorTag, const _Up* __ptr)
	  : basic_vec(_RealSimd(_LoadCtorTag(), __ptr))
	{}

      template <ranges::contiguous_range _Rg, typename... _Flags>
	requires __static_sized_range<_Rg, size.value>
	  && __vectorizable<ranges::range_value_t<_Rg>>
	  && __explicitly_convertible_to<ranges::range_value_t<_Rg>, value_type>
	[[__gnu__::__always_inline__]]
	constexpr
	basic_vec(_Rg&& __range, flags<_Flags...> __flags = {})
	: basic_vec(_LoadCtorTag(), __flags.template _S_adjust_pointer<basic_vec>(
				      ranges::data(__range)))
	{
	  static_assert(__loadstore_convertible_to<ranges::range_value_t<_Rg>, value_type,
						   _Flags...>);
	}

      // [simd.ctor] complex init ---------------------------------------------
      // This uses _RealSimd as proposed in LWG4230
      [[__gnu__::__always_inline__]]
      constexpr
      basic_vec(const _RealSimd& __re, const _RealSimd& __im = {}) noexcept
      {
	_M_data._M_complex_set_real(__re);
	_M_data._M_complex_set_imag(__im);
      }

      // [simd.subscr] --------------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      { return value_type(_M_data[__i * 2], _M_data[__i * 2 + 1]); }
#if VIR_PATCH_PERMUTE_DYNAMIC

      // [simd.subscr] and [simd.permute.dynamic] -----------------------------
      template <__simd_integral _IV>
	[[__gnu__::__always_inline__]]
	constexpr resize_t<_IV::size.value, basic_vec>
	operator[](const _IV& __perm) const
	{ return resize_t<_IV::size.value, basic_vec>::_S_dynamic_permute(*this, __perm); }

      template <typename _A0, __simd_integral _IV>
	[[__gnu__::__always_inline__]]
	static constexpr basic_vec
	_S_dynamic_permute(const basic_vec<value_type, _A0>& __v, const _IV& __perm)
	{
	  static_assert(_IV::_S_size == _S_size);
	  if constexpr (sizeof(value_type) == sizeof(float) || sizeof(value_type) == sizeof(double))
	    {
	      using _Up = conditional_t<sizeof(value_type) == sizeof(float), float, double>;
	      using _From = __similar_vec<_Up, _A0::_S_size, _A0>;
	      using _To = __similar_vec<_Up, _S_size, abi_type>;
	      return basic_vec::_S_recursive_bit_cast(
		       _To::_S_dynamic_permute(_From::_S_recursive_bit_cast(__v), __perm));
	    }
	  else
	    {
	      static_assert(sizeof(value_type) == 16); // 128-bit shuffles
	      // we don't use __builtin_shuffle directly since we don't have support for 128-bit
	      // value types. If __int128 is available then it is actually usable in vector builtins
	      // but GCC doesn't recognize/optimize the shuffle patterns.
	      __glibcxx_simd_precondition(
		(__perm >= cw<0> && __perm <= cw<_A0::_S_size - 1>)._M_all_of(),
		"a dynamic permute index is out of bounds");
	      if constexpr (_TSimd::abi_type::_S_nreg == 1 && _A0::_S_nreg == 1)
		{
#if _GLIBCXX_X86
		  // Use VPERMPS (permute floats across all lanes). We therefore need to expand
		  // __perm to shuffle 4 consecutive 32-Byte blocks.
		  constexpr int __n = sizeof(__v) / sizeof(float);
		  const auto __vf = __builtin_bit_cast(vec<float, __n>, __v);
		  if constexpr (sizeof(typename _IV::value_type) == sizeof(int)
				  && _A0::_S_size == _S_size)
		    {
		      const vec<unsigned, __n> __idx
			= __builtin_bit_cast(vec<unsigned char, __n>,
					     __perm * 0x04'04'04'04 + 0x03'02'01'00);
		      return __builtin_bit_cast(basic_vec,
						__builtin_shuffle(__vf._M_concat_data(),
								  __idx._M_concat_data()));
		    }
#endif
		  const array __elems = chunk<2>(__v._M_data);
		  constexpr auto [...__is] = _IotaArray<_S_size>;
		  return _S_init(cat(__elems[__perm[__is]]...));
		}
	      else if (__is_const_known(__perm))
		{
		  const array __elems = chunk<2>(__v._M_data);
		  constexpr auto [...__is] = _IotaArray<_S_size>;
		  return _S_init(cat(__elems[__perm[__is]]...));
		}
	      else
		{
		  basic_vec __r = {};
		  byte* __dst = reinterpret_cast<byte*>(&__r);
		  const byte* __src = reinterpret_cast<const byte*>(&__v);
		  constexpr int __block = sizeof(value_type);
		  for (int __i = 0; __i < _S_size; ++__i)
		    __builtin_memcpy(__dst + __i * __block, __src + __perm[__i] * __block, __block);
		  return __r;
		}
	    }
	}
#endif

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
      { return _S_init(!_M_data); }

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
      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator+=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a + __a; }
      {
	__x._M_data += __y._M_data;
	return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator-=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a - __a; }
      {
	__x._M_data -= __y._M_data;
	return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator*=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a * __a; }
      {
	__x._M_data.template _M_complex_multiply_with<basic_vec>(__y._M_data);
	return __x;
      }

      template <int _RemoveMe = 0>
      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator/=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a / __a; }
      {
	static_assert(false, "TODO");
      }

      // [simd.comparison] compare operators ----------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator==(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_and_neighbors(__x._M_data == __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator!=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type::_S_or_neighbors(__x._M_data != __y._M_data); }

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
      __select_impl(const mask_type& __k, const basic_vec& __t, const basic_vec& __f) noexcept
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

  template <size_t _Bytes, __abi_tag _Ap>
    requires _Ap::_S_is_cx_ctgus
    class basic_mask<_Bytes, _Ap> : public _MaskBase<_Bytes, _Ap>
    {
      using _Base = _MaskBase<_Bytes, _Ap>;

      template <size_t, typename>
	friend class basic_mask;

      template <typename, typename>
	friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      using _DataType
	= basic_mask<_Bytes / 2, _Abi_t<_S_size, _Ap::_S_nreg,
					__filter_abi_variant(_Ap::_S_variant,
							     _AbiVariant::_MaskVariants)>>;

      static_assert(_DataType::abi_type::_S_nreg == _Ap::_S_nreg);

      using _VecType = __similar_vec<__integer_from<_Bytes>, _S_size, _Ap>;

      static constexpr bool _S_is_scalar = _DataType::_S_is_scalar;

      static constexpr bool _S_use_bitmask = _DataType::_S_use_bitmask;

      static constexpr int _S_full_size = _DataType::_S_full_size;

      static constexpr bool _S_is_partial = _DataType::_S_is_partial;

      static constexpr bool _S_has_bool_member = _DataType::_S_has_bool_member;

      static constexpr size_t _S_padding_bytes = _DataType::_S_padding_bytes;

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

      static constexpr auto size = __simd_size_c<_S_size>;

      // internal but public API ----------------------------------------------
      [[__gnu__::__always_inline__]]
      static constexpr basic_mask
      _S_init(const _DataType& __x)
      {
	basic_mask __r;
	__r._M_data = __x;
	return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr const _DataType&
      _M_get() const
      { return _M_data; }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_concat_data() const
      { return _M_data._M_concat_data(); }

      template <_ArchTraits _Traits = {}>
	[[__gnu__::__always_inline__]]
	static constexpr basic_mask
	_S_partial_mask_of_n(int __n)
	{ return _S_init(_DataType::_S_partial_mask_of_n(__n)); }

      template <typename _Mp>
	[[__gnu__::__always_inline__]]
	constexpr auto
	_M_chunk() const noexcept
	{
	  if constexpr (_Mp::abi_type::_S_variant != _Ap::_S_variant)
	    {
	      using _M2 = resize_t<_S_size, _Mp>;
	      static_assert(!is_same_v<_M2, basic_mask>);
	      return static_cast<_M2>(*this).template _M_chunk<_Mp>();
	    }
	  else // _Mp is the same partial specialization
	    {
	      constexpr int __rem = _S_size % _Mp::_S_size;
	      const auto [...__xs, __last] = _M_data.template _M_chunk<typename _Mp::_DataType>();
	      if constexpr (__rem == 0)
		return array{_Mp::_S_init(__xs)..., _Mp::_S_init(__last)};
	      else
		return tuple(_Mp::_S_init(__xs)..., resize_t<__rem, _Mp>(__last));
	    }
	}

      [[__gnu__::__always_inline__]]
      static constexpr const basic_mask&
      _S_concat(const basic_mask& __x0) noexcept
      { return __x0; }

      template <typename _A0>
	[[__gnu__::__always_inline__]]
	static constexpr const auto&
	_S_unwrap_cx_ctgus(const basic_mask<_Bytes, _A0>& __x) noexcept
	{
	  static_assert(_A0::_S_is_cx_ctgus);
	  return __x._M_data;
	}

      [[__gnu__::__always_inline__]]
      static constexpr basic_mask<_Bytes / 2, _ScalarAbi<1>>
      _S_unwrap_cx_ctgus(const basic_mask<_Bytes, _ScalarAbi<1>>& __x) noexcept
      { return basic_mask<_Bytes / 2, _ScalarAbi<1>>(__x._M_data); }

      template <typename... _As>
	requires (sizeof...(_As) > 1)
	[[__gnu__::__always_inline__]]
	static constexpr basic_mask
	_S_concat(const basic_mask<_Bytes, _As>&... __xs) noexcept
	{ return basic_mask::_S_init(_DataType::_S_concat(_S_unwrap_cx_ctgus(__xs)...)); }

      // [simd.mask.overview] default constructor -----------------------------
      basic_mask() = default;

      // [simd.mask.overview] conversion extensions ---------------------------
      template <__vec_builtin _TV>
	[[__gnu__::__always_inline__]]
	constexpr
	basic_mask(const _TV& __x) requires convertible_to<_TV, _DataType>
	: _M_data(__x)
	{}

      template <__vec_builtin _TV>
	[[__gnu__::__always_inline__]]
	constexpr
	operator _TV() requires convertible_to<_DataType, _TV>
	{ return _M_data; }

      // [simd.mask.ctor] broadcast constructor -------------------------------
      [[__gnu__::__always_inline__]]
      constexpr explicit
      basic_mask(same_as<bool> auto __x) noexcept // LWG 4382.
      : _M_data(__x)
      {}

      // [simd.mask.ctor] conversion constructor ------------------------------
      template <size_t _UBytes, typename _UAbi>
	requires (_S_size == _UAbi::_S_size)
	[[__gnu__::__always_inline__]]
	constexpr explicit(__is_mask_conversion_explicit<_Ap, _UAbi>(_Bytes, _UBytes))
	basic_mask(const basic_mask<_UBytes, _UAbi>& __x) noexcept
	: _M_data(__x)
	{}

      using _Base::_MaskBase;

      // [simd.mask.ctor] generator constructor -------------------------------
      template <__simd_generator_invokable<bool, _S_size> _Fp>
	[[__gnu__::__always_inline__]]
	constexpr explicit
	basic_mask(_Fp&& __gen)
	: _M_data(__gen)
	{}

      // [simd.mask.ctor] bitset constructor ----------------------------------
      [[__gnu__::__always_inline__]]
      constexpr
      basic_mask(const same_as<bitset<_S_size>> auto& __b) noexcept // LWG 4382.
      : _M_data(__b)
      {}

      // [simd.mask.ctor] uint constructor ------------------------------------
      template <unsigned_integral _Tp>
	requires (!same_as<_Tp, bool>) // LWG 4382.
	[[__gnu__::__always_inline__]]
	constexpr explicit
	basic_mask(_Tp __val) noexcept
	: _M_data(__val)
	{}

      // [simd.mask.subscr] ---------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      { return _M_data[__i]; }

      // [simd.mask.unary] ----------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr basic_mask
      operator!() const noexcept
      { return _S_init(!_M_data); }

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator+() const noexcept requires destructible<_VecType>
      { return static_cast<_VecType>(_M_data); }

      constexpr _VecType
      operator+() const noexcept = delete;

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator-() const noexcept requires destructible<_VecType>
      {
	using _Ip = typename _VecType::value_type;
	if constexpr (_S_use_bitmask)
	  return __select_impl(*this, _Ip(-1), _Ip());
	else
	  return -_M_data; // sign-extends
      }

      constexpr _VecType
      operator-() const noexcept = delete;

      [[__gnu__::__always_inline__]]
      constexpr _VecType
      operator~() const noexcept requires destructible<_VecType>
      {
	using _Ip = typename _VecType::value_type;
	if constexpr (_S_use_bitmask)
	  return __select_impl(*this, _Ip(-2), _Ip(-1));
	else
	  return ~_M_data; // sign-extends
      }

      constexpr _VecType
      operator~() const noexcept = delete;

      // [simd.mask.conv] -----------------------------------------------------
      template <typename _Up, typename _UAbi>
	requires (_UAbi::_S_size == _S_size)
	[[__gnu__::__always_inline__]]
	constexpr explicit(sizeof(_Up) != _Bytes)
	operator basic_vec<_Up, _UAbi>() const noexcept
	{
	  using _UV = basic_vec<_Up, _UAbi>;
	  using _Mp = typename _UV::mask_type;
	  return __select_impl(static_cast<_Mp>(_M_data), _UV(1), _UV(0));
	}

      using _Base::operator basic_vec;

      // [simd.mask.namedconv] ------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr bitset<_S_size>
      to_bitset() const noexcept
      { return _M_data.to_bitset(); }

      template <int _Offset = 0, _ArchTraits _Traits = {}>
	[[__gnu__::__always_inline__]]
	constexpr auto
	_M_to_uint() const
	{ return _M_data.template _M_to_uint<_Offset>(); }

      [[__gnu__::__always_inline__]]
      constexpr unsigned long long
      to_ullong() const
      { return _M_data.to_ullong(); }

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
      { return _S_init(__x._M_data == __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator!=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data != __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator>=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data >= __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator<=(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data <= __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator>(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data > __y._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      operator<(const basic_mask& __x, const basic_mask& __y) noexcept
      { return _S_init(__x._M_data < __y._M_data); }

      // [simd.mask.cond] -----------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      __select_impl(const basic_mask& __k, const basic_mask& __t, const basic_mask& __f) noexcept
      { return __select_impl(__k._M_data, __t._M_data, __f._M_data); }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_mask
      __select_impl(const basic_mask& __k, same_as<bool> auto __t, same_as<bool> auto __f) noexcept
      { return _S_init(__select_impl(__k._M_data, __t, __f)); }

      template <__vectorizable _T0, same_as<_T0> _T1>
	requires (sizeof(_T0) == _Bytes)
	[[__gnu__::__always_inline__]]
	friend constexpr vec<_T0, _S_size>
	__select_impl(const basic_mask& __k, const _T0& __t, const _T1& __f) noexcept
	{
	  using _Vp = vec<_T0, _S_size>;
	  return __select_impl(static_cast<typename _Vp::mask_type>(__k), _Vp(__t), _Vp(__f));
	}

      // [simd.mask.reductions] implementation --------------------------------
      [[__gnu__::__always_inline__]]
      constexpr bool
      _M_all_of() const noexcept
      { return _M_data._M_all_of(); }

      [[__gnu__::__always_inline__]]
      constexpr bool
      _M_any_of() const noexcept
      { return _M_data._M_any_of(); }

      [[__gnu__::__always_inline__]]
      constexpr bool
      _M_none_of() const noexcept
      { return _M_data._M_none_of(); }

      [[__gnu__::__always_inline__]]
      constexpr __simd_size_type
      _M_reduce_count() const noexcept
      { return _M_data._M_reduce_count(); }

      [[__gnu__::__always_inline__]]
      constexpr __simd_size_type
      _M_reduce_min_index() const
      { return _M_data._M_reduce_min_index(); }

      [[__gnu__::__always_inline__]]
      constexpr __simd_size_type
      _M_reduce_max_index() const
      { return _M_data._M_reduce_max_index(); }

      [[__gnu__::__always_inline__]]
      friend constexpr bool
      __is_const_known(const basic_mask& __x)
      { return __is_const_known(__x._M_data); }
    };

  template <__vectorizable _Tp, __abi_tag _Ap>
    requires __complex_like<_Tp>
      && (_Ap::_S_is_cx_ctgus || __scalar_abi_tag<_Ap>)
    class basic_vec<_Tp, _Ap>
    : _VecBase<_Tp, _Ap>
    {
      template <typename, typename>
	friend class basic_vec;

      static constexpr int _S_size = _Ap::_S_size;

      static constexpr int _S_full_size = __bit_ceil(unsigned(_S_size));

      using _T0 = typename _Tp::value_type;

      using _RealSimd = __similar_resized_vec<_T0, _S_size, _Ap>;

      _RealSimd _M_real = {};

      _RealSimd _M_imag = {};

      static constexpr bool _S_is_scalar = __scalar_abi_tag<_Ap>;

      static_assert(_S_is_scalar == _RealSimd::_S_is_scalar);

      static constexpr bool _S_use_bitmask = _RealSimd::_S_use_bitmask;

      static constexpr bool _S_is_partial = _RealSimd::_S_is_partial;

    public:
      using value_type = _Tp;

      using abi_type = _Ap;

      // We can't use _RealSimd::mask_type here because that would have the wrong value for _Bytes,
      // which bites us in __select_impl(basic_mask, T, T) where sizeof(T) is constrained to _Bytes.
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

      static constexpr auto size = __simd_size_c<_S_size>;

      // internal but public API ----------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr const _RealSimd&
      _M_get_real() const
      { return _M_real; }

      [[__gnu__::__always_inline__]]
      constexpr const _RealSimd&
      _M_get_imag() const
      { return _M_imag; }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_get_low() const requires (_Ap::_S_nreg >= 2)
      {
	return resize_t<_M_real._N0, basic_vec>(
		 _M_real._M_get_low(), _M_imag._M_get_low());
      }

      [[__gnu__::__always_inline__]]
      constexpr auto
      _M_get_high() const requires (_Ap::_S_nreg >= 2)
      {
	return resize_t<_M_real._N1, basic_vec>(
		 _M_real._M_get_high(), _M_imag._M_get_high());
      }
#if VIR_PATCH_PERMUTE_DYNAMIC

      constexpr auto
      _M_concat_data(bool __do_sanitize = false) = delete("not for _CxCtgus");
#endif

      [[__gnu__::__always_inline__]]
      friend constexpr bool
      __is_const_known(const basic_vec& __x)
      { return __is_const_known(__x._M_real) && __is_const_known(__x._M_imag); }

      template <typename _Vp>
	[[__gnu__::__always_inline__]]
	constexpr auto
	_M_chunk() const noexcept
	{
	  constexpr int __n = _S_size / _Vp::_S_size;
	  constexpr int __rem = _S_size % _Vp::_S_size;
	  const auto [...__rs, __rN] = _M_real.template _M_chunk<typename _Vp::_RealSimd>();
	  const auto [...__is, __iN] = _M_imag.template _M_chunk<typename _Vp::_RealSimd>();
	  if constexpr (__rem == 0)
	    return array<_Vp, __n>{_Vp(__rs, __is)..., _Vp(__rN, __iN)};
	  else
	    return tuple(_Vp(__rs, __is)..., resize_t<__rem, _Vp>(__rN, __iN));
	}

      template <typename _A0>
	[[__gnu__::__always_inline__]]
	static constexpr basic_vec
	_S_concat(const basic_vec<value_type, _A0>& __x0) noexcept
	{ return static_cast<basic_vec>(__x0); }

      template <typename... _As>
	requires (sizeof...(_As) > 1)
	[[__gnu__::__always_inline__]]
	static constexpr basic_vec
	_S_concat(const basic_vec<value_type, _As>&... __xs) noexcept
	{ return {_RealSimd::_S_concat(__xs._M_real...), _RealSimd::_S_concat(__xs._M_imag...) }; }

      template <typename _BinaryOp>
	[[__gnu__::__always_inline__]]
	constexpr auto
	_M_reduce_to_register(_BinaryOp __binary_op) const
	{
	  if constexpr (_RealSimd::abi_type::_S_nreg == 1)
	    return *this;
	  else
	    {
	      auto [__lo, __hi] = _M_chunk<resize_t<_RealSimd::_N0, basic_vec>>();
	      auto __a = __lo._M_reduce_to_register(__binary_op);
	      auto __b = __hi._M_reduce_to_register(__binary_op);
	      if constexpr (__a._S_size == __b._S_size)
		return __binary_op(__a, __b);
	      else
		{
		  using _V1 = resize_t<1, basic_vec>;
		  return __binary_op(_V1(__a._M_reduce(__binary_op)),
				     _V1(__b._M_reduce(__binary_op)));
		}
	    }
	}

      template <typename _BinaryOp, _ArchTraits _Traits = {}>
	[[__gnu__::__always_inline__]]
	constexpr value_type
	_M_reduce(_BinaryOp __binary_op) const
	{
	  if constexpr (_S_size == 1)
	    return operator[](0);
	  else if constexpr (_Traits.template _M_eval_as_f32<_T0>())
	    return value_type(rebind_t<complex<float>, basic_vec>(*this)._M_reduce(__binary_op));
	  else if constexpr (_RealSimd::abi_type::_S_nreg >= 2)
	    return _M_reduce_to_register(__binary_op)._M_reduce(__binary_op);
	  else if constexpr (__has_single_bit(unsigned(_S_size)))
	    {
	      const auto [__a, __b] = _M_chunk<resize_t<_S_size / 2, basic_vec>>();
	      return __binary_op(__a, __b)._M_reduce(__binary_op);
	    }
	  else
	    {
	      const auto [__a, __b, __c, ...__rest]
		= _M_chunk<resize_t<__bit_floor(unsigned(_S_size)) / 2, basic_vec>>();
	      const auto __ab = __binary_op(__a, __b);
	      static_assert(sizeof...(__rest) <= 1);
	      if constexpr (__a._S_size != __c._S_size)
		return cat(__ab, __c)._M_reduce(__binary_op);
	      else
		return cat(__binary_op(__ab, __c), __rest...)._M_reduce(__binary_op);
	    }
	}

      /** @internal
       * Implementation of @ref partial_load.
       *
       * If @p __mem stores complex numbers, this needs to load @c abcdefgh from memory into two
       * basic_vec: @c aceg and @c bdfh.
       *
       * @param __mem  A pointer to an array of @p __n values. Can be complex or real.
       * @param __n    Read no more than @p __n values from memory.
       *
       * @todo Optimize with deinterleaving loads or loads + deinterleaving fixup.
       */
      template <typename _Up>
	[[__gnu__::__always_inline__]]
	static inline basic_vec
	_S_partial_load(const _Up* __mem, size_t __n)
	{
	  if constexpr (__complex_like<_Up>)
	    return basic_vec(
		     _RealSimd([&](size_t __i) -> _T0 {
		       return __i < __n ? __mem[__i].real() : _T0();
		     }),
		     _RealSimd([&](size_t __i) -> _T0 {
		       return __i < __n ? __mem[__i].imag() : _T0();
		     }));
	  else
	    return basic_vec(_RealSimd::_S_partial_load(__mem, __n));
	}

      /** @internal
       *
       * @todo Optimize with deinterleaving loads or loads + deinterleaving fixup.
       */
      template <typename _Up, _ArchTraits _Traits = {}>
	static inline basic_vec
	_S_masked_load(const _Up* __mem, mask_type __k)
	{
	  if constexpr (__complex_like<_Up>)
	    { // TODO: optimize
	      return basic_vec(_RealSimd([&](int __i) {
				 return __k[__i] ? __mem[__i].real() : _T0();
			       }), _RealSimd([&](int __i) {
				     return __k[__i] ? __mem[__i].imag() : _T0();
				   }));
	    }
	  else
	    return basic_vec(_RealSimd::_S_masked_load(__mem, typename _RealSimd::mask_type(__k)));
	}

      template <typename _Up>
	[[__gnu__::__always_inline__]]
	inline void
	_M_store(_Up* __mem) const
	{
	  static_assert(__complex_like<_Up>);
	  for (int __i = 0; __i < _S_size; ++__i)
	    {
	      __mem[__i].real(_M_real[__i]);
	      __mem[__i].imag(_M_imag[__i]);
	    }
	}

      template <typename _Up>
	[[__gnu__::__always_inline__]]
	static inline void
	_S_partial_store(const basic_vec& __v, _Up* __mem, size_t __n)
	{
	  static_assert(__complex_like<_Up>);
	  for (size_t __i = 0; __i < std::min(__n, size_t(_S_size)); ++__i)
	    {
	      __mem[__i].real(__v._M_real[__i]);
	      __mem[__i].imag(__v._M_imag[__i]);
	    }
	}

      template <typename _Up>
	[[__gnu__::__always_inline__]]
	static inline void
	_S_masked_store(const basic_vec& __v, _Up* __mem, const mask_type& __k)
	{
	  // TODO: optimize
	  static_assert(__complex_like<_Up>);
	  for (int __i = 0; __i < _S_size; ++__i)
	    {
	      if (__k[__i])
		__mem[__i] = __v[__i];
	    }
	}

      basic_vec() = default;

      // TODO: conversion extensions

      // [simd.ctor] broadcast constructor ------------------------------------
      template <__explicitly_convertible_to<value_type> _Up>
	requires __complex_like<_Up>
	[[__gnu__::__always_inline__]]
	constexpr explicit(!__broadcast_constructible<_Up, value_type>)
	basic_vec(_Up&& __x) noexcept
	  : _M_real(__x.real()), _M_imag(__x.imag())
	{}

      template <__explicitly_convertible_to<value_type> _Up>
	[[__gnu__::__always_inline__]]
	constexpr explicit(!__broadcast_constructible<_Up, value_type>)
	basic_vec(_Up&& __x) noexcept
	  : _M_real(__x), _M_imag()
	{}

      template <__simd_vec_bcast_consteval<value_type> _Up>
	consteval
	basic_vec(const _Up& __x)
	: _M_real(__x), _M_imag()
	{}

      // [simd.ctor] conversion constructor -----------------------------------
      template <__complex_like _Up, typename _UAbi>
	requires (_S_size == _UAbi::_S_size)
	  && __explicitly_convertible_to<_Up, value_type>
	  && _UAbi::_S_is_cx_ileav
	[[__gnu__::__always_inline__]]
	constexpr
	explicit(!convertible_to<_Up, value_type>)
	basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
	: _M_real(__x.real()), _M_imag(__x.imag())
	{}

      template <__complex_like _Up, typename _UAbi>
	requires (_S_size == _UAbi::_S_size)
	  && __explicitly_convertible_to<_Up, value_type>
	  && (!_UAbi::_S_is_cx_ileav)
	[[__gnu__::__always_inline__]]
	constexpr
	explicit(!convertible_to<_Up, value_type>)
	basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
	: _M_real(__x._M_real), _M_imag(__x._M_imag) // using real() instead of _M_real is possible
	  // but potentially leads to memcpy because of oversized _M_real (likewise for imag)
	{}

      template <typename _Up, typename _UAbi> // _Up is not complex!
	requires (!__complex_like<_Up>)
	  && (_S_size == _UAbi::_S_size)
	  && __explicitly_convertible_to<_Up, value_type>
	[[__gnu__::__always_inline__]]
	constexpr
	explicit(!convertible_to<_Up, value_type>)
	basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
	: _M_real(__x), _M_imag()
	{}

      using _VecBase<_Tp, _Ap>::_VecBase;

      // [simd.ctor] generator constructor ------------------------------------
      template <__simd_generator_invokable<value_type, _S_size> _Fp>
	[[__gnu__::__always_inline__]]
	constexpr explicit
	basic_vec(_Fp&& __gen)
	: _M_real(),
	  _M_imag([&] {
	    _T0 __re[sizeof(_RealSimd) / sizeof(_T0)] = {};
	    _T0 __im[sizeof(_RealSimd) / sizeof(_T0)] = {};
	    template for (constexpr int __i : _IotaArray<_S_size>)
	      {
		const value_type __c = static_cast<value_type>(__gen(__simd_size_c<__i>));
		__re[__i] = __c.real();
		__im[__i] = __c.imag();
	      }
	    _M_real = __builtin_bit_cast(_RealSimd, __re);
	    return __builtin_bit_cast(_RealSimd, __im);
	  }())
	{}

      // [simd.ctor] load constructor -----------------------------------------
      template <__complex_like _Up>
	[[__gnu__::__always_inline__]]
	constexpr
	basic_vec(_LoadCtorTag, const _Up* __ptr)
	: _M_real([&](int __i) -> _T0 { return __ptr[__i].real(); }),
	  _M_imag([&](int __i) -> _T0 { return __ptr[__i].imag(); })
	{}

      template <typename _Up>
	[[__gnu__::__always_inline__]]
	constexpr
	basic_vec(_LoadCtorTag, const _Up* __ptr)
	: _M_real(_LoadCtorTag(), __ptr), _M_imag()
	{}

      template <ranges::contiguous_range _Rg, typename... _Flags>
	requires __static_sized_range<_Rg, size.value>
	  && __vectorizable<ranges::range_value_t<_Rg>>
	  && __explicitly_convertible_to<ranges::range_value_t<_Rg>, value_type>
	[[__gnu__::__always_inline__]]
	constexpr
	basic_vec(_Rg&& __range, flags<_Flags...> __flags = {})
	: basic_vec(_LoadCtorTag(), __flags.template _S_adjust_pointer<basic_vec>(
				      ranges::data(__range)))
	{
	  static_assert(__loadstore_convertible_to<ranges::range_value_t<_Rg>, value_type,
						   _Flags...>);
	}

      // [simd.ctor] complex init ---------------------------------------------
      // This uses _RealSimd as proposed in LWG4230
      [[__gnu__::__always_inline__]]
      constexpr
      basic_vec(const _RealSimd& __re, const _RealSimd& __im = {}) noexcept
      : _M_real(__re), _M_imag(__im)
      {}

      // [simd.subscr] --------------------------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr value_type
      operator[](__simd_size_type __i) const
      { return value_type(_M_real[__i], _M_imag[__i]); }
#if VIR_PATCH_PERMUTE_DYNAMIC

      // [simd.subscr] and [simd.permute.dynamic] -----------------------------
      template <__simd_integral _IV>
	[[__gnu__::__always_inline__]]
	constexpr resize_t<_IV::size.value, basic_vec>
	operator[](const _IV& __perm) const
	{ return resize_t<_IV::size.value, basic_vec>::_S_dynamic_permute(*this, __perm); }

      template <typename _A0, __simd_integral _IV>
	[[__gnu__::__always_inline__]]
	static constexpr basic_vec
	_S_dynamic_permute(const basic_vec<value_type, _A0>& __v, const _IV& __perm)
	{
	  return basic_vec(_RealSimd::_S_dynamic_permute(__v._M_real, __perm),
			   _RealSimd::_S_dynamic_permute(__v._M_imag, __perm));
	}
#endif

      // [simd.unary] unary operators -----------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator++() noexcept requires requires(value_type __a) { ++__a; }
      {
	++_M_real;
	return *this;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator++(int) noexcept requires requires(value_type __a) { __a++; }
      {
	basic_vec __r = *this;
	++_M_real;
	return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec&
      operator--() noexcept requires requires(value_type __a) { --__a; }
      {
	--_M_real;
	return *this;
      }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator--(int) noexcept requires requires(value_type __a) { __a--; }
      {
	basic_vec __r = *this;
	--_M_real;
	return __r;
      }

      [[__gnu__::__always_inline__]]
      constexpr mask_type
      operator!() const noexcept requires requires(value_type __a) { !__a; }
      { return !_M_real && !_M_imag; }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator+() const noexcept requires requires(value_type __a) { +__a; }
      { return *this; }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      operator-() const noexcept requires requires(value_type __a) { -__a; }
      { return basic_vec(-_M_real, -_M_imag); }

      // [simd.cassign] compound assignment -----------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator+=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a + __a; }
      {
	__x._M_real += __y._M_real;
	__x._M_imag += __y._M_imag;
	return __x;
      }

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator-=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a - __a; }
      {
	__x._M_real -= __y._M_real;
	__x._M_imag -= __y._M_imag;
	return __x;
      }


      template <_TargetTraits _Traits = {}>
	[[__gnu__::__always_inline__]]
	friend constexpr basic_vec&
	operator*=(basic_vec& __x, const basic_vec& __y) noexcept
	requires requires(value_type __a) { __a * __a; }
	{
	  _RealSimd::template _S_cxctgus_mul<value_type>(
	    __x._M_real, __x._M_imag, __y._M_real, __y._M_imag);
	  return __x;
	}

      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec&
      operator/=(basic_vec& __x, const basic_vec& __y) noexcept
      requires requires(value_type __a) { __a / __a; }
      {
	const _RealSimd __r =  __x._M_real * __y._M_real + __x._M_imag * __y._M_imag;
	const _RealSimd __n = __y._M_norm();
	__x._M_imag = (__x._M_imag * __y._M_real - __x._M_real * __y._M_imag) / __n;
	__x._M_real = __r / __n;
	return __x;
      }

      // [simd.comparison] compare operators ----------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator==(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type(__x._M_real == __y._M_real && __x._M_imag == __y._M_imag); }

      [[__gnu__::__always_inline__]]
      friend constexpr mask_type
      operator!=(const basic_vec& __x, const basic_vec& __y) noexcept
      { return mask_type(__x._M_real != __y._M_real || __x._M_imag != __y._M_imag); }

      // [simd.complex.access] complex-value accessors ------------------------
      // LWG4230: returns _RealSimd instead of auto
      [[__gnu__::__always_inline__]]
      constexpr _RealSimd
      real() const noexcept
      { return _M_real; }

      [[__gnu__::__always_inline__]]
      constexpr _RealSimd
      imag() const noexcept
      { return _M_imag; }

      [[__gnu__::__always_inline__]]
      constexpr void
      real(const _RealSimd& __x) noexcept
      { _M_real = __x; }

      [[__gnu__::__always_inline__]]
      constexpr void
      imag(const _RealSimd& __x) noexcept
      { _M_imag = __x; }

      // [simd.cond] ---------------------------------------------------------
      [[__gnu__::__always_inline__]]
      friend constexpr basic_vec
      __select_impl(const mask_type& __k, const basic_vec& __t, const basic_vec& __f) noexcept
      {
	typename basic_vec::_RealSimd::mask_type __kk(__k);
	return basic_vec(__select_impl(__kk, __t._M_real, __f._M_real),
			 __select_impl(__kk, __t._M_imag, __f._M_imag));
      }

      // [simd.complex.math] internals ---------------------------------------
      [[__gnu__::__always_inline__]]
      constexpr _RealSimd
      _M_abs() const
      {
	// FIXME: avoid overflow & underflow in _M_norm
	return sqrt(_M_norm());
      }

      // associated functions
      [[__gnu__::__always_inline__]]
      constexpr _RealSimd
      _M_norm() const
      { return _M_real * _M_real + _M_imag * _M_imag; }

      [[__gnu__::__always_inline__]]
      constexpr basic_vec
      _M_conj() const
      { return basic_vec(_M_real, -_M_imag); }
    };

  // [P3319R5] (extension) ----------------------------------------------------
  template <__complex_like _Tp, typename _Ap>
    inline constexpr basic_vec<_Tp, _Ap>
    __iota<basic_vec<_Tp, _Ap>> = basic_vec<_Tp, _Ap>([](typename _Tp::value_type __i)
							  -> typename _Tp::value_type {
      static_assert(_Ap::_S_size - 1 <= numeric_limits<typename _Tp::value_type>::max(),
		    "iota object would overflow");
      return __i;
    });
} // namespace simd
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std

#pragma GCC diagnostic pop
#endif // C++26
#endif // _GLIBCXX_SIMD_COMPLEX_H
