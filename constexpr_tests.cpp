/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "include/simd"
#include <complex>
#include <stdfloat>

namespace simd = std::simd;

using std::complex;

namespace test01
{
  using std::same_as;

  static_assert(same_as<simd::vec<int, 1>::abi_type, simd::_ScalarAbi<1>>);
  static_assert(same_as<simd::vec<float, 1>::abi_type, simd::_ScalarAbi<1>>);

#if defined __SSE__ and not defined __AVX__
  static_assert(same_as<simd::vec<float>::abi_type, simd::_Abi<4, 1>>);
  static_assert(same_as<simd::vec<float, 3>::abi_type, simd::_Abi<3, 1>>);
  static_assert(same_as<simd::vec<float, 7>::abi_type, simd::_Abi<7, 2>>);

  static_assert(simd::vec<float>::size > 1);
  static_assert(alignof(simd::vec<float>) > alignof(float));
  static_assert(alignof(simd::vec<float, 4>) > alignof(float));
  static_assert(alignof(simd::vec<float, 3>) > alignof(float));
  static_assert(sizeof(simd::vec<float, 7>) == 2 * sizeof(simd::vec<float>));
  static_assert(alignof(simd::vec<float, 7>) == alignof(simd::vec<float>));
#endif
}

namespace test02
{
  using namespace std;
  using namespace std::simd;

  static_assert(not destructible<simd::basic_mask<7>>);

  template <int N>
    using expected_abi
#ifdef __AVX512F__
      = _Abi<N, 1, _AbiVariant::_BitMask | _AbiVariant::_CxIleav>;
#else
      = _Abi<N, 1, _AbiVariant::_VecMask | _AbiVariant::_CxIleav>;
#endif

  static_assert(same_as<simd::vec<complex<float>, 1>::abi_type, expected_abi<1>>);
  static_assert(same_as<simd::vec<complex<double>, 1>::abi_type, expected_abi<1>>);

  static_assert(same_as<simd::vec<int>::mask_type, simd::mask<int>>);
  static_assert(same_as<simd::vec<float>::mask_type, simd::mask<float>>);
  static_assert(same_as<simd::vec<float, 1>::mask_type, simd::mask<float, 1>>);

  static_assert(destructible<simd::vec<complex<float>>>);
  static_assert(same_as<simd::vec<complex<float>>::mask_type, simd::mask<complex<float>>>);
  static_assert(same_as<simd::vec<complex<float>, 1>::mask_type, simd::mask<complex<float>, 1>>);
  static_assert(same_as<simd::vec<complex<double>>::mask_type::abi_type,
                        expected_abi<simd::vec<complex<double>>::size()>>);

  // not the same because of the __deduce_t difference above
  static_assert(not same_as<simd::vec<complex<float>, 1>::mask_type, simd::vec<double, 1>::mask_type>);

  static_assert(    __value_preserving_convertible_to<float, double>);
  static_assert(not __value_preserving_convertible_to<double, float>);
  static_assert(    __value_preserving_convertible_to<float, complex<float>>);
  static_assert(    __value_preserving_convertible_to<float, complex<double>>);
  static_assert(    __value_preserving_convertible_to<double, complex<double>>);
  static_assert(not __value_preserving_convertible_to<double, complex<float>>);

  static_assert(not __broadcast_constructible<int, float>);
  static_assert(not __broadcast_constructible<int&, float>);
  static_assert(not __broadcast_constructible<int&&, float>);
  static_assert(not __broadcast_constructible<const int&, float>);
  static_assert(not __broadcast_constructible<const int, float>);

  static_assert(    __broadcast_constructible<complex<float>, complex<float>>);
  static_assert(    __broadcast_constructible<complex<float>, complex<double>>);
  static_assert(not __broadcast_constructible<complex<double>, complex<float>>);

  static_assert(not __math_floating_point<int>);
  static_assert(not __math_floating_point<float>);
  static_assert(not __math_floating_point<simd::vec<int>>);
  static_assert(    __math_floating_point<simd::vec<float>>);

  // ensure 'true ? int : vec<float>' doesn't work
  template <typename T>
    concept has_type_member = requires { typename T::type; };
#ifdef _GLIBCXX_SIMD_CONSTEVAL_BROADCAST
  static_assert(has_type_member<common_type<int, simd::vec<float>>>);
#else
  static_assert(not has_type_member<common_type<int, simd::vec<float>>>);
#endif

  constexpr simd::vec<complex<double>>::mask_type k = {};
}

