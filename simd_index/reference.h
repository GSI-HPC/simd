/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2024–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

/* Reference Proxy (simd_index::reference)
 * =======================================
 *
 * Converts to T.
 * Assignable from T (also compound assignment).
 */
template <class T>
  struct simd_index_const_reference
  {
    using simd_type = T;
    using value_type = typename T::value_type;

    value_type *m_ptr;

    constexpr simd_type get() const
    { return simd_type (m_ptr, stdx::element_aligned); }

    constexpr operator simd_type () const // decay
    { return get(); }

    template <stdx::simd_type<simd_type::size()> V>
      requires std::convertible_to<simd_type, V>
      constexpr operator V() const
      { return V(m_ptr, stdx::element_aligned); }
  };

template <class T>
  struct simd_index_reference : simd_index_const_reference<T>
  {
    using simd_index_const_reference<T>::m_ptr;
    using simd_index_const_reference<T>::get;
    using simd_type = T;

    constexpr void set(stdx::simd_type<simd_type::size()> auto x)
    { x.copy_to(m_ptr, stdx::element_aligned); }

    constexpr simd_index_reference operator=(simd_type x) &&
    { set(x); return {m_ptr}; }

    template <stdx::simd_type<simd_type::size()> V>
      requires std::convertible_to<V, simd_type>
    constexpr simd_index_reference operator=(V x) &&
    { set(x); return {m_ptr}; }

    constexpr simd_index_reference operator+=(simd_type x) &&
    { set(get() + x); return {m_ptr}; }

    constexpr simd_index_reference operator-=(simd_type x) &&
    { set(get() - x); return {m_ptr}; }

    // [...]
  };

/* Simulate a language decay feature for simd_index::reference
 * ===========================================================
 */
template <class T>
  struct std::decay<simd_index_const_reference<T>>
  { using type = T; };

template <class T>
  struct std::decay<simd_index_reference<T>>
  { using type = T; };

template <class T>
  std::decay_t<T>
  _(T &&x)
  { return std::forward<T> (x); }
