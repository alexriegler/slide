# slide

> An implementation of `std::ranges::views::slide` based on [P2442R1](https://wg21.link/p2442r1).

## About
This project is an attempt at implementing `std::ranges::views::slide` based on [P2442R1: Windowing range adaptors: `views::chunk` and `views::slide`](https://wg21.link/p2442r1) by Tim Song. At the moment, it seems only bidirectional ranges, i.e., something like `std::vector`, work with the library (bidirectional ranges were the easiest case to implement). More work is needed to make it work for forward and input ranges (caching and iterator capability checks). The proposal paper by Tim Song goes into detail on how one might implement `views::slide` to work with forward and input ranges, but unfortunately my library writing skills and standardese are not up to par, yet. These are skills I will continue to develop and, someday, I might revisit this project to make it work in a similar fashion to the proposed C++23 version. Until then, I would not suggest using this project for anything serious but it might be a good reference for how a range adaptor is implemented.

## Compilers

The library has been tested with the following compilers:

* MSVC 19.31.30818
* GCC 11.1.0

## Code Examples

Using the pipe syntax:
```cpp
std::vector v{ 1, 2, 3, 4 };
// Sliding window containing two elements
for (auto&& i : v | ar::views::slide(2))
{
    std::cout << "[" << i[0] << ", " << i[1] << "]";
}
// Output: "[1, 2][2, 3][3, 4]"
```

Using the function syntax:
```cpp
std::vector v{ 1, 2, 3, 4 };
// Sliding window containing two elements
for (auto&& i : ar::views::slide(v, 2))
{
    s << "[" << i[0] << ", " << i[1] << "]";
}
// Output: "[1, 2][2, 3][3, 4]"
```

The differences between the two syntaxes is more apparent when chaining adaptors together. In general, the pipe syntax is preferable when chaining.

## References

Here are some of the papers I referenced while working on this library. I also used some of Sy Brand's Ranges Livecoding sessions as references.

* [P2214R0] Barry Revzin, Conor Hoekstra, Tim Song. 2020-10-15. A Plan for C++23 Ranges.
  * https://wg21.link/p2214r0
* [P2442R1] Tim Song. 2021-12-05. Windowing range adaptors: `views::chunk` and `views::slide`.
  * https://wg21.link/p2442r1
* Sy Brand. 2021-04-12. Livecoding C++20 Ranges: enumerate.
  * https://youtu.be/vTQYdoYGr5g
* Sy Brand. 2021-06-14. Livecoding C++ Ranges: standardisation, views::stride and views::transform_maybe.
  * https://youtu.be/JQUpD0mqtVk
