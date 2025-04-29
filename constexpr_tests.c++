/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2023–2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "simd"

namespace dp = std::datapar;

namespace test01
{
  using namespace std::__detail;

  int carr4[4] = {};

  static_assert(__static_range_size(std::array<int, 4> {}) == 4);
  static_assert(__static_range_size(carr4) == 4);
  static_assert(__static_range_size(std::span<int, 4>(carr4, 4)) == 4);
  static_assert(__static_range_size(std::span<int>(carr4, 4)) == std::dynamic_extent);
  static_assert(__static_range_size(std::vector<int>()) == std::dynamic_extent);

  static_assert(__static_sized_range<std::array<int, 4>, 4>);

  static_assert(std::same_as<__nopromot_common_type_t<short, signed char>, short>);
  static_assert(std::same_as<__nopromot_common_type_t<short, unsigned char>, short>);
  static_assert(std::same_as<__nopromot_common_type_t<short, unsigned short>, unsigned short>);
  static_assert(std::same_as<__nopromot_common_type_t<short, char>, short>);

  static_assert(    __non_narrowing_constexpr_conversion<_Ic< 1>, float>);
  static_assert(    __non_narrowing_constexpr_conversion<_Ic< 1>, unsigned short>);
  static_assert(not __non_narrowing_constexpr_conversion<_Ic<-1>, unsigned short>);
  static_assert(    __non_narrowing_constexpr_conversion<_Ic<1.f>, unsigned short>);
  static_assert(    __non_narrowing_constexpr_conversion<_Ic<1.>, float>);
  static_assert(not __non_narrowing_constexpr_conversion<_Ic<1.1>, float>);
  static_assert(    __non_narrowing_constexpr_conversion<_Ic<1.1f>, double>);


  static_assert(    __broadcast_constructible<_Ic<1>, float>);
  static_assert(    __broadcast_constructible<_Ic<1.1f>, double>);
  static_assert(not __broadcast_constructible<_Ic<1.1>, float>);

  static_assert(__value_preserving_convertible_to<bool, bool>);
  static_assert(__broadcast_constructible<bool, bool>);
  static_assert(__simd_generator_invokable<decltype([] (int) { return true; }), bool, 4>);

  static_assert(std::is_trivially_copyable_v<_SimdTuple<float, _NativeAbi<float>>>);
  static_assert(std::is_trivially_copyable_v<
                  std::_AbiCombine<63, _NativeAbi<float>>::__traits<float>::_SimdMember>);

#if defined __SSE__ and not defined __AVX__
  static_assert(std::same_as<__deduce_t<float, 7>, std::_AbiCombine<7, std::_VecAbi<4>>>);
  static_assert(std::_VecAbi<7>::_S_size == 7);
  static_assert(std::_VecAbi<7>::_S_full_size == 8);
  static_assert(std::_VecAbi<7>::_IsValid<float>::value == false);
  static_assert(std::_VecAbi<std::__bit_ceil(7u) / 2>::_S_is_partial == false);
  static_assert(std::_VecAbi<std::__bit_ceil(7u) / 2>::_IsValid<float>::value == true);
  static_assert(std::same_as<_AllNativeAbis::_BestPartialAbi<float, 7>, std::_VecAbi<4>>);
  static_assert(std::same_as<__fixed_size_storage_t<float, 7>,
                             _SimdTuple<float, std::_VecAbi<4>, std::_VecAbi<3>>>);

  static_assert(dp::simd<float>::size > 1);
  static_assert(alignof(dp::simd<float>) > alignof(float));
  static_assert(alignof(dp::simd<float, 4>) > alignof(float));
  static_assert(alignof(dp::simd<float, 3>) > alignof(float));
  static_assert(alignof(dp::simd<float, 7>) > alignof(float));
#endif
#if defined __AVX__ and not defined __AVX512F__
  static_assert(std::same_as<__deduce_t<float, 8>, std::_VecAbi<8>>);
  static_assert(std::same_as<__deduce_t<float, 16>, std::_AbiArray<std::_VecAbi<8>, 2>>);
  static_assert(std::same_as<__deduce_t<float, 16>::_SimdMember<float>,
                             std::array<__vec_builtin_type<float, 8>, 2>>);
  static_assert(std::same_as<__deduce_t<float, 16>::_MaskMember<int>,
                             std::array<__vec_builtin_type<int, 8>, 2>>);
  static_assert(std::same_as<dp::simd_mask<float, 16>::abi_type, __deduce_t<float, 16>>);
  static_assert(std::same_as<_SimdMaskTraits<4, __deduce_t<float, 16>>::_MaskMember,
                             std::array<__vec_builtin_type<int, 8>, 2>>);
#endif
}