#if defined __AVX__ and not defined __AVX2__
static_assert(alignof(simd::mask<int, 8>) == 16);
static_assert(alignof(simd::mask<float, 8>) == 32);
static_assert(alignof(simd::mask<int, 16>) == 16);
static_assert(alignof(simd::mask<float, 16>) == 32);
static_assert(alignof(simd::mask<long long, 4>) == 16);
static_assert(alignof(simd::mask<double, 4>) == 32);
static_assert(alignof(simd::mask<long long, 8>) == 16);
static_assert(alignof(simd::mask<double, 8>) == 32);
static_assert(std::same_as<decltype(+simd::mask<float, 8>()), simd::vec<int, 8>>);
#endif

#if defined __SSE__ and not defined __F16C__
static_assert(simd::vec<std::float16_t>::size() == 1);
static_assert(simd::vec<std::complex<std::float16_t>>::size() == 1);
static_assert(simd::mask<std::float16_t>::size() == 1);
static_assert(simd::mask<std::complex<std::float16_t>>::size() == 1);
static_assert(alignof(simd::vec<std::float16_t, 8>) == alignof(std::float16_t));
static_assert(alignof(simd::rebind_t<std::float16_t, simd::vec<float>>) == alignof(std::float16_t));
static_assert(simd::rebind_t<std::float16_t, simd::mask<float>>::abi_type::_S_nreg
                == simd::vec<float>::size());
#endif

template <auto X>
  using Ic = std::integral_constant<std::remove_const_t<decltype(X)>, X>;

static_assert(    std::convertible_to<Ic<1>, simd::vec<float>>);
static_assert(not std::convertible_to<Ic<1.1>, simd::vec<float>>);
static_assert(not std::convertible_to<simd::vec<int, 4>, simd::vec<float, 4>>);
static_assert(not std::convertible_to<simd::vec<float, 4>, simd::vec<int, 4>>);
#ifdef _GLIBCXX_SIMD_CONSTEVAL_BROADCAST
static_assert(    std::convertible_to<int, simd::vec<float>>);
#else
static_assert(not std::convertible_to<int, simd::vec<float>>);
#endif
static_assert(    std::convertible_to<simd::vec<int, 4>, simd::vec<double, 4>>);

template <typename V>
  concept has_static_size = requires {
    { V::size } -> std::convertible_to<int>;
    { V::size() } -> std::signed_integral;
    { auto(V::size.value) } -> std::signed_integral;
  };

template <typename V, typename T = typename V::value_type>
  concept usable_vec_or_mask
    = std::destructible<V>
        and std::is_nothrow_move_constructible_v<V>
        and std::is_nothrow_move_assignable_v<V>
        and std::is_nothrow_default_constructible_v<V>
        and std::is_trivially_copyable_v<V>
        and std::is_standard_layout_v<V>
        and std::ranges::random_access_range<V&>
        and not std::ranges::output_range<V&, T>
        and std::constructible_from<V, T> // broadcast
#if SIMD_CONCEPTS
        and simd::regular<V>
        and simd::equality_comparable<V>
#endif
        and has_static_size<V>
      ;

template <typename V, typename T = typename V::value_type>
  concept usable_vec
    = usable_vec_or_mask<V, T>
        and not std::convertible_to<V, std::array<T, V::size()>>
        and std::convertible_to<std::array<T, V::size()>, V>
#if SIMD_CONCEPTS
      // Not for masks because std::integral<bool> is true but datapar::integral looks for a
      // basic_vec specialization.
        and simd::integral<V> == std::integral<T>
      // Not for masks because no implicit conversion from bool -> mask
        and simd::equality_comparable_with<V, T>
        and simd::equality_comparable_with<T, V>
#endif
      ;

template <typename M, typename T = typename M::value_type>
  concept usable_mask
    = std::is_same_v<T, bool>
        and usable_vec_or_mask<M, T>
        and std::convertible_to<std::bitset<M::size()>, M>
        and std::constructible_from<M, unsigned long long>
        and std::constructible_from<M, unsigned char>
        and not std::convertible_to<unsigned long long, M>
        and not std::convertible_to<unsigned char, M>
        and not std::convertible_to<bool, M>
        and not std::constructible_from<M, std::bitset<M::size() + 1>>
        and not std::constructible_from<M, std::bitset<M::size() - 1>>
        and not std::constructible_from<M, int>
        and not std::constructible_from<M, float>
      ;

