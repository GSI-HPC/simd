/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef TESTS_UNITTEST_H_
#define TESTS_UNITTEST_H_

#include "unittest_pch.h"

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
  if constexpr (std::destructible<dp::vec<T>>)
    {
      constexpr int N = Width <= 64 ? Width // 1-64
                                    : 64 + (Width - 64) * dp::vec<T>::size();
      if constexpr (std::destructible<dp::vec<T, N>>)
        {
          static_assert(std::destructible<dp::mask<T, N>>);
          static_assert(std::destructible<typename dp::vec<T, N>::mask_type>);
          static_assert(dp::vec<T, N>::size() == N);
          static_assert(dp::mask<T, N>::size() == N);
          std::cout << "Testing " << type_to_string<dp::vec<T, N>>()
                    << ':' << std::endl;
          run_functions.clear();
          [[maybe_unused]] Tests<dp::vec<T, N>> t = {};
          for (auto f : run_functions)
            f();

          using std::__detail::__canonical_vec_type_t;
          static_assert(std::is_same_v<__canonical_vec_type_t<T>, T>);

          if constexpr (std::is_same_v<__canonical_vec_type_t<std::byte>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<std::byte, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<std::byte, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<__canonical_vec_type_t<long>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<long, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<long, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<__canonical_vec_type_t<unsigned long>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<unsigned long, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<unsigned long, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<__canonical_vec_type_t<char>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<char, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<char, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<__canonical_vec_type_t<char8_t>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<char8_t, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<char8_t, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<__canonical_vec_type_t<char16_t>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<char16_t, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<char16_t, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<__canonical_vec_type_t<char32_t>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<char32_t, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<char32_t, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<__canonical_vec_type_t<wchar_t>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<wchar_t, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<wchar_t, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<__canonical_vec_type_t<_Float64>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<_Float64, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<_Float64, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
          if constexpr (std::is_same_v<__canonical_vec_type_t<_Float32>, T>)
            {
              std::cout << "Testing " << type_to_string<dp::vec<_Float32, N>>()
                        << ':' << std::endl;
              run_functions.clear();
              [[maybe_unused]] Tests<dp::vec<_Float32, N>> t0 = {};
              for (auto f : run_functions)
                f();
            }
        }
      else
        {
          std::cout << "Test type not supported.\n";
          static_assert(not std::default_initializable<dp::vec<T, N>>);
          static_assert(not std::copy_constructible<dp::vec<T, N>>);
          static_assert(not std::is_copy_assignable_v<dp::vec<T, N>>);
        }
    }
  else
    {
      std::cout << "Test type not supported.\n";
      static_assert(not std::default_initializable<dp::vec<T>>);
      static_assert(not std::copy_constructible<dp::vec<T>>);
      static_assert(not std::is_copy_assignable_v<dp::vec<T>>);
    }
}

#endif  // TESTS_UNITTEST_H_
