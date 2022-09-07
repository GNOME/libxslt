#!/bin/sh

set -e

git clone --depth 1 https://gitlab.gnome.org/GNOME/libxml2.git
cd libxml2
sh autogen.sh $CONFIG
make -j$(nproc)
cd ..

mkdir -p libxslt-build
cd libxslt-build
sh ../autogen.sh $CONFIG --with-libxml-src=../libxml2
make -j$(nproc) V=1 CFLAGS="$CFLAGS -Werror"
make -s CFLAGS="$CFLAGS -Werror" check
