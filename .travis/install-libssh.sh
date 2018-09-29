#!/bin/sh
# install libssh manually, because the version on travis CI is too old
LIBSSH=libssh-0.8.3
wget https://git.libssh.org/projects/libssh.git/snapshot/$LIBSSH.tar.gz
tar xf $LIBSSH.tar.gz
cd $LIBSSH
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
