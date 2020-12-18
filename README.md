[![CI](https://github.com/MichaelUnknown/mkpoker/workflows/CI/badge.svg)](https://github.com/MichaelUnknown/mkpoker/actions)
[![codecov](https://codecov.io/gh/MichaelUnknown/mkpoker/branch/main/graph/badge.svg)](https://codecov.io/gh/MichaelUnknown/mkpoker)

# mkpoker
A Texas Holdem poker framework written in C++ 20.

This is my personal take on a poker library that provides
* rank, suit and card representation
* hands and ranges
* a hand strength evaluator
* a game/table representation and implementation of poker rules (cash game)
* an implementation of the counterfactual regret algorithm (to be done)

### Goals for this library / project that I had in mind when I started:
* cross platform library, support all the major compilers
* header only
* clean, good quality code, consistent formatting
* const correctness, constexpr and  [[nodiscard]] whenevery possible
* simple api, value types (most of the types are just wrappers around ints/bitfields)
* unit test everything
* CI integration: build, test, install on all platform, code coverage (done with Github actions and ccov + codecov.io)
* optimize for speed & size

### Roadmap:
* small equity calculator example
* simple implementation of cfr to play around with
* add more documentation

## Building tests & examples

This library comes with full cmake integrated dependency management and thus can be built easily. To build/run the tests, use
```git clone https://github.com/MichaelUnknown/mkpoker.git
cd mkpoker
cmake -S . -B build \
 -D MKPOKER_BUILD_EXAMPLES=1 \
 -D MKPOKER_BUILD_TESTS=1
 ```
 CMake should download and configure everything necessary. Then just `cmake --build build`, you can find the executables in the build/test and build/example directories.

### Dependencies

Google test is used for unit tests, CPM.cmake for adding dependendcies and PackageProject.cmake to create an installable object.

## Misc

### C++ 20 features used

* bit operations (<bit> header)
* three-way-comparison <=>
* std::span
* constexpr <algorithm>
* using enum (planned, unsupported by some compilers)
* constexpr std::string (planned)
* constexpr std::vector (planned, with constexpr string + vector, the whole library could be constexpr)