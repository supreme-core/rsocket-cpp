# rsocket-cpp

<b>
Fork off rsocket-cpp for the purpose of creating c++ to python binding using scapix
</b>

# DEPENDENCIES relates to Facebook Folly

```
sudo apt-get install \
    g++ \
    cmake \
    libboost-all-dev \
    libevent-dev \
    libdouble-conversion-dev \
    libgoogle-glog-dev \
    libgflags-dev \
    libiberty-dev \
    liblz4-dev \
    liblzma-dev \
    libsnappy-dev \
    make \
    zlib1g-dev \
    binutils-dev \
    libjemalloc-dev \
    libssl-dev \
    pkg-config
    
mkdir zstd
cd zstd
git clone https://github.com/facebook/zstd
ls
cd zstd
cd build
make ..
cd ..
make 
sudo make install
sudo apt-get install libdw-dev
sudo apt-get install libelf-dev libdwarf-dev
sudo apt-get install libaio1
```

# Setting Compilers
Ideally set it to GCC 7 or 8

```
sudo update-alternatives --config gcc
sudo update-alternatives --config g++
```

# Build Folly

```
git clone https://github.com/facebook/folly
cd folly
cmake -B _build -DFOLLY_USE_JEMALLOC=0 -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true
cd _build
make -j $(nproc)
make install # with either sudo or DESTDIR as necessary
```
Note: GCC compiler use for building Folly should be the same as the compiler to build Scapix

# STEPS TO COMPILE

```
$ mkdir scapix_test
$ cd scapix_test
$ git clone https://github.com/supreme-core/rsocket-cpp
$ git clone https://github.com/scapix-com/scapix
$ cd rsocket-cpp
$ sudo sh python.sh
```
Note: GCC used to compile Scapix was GCC 5

