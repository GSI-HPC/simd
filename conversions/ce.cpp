/* SPDX-License-Identifier: GPL-3.0-or-later WITH GCC-exception-3.1 */
// Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
//                       Matthias Kretz <m.kretz@gsi.de>

#include "../simd.h"

template <std::__detail::__vectorizable T, typename U = T>
  requires std::__detail::__simd_type<std::simd<U>>
  using V = std::rebind_t<T, std::simd<U>>;

template <std::__detail::__vectorizable T>
  using V4 = std::resize_t<4, std::simd<T>>;

template <auto X>
  inline constexpr std::integral_constant<std::remove_const_t<decltype(X)>, X> c {};

struct Error {};
using Err = Error;
struct LHS {};
struct RHS {};

template <typename Op1, typename Op2, typename RR>
  void expect()
  {
    using R = std::conditional_t<std::same_as<RR, LHS>,
                                 typename std::conditional_t<std::__detail::__vectorizable<Op1>,
                                                             std::rebind<Op1, Op2>,
                                                             std::type_identity<Op1>>::type,
                                 std::conditional_t<std::same_as<RR, RHS>, Op2, RR>>;
    if constexpr (std::convertible_to<Op1, Op2> or std::convertible_to<Op2, Op1>
                    or requires (Op1 a, Op2 b) { {a + b}; })
      static_assert(std::same_as<R, decltype(std::declval<Op1>() + std::declval<Op2>())>);
    else
      static_assert(std::same_as<R, Error>);
  }

