# C++26 Data-parallel types (simd)

[![REUSE status](https://github.com/GSI-HPC/simd/actions/workflows/reuse.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/reuse.yml)

## Implementation status

| Feature | Status |
| ------- | ------ |
| P1928R15 std::simd — merge data-parallel types from the Parallelism TS 2 | in progress |
| P3430R3 simd issues: explicit, unsequenced, identity-element position, and members of disabled simd | done |
| P3441R2 Rename simd_split to simd_chunk                           | done        |
| P3287R3 Exploration of namespaces for std::simd                   | not started |
| P2933R4 Extend ⟨bit⟩ header function with overloads for std::simd | not started |
| P2663R7 Interleaved complex values support in std::simd           | not started |

## Build, install, use?

This implementation is not intended for use yet. Nevertheless, it should be 
usable for experimenting. There is no build system support for installation. But 
you should be able to point your compiler's include path to this repository and 
simply `#include <simd>`.

To build the tests there are multiple targets available. `make help` will list 
all of them.

## Optional/exploratory features:

### `-D SIMD_IS_A_RANGE=0`

The default setting makes `simd::vec` and `simd::mask` read-only random-access 
ranges. That's not part of P1928, so you can turn it off. However, that breaks 
`simd::iota_v`.

### `-D RANGES_TO_SIMD=1`

`std::array{1, 2, 3} | std::ranges::to<simd::basic_vec>()` works without this, 
and constructs a `vec<int, 3>`. However, ranges without static extent and 
non-contiguous ranges are not supported without this feature.

Enables `any_rg | std::ranges::to<simd::basic_vec>()`. Precondition: 
`std::ranges::size(any_rg)` is equal to the width of the `simd::vec`.

### `-D IFNDR_SIMD_PRECONDITIONS=0`

This implementation will render you program ill-formed if it can detect a 
potential precondition violation. This is not conforming, but maybe it should 
be?

The default on precondition violation otherwise is hard UB, which you can 
detect reliably with UBsan. You can change that to a 'trap' if 
`_GLIBCXX_HARDEN` is `>= 3`, or to a more verbose assertion failure by defining 
`_GLIBCXX_ASSERTIONS`.

### `-D SIMD_IN_STD=1`

Per default the `simd` namespace is nested into the `cpp26` namespace. With 
this flag it's `std::simd` instead.

### `-D SIMD_HAS_SUBSCRIPT_GATHER=1`

Adds a subscript operator to `basic_vec` with integral `basic_vec` argument. 
The operator provides a permute/gather operation.
