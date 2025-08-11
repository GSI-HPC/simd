/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef TESTS_COMPLEX_INIT_H_
#define TESTS_COMPLEX_INIT_H_

#include <complex>

/**
 * This class is a workaround for std::complex not being allowed in template arguments.
 *
 * All it does is carry the real & imag values until it can "decay" into a std::complex.
 * There's no other interface.
 */
template <typename T>
  struct C
  {
    T re, im;

    template <typename U>
      constexpr operator std::complex<U>() const
      { return {U(re), U(im)}; }
  };

#endif  // TESTS_COMPLEX_INIT_H_