namespace test02
{
  using namespace std;
  using namespace std::__detail;
  using std::datapar::simd;
  using std::datapar::simd_mask;

  static_assert(not destructible<dp::basic_simd_mask<7>>);

  static_assert(    __complex_like<complex<float>>);
  static_assert(not __complex_like<simd<complex<float>>>);

  static_assert(_AbiMaxSize<float, _VecAbi>::value == simd<float>::size());
  static_assert(_AbiMaxSize<double, _VecAbi>::value == simd<double>::size());
  static_assert(_AbiMaxSize<complex<double>, _VecAbi>::value == simd<complex<double>>::size());

  template <_SimdSizeType N>
    using expected_abi
#ifdef __AVX512F__
      = _Avx512Abi<N>;
#else
      = _VecAbi<N>;
#endif

  static_assert(same_as<__deduce_t<int, 1>, _ScalarAbi>);
  static_assert(same_as<__deduce_t<float, 1>, _ScalarAbi>);
  static_assert(same_as<__deduce_t<complex<float>, 1>, expected_abi<1>>);
  static_assert(same_as<__deduce_t<complex<double>, 1>, expected_abi<1>>);

  static_assert(same_as<simd<int>::mask_type, simd_mask<int>>);
  static_assert(same_as<simd<float>::mask_type, simd_mask<float>>);
  static_assert(same_as<simd<float, 1>::mask_type, simd_mask<float, 1>>);
  static_assert(same_as<simd<complex<float>>::mask_type, simd_mask<complex<float>>>);
  static_assert(same_as<simd<complex<float>, 1>::mask_type, simd_mask<complex<float>, 1>>);
  static_assert(same_as<simd<complex<double>>::mask_type::abi_type,
                        expected_abi<simd<complex<double>>::size()>>);

  // not the same because of the __deduce_t difference above
  static_assert(not same_as<simd<complex<float>, 1>::mask_type, simd<double, 1>::mask_type>);

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
  static_assert(not __math_floating_point<simd<int>>);
  static_assert(    __math_floating_point<simd<float>>);

  template <typename T>
    concept has_type_member = requires { typename T::type; };
  static_assert(not has_type_member<common_type<int, simd<float>>>);

#if __AVX__
  static_assert(_VecAbi<2>::_IsValid<complex<double>>::value);
  static_assert(same_as<_VecAbi<2>::_Rebind<complex<double>>, _VecAbi<2>>);
#endif

#if __AVX512F__
  static_assert(__valid_abi_tag<_Avx512Abi<2>, complex<double>>);
  static_assert(same_as<_NativeAbi<complex<double>>, _Avx512Abi<4>>);
  static_assert(__simd_size_v<complex<double>, _NativeAbi<complex<double>>> == 4);
  static_assert(same_as<__deduce_t<complex<double>, 4>, _Avx512Abi<4>>);
  static_assert(same_as<simd<complex<double>>::abi_type, _Avx512Abi<4>>);
  static_assert(same_as<simd_mask<complex<double>>::abi_type, _Avx512Abi<4>>);
  static_assert(_Avx512Abi<4>::_IsValid<complex<double>>::value);
  static_assert(same_as<_Avx512Abi<4>::_Rebind<complex<double>>, _Avx512Abi<4>>);
  static_assert(same_as<simd<complex<double>>::mask_type::abi_type, _Avx512Abi<4>>);
#endif

  static_assert(__why_is_it_disabled<simd<complex<double>>::mask_type>());
  static_assert(__why_is_it_disabled<simd_mask<complex<double>, 3>>());
  static_assert(__why_is_it_disabled<simd<complex<double>, 3>::mask_type>());
  constexpr simd<complex<double>>::mask_type k = {};
}

