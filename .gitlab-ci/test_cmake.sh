#!/bin/sh

set -e

git clone --depth 1 https://gitlab.gnome.org/GNOME/libxml2.git libxml2-source
cmake "$@" \
    -DBUILD_SHARED_LIBS=$BUILD_SHARED_LIBS \
    -DCMAKE_INSTALL_PREFIX=libxml2-install \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DLIBXML2_WITH_TESTS=OFF \
    -S libxml2-source -B libxml2-build
cmake --build libxml2-build --target install
export CMAKE_PREFIX_PATH="$CI_PROJECT_DIR/libxml2-install;$CMAKE_PREFIX_PATH"
export PATH="$CI_PROJECT_DIR/libxml2-install/bin:$PATH"
export LD_LIBRARY_PATH="$CI_PROJECT_DIR/libxml2-install/lib:$LD_LIBRARY_PATH"

cmake "$@" \
    -DBUILD_SHARED_LIBS="$BUILD_SHARED_LIBS" \
    -DCMAKE_INSTALL_PREFIX=libxslt-install \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_FLAGS='-Werror' \
    -DLIBXSLT_WITH_CRYPTO=ON \
    -DLIBXSLT_WITH_MODULES=ON \
    -DLIBXSLT_WITH_DEBUG=ON \
    -DLIBXSLT_WITH_DEBUGGER=ON \
    $CMAKE_OPTIONS \
    -S . -B libxslt-build
cmake --build libxslt-build --target install

(cd libxslt-build && ctest -VV)

mkdir -p libxslt-install/share/libxslt
cp Copyright libxslt-install/share/libxslt
(cd libxslt-install &&
    tar -czf ../libxslt-$CI_COMMIT_SHORT_SHA-$SUFFIX.tar.gz *)
