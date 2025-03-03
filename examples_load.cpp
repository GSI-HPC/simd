/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#define UB_LOADS 1
#include "simd"

void g(auto);

void cvt(const std::array<float, 4>& x)
{
  g(std::simd(x));
}

void cvt()
{
  int carr[] = {0, 1, 2, 3, 4};
  g(std::simd(carr));
}

void cvt2()
{
  g(std::simd(std::array{0, 1, 2, 3, 4}));
}

void load_ub(const std::vector<float>& x)
{
  g(std::simd_load(x));
}

void load_zero(const std::vector<float>& x)
{
  g(std::simd_load(x, std::simd_flag_default_init));
}

#if 0
void load_IFNDR(const std::vector<float>& x)
{
  std::span<const float> chunk(x.begin(), 7);
  g(std::simd_load(chunk));
}
#endif

void iter_ub1(const std::vector<float>& x)
{
  using V = std::simd<float>;
  V acc {};
  for (auto it = x.begin(); it < x.end(); it += V::size)
    {
      std::span<const float, V::size()> chunk(it, V::size());
      acc += chunk;
    }
  g(acc);
}

void iter_ub2(const std::vector<float>& x)
{
  using V = std::simd<float>;
  V acc {};
  for (auto it = x.begin(); it < x.end(); it += V::size)
    acc += std::simd_load(it, x.end());
  g(acc);
}

void iter_zero1(const std::vector<float>& x)
{
  using V = std::simd<float>;
  V acc {};
  for (auto it = x.begin(); it < x.end(); it += V::size)
    acc += std::simd_load(it, x.end(), std::simd_flag_default_init);
  g(acc);
}

void iter_ub3(const std::vector<float>& x)
{
  using V = std::simd<float>;
  V acc {};
  for (auto it = x.begin(); it < x.end(); it += V::size)
    {
      std::span<const float> chunk(it, V::size());
      acc += std::simd_load(chunk, std::simd_flag_default_init);
    }
  g(acc);
}

void iter_zero_efficient1(const std::vector<float>& x)
{
  using V = std::simd<float>;
  V acc {};
  auto it = x.begin();
  for (; it + V::size() <= x.end(); it += V::size)
    {
      std::span<const float, V::size> chunk(it, V::size());
      acc += chunk;
    }
  std::span<const float> chunk(it, x.end());
  acc += std::simd_load(chunk, std::simd_flag_default_init);
  g(acc);
}

void iter_zero_efficient2(const std::vector<float>& x)
{
  using V = std::simd<float>;
  V acc {};
  auto it = x.begin();
  for (; it + V::size() <= x.end(); it += V::size)
    {
      std::span<const float> chunk(it, x.end());
      acc += std::simd_load(chunk);
    }
  std::span<const float> chunk(it, x.end());
  acc += std::simd_load(chunk, std::simd_flag_default_init);
  g(acc);
}

void iter_zero_sadly_not_efficient(const std::vector<float>& x)
{
  using V = std::simd<float>;
  V acc {};
  auto it = x.begin();
  for (; it + V::size() <= x.end(); it += V::size)
    {
      std::span<const float> chunk(it, x.end());
      acc += std::simd_load(chunk, std::simd_flag_default_init);
    }
  std::span<const float> chunk(it, x.end());
  acc += std::simd_load(chunk, std::simd_flag_default_init);
  g(acc);
}
