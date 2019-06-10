# rsocket-cpp

<b>
Fork off rsocket-cpp for the purpose of creating c++ to python binding using scapix
</b>

# STEPS TO COMPILE

```
$ mkdir scapix_test
$ cd scapix_test
$ git clone https://github.com/supreme-core/rsocket-cpp
$ git clone https://github.com/scapix-com/scapix
$ cd rsocket-cpp
$ sudo sh python.sh
```

C++ implementation of [RSocket](https://rsocket.io)

<a href='https://travis-ci.org/rsocket/rsocket-cpp/builds'><img src='https://travis-ci.org/rsocket/rsocket-cpp.svg?branch=master'></a>
[![Coverage Status](https://coveralls.io/repos/github/rsocket/rsocket-cpp/badge.svg?branch=master)](https://coveralls.io/github/rsocket/rsocket-cpp?branch=master)

# Dependencies

Install Scapix
https://github.com/scapix-com/scapix

Requires cmake,  g++5, gcc5 and not gcc7
.

Install `folly`:

```
brew install --HEAD folly
```

# Building and running tests

After installing dependencies as above, you can build and run tests with:

```
# inside root ./rsocket-cpp
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=DEBUG ../
make -j
./tests
```

# License

By contributing to rsocket-cpp, you agree that your contributions will be licensed
under the LICENSE file in the root directory of this source tree.