#if defined __AVX__ and not defined __AVX2__
static_assert(alignof(dp::simd_mask<int, 8>) == 16);
static_assert(alignof(dp::simd_mask<float, 8>) == 32);
static_assert(alignof(dp::simd_mask<int, 16>) == 16);
static_assert(alignof(dp::simd_mask<float, 16>) == 32);
static_assert(alignof(dp::simd_mask<long long, 4>) == 16);
static_assert(alignof(dp::simd_mask<double, 4>) == 32);
static_assert(alignof(dp::simd_mask<long long, 8>) == 16);
static_assert(alignof(dp::simd_mask<double, 8>) == 32);
static_assert(std::same_as<decltype(+dp::simd_mask<float, 8>()), dp::simd<int, 8>>);
#endif

template <auto X>
  using Ic = std::integral_constant<std::remove_const_t<decltype(X)>, X>;

static_assert(    std::convertible_to<Ic<1>, dp::simd<float>>);
static_assert(not std::convertible_to<Ic<1.1>, dp::simd<float>>);
static_assert(not std::convertible_to<dp::simd<int, 4>, dp::simd<float, 4>>);
static_assert(not std::convertible_to<dp::simd<float, 4>, dp::simd<int, 4>>);
static_assert(not std::convertible_to<int, dp::simd<float>>);
static_assert(    std::convertible_to<dp::simd<int, 4>, dp::simd<double, 4>>);

template <typename V>
  concept has_static_size = requires {
    { V::size } -> std::convertible_to<int>;
    { V::size() } -> std::signed_integral;
    { auto(V::size.value) } -> std::signed_integral;
  };

template <typename V, typename T = typename V::value_type>
  concept usable_simd_or_mask
    = std::is_nothrow_move_constructible_v<V>
        and std::is_nothrow_move_assignable_v<V>
        and std::is_nothrow_default_constructible_v<V>
        and std::is_trivially_copyable_v<V>
        and std::is_standard_layout_v<V>
#if SIMD_IS_A_RANGE
        and std::ranges::random_access_range<V&>
        and not std::ranges::output_range<V&, T>
#endif
        and std::constructible_from<V, V> // broadcast
        and std::__detail::__simd_floating_point<V> == std::floating_point<T>
#if SIMD_CONCEPTS
        and std::datapar::regular<V>
        and std::datapar::equality_comparable<V>
#endif
        and has_static_size<V>
      ;

template <typename V, typename T = typename V::value_type>
  concept usable_simd
    = usable_simd_or_mask<V, T>
        and std::convertible_to<V, std::array<T, V::size()>>
        and std::convertible_to<std::array<T, V::size()>, V>
#if SIMD_CONCEPTS
      // Not for masks because std::integral<bool> is true but datapar::integral looks for a
      // basic_simd specialization.
        and std::datapar::integral<V> == std::integral<T>
      // Not for masks because no implicit conversion from bool -> mask
        and std::datapar::equality_comparable_with<V, T>
        and std::datapar::equality_comparable_with<T, V>
#endif
      ;

template <typename T>
  struct test_usable_simd
  {
    static_assert(not usable_simd<dp::simd<T, 0>>);
    static_assert(not has_static_size<dp::simd<T, 0>>);
    static_assert(usable_simd<dp::simd<T, 1>>);
    static_assert(usable_simd<dp::simd<T, 2>>);
    static_assert(usable_simd<dp::simd<T, 3>>);
    static_assert(usable_simd<dp::simd<T, 4>>);
    static_assert(usable_simd<dp::simd<T, 7>>);
    static_assert(usable_simd<dp::simd<T, 8>>);
    static_assert(usable_simd<dp::simd<T, 16>>);
    static_assert(usable_simd<dp::simd<T, 32>>);
    static_assert(usable_simd<dp::simd<T, 63>>);
    static_assert(usable_simd<dp::simd<T, 64>>);

    static_assert(not has_static_size<dp::simd_mask<T, 0>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 1>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 2>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 3>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 4>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 7>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 8>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 16>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 32>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 63>>);
    static_assert(usable_simd_or_mask<dp::simd_mask<T, 64>>);
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
    //Tpl<std::float16_t> q;
#endif
#ifdef __STDCPP_FLOAT32_T__
    Tpl<std::float32_t> r;
#endif
#ifdef __STDCPP_FLOAT64_T__
    Tpl<std::float64_t> s;
#endif
#if SIMD_STD_BYTE
    Tpl<std::byte> t;
#endif
    Tpl<std::complex<float>> u;
    Tpl<std::complex<double>> v;
  };

template struct instantiate_all_vectorizable<test_usable_simd>;

// simd broadcast ctor ///////////////
namespace test_broadcast
{
  using std::constructible_from;
  using std::complex;
  using std::datapar::simd;

