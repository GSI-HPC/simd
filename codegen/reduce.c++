/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "../include/simd"
namespace simd = std::simd;

/* codegen
^"f0(
vextracti128
vpaddd
vpsrldq
vpaddd
vpsrldq
vpaddd
vmovd	eax
ret
 */
auto
f0(simd::vec<int, 8> x)
{ return reduce(x); }

/* codegen
^"f1(
v(permil|movhl)ps
vaddps
v(permil|shuf)ps
vaddss	xmm0
ret
 */
auto
f1(simd::vec<float, 4> x)
{ return reduce(x); }

/* codegen
^"f2(
?vmovapd	xmm1, xmm0
v(unpckh|permil)pd
vaddsd	xmm0
ret
 */
auto
f2(simd::vec<double, 2> x)
{ return reduce(x); }

/* codegen
^"f3(
vpaddw
vpsrldq
vpaddw
vpsrldq
vpaddw
vpsrldq
vpaddw
vpextrw	eax
ret
 */
auto
f3(simd::vec<unsigned short, 8> x)
{ return reduce(x + x); }

/* codegen
^"f4(
vpaddw
?vmovq
vp(shuflw|srldq)
vpaddw
?vmovq
vp(shuflw|srldq)
vpaddw
vpextrw	eax, xmm., 0
ret
 */
auto
f4(simd::vec<unsigned short, 4> x)
{ return reduce(x + x); }

/* codegen
^"f5(
vpaddb
vpsrldq
vpaddb
vpsrldq
vpaddb
vpsrldq
vpaddb
vpsrldq
vpaddb
vpextrb	eax, xmm., 0
ret
 */
auto
f5(simd::vec<signed char, 16> x)
{ return reduce(x + x); }

/* codegen
^"f6(
vpaddb	xmm., xmm0, xmm0
vpsrldq	xmm., xmm., 8
vpaddb	xmm., xmm., xmm.
vpsrldq	xmm., xmm., 4
vpaddb	xmm., xmm., xmm.
vpsrldq	xmm., xmm., 2
vpaddb	xmm., xmm., xmm.
vpsrldq	xmm., xmm., 1
vpaddb	xmm., xmm., xmm.
vpextrb	eax, xmm., 0
ret
 */
auto
f6(simd::vec<unsigned char, 16> x)
{ return reduce(x + x); }

/* codegen
^"f7(
vpaddq	xmm., xmm0, xmm0
vpsrldq	xmm., xmm0, 8
vpaddq	xmm., xmm., xmm.
vmovq	rax, xmm.
ret
 */
auto
f7(simd::vec<long long, 2> x)
{ return reduce(x + x); }
