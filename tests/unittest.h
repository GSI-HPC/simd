/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef TESTS_UNITTEST_H_
#define TESTS_UNITTEST_H_

#include "unittest_pch.h"
#include <stdfloat>

#ifndef UNITTEST_TYPE
#define UNITTEST_TYPE int
#endif
#ifndef UNITTEST_WIDTH
#define UNITTEST_WIDTH 8
#endif

template <typename U>
void test_runner()
{
  using T = std::conditional_t<true, UNITTEST_TYPE, U>; // makes T dependent => constexpr-if works
  constexpr int Width = UNITTEST_WIDTH;
  if constexpr (std::destructible<simd::vec<T>>)
    {
      constexpr int N = Width <= 64 ? Width // 1-64
                                    : 64 + (Width - 64) * simd::vec<T>::size();
      if constexpr (std::destructible<simd::vec<T, N>>)
        {
          static_assert(std::destructible<simd::mask<T, N>>);
          static_assert(std::destructible<typename simd::vec<T, N>::mask_type>);
          static_assert(simd::vec<T, N>::size() == N);
          static_assert(simd::mask<T, N>::size() == N);
          invoke_test_members<simd::vec<T, N>>();
          static_assert(std::is_same_v<canonical_vec_type_t<T>, T>);

          if constexpr (std::is_same_v<canonical_vec_type_t<std::byte>, T>)
            invoke_test_members<simd::vec<std::byte, N>>();
          if constexpr (std::is_same_v<canonical_vec_type_t<long>, T>)
            invoke_test_members<simd::vec<long, N>>();
          if constexpr (std::is_same_v<canonical_vec_type_t<unsigned long>, T>)
            invoke_test_members<simd::vec<unsigned long, N>>();
          if constexpr (std::is_same_v<canonical_vec_type_t<char>, T>)
            invoke_test_members<simd::vec<char, N>>();
          if constexpr (std::is_same_v<canonical_vec_type_t<char8_t>, T>)
            invoke_test_members<simd::vec<char8_t, N>>();
          if constexpr (std::is_same_v<canonical_vec_type_t<char16_t>, T>)
            invoke_test_members<simd::vec<char16_t, N>>();
          if constexpr (std::is_same_v<canonical_vec_type_t<char32_t>, T>)
            invoke_test_members<simd::vec<char32_t, N>>();
          if constexpr (std::is_same_v<canonical_vec_type_t<wchar_t>, T>)
            invoke_test_members<simd::vec<wchar_t, N>>();
          if constexpr (std::is_same_v<canonical_vec_type_t<_Float64>, T>)
            invoke_test_members<simd::vec<_Float64, N>>();
          if constexpr (std::is_same_v<canonical_vec_type_t<_Float32>, T>)
            invoke_test_members<simd::vec<_Float32, N>>();
#if __SSSE3__ // ICE without PABS instructions (PR123575) TODO: remove ASAP
          if constexpr (!std::is_same_v<typename simd::vec<T, N>::abi_type, simd::_ScalarAbi<N>>)
            invoke_test_members<simd::basic_vec<T, simd::_ScalarAbi<N>>>();
#endif
          using Abi = typename simd::vec<T, N>::abi_type;
          if constexpr (!simd::__scalar_abi_tag<Abi>)
            {
              if constexpr (complex_like<T>)
                {
                  using V = simd::resize_t<N, simd::basic_vec<T, simd::_Abi<
                              2, 1, unsigned(Abi::_S_variant)
                                      ^ unsigned(simd::_AbiVariant::_CxVariants)>>>;
                  invoke_test_members<V>();
                }
#if 0 // TODO: This needs more implementation work. Vec-masks are only implemented up to 256 bits.
              if constexpr (Abi::_S_is_bitmask)
                {
                  using V = simd::basic_vec<T, simd::_Abi_t<
                              N, Abi::_S_nreg, __filter_abi_variant(
                                                 Abi::_S_variant, simd::_AbiVariant::_CxVariants)>>;
                  invoke_test_members<V>();
                }
#endif
            }
        }
      else
        {
          std::puts("Test type not supported.");
          static_assert(!std::default_initializable<simd::vec<T, N>>);
          static_assert(!std::copy_constructible<simd::vec<T, N>>);
          static_assert(!std::is_copy_assignable_v<simd::vec<T, N>>);
        }
    }
  else
    {
      std::puts("Test type not supported.");
      static_assert(!std::default_initializable<simd::vec<T>>);
      static_assert(!std::copy_constructible<simd::vec<T>>);
      static_assert(!std::is_copy_assignable_v<simd::vec<T>>);
    }
}

#endif  // TESTS_UNITTEST_H_
