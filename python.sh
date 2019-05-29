#cmake -B _build/python -DSCAPIX_BRIDGE=python
cmake -B _build/python -DSCAPIX_BRIDGE=python -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DOPENSSL_INCLUDE_DIR=/usr/local/opt/openssl/include
cmake --build _build/python
