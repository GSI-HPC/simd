/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "../simd"

#include <numeric>
#include <bit>

namespace std
{
  namespace __detail
  {
    template <unsigned __n>
      requires(std::has_single_bit(__n))
      struct __prefix_sum_permutation
      {
        constexpr unsigned
        operator()(unsigned __i) const
        { return __i & __n ? (__i ^ __n) | (__n - 1) : __i; }
      };

    constexpr void
    __for_template(auto __begin, auto __end, auto&& __f)
    {
      [&]<int... _Is>(std::integer_sequence<int, _Is...>) {
        (__f(simd::__detail::__ic<__begin + _Is>), ...);
      }(std::make_integer_sequence<int, __end - __begin>());
    }

    constexpr void
    __for_template(auto __end, auto&& __f)
    {
      [&]<int... _Is>(std::integer_sequence<int, _Is...>) {
        (__f(simd::__detail::__ic<_Is>), ...);
      }(std::make_integer_sequence<int, __end>());
    }
  }

  template <typename _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    inclusive_scan(basic_simd<_Tp, _Abi> __v, auto&& __binary_op)
    {
      using _V = basic_simd<_Tp, _Abi>;
      using _M = typename _V::mask_type;
      using _Ip = __detail::__make_signed_int_t<_Tp>;
      using _IV = rebind_simd_t<_Ip, _V>;
      __detail::__for_template(
        __detail::__ic<std::bit_width(__v.size()) - 1>, [&](auto __i) {
          constexpr int __n = 1 << __i;
          const _M __k = std::bit_cast<_M>((simd_iota<_IV> & _IV(__n)) != 0);
          const _V __permuted = permute(__v, __detail::__prefix_sum_permutation<__n>{});
          __v = __k ? __binary_op(__v, __permuted) : __v;
        });
      return __v;
    }

  template <typename _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    inclusive_scan(basic_simd<_Tp, _Abi> const& __v)
    { return inclusive_scan(__v, std::plus<>()); }

  template <typename _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    scaled_inclusive_scan(basic_simd<_Tp, _Abi> __v, _Tp __a, auto&& __binary_op)
    {
      using _V = basic_simd<_Tp, _Abi>;
      using _M = typename _V::mask_type;
      using _Ip = __detail::__make_signed_int_t<_Tp>;
      using _IV = rebind_simd_t<_Ip, _V>;
      _V __factor = __a;
      __detail::__for_template(
        __detail::__ic<std::bit_width(__v.size()) - 1>, [&](auto __i) {
          constexpr int __n = 1 << __i;
          const _M __k = std::bit_cast<_M>((simd_iota<_IV> & _IV(__n)) != 0);
          const _V __permuted = permute(__v, __detail::__prefix_sum_permutation<__n>{});
          __v = __k ? __binary_op(__v, __factor * __permuted) : __v;
          __factor = __k ? __a * __factor : __factor;
          __a *= __a;
        });
      return __v;
    }

  template <typename _Tp, typename _Abi>
    constexpr basic_simd<_Tp, _Abi>
    scaled_inclusive_scan(basic_simd<_Tp, _Abi> const& __v, _Tp __a)
    { return scaled_inclusive_scan(__v, __a, std::plus<>()); }

} // namespace std

namespace simd = std;

auto f(float last, std::span<float> data)
{
  using V = simd::simd<float>;
  constexpr float a = 0.125f;
  constexpr float b = 1.f - a;
  constexpr auto aa = simd::inclusive_scan(V(a), std::multiplies<>());
  for (std::size_t i = 0; i < 1024; i += V::size)
    {
      __asm volatile("# LLVM-MCA-BEGIN simd":::"memory");
      auto x = simd::simd_unchecked_load<V>(data.begin() + i, data.end());
      V y = aa * last + scaled_inclusive_scan(b * x, a);
      last = y[V::size - 1];
      store(y, data.begin() + i, data.end());
      __asm volatile("# LLVM-MCA-END":::"memory");
    }
}

auto f_scalar(float last, std::span<float> data)
{
  constexpr float a = 0.125f;
  constexpr float b = 1.f - a;
  for (std::size_t i = 0; i < 1024; ++i)
    {
      __asm volatile("# LLVM-MCA-BEGIN scalar":::"memory");
      const float y = a * last + b * data[i];
      last = y;
      data[i] = y;
      __asm volatile("# LLVM-MCA-END":::"memory");
    }
}

constexpr auto x = scaled_inclusive_scan(simd::simd<int>(1), 2);
auto xx = x;