template <typename T>
  struct test_usable_simd
  {
    static_assert(not usable_vec<simd::vec<T, 0>>);
    static_assert(not has_static_size<simd::vec<T, 0>>);
    static_assert(usable_vec<simd::vec<T, 1>>);
    static_assert(usable_vec<simd::vec<T, 2>>);
    static_assert(usable_vec<simd::vec<T, 3>>);
    static_assert(usable_vec<simd::vec<T, 4>>);
    static_assert(usable_vec<simd::vec<T, 7>>);
    static_assert(usable_vec<simd::vec<T, 8>>);
    static_assert(usable_vec<simd::vec<T, 16>>);
    static_assert(usable_vec<simd::vec<T, 32>>);
    static_assert(usable_vec<simd::vec<T, 63>>);
    static_assert(usable_vec<simd::vec<T, 64>>);

    static_assert(not usable_mask<simd::mask<T, 0>>);
    static_assert(not has_static_size<simd::mask<T, 0>>);
    static_assert(usable_mask<simd::mask<T, 1>>);
    static_assert(usable_mask<simd::mask<T, 2>>);
    static_assert(usable_mask<simd::mask<T, 3>>);
    static_assert(usable_mask<simd::mask<T, 4>>);
    static_assert(usable_mask<simd::mask<T, 7>>);
    static_assert(usable_mask<simd::mask<T, 8>>);
    static_assert(usable_mask<simd::mask<T, 16>>);
    static_assert(usable_mask<simd::mask<T, 32>>);
    static_assert(usable_mask<simd::mask<T, 63>>);
    static_assert(usable_mask<simd::mask<T, 64>>);
  };

template <template <typename> class Tpl>
  struct instantiate_all_vectorizable
  {
    Tpl<float> a;
    Tpl<double> b;
    Tpl<char> c;
    Tpl<char8_t> c8;
    Tpl<char16_t> d;
    Tpl<char32_t> e;
    Tpl<wchar_t> f;
    Tpl<signed char> g;
    Tpl<unsigned char> h;
    Tpl<short> i;
    Tpl<unsigned short> j;
    Tpl<int> k;
    Tpl<unsigned int> l;
    Tpl<long> m;
    Tpl<unsigned long> n;
    Tpl<long long> o;
    Tpl<unsigned long long> p;
#ifdef __STDCPP_FLOAT16_T__
    Tpl<std::float16_t> q;
    Tpl<std::complex<std::float16_t>> qc;
#endif
#ifdef __STDCPP_FLOAT32_T__
    Tpl<std::float32_t> r;
    Tpl<std::complex<std::float32_t>> rc;
#endif
#ifdef __STDCPP_FLOAT64_T__
    Tpl<std::float64_t> s;
    Tpl<std::complex<std::float64_t>> sc;
#endif
#if SIMD_STD_BYTE
    Tpl<std::byte> t;
#endif
    Tpl<std::complex<float>> u;
    Tpl<std::complex<double>> v;
  };

template struct instantiate_all_vectorizable<test_usable_simd>;

// vec broadcast ctor ///////////////
namespace test_broadcast
{
  using std::constructible_from;
  using std::complex;
  using simd::vec;

  static_assert(constructible_from<simd::vec<complex<float>>, complex<float>>);
  static_assert(constructible_from<simd::vec<complex<double>>, complex<float>>);

  constexpr simd::vec<complex<double>, 2> cd2 = 1.f; // broadcast real from float
  static_assert(all_of(cd2.real() == 1));
  static_assert(all_of(cd2.imag() == 0));
  static_assert(all_of(cd2 == complex{1.f, 0.f}));
}

// vec generator ctor ///////////////

namespace test_generator
{
  struct udt_convertible_to_float
  { operator float() const; };

  static_assert(    std::constructible_from<simd::vec<float>, float (&)(int)>);
  static_assert(not std::convertible_to<float (&)(int), simd::vec<float>>);
  static_assert(not std::constructible_from<simd::vec<float>, int (&)(int)>);
  static_assert(not std::constructible_from<simd::vec<float>, double (&)(int)>);
  static_assert(    std::constructible_from<simd::vec<float>, short (&)(int)>);
  static_assert(not std::constructible_from<simd::vec<float>, long double (&)(int)>);
  static_assert(    std::constructible_from<simd::vec<float>,
                                            udt_convertible_to_float (&)(int)>);
  static_assert(    std::constructible_from<simd::vec<std::complex<double>>, std::complex<double> (&)(int)>);
  static_assert(    std::constructible_from<simd::vec<std::complex<double>>, std::complex<float> (&)(int)>);
}

// mask generator ctor ///////////////

static_assert(
  all_of(simd::mask<float, 4>([](int) { return true; }) == simd::mask<float, 4>(true)));
static_assert(
  all_of(simd::mask<float, 4>([](int) { return false; }) == simd::mask<float, 4>(false)));
static_assert(
  all_of(simd::mask<float, 4>([](int i) { return i < 2; })
           == simd::mask<float, 4>([](int i) {
                return std::array{true, true, false, false}[i];
              })));

static_assert(all_of((simd::vec<int, 4>([](int i) { return i << 10; }) >> 10)
                == simd::__iota<simd::vec<int, 4>>));

// vec iterators /////////////////////

