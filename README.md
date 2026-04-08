# ozo

[![CI](https://github.com/haomingbai/ozo/actions/workflows/ci.yml/badge.svg)](https://github.com/haomingbai/ozo/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/yandex/ozo/branch/master/graph/badge.svg)](https://codecov.io/gh/yandex/ozo)

## What's this

OZO is a C++20 library for asyncronous communication with PostgreSQL DBMS.
The library leverages the power of template metaprogramming, providing convenient mapping from C++ types to SQL along with rich query building possibilities. OZO supports different concurrency paradigms (callbacks, futures, coroutines), using Boost.Asio under the hood. Low-level communication with PostgreSQL server is done via libpq. All concepts in the library are designed to be easily extendable (even replaceable) by the user to simplify adaptation to specific project requirements.

### API

Since the project is on early state of development it lacks of documentation. We understand the importance of good docs and are working hard on this problem. Complete documentation is on the way, but now:

* look at our brand new [HOW TO](docs/howto.md),
* try our [generated from sources documentation](https://yandex.github.io/ozo/) - it is under construction but readable,
* learn more about main use-cases from [unit tests](tests/integration/request_integration.cpp),
* See our [C++Now'18 talk about OZO](https://youtu.be/-1zbaxuUsMA) with [presentation](https://github.com/boostcon/cppnow_presentations_2018/blob/master/05-09-2018_wednesday/design_and_implementation_of_dbms_asynchronous_client_library__roman_siromakha__cppnow_05092018.pdf).

## Compatibilities

OZO now targets the default executor model used by modern Boost.Asio releases.
`BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT` is no longer required and is no longer
exported to consumers.
## Dependencies

These things are needed:

* **CMake** is used as build system
* **GCC** or **Clang** C++ compiler with C++17 support (tested with GCC 7.0, Clang 5.0 and Apple LLVM version 9.0.0)
* **Boost** >= 1.74 with `BOOST_HANA_CONFIG_ENABLE_STRING_UDL` defined.
* **libpq** >= 9.3
* OZO vendors the [resource_pool](https://github.com/elsid/resource_pool) sources directly under `contrib/resource_pool`.

If you want to run PostgreSQL integration tests locally:
* **Podman**
* a local copy of `docker.io/library/postgres:16` or another PostgreSQL image

## Build

The library is header-only, but if you want to build and run unit-tests you can do it as listed below.

### Build and run tests on custom environment

First of all you need to satsfy requirements listed above. You can run tests using these commands.

```bash
mkdir -p build
cd build
cmake .. -DOZO_BUILD_TESTS=ON
make -j$(nproc)
ctest -V
```

Or use [build.sh](scripts/build.sh) which accepts folowing commands:

```bash
scripts/build.sh help
```

prints help.

```bash
scripts/build.sh <compiler> <target>
```

build and run tests with specified **compiler** and **target**, the **compiler** parameter can be:

* **gcc** - for build with gcc,
* **clang** - for build with clang.

The **target** parameter depends on **compiler**.
For **gcc**:

* **debug** - for debug build and tests
* **release** - for release build and tests
* **coverage** - for code coverage calculation

For **clang**:

* **debug** - for debug build and tests
* **release** - for release build and tests
* **asan** - for [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) launch
* **ubsan** - for [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) launch
* **tsan** - for [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) launch

```bash
scripts/build.sh all
```

build all possible configuration.

```bash
scripts/build.sh docs
```

generates documentation.

### Build and run tests on MacOS 10.X

For MacOS the best way to satisfy minimum requirements is [brew](https://brew.sh/)

```bash
brew install cmake boost libpq postresql
```

### Build and run tests within Docker

To build code and run tests inside docker container:

```bash
scripts/build.sh docker <compiler> <target>
```

To generate documentation using docker container:

```bash
scripts/build.sh docker docs
```

### Test against a local postgres

The recommended path is the Podman helper script:

```bash
scripts/run_pg_tests_podman.sh
```

The script starts a local PostgreSQL container from `docker.io/library/postgres:16`
by default, builds `ozo_tests` with `OZO_BUILD_PG_TESTS=ON`, runs the full test
suite, and removes the container afterwards.

You can override the image, port, build directory, and PostgreSQL credentials
with environment variables:

```bash
export OZO_PODMAN_POSTGRES_IMAGE=docker.io/library/postgres:16
export OZO_PG_TEST_PORT=55432
export OZO_PG_BUILD_DIR=build-podman-pg

scripts/run_pg_tests_podman.sh
```

Or you can point OZO tests to a PostgreSQL instance of your choosing by setting
these environment variables prior to building:

```bash
export OZO_BUILD_PG_TESTS=ON
export OZO_PG_TEST_CONNINFO='your conninfo (connection string)'

cmake -S . -B build-pg -DOZO_BUILD_TESTS=ON -DOZO_BUILD_PG_TESTS=ON -DOZO_PG_TEST_CONNINFO="$OZO_PG_TEST_CONNINFO"
cmake --build build-pg -j$(nproc)
ctest --test-dir build-pg -V
```

The older `docker-compose` based scripts remain in the repository for historical
development workflows, but the maintained path for local PostgreSQL testing is
the Podman flow above.
