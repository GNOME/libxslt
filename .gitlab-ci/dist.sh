#!/bin/sh

set -e

LIBXML2_PREFIX="$CI_PROJECT_DIR/libxml2/install"
git clone --depth 1 https://gitlab.gnome.org/GNOME/libxml2.git
cd libxml2
sh autogen.sh --prefix="$LIBXML2_PREFIX" $CONFIG
make -j$(nproc)
make install
cd ..

mkdir -p libxslt-dist
cd libxslt-dist
sh ../autogen.sh --with-libxml-prefix="$LIBXML2_PREFIX"
export PYTHONPATH="$(echo $LIBXML2_PREFIX/lib/python*/site-packages):$PYTHONPATH"
make distcheck V=1 DISTCHECK_CONFIGURE_FLAGS=" \
    --with-crypto \
    --with-plugins \
    --with-libxml-prefix=$LIBXML2_PREFIX \
"
if [ -z "$CI_COMMIT_TAG" ]; then
    mv libxslt-*.tar.xz libxslt-git-$CI_COMMIT_SHORT_SHA.tar.xz
fi