#if SIMD_IS_A_RANGE
static_assert([] { simd::vec<float> x = {}; return x.begin() == x.begin(); }());
static_assert([] { simd::vec<float> x = {}; return x.begin() == x.cbegin(); }());
static_assert([] { simd::vec<float> x = {}; return x.cbegin() == x.begin(); }());
static_assert([] { simd::vec<float> x = {}; return x.cbegin() == x.cbegin(); }());
static_assert([] { simd::vec<float> x = {}; return x.begin() + x.size() == x.end(); }());
static_assert([] { simd::vec<float> x = {}; return x.end() == x.begin() + x.size(); }());
static_assert([] { simd::vec<float> x = {}; return x.begin() < x.end(); }());
static_assert([] { simd::vec<float> x = {}; return x.begin() <= x.end(); }());
static_assert(not [] { simd::vec<float> x = {}; return x.begin() > x.end(); }());
static_assert(not [] { simd::vec<float> x = {}; return x.begin() >= x.end(); }());
static_assert(not [] { simd::vec<float> x = {}; return x.end() < x.begin(); }());
static_assert(not [] { simd::vec<float> x = {}; return x.end() <= x.begin(); }());
static_assert([] { simd::vec<float> x = {}; return x.end() > x.begin(); }());
static_assert([] { simd::vec<float> x = {}; return x.end() >= x.begin(); }());
static_assert([] { simd::vec<float> x = {}; return x.end() - x.begin(); }() == simd::vec<float>::size());
static_assert([] { simd::vec<float> x = {}; return x.begin() - x.end(); }() == -simd::vec<float>::size());
static_assert([] { simd::vec<float> x = {}; return x.begin() - x.begin(); }() == 0);
static_assert([] { simd::vec<float> x = {}; return x.begin() + 1 - x.begin(); }() == 1);
static_assert([] { simd::vec<float> x = {}; return x.begin() + 1 - x.cbegin(); }() == 1);
#endif

// mask to vec ///////////////////////

// Clang says all kinds of expressions are not constant expressions. Why? Come on … explain! 🤷
#ifdef __clang__
#define AVOID_BROKEN_CLANG_FAILURES 1
#endif

#ifndef AVOID_BROKEN_CLANG_FAILURES

static_assert([] constexpr {
  constexpr simd::mask<float, 7> a([](int i) -> bool { return i < 3; });
  constexpr simd::basic_vec b = -a;
  static_assert(b[0] == -(0 < 3));
  static_assert(b[1] == -(1 < 3));
  static_assert(b[2] == -(2 < 3));
  static_assert(b[3] == -(3 < 3));
  return all_of(b == simd::vec<int, 7>([](int i) { return -int(i < 3); }));
}());

static_assert([] constexpr {
  constexpr simd::mask<float, 7> a([](int i) -> bool { return i < 3; });
  constexpr simd::basic_vec b = ~a;
  static_assert(b[0] == ~int(0 < 3));
  static_assert(b[1] == ~int(1 < 3));
  static_assert(b[2] == ~int(2 < 3));
  static_assert(b[3] == ~int(3 < 3));
  return all_of(b == simd::vec<int, 7>([](int i) { return ~int(i < 3); }));
}());

static_assert([] constexpr {
  constexpr simd::mask<float, 4> a([](int i) -> bool { return i < 2; });
  constexpr simd::basic_vec b = a;
  static_assert(b[0] == 1);
  static_assert(b[1] == 1);
  static_assert(b[2] == 0);
  return b[3] == 0;
}());

static_assert([] constexpr {
  // Corner case on AVX w/o AVX2 systems. <float, 5> is an AVX register;
  // <int, 5> is deduced as SSE + scalar.
  constexpr simd::mask<float, 5> a([](int i) -> bool { return i >= 2; });
  constexpr simd::basic_vec b = a;
  static_assert(b[0] == 0);
  static_assert(b[1] == 0);
  static_assert(b[2] == 1);
  static_assert(b[3] == 1);
  static_assert(b[4] == 1);
#if defined __AVX2__ or not defined __AVX__
  static_assert(all_of((b == 1) == a));
#endif
  constexpr simd::mask<float, 8> a8([](int i) -> bool { return i <= 4; });
  constexpr simd::basic_vec b8 = a8;
  static_assert(b8[0] == 1);
  static_assert(b8[1] == 1);
  static_assert(b8[2] == 1);
  static_assert(b8[3] == 1);
  static_assert(b8[4] == 1);
  static_assert(b8[5] == 0);
  static_assert(b8[6] == 0);
  static_assert(b8[7] == 0);
#if SIMD_MASK_IMPLICIT_CONVERSIONS or defined __AVX2__ or not defined __AVX__
  static_assert(all_of((b8 == 1) == a8));
#endif
  constexpr simd::mask<float, 15> a15([](int i) -> bool { return i <= 4; });
  constexpr simd::basic_vec b15 = a15;
  static_assert(b15[0] == 1);
  static_assert(b15[4] == 1);
  static_assert(b15[5] == 0);
  static_assert(b15[8] == 0);
  static_assert(b15[14] == 0);
  static_assert(all_of((b15 == 1) == a15));
  return true;
}());

