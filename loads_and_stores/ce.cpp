/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "../simd"

auto f(std::vector<float>& data) {
  using V = std::simd<float>;
  const auto end = data.end();
  for (auto it = data.begin(); it + V::size() <= end; it += V::size()) {
    std::simd<float> x = std::simd_partial_load(it, V::size());
    x += 1.f;
    std::store(x, it, V::size(), x > 0.f);
  }
}

auto runtime_sized(std::vector<float>& data) {
  std::simd<float> x = std::simd_unchecked_load(data, std::flag_throw);
  return x + 1.f;
}

auto compiletime_sized(std::array<float, 8>& data) {
  std::simd<float> x = std::simd_unchecked_load(data, std::flag_throw);
  return x + 1.f;
}

  /*void naive_range_iteration(std::vector<float> &data) {
  using V = std::simd<float>;
  for (auto it = data.begin(); it < data.end(); it += V::size())
    {
      auto chunk = std::ranges::subrange(it, data.end());
      static_assert(std::__detail::__static_range_size<decltype(chunk)> == std::dynamic_extent);
      std::simd<float> x = std::simd_partial_load(chunk, std::flag_default_init);
      x += 1.f;
      std::store(x, chunk);
    }
}

void smart_range_iteration(std::vector<float> &data) {
  using V = std::simd<float>;
  auto it = data.begin();
  auto simd_end = data.end() - V::size();
  for (; it <= simd_end; it += V::size())
    {
      auto chunk = std::ranges::subrange(it, it + V::size());
      std::simd<float> x = std::simd_partial_load(chunk, std::flag_default_init);
      x += 1.f;
      std::store(x, chunk);
    }
  auto chunk = std::ranges::subrange(it, data.end());
  std::simd<float> x = std::simd_partial_load(chunk, std::flag_default_init);
  x += 1.f;
  std::store(x, chunk);
}*/