  static_assert(constructible_from<simd<complex<float>>, complex<float>>);
  static_assert(constructible_from<simd<complex<double>>, complex<float>>);

  constexpr simd<complex<double>, 2> cd2 = 1.f; // broadcast real from float
  static_assert(all_of(cd2.real() == 1));
  static_assert(all_of(cd2.imag() == 0));
  static_assert(all_of(cd2 == complex{1.f, 0.f}));
}

// simd generator ctor ///////////////

namespace test_generator
{
  struct udt_convertible_to_float
  { operator float() const; };

  static_assert(    std::constructible_from<dp::simd<float>, float (&)(int)>);
  static_assert(not std::convertible_to<float (&)(int), dp::simd<float>>);
  static_assert(not std::constructible_from<dp::simd<float>, int (&)(int)>);
  static_assert(not std::constructible_from<dp::simd<float>, double (&)(int)>);
  static_assert(    std::constructible_from<dp::simd<float>, short (&)(int)>);
  static_assert(not std::constructible_from<dp::simd<float>, long double (&)(int)>);
  static_assert(    std::constructible_from<dp::simd<float>,
                                            udt_convertible_to_float (&)(int)>);
  static_assert(    std::constructible_from<dp::simd<std::complex<double>>, std::complex<double> (&)(int)>);
  static_assert(    std::constructible_from<dp::simd<std::complex<double>>, std::complex<float> (&)(int)>);
}

// mask generator ctor ///////////////

static_assert(
  all_of(dp::simd_mask<float, 4>([](int) { return true; }) == dp::simd_mask<float, 4>(true)));
static_assert(
  all_of(dp::simd_mask<float, 4>([](int) { return false; }) == dp::simd_mask<float, 4>(false)));
static_assert(
  all_of(dp::simd_mask<float, 4>([](int i) { return i < 2; })
           == dp::simd_mask<float, 4>([](int i) {
                return std::array{true, true, false, false}[i];
              })));

static_assert(all_of((dp::simd<int, 4>([](int i) { return i << 10; }) >> 10)
                == dp::iota<dp::simd<int, 4>>));

// simd iterators /////////////////////

#if SIMD_IS_A_RANGE
static_assert([] { dp::simd<float> x = {}; return x.begin() == x.begin(); }());
static_assert([] { dp::simd<float> x = {}; return x.begin() == x.cbegin(); }());
static_assert([] { dp::simd<float> x = {}; return x.cbegin() == x.begin(); }());
static_assert([] { dp::simd<float> x = {}; return x.cbegin() == x.cbegin(); }());
static_assert([] { dp::simd<float> x = {}; return x.begin() + x.size() == x.end(); }());
static_assert([] { dp::simd<float> x = {}; return x.end() == x.begin() + x.size(); }());
static_assert([] { dp::simd<float> x = {}; return x.begin() < x.end(); }());
static_assert([] { dp::simd<float> x = {}; return x.begin() <= x.end(); }());
static_assert(not [] { dp::simd<float> x = {}; return x.begin() > x.end(); }());
static_assert(not [] { dp::simd<float> x = {}; return x.begin() >= x.end(); }());
static_assert(not [] { dp::simd<float> x = {}; return x.end() < x.begin(); }());
static_assert(not [] { dp::simd<float> x = {}; return x.end() <= x.begin(); }());
static_assert([] { dp::simd<float> x = {}; return x.end() > x.begin(); }());
static_assert([] { dp::simd<float> x = {}; return x.end() >= x.begin(); }());
static_assert([] { dp::simd<float> x = {}; return x.end() - x.begin(); }() == dp::simd<float>::size());
static_assert([] { dp::simd<float> x = {}; return x.begin() - x.end(); }() == -dp::simd<float>::size());
static_assert([] { dp::simd<float> x = {}; return x.begin() - x.begin(); }() == 0);
static_assert([] { dp::simd<float> x = {}; return x.begin() + 1 - x.begin(); }() == 1);
static_assert([] { dp::simd<float> x = {}; return x.begin() + 1 - x.cbegin(); }() == 1);
#endif

// mask to simd ///////////////////////

// Clang says all kinds of expressions are not constant expressions. Why? Come on … explain! 🤷
#ifdef __clang__
#define AVOID_BROKEN_CLANG_FAILURES 1
#endif

#ifndef AVOID_BROKEN_CLANG_FAILURES

