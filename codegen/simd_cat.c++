/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

/* codegen
^f0(
.
.
vinserti128	ymm0, ymm0, xmm1, 1

^f1(
.
.
vinsertf128	ymm0, ymm0, xmm1, 1

^f2(
.
.
vmovlhps	xmm0, xmm0, xmm1
*/

#include "../simd"
namespace dp = std::datapar;

void g(auto);

void f0(dp::simd<int, 4> a, dp::simd<int, 4> b) {
  g(dp::cat(a, b));
}

void f1(dp::simd<float, 4> a, dp::simd<float, 4> b) {
  g(dp::cat(a, b));
}

void f2(dp::simd<float, 2> a, dp::simd<float, 2> b) {
  g(dp::cat(a, b));
}

void f3(dp::simd<float, 3> a, dp::simd<float, 1> b) {
  g(dp::cat(a, b));
}
