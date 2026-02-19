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

^"f6(
vextractf128
vunpcklpd
vinsertf128

^"f7(
vunpcklps

^"f8(
?vmovq
?vmovq
?vmovq
?vmovq
vpunpcklqdq
vpunpcklqdq
?vmovdqa
?vmovdqa
vperm2i128

^"f9(
?vmovq
?vmovq
?vmovq
?vmovq
vmovlhps
vmovlhps
?vmovaps
?vmovaps
vinsertf128

^"f10(
?vmovdqa
?vmovd
?vmovd
vinsertps
?vmovd
?vmovd
vinsertps
?vmovq
?vmovq
vpunpcklqdq
?vmovdqa
?vmovdqa
vperm2i128
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

void f6(simd::vec<float, 6> a, simd::vec<float, 2> b) {
  g(simd::cat(a, b));
}

void f7(simd::vec<float, 1> a, simd::vec<float, 1> b) {
  g(simd::cat(a, b));
}

void f8(simd::vec<short, 4> a, simd::vec<short, 4> b,
	simd::vec<short, 4> c, simd::vec<short, 4> d) {
  g(simd::cat(a, b, c, d));
}

void f9(simd::vec<float, 2> a, simd::vec<float, 2> b,
	simd::vec<float, 2> c, simd::vec<float, 2> d) {
  g(simd::cat(a, b, c, d));
}

auto f10(simd::vec<short, 2> a, simd::vec<short, 2> b,
	 simd::vec<short, 2> c, simd::vec<short, 2> d,
	 simd::vec<short, 8> e) {
  g(simd::cat(a, b, c, d, e));
}
