/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "../simd"
namespace simd = std;

/* codegen
^f0(
vpsrldq	.*, 8
vpaddd
vpshufd	.*, 229
vpaddd
vmovd	eax
ret
 */
auto
f0(simd::simd<int, 4> x)
{ return reduce(x); }

/* codegen
^f1(
vmovhlps
vaddps
vpermilps	.*, 229
vaddss	xmm0
ret
 */
auto
f1(simd::simd<float, 4> x)
{ return reduce(x); }

/* codegen
^f2(
vpermilpd
vaddsd	xmm0
ret
 */
auto
f2(simd::simd<double, 2> x)
{ return reduce(x); }

/* codegen
^f3(
vpsrldq	xmm., xmm0, 8
vpaddw	xmm., xmm., xmm.
vpshuflw	xmm., xmm., 238
vpaddw	xmm., xmm., xmm.
vpshuflw	xmm., xmm., 229
vpaddw	xmm., xmm., xmm.
vpextrw	eax, xmm., 0
ret
 */
auto
f3(simd::simd<unsigned short, 8> x)
{ return reduce(x); }

/* codegen
^f4(
vmovq	xmm., xmm0
vpshuflw	xmm., xmm., 238
vpaddw	xmm., xmm., xmm.
vpshuflw	xmm., xmm., 229
vpaddw	xmm., xmm., xmm.
vpextrw	eax, xmm., 0
ret
 */
auto
f4(simd::simd<unsigned short, 4> x)
{ return reduce(x); }

/* codegen
^f5(
vmovdqa	xmm1, xmm0
vpsraw	xmm0, xmm0, 8
vpaddw	xmm0, xmm0, xmm1
vpsrldq	xmm1, xmm0, 8
vpaddw	xmm0, xmm0, xmm1
vpshuflw	xmm1, xmm0, 238
vpaddw	xmm0, xmm0, xmm1
vpshuflw	xmm1, xmm0, 229
vpaddw	xmm0, xmm0, xmm1
vpextrw	eax, xmm0, 0
ret
 */
auto
f5(simd::simd<signed char, 16> x)
{ return reduce(x); }

/* codegen
^f6(
vmovdqa	xmm1, xmm0
vpsraw	xmm0, xmm0, 8
vpaddw	xmm0, xmm0, xmm1
vpsrldq	xmm1, xmm0, 8
vpaddw	xmm0, xmm0, xmm1
vpshuflw	xmm1, xmm0, 238
vpaddw	xmm0, xmm0, xmm1
vpshuflw	xmm1, xmm0, 229
vpaddw	xmm0, xmm0, xmm1
vpextrw	eax, xmm0, 0
ret
 */
auto
f6(simd::simd<unsigned char, 16> x)
{ return reduce(x); }

/* codegen
^f7(
vpsrldq	xmm., xmm0, 8
vpaddq	xmm., xmm., xmm.
vmovq	rax, xmm.
ret
 */
auto
f7(simd::simd<long long, 2> x)
{ return reduce(x); }
