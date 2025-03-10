# C++26 Data-parallel types (simd)

[![GCC](https://github.com/GSI-HPC/simd/actions/workflows/GCC.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/GCC.yml)
[![Clang](https://github.com/GSI-HPC/simd/actions/workflows/Clang.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/Clang.yml)
[![REUSE status](https://github.com/GSI-HPC/simd/actions/workflows/reuse.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/reuse.yml)
[![fair-software.eu](https://img.shields.io/badge/fair--software.eu-%E2%97%8F%20%20%E2%97%8B%20%20%E2%97%8B%20%20%E2%97%8B%20%20%E2%97%8B-red)](https://fair-software.eu)

## Implementation status

| Feature | Status |
| ------- | ------ |
| P1928R15 std::simd — merge data-parallel types from the Parallelism TS 2 | done (exceptions see below) |
| P3430R3 simd issues: explicit, unsequenced, identity-element position, and members of disabled simd | done |
| P3441R2 Rename simd_split to simd_chunk                           | done        |
| P3287R3 Exploration of namespaces for std::simd                   | done        |
| P2933R4 Extend ⟨bit⟩ header function with overloads for std::simd | done        |
| P2663R7 Interleaved complex values support in std::simd           | not started |

## Missing features

- `simd<std::float16_t>` is currently disabled / not implemented

## Build, install, use?

This implementation is not intended for use yet. Nevertheless, it should be 
usable for experimenting. There is very simple build system support for 
installation via:
```sh
make prefix=~/.local install
```
But you can also just point your compiler's include path to this repository 
instead.

Build your own project with C++26 (latest GCC, PRs to support Clang are 
welcome). Just include `<simd>` and you're good to go:
```c++
#include <simd>
```

To build the tests there are multiple targets available. `make help` will list 
all of them.

## Optional/exploratory features:

### `-D SIMD_IS_A_RANGE=1`

If you turn this on you get an implementation of 
[P3480](https://wg21.link/p3480) which makes `simd` and `simd_mask` read-only 
random-access ranges.

### `-D RANGES_TO_SIMD=1`

`std::array{1, 2, 3} | std::ranges::to<dp::basic_simd>()` works without this, 
and constructs a `simd<int, 3>`. However, ranges without static extent and 
non-contiguous ranges are not supported without this feature.

Enables `any_rg | std::ranges::to<dp::simd<T, N>>()`. Precondition: 
`std::ranges::size(any_rg)` is equal to `N`.

### `-D IFNDR_SIMD_PRECONDITIONS=0`

This implementation will render you program ill-formed if it can detect a 
potential precondition violation. This is not conforming, but maybe it should 
be?

The default on precondition violation otherwise is hard UB, which you can 
detect reliably with UBsan. You can change that to a 'trap' if 
`_GLIBCXX_HARDEN` is `>= 3`, or to a more verbose assertion failure by defining 
`_GLIBCXX_ASSERTIONS`.

### `-D SIMD_HAS_SUBSCRIPT_GATHER=1`

Adds a subscript operator to `basic_simd` with integral `basic_simd` argument. 
The operator provides a permute/gather operation.

### `-D SIMD_CONCEPTS=0`

This implementation defines several concepts in the `std::datapar` namespace 
with the same names, but extended to data-parallel types, as corresponding 
concepts from `std`. That's non-conforming and can be disabled.

### `-D SIMD_STD_BYTE=0`

This implementation enables `std::byte` as vectorizable type. As this is not 
conforming, you can disable it.
