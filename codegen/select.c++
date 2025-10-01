/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "../include/simd"

namespace simd = std::simd;

/* codegen
^"test0a(
vpabsb	xmm0, xmm0
ret

^"test0b(
vpabsb	xmm0, xmm0
ret

^"test0c(
vpabsb	xmm0, xmm0
ret

^"test0d(
vmovdqa	xmm0, xmm1
ret

^"test0e(
vmovdqa	xmm0, xmm2
ret

^"test0f(
vpcmpeqd
vpabsb
vpaddb	xmm0,
ret

^"test0g(
vpcmpeqd
vpxor
ret

^"test0h(
ret
*/
using V0 = simd::vec<char, 15>;
V0 test0a(V0::mask_type a)
{ return +a; }

V0 test0b(V0::mask_type a)
{ return simd::select(a, char(1), char(0)); }

V0 test0c(V0::mask_type a)
{ return simd::select(a, V0(1), V0()); }

V0 test0d(V0 a, V0 b, V0 c)
{ return simd::select(a == a, b, c); }

V0 test0e(V0 a, V0 b, V0 c)
{ return simd::select(a != a, b, c); }

V0 test0f(V0::mask_type a)
{ return simd::select(a, char(0), char(1)); }

V0 test0g(V0::mask_type a)
{ return simd::select(a, char(0), char(-1)); }

V0 test0h(V0::mask_type a)
{ return simd::select(a, char(-1), char(0)); }

/* codegen
^"test1a(
mov
and	eax,
ret

^"test1b(
mov
and	eax,
ret

^"test1c(
mov
and	eax,
ret
*/
using T1 = unsigned char;
using V1 = simd::vec<T1, 3>;
auto test1a(V1::mask_type a)
{ return +a; }

V1 test1b(V1::mask_type a)
{ return simd::select(a, T1(1), T1()); }

V1 test1c(V1::mask_type a)
{ return simd::select(a, V1(1), V1()); }

/* codegen
^"test2a(
vpabsw	xmm0, xmm0
ret

^"test2b(
vpabsw	xmm0, xmm0
ret

^"test2c(
vpabsw	xmm0, xmm0
ret

^"test2d(
vmovdqa	xmm0, xmm1
ret

^"test2e(
vmovdqa	xmm0, xmm2
ret
*/
using V2 = simd::vec<short, 7>;
V2 test2a(V2::mask_type a)
{ return +a; }

V2 test2b(V2::mask_type a)
{ return simd::select(a, short(1), short()); }

V2 test2c(V2::mask_type a)
{ return simd::select(a, V2(1), V2()); }

V2 test2d(V2 a, V2 b, V2 c)
{ return simd::select(a == a, b, c); }

V2 test2e(V2 a, V2 b, V2 c)
{ return simd::select(a != a, b, c); }

/* codegen
^"test3a(
vpabsd	ymm0, ymm0
ret

^"test3b(
vpabsd	ymm0, ymm0
ret

^"test3c(
vpabsd	ymm0, ymm0
ret

^"test3d(
vmovdqa	ymm0, ymm1
ret

^"test3e(
vmovdqa	ymm0, ymm2
ret
*/
using V3 = simd::vec<int, 7>;
V3 test3a(V3::mask_type a)
{ return +a; }

V3 test3b(V3::mask_type a)
{ return simd::select(a, 1, 0); }

V3 test3c(V3::mask_type a)
{ return simd::select(a, V3(1), V3()); }

V3 test3d(V3 a, V3 b, V3 c)
{ return simd::select(a == a, b, c); }

V3 test3e(V3 a, V3 b, V3 c)
{ return simd::select(a != a, b, c); }