static_assert([] constexpr {
  constexpr dp::simd_mask<float, 7> a([](int i) -> bool { return i < 3; });
  constexpr dp::basic_simd b = -a;
  static_assert(b[0] == -(0 < 3));
  static_assert(b[1] == -(1 < 3));
  static_assert(b[2] == -(2 < 3));
  static_assert(b[3] == -(3 < 3));
  return all_of(b == dp::simd<int, 7>([](int i) { return -int(i < 3); }));
}());

static_assert([] constexpr {
  constexpr dp::simd_mask<float, 7> a([](int i) -> bool { return i < 3; });
  constexpr dp::basic_simd b = ~a;
  static_assert(b[0] == ~int(0 < 3));
  static_assert(b[1] == ~int(1 < 3));
  static_assert(b[2] == ~int(2 < 3));
  static_assert(b[3] == ~int(3 < 3));
  return all_of(b == dp::simd<int, 7>([](int i) { return ~int(i < 3); }));
}());

static_assert([] constexpr {
  constexpr dp::simd_mask<float, 4> a([](int i) -> bool { return i < 2; });
  constexpr dp::basic_simd b = a;
  static_assert(b[0] == 1);
  static_assert(b[1] == 1);
  static_assert(b[2] == 0);
  return b[3] == 0;
}());

static_assert([] constexpr {
  // Corner case on AVX w/o AVX2 systems. <float, 5> is an AVX register;
  // <int, 5> is deduced as SSE + scalar.
  constexpr dp::simd_mask<float, 5> a([](int i) -> bool { return i >= 2; });
  constexpr dp::basic_simd b = a;
  static_assert(b[0] == 0);
  static_assert(b[1] == 0);
  static_assert(b[2] == 1);
  static_assert(b[3] == 1);
  static_assert(b[4] == 1);
#if SIMD_MASK_IMPLICIT_CONVERSIONS or defined __AVX2__ or not defined __AVX__
  static_assert(all_of((b == 1) == a));
#endif
  constexpr dp::simd_mask<float, 8> a8([](int i) -> bool { return i <= 4; });
  constexpr dp::basic_simd b8 = a8;
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
  constexpr dp::simd_mask<float, 15> a15([](int i) -> bool { return i <= 4; });
  constexpr dp::basic_simd b15 = a15;
  static_assert(b15[0] == 1);
  static_assert(b15[4] == 1);
  static_assert(b15[5] == 0);
  static_assert(b15[8] == 0);
  static_assert(b15[14] == 0);
  static_assert(all_of((b15 == 1) == a15));
  return true;
}());

static_assert([] constexpr {
  constexpr dp::simd_mask<float, 4> a([](int i) -> bool { return i < 2; });
  constexpr dp::basic_simd b = ~a;
  constexpr dp::basic_simd c = a;
  static_assert(c[0] == int(a[0]));
  static_assert(c[1] == int(a[1]));
  static_assert(c[2] == int(a[2]));
  static_assert(c[3] == int(a[3]));
  static_assert(b[0] == ~int(0 < 2));
  static_assert(b[1] == ~int(1 < 2));
  static_assert(b[2] == ~int(2 < 2));
  static_assert(b[3] == ~int(3 < 2));
  return all_of(b == dp::simd<int, 4>([](int i) { return ~int(i < 2); }));
}());
#endif

