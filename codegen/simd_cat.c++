/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

/* codegen
^"f0(
?vmov
?vmov
vinserti128	ymm0, ymm0, xmm1, 1

^"f1(
?vmov
?vmov
vinsertf128	ymm0, ymm0, xmm1, 1

^"f2(
?vmov
?vmov
vmovlhps	xmm0, xmm0, xmm1

^"f3(
vinsertps	xmm0, xmm0, xmm1, 48

^"f4(
vextractf128
vinsertps
?vmovshdup
vinsertps
vinsertf128

^"f5(
?vmov
?vmov
?vmov
v(perm|shuf)
vblendp
*/

#include "../include/simd"
namespace simd = std::simd;

void g(auto);

void f0(simd::vec<int, 4> a, simd::vec<int, 4> b) {
  g(simd::cat(a, b));
}

void f1(simd::vec<float, 4> a, simd::vec<float, 4> b) {
  g(simd::cat(a, b));
}

void f2(simd::vec<float, 2> a, simd::vec<float, 2> b) {
  g(simd::cat(a, b));
}

void f3(simd::vec<float, 3> a, simd::vec<float, 1> b) {
  g(simd::cat(a, b));
}

void f4(simd::vec<float, 5> a, simd::vec<float, 2> b) {
  g(simd::cat(a, b));
}

void f5(simd::vec<float, 5> a, simd::vec<float, 3> b) {
  g(simd::cat(a, b));
}
