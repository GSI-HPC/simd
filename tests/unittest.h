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


  template <typename _Tp>
    struct canonical_vec_type
    { using type = _Tp; };

  template <typename _Tp>
    using canonical_vec_type_t = typename canonical_vec_type<_Tp>::type;

  template <std::same_as<long> _Tp>
    requires (sizeof(_Tp) == sizeof(int))
    struct canonical_vec_type<_Tp>
    { using type = int; };

  template <std::same_as<long> _Tp>
    requires (sizeof(_Tp) == sizeof(long long))
    struct canonical_vec_type<_Tp>
    { using type = long long; };

  template <std::same_as<unsigned long> _Tp>
    requires (sizeof(_Tp) == sizeof(unsigned int))
    struct canonical_vec_type<_Tp>
    { using type = unsigned int; };

  template <std::same_as<unsigned long> _Tp>
    requires (sizeof(_Tp) == sizeof(unsigned long long))
    struct canonical_vec_type<_Tp>
    { using type = unsigned long long; };

  /* Not yet
  template <typename _Tp>
    requires std::is_enum_v<_Tp>
    struct canonical_vec_type<_Tp>
    { using type = canonical_vec_type<std::underlying_type_t<_Tp>>::type; };
   */

  template <>
    struct canonical_vec_type<char>
    { using type = std::conditional_t<std::is_signed_v<char>, signed char, unsigned char>; };

  template <>
    struct canonical_vec_type<char8_t>
    { using type = unsigned char; };

  template <>
    struct canonical_vec_type<char16_t>
    { using type = uint_least16_t; };

  template <>
    struct canonical_vec_type<char32_t>
    { using type = uint_least32_t; };

  template <>
    struct canonical_vec_type<wchar_t>
    {
      using type = std::conditional_t<std::is_signed_v<wchar_t>,
                                      simd::__integer_from<sizeof(wchar_t)>,
                                      simd::_UInt<sizeof(wchar_t)>>;
    };

  template <>
    struct canonical_vec_type<_Float64>
    { using type = double; };

  template <>
    struct canonical_vec_type<_Float32>
    { using type = float; };

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
          std::cout << "Testing " << display_string_of(^^simd::vec<T, N>)
                    << ':' << std::endl;
          run_functions.clear();
          [[maybe_unused]] Tests<simd::vec<T, N>> t = {};
          for (auto f : run_functions)
            f();

          static_assert(std::is_same_v<canonical_vec_type_t<T>, T>);

          if constexpr (std::is_same_v<canonical_vec_type_t<std::byte>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<std::byte, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<std::byte, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<canonical_vec_type_t<long>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<long, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<long, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<canonical_vec_type_t<unsigned long>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<unsigned long, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<unsigned long, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<canonical_vec_type_t<char>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<char, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<char, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<canonical_vec_type_t<char8_t>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<char8_t, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<char8_t, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<canonical_vec_type_t<char16_t>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<char16_t, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<char16_t, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<canonical_vec_type_t<char32_t>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<char32_t, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<char32_t, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<canonical_vec_type_t<wchar_t>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<wchar_t, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<wchar_t, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<canonical_vec_type_t<_Float64>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<_Float64, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<_Float64, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<canonical_vec_type_t<_Float32>, T>)
            {
              std::cout << "Testing " << display_string_of(^^simd::vec<_Float32, N>)
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<simd::vec<_Float32, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
#if __SSSE3__ // ICE without PABS instructions (PR123575) TODO: remove ASAP
          if constexpr (!std::is_same_v<typename simd::vec<T, N>::abi_type, simd::_ScalarAbi<N>>)
            {
              using V = simd::basic_vec<T, simd::_ScalarAbi<N>>;
              std::cout << "Testing " << display_string_of(^^V) << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<V> t0 = {};
              for (auto f : run_functions)
                f();
            }
#endif
          using Abi = typename simd::vec<T, N>::abi_type;
          if constexpr (!simd::__scalar_abi_tag<Abi>)
            {
              if constexpr (complex_like<T>)
                {
                  using V = simd::resize_t<N, simd::basic_vec<T, simd::_Abi<
                              2, 1, unsigned(Abi::_S_variant)
                                      ^ unsigned(simd::_AbiVariant::_CxVariants)>>>;
                  std::cout << "Testing " << display_string_of(^^V) << ':' << std::endl;
                  run_functions.clear();
                  [[maybe_unused]] Tests<V> t0 = {};
                  for (auto f : run_functions)
                    f();
                }
#if 0 // TODO: This needs more implementation work. Vec-masks are only implemented up to 256 bits.
              if constexpr (Abi::_S_is_bitmask)
                {
                  using V = simd::basic_vec<T, simd::_Abi_t<
                              N, Abi::_S_nreg, __filter_abi_variant(
                                                 Abi::_S_variant, simd::_AbiVariant::_CxVariants)>>;
                  std::cout << "Testing " << display_string_of(^^V) << ':' << std::endl;
                  run_functions.clear();
                  [[maybe_unused]] Tests<V> t0 = {};
                  for (auto f : run_functions)
                    f();
                }
#endif
            }
        }
      else
        {
          std::cout << "Test type not supported.\n";
          static_assert(!std::default_initializable<simd::vec<T, N>>);
          static_assert(!std::copy_constructible<simd::vec<T, N>>);
          static_assert(!std::is_copy_assignable_v<simd::vec<T, N>>);
        }
    }
  else
    {
      std::cout << "Test type not supported.\n";
      static_assert(!std::default_initializable<simd::vec<T>>);
      static_assert(!std::copy_constructible<simd::vec<T>>);
      static_assert(!std::is_copy_assignable_v<simd::vec<T>>);
    }
}

#endif  // TESTS_UNITTEST_H_
