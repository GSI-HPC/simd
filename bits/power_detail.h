/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef PROTOTYPE_POWER_DETAIL_H_
#define PROTOTYPE_POWER_DETAIL_H_

#include "detail.h"

#if __powerpc__
namespace std::__detail
{
  struct _ArchFlags
  {
    uint64_t _M_flags = 0
#ifdef _ARCH_PWR10
                          + 5
#elif defined __POWER9_VECTOR__
                          + 4
#elif defined __POWER8_VECTOR__
                          + 3
#elif defined __VSX__
                          + 2
#elif defined __VMX__
                          + 1
#endif
                        ;

    consteval bool _M_vmx()
    { return (_M_flags & 0xf) >= 1; }

    consteval bool _M_vsx()
    { return (_M_flags & 0xf) >= 2; }

    consteval bool _M_power8()
    { return (_M_flags & 0xf) >= 3; }

    consteval bool _M_power9()
    { return (_M_flags & 0xf) >= 4; }

    consteval bool _M_power10()
    { return (_M_flags & 0xf) >= 5; }
  };
}
#endif

#endif // PROTOTYPE_POWER_DETAIL_H_
