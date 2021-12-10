[![CI](https://github.com/MichaelUnknown/mkpoker/workflows/CI/badge.svg)](https://github.com/MichaelUnknown/mkpoker/actions)
[![codecov](https://codecov.io/gh/MichaelUnknown/mkpoker/branch/main/graph/badge.svg)](https://codecov.io/gh/MichaelUnknown/mkpoker)

# mkpoker
A Texas Holdem poker framework written in C++ 20.

This is my personal take on a poker library that provides
* rank, suit and card representation
* hands and ranges
* a hand strength calculator / evaluator
* a game / table representation and implementation of poker rules (cash game)
* an implementation of the counterfactual regret algorithm

## Usage, building tests & examples

This library comes with fully integrated dependency management via cmake and thus can be used easily on most platforms.
The library is header only and doesn't need to be built, however you can build and run the tests and examples (see below).

To take mkpoker as a dependency in your CMake Project, just [add CPM](https://github.com/TheLartians/CPM.cmake#adding-cpm) and include the following code in your CMakeLists.txt:
```cmake
# gh is short for GitHub, specify the version after the 'at' or use a hashtag for a specific commit
CPMAddPackage("gh:MichaelUnknown/mkpoker@0.1.0")
#CPMAddPackage("gh:MichaelUnknown/mkpoker#6a5693c50d563f13ebb2191a09443754732badfb")

# define your executable
add_executable(MyPokerApp MyPokerApp.cpp MoreSources.cpp ...)

# link against mkpoker
target_link_libraries(MyPokerApp mkpoker::mkpoker)
```


### Building
On Windows you need to install [Git](https://git-scm.com/) (or downlaod the source) and [CMake](https://cmake.org/). Most Unix based platforms should already have all the prerequisites.
To build/run the tests, use
```bash
git clone https://github.com/MichaelUnknown/mkpoker.git
cd mkpoker
cmake \
 -S . \
 -B build \
 -D MKPOKER_BUILD_EXAMPLES=1 \
 -D MKPOKER_BUILD_TESTS=1
 ```
CMake will download and configure the necessary dependencies.
Then just build with `cmake --build build`, you can find the executables in the build/test and build/example directories.

You can also take a look at the CI (YAML file in `.github/workflows`) to see how I set up the build of the tests for different OSes, compilers and Standard Libraries.

To install the library (headers) on your system, use (`sudo`) `cmake --build build --target install` (or provide the `-D CMAKE_INSTALL_PREFIX=<install dir>` cmake parameter for a specific installation directory)


### Dependencies

Google test is used for unit tests, [CPM.cmake](https://github.com/TheLartians/CPM.cmake) for adding dependendcies and [PackageProject.cmake](https://github.com/TheLartians/PackageProject.cmake) to create an installable object.


## Goals for this library / project that I had in mind when I started:

* cross platform library, support all the major compilers
* header only
* clean, good quality code, consistent formatting
* const correctness, constexpr and  [[nodiscard]] whenevery possible
* simple api, value types (most of the types are just wrappers around ints/bitfields)
* unit test everything
* CI integration: build, test, install on all platform, code coverage (done with Github actions and ccov + codecov.io)
* optimize for speed & size

### Roadmap:

- [ ] Equity calculator example
- [x] Simple implementation of CFR to play around with
- [ ] More documentation

