# C++26 Data-parallel types (simd)

[![GCC](https://github.com/GSI-HPC/simd/actions/workflows/GCC.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/GCC.yml)
[![Clang](https://github.com/GSI-HPC/simd/actions/workflows/Clang.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/Clang.yml)
[![REUSE status](https://github.com/GSI-HPC/simd/actions/workflows/reuse.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/reuse.yml)
[![fair-software.eu](https://img.shields.io/badge/fair--software.eu-%E2%97%8F%20%20%E2%97%8B%20%20%E2%97%8B%20%20%E2%97%8B%20%20%E2%97%8B-red)](https://fair-software.eu)

## Implementation status

| Feature | Status |
| ------- | ------ |
| [P1928R15](https://wg21.link/P1928R15) std::simd — merge data-parallel types from the Parallelism TS 2 | ✅ done (except math)  |
| [P3430R3](https://wg21.link/P3430R3) simd issues: explicit, unsequenced, identity-element position, and members of disabled simd | ✅ done |
| [P3441R2](https://wg21.link/P3441R2) Rename simd_split to simd_chunk                           | ✅ done        |
| [P3287R3](https://wg21.link/P3287R3) Exploration of namespaces for std::simd                   | ✅ done        |
| [P2933R4](https://wg21.link/P2933R4) Extend ⟨bit⟩ header function with overloads for std::simd | ✅ done        |
| [P2663R7](https://wg21.link/P2663R7) Interleaved complex values support in std::simd           | ✅ done (except math) |
| [P2664R11](https://wg21.link/P2664R11) Proposal to extend std::simd with permutation API        | 🟡 partial |
| [P2876R3](https://wg21.link/P2876R3) Proposal to extend std::simd with more constructors and accessors | ✅ done |
| [P2933R4](https://wg21.link/P2933R4) std::simd overloads for `<bit>` header                    | ✅ done (not optimized) |
| [P3480R6](https://wg21.link/P3480R6) std::simd is a range                                      | ✅ done         |
| [P3691R1](https://wg21.link/P3691R1) Reconsider naming of the namespace for 'std::simd'        | ✅ done         |
| [P3932R0](https://wg21.link/P3932R0) Fix LWG4470: Fix *integer-from* in [simd]                 | ✅ done         |
| [P4042R0](https://wg21.link/P4042R0) Fix LWG4543: incorrect cast between `simd::vec` and `simd::mask` via conversion to and from impl-defined vector types | ✅ done |
| [P3440R2](https://wg21.link/P3440R2) Add n_elements named constructor to std::simd             | 🔴 open (still in design phase) |
| [P2929R2](https://wg21.link/P2929R2) simd_invoke                                               | 🔴 open (still in design phase) |
| [P2964R2](https://wg21.link/P2964R2) Allowing user-defined types in std::simd                  | 🔴 open (still in design phase) |
| [P3973R0](https://wg21.link/P3973R0) bit_cast_as: Element type reinterpretation for std::simd  | 🔴 open (still in design phase) |
| [P3983R0](https://wg21.link/P3983R0) simd object representation                                | 🔴 open (still in design phase) |
| [P3985R0](https://wg21.link/P3985R0) Concepts for std::simd                                    | 🔴 open (still in design phase) |
| [P3844R4](https://wg21.link/P3844R4) Reword [simd.math] for consteval conversions              | ✅ done         |
| [P4012R1](https://wg21.link/P4012R1) value-preserving consteval broadcast to simd::vec         | ✅ done         |
| [P4042R0](https://wg21.link/P4042R0) Fix LWG4543: incorrect cast between `simd::vec` and `simd::mask` [...] | ✅ done |

| Issue | Status |
| ------- | ------ |
| [LWG4385](https://wg21.link/lwg4385) Including `<simd>` doesn't provide `std::begin/end` | ✅ done (via inclusion of `<span>`) |
| [LWG4409](https://wg21.link/lwg4409) Constant expression `ranges::size(r)` Constraints and Mandates in [simd] | 🔴 open |
| [LWG4420](https://wg21.link/lwg4420) §[simd] conversions (constructor, load, stores, gather, and scatter) are incorrectly constrained for `<stdfloat>` types | ✅ done |
| [LWG4470](https://wg21.link/lwg4470) The use of *integer-from*`<Bytes>` all over [simd] is incorrect for `Bytes=sizeof(complex<double>)` | ✅ done ([P3932R0](https://wg21.link/P3932R0)) |
| [LWG4535](https://wg21.link/lwg4535) Disallow user specialization of `<simd>` templates | ✅ done (not actionable) |
| [LWG4414](https://wg21.link/lwg4414) §[simd.expos.abi] *deduce-abi-t* is underspecified and incorrectly referenced from `rebind` and `resize` | ✅ done ([P3932R0](https://wg21.link/P3932R0)) |
| [LWG4412](https://wg21.link/lwg4412) Fix declaration of `zero_element` and `uninit_element` | ✅ done |
| [LWG4238](https://wg21.link/lwg4238) `simd_mask<complex<double>>::operator+/-/~` return a disabled simd specialization | ✅ done |
| [LWG4231](https://wg21.link/lwg4231) `datapar::chunk<N>` should use *simd-size-type* instead of `size_t` | ✅ done |
| [LWG4518](https://wg21.link/lwg4518) `simd::cat` return type requires inefficient ABI tag change/conversion | ✅ done ([P3932R0](https://wg21.link/P3932R0)) |
| [LWG4413](https://wg21.link/lwg4413) Unused/left-over `simd::alignment` specialization for `basic_mask` | ✅ done |
| [LWG4232](https://wg21.link/lwg4232) `datapar::resize` does not resize | ✅ done |
| [LWG4543](https://wg21.link/lwg4543) Incorrect cast between `simd::vec` and `simd::mask` via conversion to and from impl-defined vector types | ✅ done ([P4042R0](https://wg21.link/P4042R0)) |
| [LWG4390](https://wg21.link/lwg4390) `simd::basic_vec(U&&)` default template parameter | ⚪ treated as NAD |
| [LWG4403](https://wg21.link/lwg4403) `simd::basic_vec` CTAD misses difference type casting | ✅ done |
| [LWG4407](https://wg21.link/lwg4407) *constexpr-wrapper-like* needs `remove_cvref_t` in `simd::basic_vec` constructor | ✅ done |
| [LWG4376](https://wg21.link/lwg4376) ABI tag in return type of [simd.mask.unary] is overconstrained | ✅ done    |
| [LWG4230](https://wg21.link/lwg4230) `simd<complex>::real/imag` is overconstrained               | ✅ done         |
| [LWG4436](https://wg21.link/lwg4436) `simd` broadcast is overconstrained — `std::cw<0.f>` is not convertible to `simd::vec<float16_t>` | 🔴 unclear |
| [LWG4391](https://wg21.link/lwg4391) Ambiguities of `simd::basic_vec` constructor | 🔴 unclear |
| [LWG4408](https://wg21.link/lwg4408) Hardening `simd::vec::operator[]` | 🔴 unclear |
| [LWG4392](https://wg21.link/lwg4392) `simd::unchecked_load` misses difference type casting | 🔴 unclear |
| [LWG4394](https://wg21.link/lwg4394) `simd::unchecked_load(I first, S last)` construct `span` maybe ill-formed | 🔴 unclear |
| [LWG4280](https://wg21.link/lwg4280) `simd::partial_load` uses undefined identifier `T` | 🔴 unclear |
| [LWG4393](https://wg21.link/lwg4393) `simd::unchecked_scatter_to` is underconstrained | 🔴 unclear |
| [LWG4386](https://wg21.link/lwg4386) `std::simd::select(bool c, const T& a, const U& b)` is underconstrained | 🔴 unclear |
| [LWG4375](https://wg21.link/lwg4375) `std::simd::bit_ceil` should not be `noexcept` | 🔴 unclear |
| [LWG4402](https://wg21.link/lwg4402) List-initialization of iterators in [simd.mask.overview] | 🔴 unclear |
| [LWG4382](https://wg21.link/lwg4382) The `simd::basic_mask(bool)` overload needs to be more constrained | 🔴 unclear |


## Build, install, use?

This implementation is not intended for use yet. Nevertheless, it should be 
usable for experimenting. There is very simple build system support for 
installation via:
```sh
make prefix=~/.local install
```
But you can also just point your compiler's include path to this repository 
instead.

Alternatively you can also install into the compiler's standard library 
directory:
```sh
make install-system
```
You can uninstall again with:
```sh
make uninstall-system
```

Build your own project with C++26 (latest GCC, PRs to support Clang are 
welcome). Just include `<simd>` and you're good to go:
```c++
#include <simd>
```

To build the tests there are multiple targets available. `make help` will list 
all of them.