static_assert([] constexpr {
  constexpr simd::mask<float, 4> a([](int i) -> bool { return i < 2; });
  constexpr simd::basic_vec b = ~a;
  constexpr simd::basic_vec c = a;
  static_assert(c[0] == int(a[0]));
  static_assert(c[1] == int(a[1]));
  static_assert(c[2] == int(a[2]));
  static_assert(c[3] == int(a[3]));
  static_assert(b[0] == ~int(0 < 2));
  static_assert(b[1] == ~int(1 < 2));
  static_assert(b[2] == ~int(2 < 2));
  static_assert(b[3] == ~int(3 < 2));
  return all_of(b == simd::vec<int, 4>([](int i) { return ~int(i < 2); }));
}());
#endif

// mask conversions //////////////////
namespace mask_conversion_tests
{
  using std::complex;
  using simd::mask;

  struct TestResult
  {
    int state;
    unsigned long long a, b;
  };

  template <auto Res>
    consteval void
    check()
    {
      if constexpr (Res.state != 0 and Res.a != Res.b)
        static_assert(Res.a == Res.b);
      else
        static_assert(Res.state == 0);
    }

  template <typename U>
    consteval TestResult
    do_test(const auto& k)
    {
      using M = simd::mask<U, k.size()>;
      if constexpr (std::is_destructible_v<M>)
        {
          if (not std::ranges::equal(M(k), k))
            {
              if constexpr (k.size() <= 64)
                return {1, M(k).to_ullong(), k.to_ullong()};
              else
                return {1, 0, 0};
            }
          else
            return {0, 0, 0};
        }
      else
        return {0, 0, 0};
    }

  template <typename T, int N, int P = 0>
    consteval void
    do_test()
    {
      if constexpr (std::is_destructible_v<simd::mask<T, N>>)
        {
          constexpr simd::mask<T, N> k([](int i) {
                      if constexpr (P == 2)
                        return std::has_single_bit(unsigned(i));
                      else if constexpr (P == 3)
                        return not std::has_single_bit(unsigned(i));
                      else
                        return (i & 1) == P;
                    });
          check<do_test<char>(    k)>();
          check<do_test<char>(not k)>();
          check<do_test<short>(    k)>();
          check<do_test<short>(not k)>();
          check<do_test<int>(    k)>();
          check<do_test<int>(not k)>();
          check<do_test<double>(    k)>();
          check<do_test<double>(not k)>();
          check<do_test<std::float16_t>(    k)>();
          check<do_test<std::float16_t>(not k)>();
          check<do_test<complex<float>>(    k)>();
          check<do_test<complex<float>>(not k)>();
          check<do_test<complex<double>>(    k)>();
          check<do_test<complex<double>>(not k)>();
          if constexpr (P <= 2)
            do_test<T, N, P + 1>();
        }
    }

  template <typename T>
    consteval bool
    test()
    {
      using V = simd::mask<T>;
      do_test<T, 1>();
      do_test<T, V::size()>();
      do_test<T, 2 * V::size()>();
      do_test<T, 4 * V::size()>();
      do_test<T, 5 * V::size()>();
      do_test<T, 2 * V::size() + 1>();
      do_test<T, 2 * V::size() - 1>();
      do_test<T, V::size() / 2>();
      do_test<T, V::size() / 3>();
      do_test<T, V::size() / 5>();
      return true;
    }

  static_assert(test<char>());
  static_assert(test<short>());
  static_assert(test<float>());
  static_assert(test<double>());
  static_assert(test<std::float16_t>());
  static_assert(test<complex<float>>());
  static_assert(test<complex<double>>());
}

// vec reductions ///////////////////

namespace simd_reduction_tests
{
  static_assert(reduce(simd::vec<int, 7>(1)) == 7);
  static_assert(reduce(simd::vec<int, 7>(2), std::multiplies<>()) == 128);
  static_assert(reduce(simd::vec<int, 8>(2), std::bit_and<>()) == 2);
  static_assert(reduce(simd::vec<int, 8>(2), std::bit_or<>()) == 2);
  static_assert(reduce(simd::vec<int, 8>(2), std::bit_xor<>()) == 0);
  static_assert(reduce(simd::vec<int, 3>(2), std::bit_and<>()) == 2);
  static_assert(reduce(simd::vec<int, 6>(2), std::bit_and<>()) == 2);
  static_assert(reduce(simd::vec<int, 7>(2), std::bit_and<>()) == 2);
  static_assert(reduce(simd::vec<int, 7>(2), std::bit_or<>()) == 2);
  static_assert(reduce(simd::vec<int, 7>(2), std::bit_xor<>()) == 2);
#ifndef AVOID_BROKEN_CLANG_FAILURES
  static_assert(reduce(simd::vec<int, 4>(2), simd::mask<int, 4>(false)) == 0);
  static_assert(reduce(simd::vec<int, 4>(2), simd::mask<int, 4>(false), std::multiplies<>()) == 1);
  static_assert(reduce(simd::vec<int, 4>(2), simd::mask<int, 4>(false), std::bit_and<>()) == ~0);
  static_assert(reduce(simd::vec<int, 4>(2), simd::mask<int, 4>(false), [](auto a, auto b) {
                  return select(a < b, a, b);
                }, __INT_MAX__) == __INT_MAX__);
#endif

