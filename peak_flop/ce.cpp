/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "../simd.h"

using V = std::simd<float, std::simd<float>::size() * 8>;

auto peak_ret(V x)
{
  for (int i = 0; i < 1'000'000; ++i)
    x = x * 2.f - 1.f;
  return x;
}
