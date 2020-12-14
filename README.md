[![CI](https://github.com/MichaelUnknown/mkpoker/workflows/CI/badge.svg)](https://github.com/MichaelUnknown/mkpoker/actions)
[![codecov](https://codecov.io/gh/MichaelUnknown/mkpoker/branch/main/graph/badge.svg)](https://codecov.io/gh/MichaelUnknown/mkpoker)

# mkpoker
A Texas Holdem poker framework written in C++ 20.

Provides building blocks

### Goals for this library / project:
* cross platform library, support all the major compilers
* write clean, good quality code (use clang-format)
* const correct and constexpr everything (also use [[nodiscard]])
* simple api, value types
* unit test everything
* CI integration: build, test, install on all platform, code coverage (done with Github actions and ccov + codecov.io)
* optimize for speed & size whenever possible

### Roadmap:
* small equity calculator example
* simple implementation of cfr to play around with
* add more documentation

## Building

### Dependencies

### x

## Misc

### some comments about cmake and 

### C++ 20 features used:
* bit operations (<bit> header)
* three-way-comparison <=>
* std::span
* constexpr <algorithm>
* using enum (planned, unsupported by some compilers)
* constexpr std::string (planned)
* constexpr std::vector (planned, with constexpr string + vector, the whole library could be constexpr)