  template <typename BinaryOperation>
    concept masked_reduce_works = requires(simd::vec<int, 4> a, simd::vec<int, 4> b) {
      reduce(a, a < b, BinaryOperation());
    };

  static_assert(not masked_reduce_works<std::minus<>>);
}

// mask reductions ///////////////////

static_assert(all_of(simd::vec<float>() == simd::vec<float>()));
static_assert(any_of(simd::vec<float>() == simd::vec<float>()));
static_assert(not none_of(simd::vec<float>() == simd::vec<float>()));
static_assert(reduce_count(simd::vec<float>() == simd::vec<float>()) == simd::vec<float>::size);
static_assert(reduce_min_index(simd::vec<float>() == simd::vec<float>()) == 0);
static_assert(reduce_max_index(simd::vec<float>() == simd::vec<float>()) == simd::vec<float>::size - 1);

// chunk ////////////////////////

static_assert([] {
  constexpr auto a = simd::vec<int, 8>([] (int i) { return i; });
  auto a4 = chunk<simd::vec<int, 4>>(a);
  auto a3 = chunk<simd::vec<int, 3>>(a);
  auto a3_ = chunk<3>(a);
  return a4.size() == 2 and std::same_as<decltype(a4), std::array<simd::vec<int, 4>, 2>>
           and std::tuple_size_v<decltype(a3)> == 3
           and all_of(std::get<0>(a3) == simd::vec<int, 3>([] (int i) { return i; }))
           and all_of(std::get<1>(a3) == simd::vec<int, 3>([] (int i) { return i + 3; }))
           and all_of(std::get<2>(a3) == simd::vec<int, 2>([] (int i) { return i + 6; }))
           and std::same_as<decltype(a3), decltype(a3_)>
           and all_of(std::get<0>(a3) == std::get<0>(a3_));
}());

static_assert([] {
  constexpr simd::mask<int, 8> a([] (int i) -> bool { return i & 1; });
  auto a4 = chunk<simd::mask<int, 4>>(a);
  auto a3 = chunk<simd::mask<int, 3>>(a);
  auto a3_ = chunk<3>(a);
  return a4.size() == 2 and std::same_as<decltype(a4), std::array<simd::mask<int, 4>, 2>>
           and std::tuple_size_v<decltype(a3)> == 3
           and all_of(std::get<0>(a3) == simd::mask<int, 3>(
                                           [] (int i) -> bool { return i & 1; }))
           and all_of(std::get<1>(a3) == simd::mask<int, 3>(
                                           [] (int i) -> bool { return (i + 3) & 1; }))
           and all_of(std::get<2>(a3) == simd::mask<int, 2>(
                                           [] (int i) -> bool { return (i + 6) & 1; }))
           and std::same_as<decltype(a3), decltype(a3_)>
           and all_of(std::get<0>(a3) == std::get<0>(a3_));
}());

// cat ///////////////////////////

static_assert(all_of(simd::cat(simd::__iota<simd::vec<int, 3>>, simd::vec<int, 1>(3))
                       == simd::__iota<simd::vec<int, 4>>));

static_assert(all_of(simd::cat(simd::__iota<simd::vec<int, 4>>, simd::__iota<simd::vec<int, 4>> + 4)
                       == simd::__iota<simd::vec<int, 8>>));

static_assert(all_of(simd::cat(simd::__iota<simd::vec<double, 4>>, simd::__iota<simd::vec<double, 2>> + 4)
                       == simd::__iota<simd::vec<double, 6>>));

static_assert(all_of(simd::cat(simd::__iota<simd::vec<double, 4>>, simd::__iota<simd::vec<double, 4>> + 4)
                       == simd::__iota<simd::vec<double, 8>>));

static_assert(all_of(simd::cat(simd::__iota<simd::vec<complex<float>, 1>>,
                               simd::__iota<simd::vec<complex<float>, 1>> + 1.f)
                       == simd::__iota<simd::vec<complex<float>, 2>>));

static_assert(all_of(simd::cat(simd::__iota<simd::vec<complex<float>, 3>>,
                               simd::__iota<simd::vec<complex<float>, 3>> + 3.f)
                       == simd::__iota<simd::vec<complex<float>, 6>>));