void
test()
{
  using schar = signed char;
  using uchar = unsigned char;
  using sshort = signed short;
  using ushort = unsigned short;
  using sint = signed int;
  using uint = unsigned int;
  using slong = signed long;
  using ulong = unsigned long;
  using sllong = signed long long;
  using ullong = unsigned long long;

  using int2 = std::integral_constant<int, 2>;

  expect< schar, V4<sshort>, RHS>();
  expect< schar, V4<sshort>, RHS>();
  expect< uchar, V4<sshort>, RHS>();
  expect<sshort, V4<sshort>, RHS>();
  expect<ushort, V4<sshort>, Err>();
  expect<  sint, V4<sshort>, Err>();
  expect<  uint, V4<sshort>, Err>();
  expect< slong, V4<sshort>, Err>();
  expect< ulong, V4<sshort>, Err>();
  expect<sllong, V4<sshort>, Err>();
  expect<ullong, V4<sshort>, Err>();
  expect< float, V4<sshort>, Err>();
  expect<double, V4<sshort>, Err>();
  expect<  bool, V4<sshort>, Err>();

  expect<V4< schar>, V4<sshort>, RHS>();
  expect<V4< uchar>, V4<sshort>, RHS>();
  expect<V4<sshort>, V4<sshort>, RHS>();
  expect<V4<ushort>, V4<sshort>, Err>();
  expect<V4<  sint>, V4<sshort>, LHS>();
  expect<V4<  uint>, V4<sshort>, Err>();
  expect<V4< slong>, V4<sshort>, LHS>();
  expect<V4< ulong>, V4<sshort>, Err>();
  expect<V4<sllong>, V4<sshort>, LHS>();
  expect<V4<ullong>, V4<sshort>, Err>();
  expect<V4< float>, V4<sshort>, LHS>();
  expect<V4<double>, V4<sshort>, LHS>();

  expect< schar, V4<ushort>, Err>();
  expect< uchar, V4<ushort>, RHS>();
  expect<sshort, V4<ushort>, Err>();
  expect<ushort, V4<ushort>, RHS>();
  expect<  sint, V4<ushort>, Err>();
  expect<  uint, V4<ushort>, Err>();
  expect< slong, V4<ushort>, Err>();
  expect< ulong, V4<ushort>, Err>();
  expect<sllong, V4<ushort>, Err>();
  expect<ullong, V4<ushort>, Err>();
  expect< float, V4<ushort>, Err>();
  expect<double, V4<ushort>, Err>();
  expect<  bool, V4<ushort>, Err>();

  expect<V4< schar>, V4<ushort>, Err>();
  expect<V4< uchar>, V4<ushort>, RHS>();
  expect<V4<sshort>, V4<ushort>, Err>();
  expect<V4<ushort>, V4<ushort>, RHS>();
  expect<V4<  sint>, V4<ushort>, LHS>();
  expect<V4<  uint>, V4<ushort>, LHS>();
  expect<V4< slong>, V4<ushort>, LHS>();
  expect<V4< ulong>, V4<ushort>, LHS>();
  expect<V4<sllong>, V4<ushort>, LHS>();
  expect<V4<ullong>, V4<ushort>, LHS>();
  expect<V4< float>, V4<ushort>, LHS>();
  expect<V4<double>, V4<ushort>, LHS>();

  expect< schar, V4<uint>, Err>();
  expect< uchar, V4<uint>, RHS>();
  expect<sshort, V4<uint>, Err>();
  expect<ushort, V4<uint>, RHS>();
  expect<  sint, V4<uint>, Err>();
  expect<  uint, V4<uint>, RHS>();
  expect< slong, V4<uint>, Err>();
  expect< ulong, V4<uint>, Err>();
  expect<sllong, V4<uint>, Err>();
  expect<ullong, V4<uint>, Err>();
  expect< float, V4<uint>, Err>();
  expect<double, V4<uint>, Err>();
  expect<  bool, V4<uint>, Err>();

  expect<V4< schar>, V4<uint>, Err>();
  expect<V4< uchar>, V4<uint>, RHS>();
  expect<V4<sshort>, V4<uint>, Err>();
  expect<V4<ushort>, V4<uint>, RHS>();
  expect<V4<  sint>, V4<uint>, Err>();
  expect<V4<  uint>, V4<uint>, RHS>();
  expect<V4< slong>, V4<uint>, LHS>();
  expect<V4< ulong>, V4<uint>, LHS>();
  expect<V4<sllong>, V4<uint>, LHS>();
  expect<V4<ullong>, V4<uint>, LHS>();
  expect<V4< float>, V4<uint>, Err>();
  expect<V4<double>, V4<uint>, LHS>();

  expect< schar, V4<ulong>, Err>();
  expect< uchar, V4<ulong>, RHS>();
  expect<sshort, V4<ulong>, Err>();
  expect<ushort, V4<ulong>, RHS>();
  expect<  sint, V4<ulong>, Err>();
  expect<  uint, V4<ulong>, RHS>();
  expect< slong, V4<ulong>, Err>();
  expect< ulong, V4<ulong>, RHS>();
  expect<sllong, V4<ulong>, Err>();
  expect<ullong, V4<ulong>, RHS>();
  expect< float, V4<ulong>, Err>();
  expect<double, V4<ulong>, Err>();
  expect<  bool, V4<ulong>, Err>();

  expect<V4< schar>, V4<ulong>, Err>();
  expect<V4< uchar>, V4<ulong>, RHS>();
  expect<V4<sshort>, V4<ulong>, Err>();
  expect<V4<ushort>, V4<ulong>, RHS>();
  expect<V4<  sint>, V4<ulong>, Err>();
  expect<V4<  uint>, V4<ulong>, RHS>();
  expect<V4< slong>, V4<ulong>, Err>();
  expect<V4< ulong>, V4<ulong>, RHS>();
  expect<V4<sllong>, V4<ulong>, Err>();
  expect<V4<ullong>, V4<ulong>, LHS>();
  expect<V4< float>, V4<ulong>, Err>();
  expect<V4<double>, V4<ulong>, Err>();

  expect< schar, V4<float>, RHS>();
  expect< uchar, V4<float>, RHS>();
  expect<sshort, V4<float>, RHS>();
  expect<ushort, V4<float>, RHS>();
  expect<  sint, V4<float>, Err>();
  expect<  uint, V4<float>, Err>();
  expect< slong, V4<float>, Err>();
  expect< ulong, V4<float>, Err>();
  expect<sllong, V4<float>, Err>();
  expect<ullong, V4<float>, Err>();
  expect< float, V4<float>, RHS>();
  expect<double, V4<float>, Err>();
  expect<  bool, V4<float>, Err>();

  expect<V4< schar>, V4<float>, RHS>();
  expect<V4< uchar>, V4<float>, RHS>();
  expect<V4<sshort>, V4<float>, RHS>();
  expect<V4<ushort>, V4<float>, RHS>();
  expect<V4<  sint>, V4<float>, Err>();
  expect<V4<  uint>, V4<float>, Err>();
  expect<V4< slong>, V4<float>, Err>();
  expect<V4< ulong>, V4<float>, Err>();
  expect<V4<sllong>, V4<float>, Err>();
  expect<V4<ullong>, V4<float>, Err>();
  expect<V4< float>, V4<float>, RHS>();
  expect<V4<double>, V4<float>, LHS>();

  expect< schar, V4<double>, RHS>();
  expect< uchar, V4<double>, RHS>();
  expect<sshort, V4<double>, RHS>();
  expect<ushort, V4<double>, RHS>();
  expect<  sint, V4<double>, RHS>();
  expect<  uint, V4<double>, RHS>();
  expect< slong, V4<double>, Err>();
  expect< ulong, V4<double>, Err>();
  expect<sllong, V4<double>, Err>();
  expect<ullong, V4<double>, Err>();
  expect< float, V4<double>, RHS>();
  expect<double, V4<double>, RHS>();
  expect<  bool, V4<double>, Err>();

  expect<V4< schar>, V4<double>, RHS>();
  expect<V4< uchar>, V4<double>, RHS>();
  expect<V4<sshort>, V4<double>, RHS>();
  expect<V4<ushort>, V4<double>, RHS>();
  expect<V4<  sint>, V4<double>, RHS>();
  expect<V4<  uint>, V4<double>, RHS>();
  expect<V4< slong>, V4<double>, Err>();
  expect<V4< ulong>, V4<double>, Err>();
  expect<V4<sllong>, V4<double>, Err>();
  expect<V4<ullong>, V4<double>, Err>();
  expect<V4< float>, V4<double>, RHS>();
  expect<V4<double>, V4<double>, RHS>();

static_assert( std::convertible_to<    char, V<short>>);
//static_assert( std::convertible_to<     int, V<short>>);
static_assert(!std::convertible_to<unsigned, V<short>>);
//static_assert( std::convertible_to<     int, V<unsigned short>>);
//static_assert( std::convertible_to<unsigned, V<unsigned short>>);
static_assert( std::convertible_to<     int, V<int>>);
//static_assert( std::convertible_to<     int, V<unsigned int>>);
static_assert( std::convertible_to<     int, V<long>>);
//static_assert( std::convertible_to<     int, V<unsigned long>>);
static_assert(!std::convertible_to<unsigned, V<int>>);
static_assert( std::convertible_to<unsigned, V<unsigned int>>);
static_assert( std::convertible_to<unsigned, V<long>>);
static_assert( std::convertible_to<unsigned, V<unsigned long>>);

static_assert( std::convertible_to<V<char, short>, V<short>>);
static_assert(!std::convertible_to<V<short>, V<char>>);
static_assert( std::convertible_to<V<long>, V<long long>>);
static_assert(!std::convertible_to<V<long long>, V<long>>);
}

void f0(std::common_type_t<V<char, short>, V<signed short>>) {}
//void f1(std::common_type_t<long, V<float>>) {}
//void f2(std::common_type_t<unsigned long long, V<float>>) {}
//void f3(decltype(float() + V<int>())) {}
//void f4(decltype(int() + V<float>())) {}
//void f5(decltype(V<int, float>() + V<float>())) {}
//void f6(decltype(V<long long, float>() + V<float>())) {}
