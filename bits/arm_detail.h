/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_ARM_DETAIL_H_
#define PROTOTYPE_ARM_DETAIL_H_

#include "detail.h"

#if _GLIBCXX_SIMD_HAVE_NEON or _GLIBCXX_SIMD_HAVE_SVE
namespace std::__detail
{
  struct _ArchFlags
  {
    uint64_t _M_flags = (_GLIBCXX_SIMD_HAVE_NEON << 0)
                          | (_GLIBCXX_SIMD_HAVE_NEON_A32 << 1)
                          | (_GLIBCXX_SIMD_HAVE_NEON_A64 << 2)
                          | (_GLIBCXX_SIMD_HAVE_SVE << 3);

    constexpr bool
    _M_test(int __bit) const
    { return ((_M_flags >> __bit) & 1) == 1; }

    constexpr bool
    _M_have_neon() const
    { return _M_test(0); }

    constexpr bool
    _M_have_neon_a32() const
    { return _M_test(1); }

    constexpr bool
    _M_have_neon_a64() const
    { return _M_test(2); }

    constexpr bool
    _M_have_sve() const
    { return _M_test(3); }
  };
}
#endif

#endif // PROTOTYPE_ARM_DETAIL_H_