static_assert(all_of(simd::cat(simd::__iota<simd::vec<complex<float>, 8>>,
                               simd::__iota<simd::vec<complex<float>, 8>> + 8.f)
                       == simd::__iota<simd::vec<complex<float>, 16>>));

// select ////////////////////////

#ifndef AVOID_BROKEN_CLANG_FAILURES
static_assert(all_of(simd::vec<long long, 8>(std::array{0, 0, 0, 0, 4, 4, 4, 4})
                       == select(simd::__iota<simd::vec<double, 8>> < 4, 0ll, 4ll)));

static_assert(all_of(simd::vec<int, 8>(std::array{0, 0, 0, 0, 4, 4, 4, 4})
                       == select(simd::__iota<simd::vec<float, 8>> < 4.f, 0, 4)));
#endif

// permute ////////////////////////

namespace permutations
{
  struct _DuplicateEven
  {
    consteval unsigned
    operator()(unsigned __i) const
    { return __i & ~1u; }
  };

  inline constexpr _DuplicateEven duplicate_even {};

  struct _DuplicateOdd
  {
    consteval unsigned
    operator()(unsigned __i) const
    { return __i | 1u; }
  };

  inline constexpr _DuplicateOdd duplicate_odd {};

  template <unsigned _Np>
    struct _SwapNeighbors
    {
      consteval unsigned
      operator()(unsigned __i, unsigned __size) const
      {
        if (__size % (2 * _Np) != 0)
          abort(); // swap_neighbors<N> permutation requires a multiple of 2N elements
        else if (std::has_single_bit(_Np))
          return __i ^ _Np;
        else if (__i % (2 * _Np) >= _Np)
          return __i - _Np;
        else
          return __i + _Np;
      }
    };

  template <unsigned _Np = 1u>
    inline constexpr _SwapNeighbors<_Np> swap_neighbors {};

  template <int _Position>
    struct _Broadcast
    {
      consteval int
      operator()(int, int __size) const
      { return _Position < 0 ? __size + _Position : _Position; }
    };

  template <int _Position>
    inline constexpr _Broadcast<_Position> broadcast {};

  inline constexpr _Broadcast<0> broadcast_first {};

  inline constexpr _Broadcast<-1> broadcast_last {};

  struct _Reverse
  {
    consteval int
    operator()(int __i, int __size) const
    { return __size - 1 - __i; }
  };

  inline constexpr _Reverse reverse {};

  template <int _Offset>
    struct _Rotate
    {
      consteval int
      operator()(int __i, int __size) const
      {
        __i += _Offset;
        __i %= __size;
        if (__i < 0)
          __i += __size;
        return __i;
      }
    };

  template <int _Offset>
    inline constexpr _Rotate<_Offset> rotate {};

  template <int _Offset>
    struct _Shift
    {
      consteval int
      operator()(int __i, int __size) const
      {
        const int __j = __i + _Offset;
        if (__j >= __size or -__j > __size)
          return simd::zero_element;
        else if (__j < 0)
          return __size + __j;
        else
          return __j;
      }
    };

  template <int _Offset>
    inline constexpr _Shift<_Offset> shift {};
}

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int>>, permutations::duplicate_even)
	   == simd::__iota<simd::vec<int>> / 2 * 2));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int>>, permutations::duplicate_odd)
	   == simd::__iota<simd::vec<int>> / 2 * 2 + 1));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int>>, permutations::swap_neighbors<1>)
           == simd::vec<int>([](int i) { return i ^ 1; })));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int, 8>>,
		      permutations::swap_neighbors<2>)
	   == simd::vec<int, 8>(std::array{2, 3, 0, 1, 6, 7, 4, 5})));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int, 12>>,
		      permutations::swap_neighbors<3>)
	   == simd::vec<int, 12>(
		std::array{3, 4, 5, 0, 1, 2, 9, 10, 11, 6, 7, 8})));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int>>, permutations::broadcast<1>)
	   == simd::vec<int>(1)));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int>>, permutations::broadcast_first)
	   == simd::vec<int>(0)));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int>>, permutations::broadcast_last)
           == simd::vec<int>(int(simd::vec<int>::size() - 1))));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int>>, permutations::reverse)
           == simd::vec<int>([](int i) { return int(simd::vec<int>::size()) - 1 - i; })));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int>>, permutations::rotate<1>)
           == (simd::__iota<simd::vec<int>> + 1) % int(simd::vec<int>::size())));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int>>, permutations::rotate<2>)
           == (simd::__iota<simd::vec<int>> + 2) % int(simd::vec<int>::size())));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int, 7>>, permutations::rotate<2>)
           == simd::vec<int, 7>(std::array {2, 3, 4, 5, 6, 0, 1})));

static_assert(
  all_of(simd::permute(simd::__iota<simd::vec<int, 7>>, permutations::rotate<-2>)
           == simd::vec<int, 7>(std::array {5, 6, 0, 1, 2, 3, 4})));

