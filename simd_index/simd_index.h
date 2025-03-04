/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
// Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
//                       Matthias Kretz <m.kretz@gsi.de>

#ifndef PROTOTYPE_SIMD_INDEX_SIMD_INDEX_H_
#define PROTOTYPE_SIMD_INDEX_SIMD_INDEX_H_

#include <bit>
#include <ranges>

/* An index type for subscripting simd objects in contiguous ranges of simd::value_type
 * ====================================================================================
 *
 * Implements a subscript operator on contiguous_range for SIMD loads and stores.
 *
 * Provides masked load/store for access at the end of the range.
 *
 * Provides an epilogue helper to iterate over remaining elements in a range with continuously
 *   smaller simd types.
 *
 * Implements comparison against other simd_index types or lower and upper bounds of a range.
 *
 * Implements a ranges::view_interface and thus behaves like a iota_view.
 */
template <class T>
  class simd_index
  : public std::ranges::view_interface<simd_index<T>>
  {
    std::size_t m_first = 0;
    using Base = std::ranges::view_interface<simd_index<T>>;

  public:
    using simd_type = T;
    using mask_type = typename T::mask_type;
    using iterator = simd_index_iterator;
    using const_iterator = iterator;
    using const_reference = simd_index_const_reference<T>;
    using reference = simd_index_reference<T>;

    // ranges::view_interface
    using Base::front;
    using Base::back;
    using Base::empty;
    using Base::operator bool;
    using Base::operator[];

    constexpr static std::size_t
    size() noexcept
    { return simd_type::size(); }

    constexpr
    simd_index(std::size_t i) noexcept
    : m_first (i) {};

    constexpr iterator begin() const noexcept { return {m_first}; }
    constexpr iterator end() const noexcept { return {m_first + size()}; }

    constexpr simd_index&
    operator++() noexcept
    {
      m_first += size();
      return *this;
    }

    constexpr simd_index
    operator++(int) noexcept
    {
      simd_index r = *this;
      m_first += size();
      return r;
    }

    template <std::ranges::contiguous_range R>
      constexpr friend simd_type
      operator[] (const R &range, simd_index i) noexcept
      { return simd_type (std::ranges::data (range) + i.front(), stdx::element_aligned); }

    template <std::ranges::contiguous_range R>
      constexpr friend reference
      operator[] (R &range, simd_index i) noexcept
      { return reference{ std::ranges::data (range) + i.front() }; }

    [[gnu::always_inline]]
    constexpr mask_type
    indices_in_range(std::size_t max) const noexcept
    {
      if (max < std::__finite_max_v<unsigned>) [[likely]]
        {
          auto iota = stdx::rebind_t<unsigned, simd_type>([&](unsigned j) { return j; });
          return stdx::static_simd_cast<mask_type>(iota + unsigned(m_first) < unsigned(max));
        }
      else
        {
          auto iota = stdx::rebind_t<std::size_t, simd_type>([&](auto j) { return j; });
          return stdx::static_simd_cast<mask_type>(iota + m_first < max);
        }
    }

    [[gnu::always_inline]]
    inline constexpr simd_type
    masked_copy_from (const std::ranges::contiguous_range auto& range, simd_type init = simd_type())
      const noexcept
    {
      const mask_type inrange = indices_in_range(range.size());
      where(inrange, init).copy_from(std::ranges::data(range) + m_first, stdx::element_aligned);
      return init;
    }

    [[gnu::always_inline]]
    inline constexpr void
    masked_copy_to (std::ranges::contiguous_range auto& range, simd_type v) const noexcept
    {
      const mask_type inrange = indices_in_range(range.size());
      where(inrange, v).copy_to(std::ranges::data(range) + m_first, stdx::element_aligned);
    }

    constexpr friend auto operator<=>(simd_index a, simd_index b) noexcept
    { return a.front() <=> b.front(); }

    constexpr friend bool
    operator<(simd_index a, std::size_t b) noexcept
    { return size() + a.front() <= b; }

    constexpr friend bool
    operator<=(simd_index a, std::size_t b) noexcept
    { return size() - 1 + a.front() <= b; }

    constexpr friend bool
    operator>(std::size_t a, simd_index b) noexcept
    { return size() + b.front() <= a; }

    constexpr friend bool
    operator>=(std::size_t a, simd_index b) noexcept
    { return size() - 1 + b.front() <= a; }

    constexpr friend bool
    operator==(simd_index, auto) = delete;
    constexpr friend bool
    operator!=(simd_index, auto) = delete;
    constexpr friend bool
    operator==(auto, simd_index) = delete;
    constexpr friend bool
    operator!=(auto, simd_index) = delete;

    template <class F>
      void epilogue(std::size_t range_size, F&& fn) const
      {
        if constexpr (size() >= 2)
          {
            constexpr std::size_t next_vsize = std::bit_ceil(size() / 2);
            simd_index<stdx::resize_t<next_vsize, simd_type>> j = front();
            if (j < range_size)
              fn(j++);
            j.epilogue(range_size, std::forward<F>(fn));
          }
      }
  };

#endif  // PROTOTYPE_SIMD_INDEX_SIMD_INDEX_H_
