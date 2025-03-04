/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
// Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
//                       Matthias Kretz <m.kretz@gsi.de>

#include "../simd.h"

#if 0
auto f0(std::simd<int> x)
{
  return x > 0 ? 2 * x : x;
}

auto f1(std::simd<int> x)
{
  return x > 0 ? 1 : 0;
}

auto f2(std::simd<int> x)
{
  return std::simd(x > 0);
}

auto f3(std::simd<int> x)
{
  return -(x > 0);
}

auto f4(std::simd<int> x)
{
  return x > 0 ? -1 : 0;
}

auto f5(std::simd<int> x)
{
  return x > 0 ? true : false;
}
#else
auto f0(std::simd<int> x)
{
  return select(x > 0 , 2 * x , x);
}

auto f1(std::simd<int> x)
{
  return std::datapar::select(x > 0 , 1 , 0.f);
}
auto f1(int x)
{
  return std::datapar::select(x > 0 , ++x , 0.f);
}
auto f1x(int x)
{
  return x > 0 ? ++x : 0.f;
}
#endif

int count_positive(const std::vector<std::simd<float>>& x)
{
  if (x.size() == 0)
    std::unreachable();
  using floatv = std::simd<float>;
  using intv = std::rebind_t<int, floatv>;
  intv counter = {};
  for (std::simd v : x)
    counter += v > 0;
  return reduce(counter);
}

int count_positive_where(const std::vector<std::experimental::native_simd<float>>& x)
{
  if (x.size() == 0)
    std::unreachable();
  using floatv = std::experimental::native_simd<float>;
  using intv = std::experimental::rebind_t<int, floatv>;
  intv counter = {};
  for (std::experimental::simd v : x)
    ++where(std::experimental::__proposed::static_simd_cast<intv::mask_type>(v > 0), counter);
  return reduce(counter);
}