// mask conversions //////////////////
namespace mask_conversion_tests
{
  using std::complex;
  using std::datapar::simd_mask;

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
      using M = simd_mask<U, k.size()>;
      if constexpr (std::is_destructible_v<M>)
        {
          if (not std::ranges::equal(M(k), k))
            {
              if constexpr (k.size() <= 64)
                return {1, M(k)._M_to_int(), k._M_to_int()};
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
      if constexpr (std::is_destructible_v<simd_mask<T, N>>)
        {
          constexpr simd_mask<T, N> k([](int i) {
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
      using V = simd_mask<T>;
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
  static_assert(test<complex<float>>());
  static_assert(test<complex<double>>());
}

// simd reductions ///////////////////

namespace simd_reduction_tests
{
  static_assert(reduce(dp::simd<int, 7>(1)) == 7);
  static_assert(reduce(dp::simd<int, 7>(2), std::multiplies<>()) == 128);
  static_assert(reduce(dp::simd<int, 8>(2), std::bit_and<>()) == 2);
  static_assert(reduce(dp::simd<int, 8>(2), std::bit_or<>()) == 2);
  static_assert(reduce(dp::simd<int, 8>(2), std::bit_xor<>()) == 0);
  static_assert(reduce(dp::simd<int, 3>(2), std::bit_and<>()) == 2);
  static_assert(reduce(dp::simd<int, 6>(2), std::bit_and<>()) == 2);
  static_assert(reduce(dp::simd<int, 7>(2), std::bit_and<>()) == 2);
  static_assert(reduce(dp::simd<int, 7>(2), std::bit_or<>()) == 2);
  static_assert(reduce(dp::simd<int, 7>(2), std::bit_xor<>()) == 2);
#ifndef AVOID_BROKEN_CLANG_FAILURES
  static_assert(reduce(dp::simd<int, 4>(2), dp::simd_mask<int, 4>(false)) == 0);
  static_assert(reduce(dp::simd<int, 4>(2), dp::simd_mask<int, 4>(false), std::multiplies<>()) == 1);
  static_assert(reduce(dp::simd<int, 4>(2), dp::simd_mask<int, 4>(false), std::bit_and<>()) == ~0);
  static_assert(reduce(dp::simd<int, 4>(2), dp::simd_mask<int, 4>(false), [](auto a, auto b) {
                  return select(a < b, a, b);
                }, INT_MAX) == INT_MAX);
#endif

  template <typename BinaryOperation>
    concept masked_reduce_works = requires(dp::simd<int, 4> a, dp::simd<int, 4> b) {
      reduce(a, a < b, BinaryOperation());
    };

  static_assert(not masked_reduce_works<std::minus<>>);
}

// mask reductions ///////////////////

static_assert(all_of(dp::simd<float>() == dp::simd<float>()));
static_assert(any_of(dp::simd<float>() == dp::simd<float>()));
static_assert(not none_of(dp::simd<float>() == dp::simd<float>()));
static_assert(reduce_count(dp::simd<float>() == dp::simd<float>()) == dp::simd<float>::size);
static_assert(reduce_min_index(dp::simd<float>() == dp::simd<float>()) == 0);
static_assert(reduce_max_index(dp::simd<float>() == dp::simd<float>()) == dp::simd<float>::size - 1);

// chunk ////////////////////////

static_assert([] {
  constexpr auto a = dp::simd<int, 8>([] (int i) { return i; });
  auto a4 = chunk<dp::simd<int, 4>>(a);
  auto a3 = chunk<dp::simd<int, 3>>(a);
  auto a3_ = chunk<3>(a);
  return a4.size() == 2 and std::same_as<decltype(a4), std::array<dp::simd<int, 4>, 2>>
           and std::tuple_size_v<decltype(a3)> == 3
           and all_of(std::get<0>(a3) == dp::simd<int, 3>([] (int i) { return i; }))
           and all_of(std::get<1>(a3) == dp::simd<int, 3>([] (int i) { return i + 3; }))
           and all_of(std::get<2>(a3) == dp::simd<int, 2>([] (int i) { return i + 6; }))
           and std::same_as<decltype(a3), decltype(a3_)>
           and all_of(std::get<0>(a3) == std::get<0>(a3_));
}());

static_assert([] {
  constexpr dp::simd_mask<int, 8> a([] (int i) -> bool { return i & 1; });
  auto a4 = chunk<dp::simd_mask<int, 4>>(a);
  auto a3 = chunk<dp::simd_mask<int, 3>>(a);
  auto a3_ = chunk<3>(a);
  return a4.size() == 2 and std::same_as<decltype(a4), std::array<dp::simd_mask<int, 4>, 2>>
           and std::tuple_size_v<decltype(a3)> == 3
           and all_of(std::get<0>(a3) == dp::simd_mask<int, 3>(
                                           [] (int i) -> bool { return i & 1; }))
           and all_of(std::get<1>(a3) == dp::simd_mask<int, 3>(
                                           [] (int i) -> bool { return (i + 3) & 1; }))
           and all_of(std::get<2>(a3) == dp::simd_mask<int, 2>(
                                           [] (int i) -> bool { return (i + 6) & 1; }))
           and std::same_as<decltype(a3), decltype(a3_)>
           and all_of(std::get<0>(a3) == std::get<0>(a3_));
}());

// cat ///////////////////////////

static_assert(all_of(dp::cat(dp::iota<dp::simd<int, 3>>, dp::simd<int, 1>(3))
                       == dp::iota<dp::simd<int, 4>>));

static_assert(all_of(dp::cat(dp::iota<dp::simd<int, 4>>, dp::iota<dp::simd<int, 4>> + 4)
                       == dp::iota<dp::simd<int, 8>>));

static_assert(all_of(dp::cat(dp::iota<dp::simd<double, 4>>, dp::iota<dp::simd<double, 2>> + 4)
                       == dp::iota<dp::simd<double, 6>>));

static_assert(all_of(dp::cat(dp::iota<dp::simd<double, 4>>, dp::iota<dp::simd<double, 4>> + 4)
                       == dp::iota<dp::simd<double, 8>>));

// select ////////////////////////

#ifndef AVOID_BROKEN_CLANG_FAILURES
static_assert(all_of(dp::simd<long long, 8>(std::array{0, 0, 0, 0, 4, 4, 4, 4})
                       == select(dp::iota<dp::simd<double, 8>> < 4, 0ll, 4ll)));

static_assert(all_of(dp::simd<int, 8>(std::array{0, 0, 0, 0, 4, 4, 4, 4})
                       == select(dp::iota<dp::simd<float, 8>> < 4.f, 0, 4)));
#endif

// interleave /////////////////////

static_assert(
  all_of(std::get<0>(dp::interleave(dp::iota<dp::simd<int>>))
	   == dp::iota<dp::simd<int>>));

static_assert(
  all_of(std::get<0>(dp::interleave(dp::simd<int>(0), dp::simd<int>(1)))
	   == (dp::iota<dp::simd<int>> & 1)));

static_assert(
  all_of(std::get<1>(dp::interleave(dp::simd<int>(0), dp::simd<int>(1)))
	   == (dp::iota<dp::simd<int>> & 1)));

// permute ////////////////////////

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int>>, dp::permutations::duplicate_even)
	   == dp::iota<dp::simd<int>> / 2 * 2));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int>>, dp::permutations::duplicate_odd)
	   == dp::iota<dp::simd<int>> / 2 * 2 + 1));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int>>, dp::permutations::swap_neighbors<1>)
           == dp::simd<int>([](int i) { return i ^ 1; })));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int, 8>>,
		      dp::permutations::swap_neighbors<2>)
	   == dp::simd<int, 8>(std::array{2, 3, 0, 1, 6, 7, 4, 5})));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int, 12>>,
		      dp::permutations::swap_neighbors<3>)
	   == dp::simd<int, 12>(
		std::array{3, 4, 5, 0, 1, 2, 9, 10, 11, 6, 7, 8})));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int>>, dp::permutations::broadcast<1>)
	   == dp::simd<int>(1)));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int>>, dp::permutations::broadcast_first)
	   == dp::simd<int>(0)));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int>>, dp::permutations::broadcast_last)
           == dp::simd<int>(int(dp::simd<int>::size() - 1))));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int>>, dp::permutations::reverse)
           == dp::simd<int>([](int i) { return int(dp::simd<int>::size()) - 1 - i; })));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int>>, dp::permutations::rotate<1>)
           == (dp::iota<dp::simd<int>> + 1) % int(dp::simd<int>::size())));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int>>, dp::permutations::rotate<2>)
           == (dp::iota<dp::simd<int>> + 2) % int(dp::simd<int>::size())));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int, 7>>, dp::permutations::rotate<2>)
           == dp::simd<int, 7>(std::array {2, 3, 4, 5, 6, 0, 1})));

