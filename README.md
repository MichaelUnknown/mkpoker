[![CI](https://github.com/MichaelUnknown/mkpoker/workflows/CI/badge.svg)](https://github.com/MichaelUnknown/mkpoker/actions)
[![codecov](https://codecov.io/gh/MichaelUnknown/mkpoker/branch/main/graph/badge.svg)](https://codecov.io/gh/MichaelUnknown/mkpoker)

# mkpoker
A Texas Holdem poker framework written in C++ 20.

This is my personal take on a poker library that provides
* rank, suit and card representation
* hands and ranges
* a hand strength calculator / evaluator
* a game / table representation and implementation of poker rules (cash game)
* an implementation of the counterfactual regret algorithm (to be done)

## Usage, building tests & examples

This library comes with full cmake integrated dependency management and thus can be built easily on most platforms.
On Windows you need to install [Git](https://git-scm.com/) (or downlaod the source) and [CMake](https://cmake.org/), most Unix based platforms should be ready to go from the get-go.
To build/run the tests, use
```
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

This will work for most platforms. You can also take a look at the CI (YAML file in `.github/workflows`) to see how to build for different OSes, compilers and Standard Libraries.

To install the library (headers) on your system, use `cmake --build build --target install`.
However, it is just as easy to add the library as a dependency in your CMake Project.
You just need to [add CPM](https://github.com/TheLartians/CPM.cmake#adding-cpm) and include the following code in your CMakeLists.txt
```
CPMAddPackage(
    NAME mkpoker
    GITHUB_REPOSITORY MichaelUnknown/mkpoker
    VERSION 0.x
)

# define your executable
...

# link against mkpoker
target_link_libraries(MyPokerApp mkpoker::mkpoker)
```



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

- [ ] Equity calculator example
- [ ] Simple implementation of CFR to play around with
- [ ] More documentation

### Dependencies

Google test is used for unit tests, [CPM.cmake](https://github.com/TheLartians/CPM.cmake) for adding dependendcies and [PackageProject.cmake](https://github.com/TheLartians/PackageProject.cmake) to create an installable object.


## Misc

### C++ 20 features used

* bit operations (<bit> header)
* three-way-comparison <=>
* std::span
* constexpr <algorithm>
* using enum (planned, unsupported by some compilers)
* constexpr std::string (planned)
* constexpr std::vector (planned, with constexpr string + vector, the whole library could be constexpr)