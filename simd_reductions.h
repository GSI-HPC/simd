/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_SIMD_REDUCTIONS_H_
#define PROTOTYPE_SIMD_REDUCTIONS_H_

#include "simd.h"
#include "simd_chunk.h"

namespace std::__detail
{
  template <typename _Tp, typename _BinaryOperation>
    inline constexpr auto
    __identity_element_for = [] {
      if constexpr (same_as<_BinaryOperation, plus<>>)
        return _Tp(0);
      else if constexpr (same_as<_BinaryOperation, multiplies<>>)
        return _Tp(1);
      else if constexpr (same_as<_BinaryOperation, bit_and<>>)
        return _Tp(~_Tp());
      else if constexpr (same_as<_BinaryOperation, bit_or<>>)
        return _Tp(0);
      else if constexpr (same_as<_BinaryOperation, bit_xor<>>)
        return _Tp(0);
      else
        return nullptr;
    }();

  template <typename _Tp, typename _BinaryOperation>
    requires same_as<_BinaryOperation, plus<>>
      or same_as<_BinaryOperation, multiplies<>>
      or same_as<_BinaryOperation, bit_and<>>
      or same_as<_BinaryOperation, bit_or<>>
      or same_as<_BinaryOperation, bit_xor<>>
    constexpr _Tp __default_identity_element()
    { return __identity_element_for<_Tp, _BinaryOperation>; }

  template <typename _Tp, typename _Abi, typename _BinaryOperation>
    constexpr std::datapar::resize_t<std::datapar::__simd_size_v<_Tp, _Abi> / 2,
                                     std::datapar::basic_simd<_Tp, _Abi>>
    __split_and_invoke_once(const std::datapar::basic_simd<_Tp, _Abi>& __x,
                            _BinaryOperation __binary_op)
    {
      using _V1 = std::datapar::basic_simd<_Tp, _Abi>;
      static_assert(std::__has_single_bit(unsigned(_V1::size.value)));
      using _V2 = std::datapar::resize_t<_V1::size.value / 2, _V1>;
      const auto [__x0, __x1] = std::datapar::chunk<_V2>(__x);
      // Mandates: binary_op can be invoked with two arguments of type basic_simd<_Tp, A1>
      // returning basic_simd<_Tp, A1> for every A1 that is an ABI tag type.
      static_assert(requires {
        { __binary_op(__x0, __x1) } -> same_as<_V2>;
      });
      return __binary_op(__x0, __x1);
    }
}

namespace std::datapar
{
  template <typename _Tp, typename _Abi, __detail::__binary_operation<_Tp> _BinaryOperation>
    constexpr _Tp
    reduce(const basic_simd<_Tp, _Abi>& __x, _BinaryOperation __binary_op)
    {
      using _V1 = basic_simd<_Tp, _Abi>;

      if constexpr (requires{__detail::_SimdTraits<_Tp, _Abi>::_SimdImpl::_S_reduce(
                               __x, __binary_op);} and _V1::size.value > 1)
        {
          if (not __builtin_is_constant_evaluated() and not __x._M_is_constprop())
            return __detail::_SimdTraits<_Tp, _Abi>::_SimdImpl::_S_reduce(__x, __binary_op);
        }

      if constexpr (_V1::size.value == 1)
        return __x[0];

      else if constexpr (_V1::size.value == 2 and sizeof(__x) == 16)
        return __binary_op(__x, _V1([&](auto __i) { return __x[__i ^ 1]; }))[0];

      else if constexpr (std::__has_single_bit(_V1::size.value))
        return reduce(__detail::__split_and_invoke_once(__x, __binary_op), __binary_op);

      else
        {
          constexpr int __left_size = std::__bit_floor(_V1::size.value);
          constexpr int __right_size = _V1::size.value - __left_size;
          constexpr int __max_size = std::__bit_ceil(_V1::size.value);
          constexpr int __missing = __max_size - _V1::size.value;
          if constexpr (sizeof(_V1) == sizeof(resize_t<__max_size, _V1>)
                          and (__missing < __right_size)
                          and not same_as<decltype(__detail::__identity_element_for
                                                     <_Tp, _BinaryOperation>), nullptr_t>)
            {
              using _V2 = resize_t<__max_size, _V1>;
              constexpr simd<_Tp, __missing> __padding
                = __detail::__identity_element_for<_Tp, _BinaryOperation>;
              const _V2 __y = cat(__x, __padding);
              return reduce(__detail::__split_and_invoke_once(__y, __binary_op), __binary_op);
            }

          using _V2 = resize_t<__left_size, _V1>;
          const auto [__x0, __x1] = chunk<_V2>(__x);
          return reduce(cat(__detail::__split_and_invoke_once(__x0, __binary_op), __x1),
                        __binary_op);
        }
    }

  template <typename _Tp, typename _Abi, __detail::__binary_operation<_Tp> _BinaryOperation>
    constexpr _Tp
    reduce(const basic_simd<_Tp, _Abi>& __x, const typename basic_simd<_Tp, _Abi>::mask_type& __k,
           _BinaryOperation __binary_op, __type_identity_t<_Tp> __identity_element)
    {
      __glibcxx_simd_precondition(__binary_op(simd<_Tp, 1>(_Tp(2)), simd<_Tp, 1>(_Tp(3)))[0]
                                    == __binary_op(simd<_Tp, 1>(_Tp(3)), simd<_Tp, 1>(_Tp(2)))[0],
                                  "The given binary operation needs to be commutative.");
      __glibcxx_simd_precondition(__binary_op(simd<_Tp, 1>(_Tp(2)),
                                              __binary_op(simd<_Tp, 1>(__identity_element),
                                                          simd<_Tp, 1>(__identity_element)))[0]
                                    == _Tp(2),
                                  "The given identity_element needs to preserve identity over the "
                                  "given binary operation.");
      return reduce(select(__k, __x, __identity_element), __binary_op);
    }

  // NaN inputs are precondition violations (_Tp satisfies and models totally_ordered)
  template <std::totally_ordered _Tp, typename _Abi>
    constexpr _Tp
    reduce_min(const basic_simd<_Tp, _Abi>& __x) noexcept
    {
      return reduce(__x, []<totally_ordered _UV>(const _UV& __a, const _UV& __b) {
               return select(__a < __b, __a, __b);
             });
    }

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr _Tp
    reduce_min(const basic_simd<_Tp, _Abi>& __x,
               const typename basic_simd<_Tp, _Abi>::mask_type& __k) noexcept
    {
      return reduce(select(__k, __x, std::__finite_max_v<_Tp>),
                    []<totally_ordered _UV>(const _UV& __a, const _UV& __b) {
                      return select(__a < __b, __a, __b);
                    });
    }

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr _Tp
    reduce_max(const basic_simd<_Tp, _Abi>& __x) noexcept
    {
      return reduce(__x, []<totally_ordered _UV>(const _UV& __a, const _UV& __b) {
               return select(__a < __b, __b, __a);
             });
    }

  template <std::totally_ordered _Tp, typename _Abi>
    constexpr _Tp
    reduce_max(const basic_simd<_Tp, _Abi>& __x,
               const typename basic_simd<_Tp, _Abi>::mask_type& __k) noexcept
    {
      return reduce(select(__k, __x, std::__finite_min_v<_Tp>),
                    []<totally_ordered _UV>(const _UV& __a, const _UV& __b) {
                      return select(__a < __b, __b, __a);
                    });
    }
}

#endif  // PROTOTYPE_SIMD_REDUCTIONS_H_