static_assert(
  all_of(dp::permute(dp::iota<dp::simd<int, 7>>, dp::permutations::rotate<-2>)
           == dp::simd<int, 7>(std::array {5, 6, 0, 1, 2, 3, 4})));

// flags ////////////////////////

static_assert(dp::flags<>()._M_is_equal(dp::flag_default));

static_assert(not dp::flag_aligned._M_is_equal(dp::flag_default));

static_assert(not dp::flag_default._M_is_equal(dp::flag_aligned));

static_assert((dp::flag_default | dp::flag_default)
                ._M_is_equal(dp::flag_default));

static_assert((dp::flag_aligned | dp::flag_default)
                ._M_is_equal(dp::flag_aligned));

static_assert((dp::flag_aligned | dp::flag_aligned)
                ._M_is_equal(dp::flag_aligned));

static_assert((dp::flag_aligned | dp::flag_convert)
                ._M_is_equal(dp::flag_convert | dp::flag_aligned));

static_assert(not ((dp::flag_aligned | dp::flag_convert)
                     ._M_and(dp::flag_aligned))
                ._M_is_equal(dp::flag_convert | dp::flag_aligned));

static_assert(((dp::flag_aligned | dp::flag_convert)
                 ._M_and(dp::flag_aligned))
                ._M_is_equal(dp::flag_aligned));