// flags ////////////////////////

static_assert(simd::flags<>()._M_is_equal(simd::flag_default));

static_assert(not simd::flag_aligned._M_is_equal(simd::flag_default));

static_assert(not simd::flag_default._M_is_equal(simd::flag_aligned));

static_assert((simd::flag_default | simd::flag_default)
                ._M_is_equal(simd::flag_default));

static_assert((simd::flag_aligned | simd::flag_default)
                ._M_is_equal(simd::flag_aligned));

static_assert((simd::flag_aligned | simd::flag_aligned)
                ._M_is_equal(simd::flag_aligned));

static_assert((simd::flag_aligned | simd::flag_convert)
                ._M_is_equal(simd::flag_convert | simd::flag_aligned));

static_assert(not ((simd::flag_aligned | simd::flag_convert)
                     ._M_and(simd::flag_aligned))
                ._M_is_equal(simd::flag_convert | simd::flag_aligned));

static_assert(((simd::flag_aligned | simd::flag_convert)
                 ._M_and(simd::flag_aligned))
                ._M_is_equal(simd::flag_aligned));

static_assert(simd::flag_aligned._M_test(simd::flag_aligned));

static_assert(simd::flag_aligned._M_test(simd::flag_default));

static_assert(not simd::flag_default._M_test(simd::flag_aligned));

// vec.math ///////////////////////////////////////

#if 0
namespace math_tests
{
  using namespace vir::literals;

  constexpr simd::vec<float, 1>
    operator""_f1(long double x)
  { return float(x); }

  constexpr simd::vec<float, 4>
    operator""_f4(long double x)
  { return float(x); }

  template <typename... Ts>
    concept not_hypot_invocable = not requires(Ts... xs) {
      simd::hypot(xs...);
    };

  template <typename R, typename... Ts>
    concept hypot_invocable_r = requires(Ts... xs) {
      { simd::hypot(xs...) } -> std::same_as<R>;
    };

  template <typename T>
    struct holder
    {
      T value;

      constexpr
      operator const T&() const
      { return value; }
    };

  static_assert(std::same_as<simd::vec<float, 2>,
                             std::__math_common_simd_t<
                               short, holder<float>, holder<simd::vec<float, 2>>>
                            >);

  static_assert(std::same_as<simd::vec<float, 3>,
                             std::__math_common_simd_t<
                               holder<simd::vec<float, 3>>, float, holder<short>>
                            >);

  static_assert(std::same_as<simd::vec<float, 3>,
                             std::__math_common_simd_t<
                               holder<simd::vec<float, 3>>, float, holder<short>,
                               simd::vec<char, 3>>
                            >);

  static_assert(std::same_as<simd::vec<float, 3>,
                             std::__math_common_simd_t<
                               simd::vec<char, 3>,
                               holder<simd::vec<float, 3>>, float, holder<short>>
                            >);

#ifndef AVOID_BROKEN_CLANG_FAILURES
  static_assert(simd::floor(1.1_f1)[0] == std::floor(1.1f));
  static_assert(simd::floor(simd::basic_vec(std::array{1.1f, 1.2f, 2.f, 3.f}))[0] == std::floor(1.1f));
  static_assert(simd::floor(holder {1.1_f1})[0] == std::floor(1.1f));
  static_assert(simd::hypot(1.1_f1, 1.2_f1)[0] == std::hypot(1.1f, 1.2f));
  static_assert(simd::hypot(1.1_f1, 1.2f)[0] == std::hypot(1.1f, 1.2f));
  // the next doesn't work with the P1928 spec, but it can be made to work
  static_assert(simd::hypot(simd::basic_vec(std::array{1.1f}), 1.2f)[0] == std::hypot(1.1f, 1.2f));
  static_assert(simd::hypot(1.1f, 1.2_f1)[0] == std::hypot(1.1f, 1.2f));
  static_assert(simd::hypot(1_cw, 1.2_f1)[0] == std::hypot(1.f, 1.2f));
  static_assert(simd::hypot(1.2_f1, 1_cw)[0] == std::hypot(1.f, 1.2f));
  static_assert(simd::hypot(holder {1.f}, 1.2_f1)[0] == std::hypot(1.f, 1.2f));
#endif
  // the following must not be valid. if you want vec<double> be explicit about it:
  static_assert(not_hypot_invocable<int, simd::vec<float, 1>>);
  static_assert(not_hypot_invocable<int, simd::vec<float, 1>, simd::vec<float, 1>>);

  static_assert(hypot_invocable_r<simd::vec<float, 1>, holder<float>,
                                  vir::constexpr_wrapper<2>, simd::vec<float, 1>>);
  static_assert(hypot_invocable_r<simd::vec<float, 1>, holder<short>,
                                  simd::vec<float, 1>, float>);
}
#endif
