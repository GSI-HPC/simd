#include <simd>
#include <complex>

using namespace std::simd;

constexpr
bool test(auto x)
{
  return all_of(x == x) and all_of(x <= x) and all_of(x >= x) and none_of(x < x) and none_of(x > x)
    and none_of(x != x);
}

static_assert(__vectorizable<int>);
static_assert(std::same_as<vec<int, 4>::abi_type, _Abi<4, 1>>);
#ifndef __AVX2__
static_assert(std::same_as<vec<int, 5>::abi_type, _Abi<5, 2>>);
static_assert(std::same_as<vec<int, 7>::abi_type, _Abi<7, 2>>);
static_assert(std::same_as<vec<int, 8>::abi_type, _Abi<8, 2>>);
#else
static_assert(std::same_as<vec<int, 5>::abi_type, _Abi<5, 1>>);
static_assert(std::same_as<vec<int, 7>::abi_type, _Abi<7, 1>>);
static_assert(std::same_as<vec<int, 8>::abi_type, _Abi<8, 1>>);
#endif

#ifndef __AVX__
static_assert(vec<std::complex<float>, 4>::abi_type::_S_nreg == 2);
#else
static_assert(vec<std::complex<float>, 4>::abi_type::_S_nreg == 1);
#endif

static_assert(__vectorizable<int>);
static_assert(__simd_vec_type<vec<int, 4>>);
static_assert(std::same_as<__deduce_abi_t<int, 4>, _Abi<4, 1>>);
//static_assert(__simd_mask_type<mask<int, 4>>);

static_assert(std::same_as<rebind_t<unsigned, vec<int, 4>>, vec<unsigned, 4>>);

static_assert(test(vec<int, 4>(0)));
static_assert(test(vec<int, 7>(0)));
static_assert(test(vec<int, 63>(0)));
