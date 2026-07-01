# Implementation Specific Behavior

**29.10.2.2 [simd.expos.abi]**
A single class template `std::simd::_Abi<N, Regs, Var>` is defined to cover all 
necessary ABI tag types.
- `N` is the number of elements in the `basic_vec`/`basic_mask`.
- `Regs` is the (minimum) number of registers needed to hold all `N` elements.
- `Var` is a bit-flag identifying mask and layout variants (vector masks vs bit 
  masks; interleaved vs contiguous storage of real and imaginary parts of 
  `complex<T>`)

**29.10.2.2 [simd.expos.abi]**
There currently is no hard maximum on the number of elements in a data-parallel 
type. However, depending on target architecture and element type, certain 
numbers will be too inefficient to compile and use.

**29.10.2.2 [simd.expos.abi]**
*native-abi*`<T>` is an alias for `_Abi<N, 1, Var>` such that `N` is maximal. 
`Var` prefers bit masks over vector masks and interleaved complex over 
contiguous complex.

**29.10.3 [simd.syn]**
`zero_element` is `INT_MIN` and `uninit_element` is `zero_element + 1`.

**29.10.7.1 [simd.overview]**
A `basic_vec<T, _Abi<N, Regs, Var>>` specialization is enabled if `N >= 1`, 
`Regs >= 1`, and
- `Var` identifies non-complex storage, vector masks, and `T` is an arithmetic type,
- `Var` identifies non-complex storage, bit masks, `T` is an arithmetic type, and the target supports AVX-512,
- `Var` identifies interleaved or contiguous complex storage, vector masks, and `T` is a specialization of `complex<U>`, or
- `Var` identifies interleaved or contiguous complex storage, bit masks, `T` is a specialization of `complex<U>`, and the target supports AVX-512,

**29.10.7.1 [simd.overview]**
- In general, `basic_vec<T, _Abi<N, Regs, Var>>` with arithmetic type `T` can be 
  converted to and from GNU vector types (see GNU attribute 
  `vector_size(Bytes)`) of element type `T` with `std::bit_ceil(N)` elements.
- On x86 targets `basic_vec<T, _Abi<N, 1, Var>>` can be converted to and from
  - `__m128` if `T` is `float` or `std::float32_t`, and `sizeof(basic_vec)` is 16;
  - `__m128d` if `T` is `double` or `std::float64_t`, and `sizeof(basic_vec)` is 16;
  - `__m128h` if `T` is `std::float16_t`, and `sizeof(basic_vec)` is 16;
  - `__m128i` if `T` is integral, and `sizeof(basic_vec)` is 16;
  - `__m256` if `T` is `float` or `std::float32_t`, and `sizeof(basic_vec)` is 32;
  - `__m256d` if `T` is `double` or `std::float64_t`, and `sizeof(basic_vec)` is 32;
  - `__m256h` if `T` is `std::float16_t`, and `sizeof(basic_vec)` is 32;
  - `__m256i` if `T` is integral, and `sizeof(basic_vec)` is 32;
  - `__m512` if `T` is `float` or `std::float32_t`, and `sizeof(basic_vec)` is 64;
  - `__m512d` if `T` is `double` or `std::float64_t`, and `sizeof(basic_vec)` is 64;
  - `__m512h` if `T` is `std::float16_t`, and `sizeof(basic_vec)` is 64;
  - `__m512i` if `T` is integral, and `sizeof(basic_vec)` is 64.

**29.10.9.1 [simd.mask.overview]**
A `basic_mask<Bytes, _Abi<N, Regs, Var>>` specialization is enabled if `N >= 1`, 
`Regs >= 1`, and
- TODO

**29.10.9.1 [simd.mask.overview]**
- If `Var` does not identify a vector mask, no additional conversions are 
  available.

- If `Var` identifies non-complex element types, then `basic_mask<Bytes, _Abi<N, 
  Regs, Var>>` can be converted to and from GNU vector types of element type 
  `T`, where `T` is the first type of `signed char`, `signed short`, `signed 
  int`, `signed long long`, or `signed long` with `sizeof(T) == Bytes`. The 
  number of elements in the GNU vector type must be equal to `std::bit_ceil(N)`.

- If `Var` identifies complex element types, `basic_mask<Bytes, _Abi<N, Regs, 
  Var>>` can be converted to and from GNU vector types of element type `T`, 
  where `T` is the first type of `signed char`, `signed short`, `signed int`, 
  `signed long long`, or `signed long` with `sizeof(T) == Bytes / 2` (matching 
  the `sizeof` a single real/imag element in the `complex` value-type).

  - If `Var` identifies interleaved storage and `N > 1`, the number of elements 
    in the GNU vector type must be equal to `std::bit_ceil(N * 2)`.

  - Otherwise, the number of elements in the GNU vector type must be equal to 
    `std::bit_ceil(N)`.

## License and Copyright
SPDX-License-Identifier: CC-BY-4.0
Copyright © 2026      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
                      Matthias Kretz <m.kretz@gsi.de>
