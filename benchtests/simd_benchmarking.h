/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2019–2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef VIR_SIMD_BENCHMARKING_H_
#define VIR_SIMD_BENCHMARKING_H_

#include "../include/simd"

namespace vir
{
#if defined(__x86_64__) or defined(__i686__)
#define VIR_SIMD_REG "vx"
#else
#define VIR_SIMD_REG
#endif

  template <typename T>
    concept simd_type = std::default_initializable<T> && requires (const T x) {
      typename T::value_type;
      typename T::abi_type;
      { T::size.value } -> std::signed_integral;
      { x[0] } -> std::same_as<typename T::value_type>;
      { all_of(x == x) } -> std::same_as<bool>;
    };

  template <typename T>
    [[gnu::always_inline]]
    inline void
    fake_modify_one(T& x)
    {
      if constexpr (std::is_floating_point_v<T>)
        asm volatile("#MODIFY(vg)\t%0" : "+" VIR_SIMD_REG "g"(x));
      else if constexpr (std::simd::__simd_vec_type<std::remove_const_t<T>>)
        {
          if constexpr (requires {x._M_get();})
            {
              auto xx = x._M_get();
              fake_modify_one(xx);
              const_cast<std::remove_const_t<T>&>(x) = xx;
            }
          else if constexpr (requires {x._M_get_real();})
            {
              fake_modify_one(x._M_get_real());
              fake_modify_one(x._M_get_imag());
            }
          else if constexpr (T::abi_type::_S_is_cx_ileav)
            fake_modify_one(x._M_get_ileav_data());
          else if constexpr (requires {x._M_get_high();})
            {
              fake_modify_one(x._M_get_low());
              fake_modify_one(x._M_get_high());
            }
          else
            static_assert(false);
        }
      else if constexpr (std::simd::__vec_builtin<T>)
        asm volatile("#MODIFY(v)\t%0" : "+" VIR_SIMD_REG (x));
      else if constexpr (std::simd::__complex_like<T>)
        {
          fake_modify_one(std::get<0>(x));
          fake_modify_one(std::get<1>(x));
        }
      else if constexpr (std::is_scalar_v<T>)
        asm volatile("#MODIFY(g)\t%0" : "+" VIR_SIMD_REG "g"(x));
      else
        asm volatile("#MODIFY(m)\t%0" : "+m"(x));
    }

  template <typename... Ts>
    [[gnu::always_inline]]
    inline void
    fake_modify(Ts&... more)
    { (fake_modify_one(more), ...); }

  template <typename T>
    [[gnu::always_inline]]
    inline void
    fake_read_one(const T& x)
    {
      if constexpr (std::is_floating_point_v<T>)
        asm volatile("#READ\t%0" ::VIR_SIMD_REG "g"(x));
      else if constexpr (std::simd::__simd_vec_type<T>)
        {
          if constexpr (requires {x._M_get();})
            fake_read_one(x._M_get());
          else if constexpr (requires {x._M_get_real();})
            {
              fake_read_one(x._M_get_real());
              fake_read_one(x._M_get_imag());
            }
          else if constexpr (T::abi_type::_S_is_cx_ileav)
            fake_read_one(x._M_get_ileav_data());
          else if constexpr (requires {x._M_get_high();})
            {
              fake_read_one(x._M_get_low());
              fake_read_one(x._M_get_high());
            }
          else
            static_assert(false);
        }
      else if constexpr (std::simd::__vec_builtin<T>)
        asm volatile("#READ\t%0" ::VIR_SIMD_REG (x));
      else if constexpr (std::simd::__complex_like<T>)
        {
          fake_read_one(x.real());
          fake_read_one(x.imag());
        }
      else if constexpr (std::is_scalar_v<T>)
        asm volatile("#READ\t%0" ::VIR_SIMD_REG "g"(x));
      else
        asm volatile("#READ\t%0" ::"m"(x));
    }

  template <typename... Ts>
    [[gnu::always_inline]]
    inline void
    fake_read(const Ts&... more)
    { (fake_read_one(more), ...); }

  template <typename T>
    [[gnu::always_inline]]
    inline T
    make_unknown(const T& x)
    {
      T r = x;
      fake_modify_one(r);
      return r;
    }

#undef VIR_SIMD_REG
} // namespace vir

#endif  // VIR_SIMD_BENCHMARKING_H_

// vim: et cc=101 tw=100 sw=2 ts=8
