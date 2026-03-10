# C++26 Data-parallel types (simd)

[![GCC](https://github.com/GSI-HPC/simd/actions/workflows/GCC.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/GCC.yml)
[![Clang](https://github.com/GSI-HPC/simd/actions/workflows/Clang.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/Clang.yml)
[![REUSE status](https://github.com/GSI-HPC/simd/actions/workflows/reuse.yml/badge.svg)](https://github.com/GSI-HPC/simd/actions/workflows/reuse.yml)
[![fair-software.eu](https://img.shields.io/badge/fair--software.eu-%E2%97%8F%20%20%E2%97%8B%20%20%E2%97%8B%20%20%E2%97%8B%20%20%E2%97%8B-red)](https://fair-software.eu)

## Implementation status

| Feature | Status |
| ------- | ------ |
| P1928R15 std::simd — merge data-parallel types from the Parallelism TS 2 | done  |
| P3430R3 simd issues: explicit, unsequenced, identity-element position, and members of disabled simd | done |
| P3441R2 Rename simd_split to simd_chunk                           | done        |
| P3287R3 Exploration of namespaces for std::simd                   | done        |
| P2933R4 Extend ⟨bit⟩ header function with overloads for std::simd | done        |
| P2663R7 Interleaved complex values support in std::simd           | done (except math) |
| P2664R11 Proposal to extend std::simd with permutation API        | partial |
| P2876R3 Proposal to extend std::simd with more constructors and accessors | done |
| P2933R4 std::simd overloads for `<bit>` header                    | done (not optimized) |
| P3480R6 std::simd is a range                                      | done         |
| P3691R1 Reconsider naming of the namespace for 'std::simd'        | done         |
| LWG4238 `simd_mask<complex<double>>::operator+/-/~` return a disabled simd specialization | done |
| LWG4376 ABI tag in return type of [simd.mask.unary] is overconstrained | done    |
| LWG4420 §[simd] conversions (constructor, load, stores, gather, and scatter) are incorrectly constrained for `<stdfloat>` types | done |
| LWG4230 simd<complex>::real/imag is overconstrained               | done         |

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

