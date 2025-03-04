/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "../simd"
namespace dp = std::datapar;

using namespace vir::literals;

/* codegen
^test0a(
vpsignb	xmm0, xmm0, xmm0
ret

^test0b(
vpsignb	xmm0, xmm0, xmm0
ret

^test0c(
vpsignb	xmm0, xmm0, xmm0
ret
*/
using V0 = dp::simd<char, 15>;
V0 test0a(V0::mask_type a)
{ return +a; }

V0 test0b(V0::mask_type a)
{ return dp::select(a, char(1), char()); }

V0 test0c(V0::mask_type a)
{ return dp::select(a, V0(1_cw), V0()); }

/* codegen
^test1a(
mov	eax, edi
and	eax, 65793
ret

^test1b(
mov	eax, edi
and	eax, 65793
ret

^test1c(
mov	eax, edi
and	eax, 65793
ret
*/
using V1 = dp::simd<char, 3>;
V1 test1a(V1::mask_type a)
{ return +a; }

V1 test1b(V1::mask_type a)
{ return dp::select(a, char(1), char()); }

V1 test1c(V1::mask_type a)
{ return dp::select(a, V1(1_cw), V1()); }