static_assert(dp::flag_aligned._M_test(dp::flag_aligned));

static_assert(dp::flag_aligned._M_test(dp::flag_default));

static_assert(not dp::flag_default._M_test(dp::flag_aligned));

// simd concepts ///////////////////////////////////
/*
static_assert(ext::simd_regular<int>);
static_assert(ext::simd_regular<dp::simd<int>>);
static_assert(ext::simd_regular<dp::simd_mask<int>>);
*/
// simd.math ///////////////////////////////////////

namespace math_tests
{
  using namespace vir::literals;

  constexpr dp::simd<float, 1>
    operator""_f1(long double x)
  { return float(x); }

  constexpr dp::simd<float, 4>
    operator""_f4(long double x)
  { return float(x); }

  template <typename... Ts>
    concept not_hypot_invocable = not requires(Ts... xs) {
      dp::hypot(xs...);
    };

  template <typename R, typename... Ts>
    concept hypot_invocable_r = requires(Ts... xs) {
      { dp::hypot(xs...) } -> std::same_as<R>;
    };

  template <typename T>
    struct holder
    {
      T value;

      constexpr
      operator const T&() const
      { return value; }
    };

  static_assert(std::same_as<dp::simd<float, 2>,
                             std::__detail::__math_common_simd_t<
                               short, holder<float>, holder<dp::simd<float, 2>>>
                            >);

  static_assert(std::same_as<dp::simd<float, 3>,
                             std::__detail::__math_common_simd_t<
                               holder<dp::simd<float, 3>>, float, holder<short>>
                            >);

  static_assert(std::same_as<dp::simd<float, 3>,
                             std::__detail::__math_common_simd_t<
                               holder<dp::simd<float, 3>>, float, holder<short>,
                               dp::simd<char, 3>>
                            >);

  static_assert(std::same_as<dp::simd<float, 3>,
                             std::__detail::__math_common_simd_t<
                               dp::simd<char, 3>,
                               holder<dp::simd<float, 3>>, float, holder<short>>
                            >);

#ifndef AVOID_BROKEN_CLANG_FAILURES
  static_assert(dp::floor(1.1_f1)[0] == std::floor(1.1f));
  static_assert(dp::floor(dp::basic_simd(std::array{1.1f, 1.2f, 2.f, 3.f}))[0] == std::floor(1.1f));
  static_assert(dp::floor(holder {1.1_f1})[0] == std::floor(1.1f));
  static_assert(dp::hypot(1.1_f1, 1.2_f1)[0] == std::hypot(1.1f, 1.2f));
  static_assert(dp::hypot(1.1_f1, 1.2f)[0] == std::hypot(1.1f, 1.2f));
  // the next doesn't work with the P1928 spec, but it can be made to work
  static_assert(dp::hypot(dp::basic_simd(std::array{1.1f}), 1.2f)[0] == std::hypot(1.1f, 1.2f));
  static_assert(dp::hypot(1.1f, 1.2_f1)[0] == std::hypot(1.1f, 1.2f));
  static_assert(dp::hypot(1_cw, 1.2_f1)[0] == std::hypot(1.f, 1.2f));
  static_assert(dp::hypot(1.2_f1, 1_cw)[0] == std::hypot(1.f, 1.2f));
  static_assert(dp::hypot(holder {1.f}, 1.2_f1)[0] == std::hypot(1.f, 1.2f));
#endif
  // the following must not be valid. if you want simd<double> be explicit about it:
  static_assert(not_hypot_invocable<int, dp::simd<float, 1>>);
  static_assert(not_hypot_invocable<int, dp::simd<float, 1>, dp::simd<float, 1>>);

  static_assert(hypot_invocable_r<dp::simd<float, 1>, holder<float>,
                                  vir::constexpr_wrapper<2>, dp::simd<float, 1>>);
  static_assert(hypot_invocable_r<dp::simd<float, 1>, holder<short>,
                                  dp::simd<float, 1>, float>);
}
